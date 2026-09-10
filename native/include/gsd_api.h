/*
 * gsd_api.h — C ABI for the Game Studio Dashboard native backend.
 *
 * Every export is plain `extern "C"` with a C-friendly signature (primitives,
 * flat arrays, POD structs) so it can be consumed from C# via P/Invoke without
 * a C++/CLI bridge. Struct field ordering keeps all doubles first, then ints,
 * so the layout matches the C# [StructLayout(LayoutKind.Sequential)] mirrors
 * on both x64 and arm64 with default 8-byte packing.
 */
#ifndef GSD_API_H
#define GSD_API_H

#include <stdint.h>

#if defined(_WIN32)
  #define GSD_API __declspec(dllexport)
  #define GSD_CALL __cdecl
#else
  #define GSD_API __attribute__((visibility("default")))
  #define GSD_CALL
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct GsdFrameBudgetReport {
    double p50;
    double p95;
    double p99;
    double pctOverBudget;
    double targetMs;
    double avgFps;
    int32_t sampleCount;
    int32_t _pad;              /* explicit tail pad -> size is a multiple of 8 */
} GsdFrameBudgetReport;

typedef struct GsdBuildInput {
    double crashRatePct;
    double durationMinutes;
    int32_t totalTests;
    int32_t failedTests;
    int32_t flakyTests;
    int32_t statusFailed;      /* 1 if the build is red */
} GsdBuildInput;

typedef struct GsdLtvResult {
    double decayLambda;
    double projectedD30;
    double projectedLtv;
} GsdLtvResult;

typedef struct GsdScheduleResult {
    double avgCpuUtilization;  /* percent 0..100 */
    double idleCapacityPct;
    int32_t scheduled;
    int32_t queued;
} GsdScheduleResult;

/* Returns the ABI version so the C# side can detect a mismatched DLL. */
GSD_API int32_t GSD_CALL gsd_abi_version(void);

/* Exponential moving average over values[0..n). */
GSD_API double GSD_CALL gsd_ewma(const double* values, int32_t n, double alpha);

/* Render-budget analysis over per-frame times (ms) against a target FPS. */
GSD_API void GSD_CALL gsd_analyze_frame_budget(
    const double* frameTimesMs, int32_t n, int32_t targetFps,
    GsdFrameBudgetReport* out);

/* Weighted 0..100 build health score. */
GSD_API double GSD_CALL gsd_build_health_score(
    const GsdBuildInput* in, double durationBaseline);

/* Best-fit bin-packing scheduler. Job/machine attributes are passed as
 * parallel flat arrays. outAssignedMachine[i] receives the machine index for
 * job i (or -1 if it could not be placed); outWaitMinutes[i] the estimated
 * wait. Both output arrays must have length jobCount. */
GSD_API void GSD_CALL gsd_schedule(
    const int32_t* jobCores, const int32_t* jobRam, const int32_t* jobGpu,
    const double* jobDurationMin, int32_t jobCount,
    const int32_t* machCores, const int32_t* machRam, const int32_t* machGpu,
    int32_t machCount,
    int32_t* outAssignedMachine, double* outWaitMinutes,
    GsdScheduleResult* outSummary);

/* Power-law retention fit + LTV projection. */
GSD_API void GSD_CALL gsd_project_ltv(
    const int32_t* days, const double* fractions, int32_t n,
    double arpdau, int32_t horizonDays, GsdLtvResult* out);

#ifdef __cplusplus
}
#endif

#endif /* GSD_API_H */
