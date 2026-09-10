/*
 * gsd_algorithms.cpp — native implementation of the dashboard's analytical core.
 * Mirrors the managed C# Algorithms.cs so results are identical whichever
 * backend is active.
 */
#include "gsd_api.h"

#include <algorithm>
#include <cmath>
#include <vector>

#define GSD_ABI 1

static double clampd(double v, double lo, double hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

// Linear-interpolated percentile over a pre-sorted vector.
static double percentile(const std::vector<double>& sorted, double p) {
    const size_t n = sorted.size();
    if (n == 0) return 0.0;
    if (n == 1) return sorted[0];
    double rank = (p / 100.0) * (double)(n - 1);
    size_t lo = (size_t)std::floor(rank);
    size_t hi = (size_t)std::ceil(rank);
    double frac = rank - (double)lo;
    double val = sorted[lo] + (sorted[hi] - sorted[lo]) * frac;
    return std::round(val * 100.0) / 100.0;
}

extern "C" {

GSD_API int32_t GSD_CALL gsd_abi_version(void) { return GSD_ABI; }

GSD_API double GSD_CALL gsd_ewma(const double* values, int32_t n, double alpha) {
    if (!values || n <= 0) return 0.0;
    double ema = values[0];
    for (int32_t i = 1; i < n; ++i)
        ema = alpha * values[i] + (1.0 - alpha) * ema;
    return ema;
}

GSD_API void GSD_CALL gsd_analyze_frame_budget(
    const double* frameTimesMs, int32_t n, int32_t targetFps,
    GsdFrameBudgetReport* out) {
    if (!out) return;
    double budget = 1000.0 / (targetFps <= 0 ? 60 : targetFps);
    if (!frameTimesMs || n <= 0) {
        *out = GsdFrameBudgetReport{0, 0, 0, 0, budget, 0, 0, 0};
        return;
    }

    std::vector<double> v(frameTimesMs, frameTimesMs + n);
    std::vector<double> sorted(v);
    std::sort(sorted.begin(), sorted.end());

    int32_t over = 0;
    double sum = 0.0;
    for (double ft : v) { if (ft > budget) ++over; sum += ft; }
    double avg = sum / (double)n;

    out->p50 = percentile(sorted, 50);
    out->p95 = percentile(sorted, 95);
    out->p99 = percentile(sorted, 99);
    out->pctOverBudget = 100.0 * (double)over / (double)n;
    out->targetMs = budget;
    out->avgFps = avg <= 0 ? 0.0 : 1000.0 / avg;
    out->sampleCount = n;
    out->_pad = 0;
}

GSD_API double GSD_CALL gsd_build_health_score(
    const GsdBuildInput* in, double durationBaseline) {
    if (!in) return 0.0;

    double passRate = in->totalTests == 0
        ? 1.0
        : (double)(in->totalTests - in->failedTests) / (double)in->totalTests;
    double passComponent = passRate * 45.0;

    double ratio = durationBaseline <= 0 ? 1.0 : in->durationMinutes / durationBaseline;
    double durComponent = clampd(2.0 - ratio, 0.0, 1.0) * 20.0;

    double flakyRatio = in->totalTests == 0
        ? 0.0 : (double)in->flakyTests / (double)in->totalTests;
    double flakyComponent = clampd(1.0 - flakyRatio * 8.0, 0.0, 1.0) * 15.0;

    double crashComponent = clampd(1.0 - in->crashRatePct / 5.0, 0.0, 1.0) * 20.0;

    double score = passComponent + durComponent + flakyComponent + crashComponent;
    if (in->statusFailed) score *= 0.35;
    score = clampd(score, 0.0, 100.0);
    return std::round(score * 10.0) / 10.0;
}

GSD_API void GSD_CALL gsd_schedule(
    const int32_t* jobCores, const int32_t* jobRam, const int32_t* jobGpu,
    const double* jobDurationMin, int32_t jobCount,
    const int32_t* machCores, const int32_t* machRam, const int32_t* machGpu,
    int32_t machCount,
    int32_t* outAssignedMachine, double* outWaitMinutes,
    GsdScheduleResult* outSummary) {

    std::vector<int32_t> usedCores(machCount, 0), usedRam(machCount, 0);
    std::vector<double> machLoad(machCount, 0.0);

    // Largest-first ordering (by cores, then ram) — classic bin-packing heuristic.
    std::vector<int32_t> order(jobCount);
    for (int32_t i = 0; i < jobCount; ++i) order[i] = i;
    std::sort(order.begin(), order.end(), [&](int32_t a, int32_t b) {
        if (jobCores[a] != jobCores[b]) return jobCores[a] > jobCores[b];
        return jobRam[a] > jobRam[b];
    });

    for (int32_t i = 0; i < jobCount; ++i) {
        if (outAssignedMachine) outAssignedMachine[i] = -1;
        if (outWaitMinutes) outWaitMinutes[i] = 0.0;
    }

    int32_t scheduled = 0;
    for (int32_t oi = 0; oi < jobCount; ++oi) {
        int32_t j = order[oi];
        int32_t best = -1;
        int32_t bestLeftover = INT32_MAX;

        for (int32_t m = 0; m < machCount; ++m) {
            if (jobGpu[j] && !machGpu[m]) continue;
            int32_t freeCores = machCores[m] - usedCores[m];
            int32_t freeRam = machRam[m] - usedRam[m];
            if (freeCores < jobCores[j] || freeRam < jobRam[j]) continue;
            int32_t leftover = (freeCores - jobCores[j]) + (freeRam - jobRam[j]);
            if (leftover < bestLeftover) { bestLeftover = leftover; best = m; }
        }

        if (best >= 0) {
            usedCores[best] += jobCores[j];
            usedRam[best] += jobRam[j];
            if (outAssignedMachine) outAssignedMachine[j] = best;
            if (outWaitMinutes) outWaitMinutes[j] = std::round(machLoad[best] * 10.0) / 10.0;
            machLoad[best] += jobDurationMin[j];
            ++scheduled;
        } else {
            double minLoad = -1.0;
            for (int32_t m = 0; m < machCount; ++m) {
                if (jobGpu[j] && !machGpu[m]) continue;
                if (minLoad < 0 || machLoad[m] < minLoad) minLoad = machLoad[m];
            }
            if (outWaitMinutes && minLoad >= 0)
                outWaitMinutes[j] = std::round((minLoad + jobDurationMin[j]) * 10.0) / 10.0;
        }
    }

    if (outSummary) {
        double sumUtil = 0.0;
        int64_t totalCores = 0, totalUsed = 0;
        for (int32_t m = 0; m < machCount; ++m) {
            sumUtil += machCores[m] == 0 ? 0.0 : (double)usedCores[m] / (double)machCores[m];
            totalCores += machCores[m];
            totalUsed += usedCores[m];
        }
        double avgCpu = machCount == 0 ? 0.0 : sumUtil / (double)machCount;
        double idle = totalCores == 0 ? 0.0
            : 100.0 * (double)(totalCores - totalUsed) / (double)totalCores;
        outSummary->avgCpuUtilization = std::round(avgCpu * 100.0 * 10.0) / 10.0;
        outSummary->idleCapacityPct = std::round(idle * 10.0) / 10.0;
        outSummary->scheduled = scheduled;
        outSummary->queued = jobCount - scheduled;
    }
}

GSD_API void GSD_CALL gsd_project_ltv(
    const int32_t* days, const double* fractions, int32_t n,
    double arpdau, int32_t horizonDays, GsdLtvResult* out) {
    if (!out) return;

    // Collect valid (day >= 1, fraction > 0) points.
    std::vector<double> lx, ly;
    double firstFrac = 0.0; bool haveFirst = false;
    for (int32_t i = 0; i < n; ++i) {
        if (days[i] >= 1 && fractions[i] > 0.0) {
            lx.push_back(std::log((double)days[i]));
            ly.push_back(std::log(fractions[i]));
            if (!haveFirst) { firstFrac = fractions[i]; haveFirst = true; }
        }
    }

    if (lx.size() < 2) {
        *out = GsdLtvResult{0.0, haveFirst ? firstFrac : 0.0, 0.0};
        return;
    }

    double N = (double)lx.size();
    double sx = 0, sy = 0, sxx = 0, sxy = 0;
    for (size_t i = 0; i < lx.size(); ++i) {
        sx += lx[i]; sy += ly[i]; sxx += lx[i] * lx[i]; sxy += lx[i] * ly[i];
    }
    double denom = N * sxx - sx * sx;
    double slope = denom == 0 ? 0.0 : (N * sxy - sx * sy) / denom;
    double intercept = (sy - slope * sx) / N;
    double lambda = -slope;
    double r1 = std::exp(intercept);

    auto retention = [&](int d) {
        return clampd(r1 * std::pow((double)d, -lambda), 0.0, 1.0);
    };

    double d30 = retention(30);
    double activeDays = 1.0;
    for (int d = 1; d <= horizonDays; ++d) activeDays += retention(d);
    double ltv = arpdau * activeDays;

    out->decayLambda = std::round(lambda * 1000.0) / 1000.0;
    out->projectedD30 = std::round(d30 * 10000.0) / 10000.0;
    out->projectedLtv = std::round(ltv * 100.0) / 100.0;
}

} // extern "C"
