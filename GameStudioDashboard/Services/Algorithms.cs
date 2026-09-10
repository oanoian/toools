using GameStudioDashboard.Models;

namespace GameStudioDashboard.Services;

/// <summary>
/// The analytical core of the dashboard. Every method here is pure (no UI, no
/// mutable global state) so behaviour is deterministic and easy to reason about.
/// </summary>
public static class Algorithms
{
    // ---------------------------------------------------------------------
    // 1. Build health score
    // ---------------------------------------------------------------------
    // Blends four independent signals into a single 0..100 score. Duration is
    // scored against an EWMA baseline so a build is judged relative to the
    // recent trend rather than an arbitrary constant.

    public static double ExponentialMovingAverage(IEnumerable<double> values, double alpha = 0.3)
    {
        if (NativeBridge.Available)
        {
            var list = values as IReadOnlyList<double> ?? values.ToList();
            if (list.Count > 0) return NativeBridge.Ewma(list, alpha);
        }
        double? ema = null;
        foreach (var v in values)
            ema = ema is null ? v : alpha * v + (1 - alpha) * ema.Value;
        return ema ?? 0;
    }

    /// <param name="durationBaseline">EWMA of recent build durations (minutes).</param>
    public static double BuildHealthScore(BuildJob job, double durationBaseline)
    {
        if (NativeBridge.Available) return NativeBridge.BuildHealthScore(job, durationBaseline);

        // pass rate: 0..1 -> weighted 45
        double passComponent = job.PassRate * 45;

        // duration: ratio vs baseline. On-or-below baseline = full marks,
        // 2x baseline or worse = 0. Weighted 20.
        double ratio = durationBaseline <= 0 ? 1 : job.DurationMinutes / durationBaseline;
        double durComponent = Math.Clamp(2 - ratio, 0, 1) * 20;

        // flaky ratio: fewer flaky tests is better. Weighted 15.
        double flakyRatio = job.TotalTests == 0 ? 0 : (double)job.FlakyTests / job.TotalTests;
        double flakyComponent = Math.Clamp(1 - flakyRatio * 8, 0, 1) * 15;

        // crash rate: 0% = full, 5%+ = 0. Weighted 20.
        double crashComponent = Math.Clamp(1 - job.CrashRatePct / 5.0, 0, 1) * 20;

        double score = passComponent + durComponent + flakyComponent + crashComponent;
        if (job.Status == BuildStatus.Failed) score *= 0.35; // a red build can't be "healthy"
        return Math.Round(Math.Clamp(score, 0, 100), 1);
    }

    // ---------------------------------------------------------------------
    // 2. Render-budget analyzer
    // ---------------------------------------------------------------------

    public record FrameBudgetReport(
        double P50, double P95, double P99,
        double PctOverBudget, double TargetMs, double AvgFps, int SampleCount);

    /// <param name="frameTimesMs">Per-frame times in milliseconds.</param>
    /// <param name="targetFps">Budget target (e.g. 60 or 120).</param>
    public static FrameBudgetReport AnalyzeFrameBudget(IReadOnlyList<double> frameTimesMs, int targetFps)
    {
        if (NativeBridge.Available && frameTimesMs.Count > 0)
            return NativeBridge.AnalyzeFrameBudget(frameTimesMs, targetFps);

        if (frameTimesMs.Count == 0)
            return new FrameBudgetReport(0, 0, 0, 0, 1000.0 / targetFps, 0, 0);

        double budget = 1000.0 / targetFps;
        var sorted = frameTimesMs.OrderBy(x => x).ToArray();
        int over = frameTimesMs.Count(ft => ft > budget);
        double avg = frameTimesMs.Average();

        return new FrameBudgetReport(
            P50: Percentile(sorted, 50),
            P95: Percentile(sorted, 95),
            P99: Percentile(sorted, 99),
            PctOverBudget: 100.0 * over / frameTimesMs.Count,
            TargetMs: budget,
            AvgFps: avg <= 0 ? 0 : 1000.0 / avg,
            SampleCount: frameTimesMs.Count);
    }

    // Linear-interpolated percentile over a pre-sorted array.
    public static double Percentile(double[] sorted, double percentile)
    {
        if (sorted.Length == 0) return 0;
        if (sorted.Length == 1) return sorted[0];
        double rank = (percentile / 100.0) * (sorted.Length - 1);
        int lo = (int)Math.Floor(rank);
        int hi = (int)Math.Ceiling(rank);
        double frac = rank - lo;
        return Math.Round(sorted[lo] + (sorted[hi] - sorted[lo]) * frac, 2);
    }

    // ---------------------------------------------------------------------
    // 3. Build-farm scheduler (best-fit bin-packing)
    // ---------------------------------------------------------------------

    public record ScheduleResult(double AvgCpuUtilization, double IdleCapacityPct, int Scheduled, int Queued);

