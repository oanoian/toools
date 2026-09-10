using GameStudioDashboard.Models;
using GameStudioDashboard.Native;

namespace GameStudioDashboard.Services;

/// <summary>
/// Thin adapter over the native C++ backend (GameStudioNative). Detects whether
/// the DLL is present and ABI-compatible at startup; every managed <see cref="Algorithms"/>
/// method consults <see cref="Available"/> and delegates here when it can, so the
/// app runs identically with or without the native backend.
/// </summary>
public static class NativeBridge
{
    public static bool Available { get; }
    public static string Status { get; }

    static NativeBridge()
    {
        try
        {
            int abi = NativeInterop.gsd_abi_version();
            if (abi == NativeInterop.ExpectedAbi)
            {
                Available = true;
                Status = $"C++ backend active (ABI {abi})";
            }
            else
            {
                Available = false;
                Status = $"C++ backend ABI mismatch (dll {abi}, expected {NativeInterop.ExpectedAbi}) — using managed";
            }
        }
        catch (Exception ex) when (ex is DllNotFoundException or BadImageFormatException or EntryPointNotFoundException)
        {
            Available = false;
            Status = "Managed backend (GameStudioNative not found)";
        }
    }

    public static double Ewma(IReadOnlyList<double> values, double alpha)
        => NativeInterop.gsd_ewma(values as double[] ?? values.ToArray(), values.Count, alpha);

    public static Algorithms.FrameBudgetReport AnalyzeFrameBudget(IReadOnlyList<double> frameTimesMs, int targetFps)
    {
        NativeInterop.gsd_analyze_frame_budget(
            frameTimesMs as double[] ?? frameTimesMs.ToArray(), frameTimesMs.Count, targetFps, out var r);
        return new Algorithms.FrameBudgetReport(r.P50, r.P95, r.P99, r.PctOverBudget, r.TargetMs, r.AvgFps, r.SampleCount);
    }

    public static double BuildHealthScore(BuildJob job, double durationBaseline)
    {
        var input = new GsdBuildInput
        {
            CrashRatePct = job.CrashRatePct,
            DurationMinutes = job.DurationMinutes,
            TotalTests = job.TotalTests,
            FailedTests = job.FailedTests,
            FlakyTests = job.FlakyTests,
            StatusFailed = job.Status == BuildStatus.Failed ? 1 : 0,
        };
        return NativeInterop.gsd_build_health_score(in input, durationBaseline);
    }

    public static Algorithms.LtvProjection ProjectLtv(IReadOnlyList<RetentionPoint> points, double arpdau, int horizonDays)
    {
        var days = points.Select(p => p.Day).ToArray();
        var fracs = points.Select(p => p.Fraction).ToArray();
        NativeInterop.gsd_project_ltv(days, fracs, points.Count, arpdau, horizonDays, out var r);
        return new Algorithms.LtvProjection(r.DecayLambda, r.ProjectedD30, r.ProjectedLtv);
    }

    public static Algorithms.ScheduleResult Schedule(IReadOnlyList<BuildJob> jobs, IReadOnlyList<Machine> machines)
    {
        foreach (var m in machines) m.Reset();
        foreach (var j in jobs) { j.AssignedMachine = null; j.EstWaitMinutes = 0; }

        var active = jobs.Where(j => j.Status is BuildStatus.Queued or BuildStatus.Running).ToList();
        int n = active.Count, mc = machines.Count;

        var jobCores = active.Select(j => j.RequiredCores).ToArray();
        var jobRam = active.Select(j => j.RequiredRamGb).ToArray();
        var jobGpu = active.Select(j => j.RequiresGpu ? 1 : 0).ToArray();
        var jobDur = active.Select(j => j.DurationMinutes).ToArray();

        var machCores = machines.Select(m => m.Cores).ToArray();
        var machRam = machines.Select(m => m.RamGb).ToArray();
        var machGpu = machines.Select(m => m.HasGpu ? 1 : 0).ToArray();

        var assigned = new int[n];
        var wait = new double[n];
        NativeInterop.gsd_schedule(
            jobCores, jobRam, jobGpu, jobDur, n,
            machCores, machRam, machGpu, mc,
            assigned, wait, out var summary);

        for (int i = 0; i < n; i++)
        {
            var job = active[i];
            job.EstWaitMinutes = wait[i];
            if (assigned[i] >= 0)
            {
                var m = machines[assigned[i]];
                m.UsedCores += job.RequiredCores;
                m.UsedRamGb += job.RequiredRamGb;
                m.AssignedJobs++;
                job.AssignedMachine = m.Name;
            }
        }

        return new Algorithms.ScheduleResult(
            summary.AvgCpuUtilization, summary.IdleCapacityPct, summary.Scheduled, summary.Queued);
    }
}