    /// <summary>
    /// Assigns queued jobs to machines using best-fit: each job goes to the
    /// eligible machine that will be left with the least spare capacity, which
    /// packs work tightly and keeps whole machines free for large jobs. Mutates
    /// the machine usage counters and each job's AssignedMachine / EstWaitMinutes.
    /// </summary>
    public static ScheduleResult Schedule(IReadOnlyList<BuildJob> jobs, IReadOnlyList<Machine> machines)
    {
        if (NativeBridge.Available) return NativeBridge.Schedule(jobs, machines);

        foreach (var m in machines) m.Reset();
        foreach (var j in jobs) { j.AssignedMachine = null; j.EstWaitMinutes = 0; }

        // Largest jobs first — classic bin-packing heuristic.
        var ordered = jobs
            .Where(j => j.Status is BuildStatus.Queued or BuildStatus.Running)
            .OrderByDescending(j => j.RequiredCores)
            .ThenByDescending(j => j.RequiredRamGb)
            .ToList();

        int scheduled = 0;
        // per-machine projected finish time, for wait estimates.
        var machineLoadMinutes = machines.ToDictionary(m => m.Name, _ => 0.0);

        foreach (var job in ordered)
        {
            Machine? best = null;
            int bestLeftover = int.MaxValue;

            foreach (var m in machines)
            {
                if (job.RequiresGpu && !m.HasGpu) continue;
                int freeCores = m.Cores - m.UsedCores;
                int freeRam = m.RamGb - m.UsedRamGb;
                if (freeCores < job.RequiredCores || freeRam < job.RequiredRamGb) continue;

                int leftover = (freeCores - job.RequiredCores) + (freeRam - job.RequiredRamGb);
                if (leftover < bestLeftover) { bestLeftover = leftover; best = m; }
            }

            if (best is not null)
            {
                best.UsedCores += job.RequiredCores;
                best.UsedRamGb += job.RequiredRamGb;
                best.AssignedJobs++;
                job.AssignedMachine = best.Name;
                job.EstWaitMinutes = Math.Round(machineLoadMinutes[best.Name], 1);
                machineLoadMinutes[best.Name] += job.DurationMinutes;
                scheduled++;
            }
            else
            {
                // No room now: estimate wait as the earliest-freeing eligible machine.
                var eligible = machines.Where(m => !job.RequiresGpu || m.HasGpu);
                job.EstWaitMinutes = eligible.Any()
                    ? Math.Round(eligible.Min(m => machineLoadMinutes[m.Name]) + job.DurationMinutes, 1)
                    : 0;
            }
        }

        double avgCpu = machines.Count == 0 ? 0 : machines.Average(m => m.CpuUtilization);
        int totalCores = machines.Sum(m => m.Cores);
        int usedCores = machines.Sum(m => m.UsedCores);
        double idlePct = totalCores == 0 ? 0 : 100.0 * (totalCores - usedCores) / totalCores;

        return new ScheduleResult(
            Math.Round(avgCpu * 100, 1),
            Math.Round(idlePct, 1),
            scheduled,
            ordered.Count - scheduled);
    }

    // ---------------------------------------------------------------------
    // 4. Sprint forecasting (EWMA velocity)
    // ---------------------------------------------------------------------

    public record SprintForecast(double VelocityPerDay, DateTime ProjectedDate, bool AtRisk, double DaysNeeded);

    /// <param name="pastVelocities">Points completed per prior sprint (chronological).</param>
    public static SprintForecast ForecastMilestone(
        Milestone milestone, IReadOnlyList<double> pastVelocities, int sprintLengthDays, DateTime today)
    {
        double perSprint = ExponentialMovingAverage(pastVelocities, 0.4);
        double perDay = sprintLengthDays <= 0 ? 0 : perSprint / sprintLengthDays;

        int remaining = milestone.TotalPoints - milestone.DonePoints;
        double daysNeeded = perDay <= 0 ? double.PositiveInfinity : remaining / perDay;

        DateTime projected = double.IsInfinity(daysNeeded)
            ? DateTime.MaxValue
            : today.AddDays(Math.Ceiling(daysNeeded));

        bool atRisk = projected > milestone.TargetDate;
        return new SprintForecast(
            Math.Round(perDay, 2),
            projected,
            atRisk,
            double.IsInfinity(daysNeeded) ? double.PositiveInfinity : Math.Ceiling(daysNeeded));
    }

    // ---------------------------------------------------------------------
    // 5. Retention / LTV projection
    // ---------------------------------------------------------------------

    public record LtvProjection(double DecayLambda, double ProjectedD30Retention, double ProjectedLtv);

    /// <summary>
    /// Fits a power-law retention curve r(d) = r1 * d^(-lambda) to the observed
    /// cohort points via least-squares in log space, then projects D30 retention
    /// and lifetime value across <paramref name="horizonDays"/>.
    /// </summary>
    public static LtvProjection ProjectLtv(
        IReadOnlyList<RetentionPoint> points, double arpdau, int horizonDays = 90)
    {
        if (NativeBridge.Available) return NativeBridge.ProjectLtv(points, arpdau, horizonDays);

        var valid = points.Where(p => p.Day >= 1 && p.Fraction > 0).ToList();
        if (valid.Count < 2)
            return new LtvProjection(0, valid.FirstOrDefault()?.Fraction ?? 0, 0);

        // log r = log r1 - lambda * log d  => linear regression of logR on logD.
        double n = valid.Count;
        double sx = valid.Sum(p => Math.Log(p.Day));
        double sy = valid.Sum(p => Math.Log(p.Fraction));
        double sxx = valid.Sum(p => Math.Log(p.Day) * Math.Log(p.Day));
        double sxy = valid.Sum(p => Math.Log(p.Day) * Math.Log(p.Fraction));

        double denom = n * sxx - sx * sx;
        double slope = denom == 0 ? 0 : (n * sxy - sx * sy) / denom;
        double intercept = (sy - slope * sx) / n;
        double lambda = -slope;
        double r1 = Math.Exp(intercept);

        double Retention(int d) => Math.Clamp(r1 * Math.Pow(d, -lambda), 0, 1);

        double d30 = Retention(30);
        // LTV ~ ARPDAU * expected active days over the horizon.
        double activeDays = 1; // day 0 counts as active
        for (int d = 1; d <= horizonDays; d++) activeDays += Retention(d);
        double ltv = arpdau * activeDays;

        return new LtvProjection(Math.Round(lambda, 3), Math.Round(d30, 4), Math.Round(ltv, 2));
    }
}
