using System.Runtime.InteropServices;

namespace GameStudioDashboard.Native;

// Layouts mirror the POD structs in native/include/gsd_api.h. All doubles come
// first, then ints, so Sequential layout with default 8-byte packing matches the
// C compiler's layout on x64/arm64.

[StructLayout(LayoutKind.Sequential)]
public struct GsdFrameBudgetReport
{
    public double P50;
    public double P95;
    public double P99;
    public double PctOverBudget;
    public double TargetMs;
    public double AvgFps;
    public int SampleCount;
    public int Pad;
}

[StructLayout(LayoutKind.Sequential)]
public struct GsdBuildInput
{
    public double CrashRatePct;
    public double DurationMinutes;
    public int TotalTests;
    public int FailedTests;
    public int FlakyTests;
    public int StatusFailed;
}

[StructLayout(LayoutKind.Sequential)]
public struct GsdLtvResult
{
    public double DecayLambda;
    public double ProjectedD30;
    public double ProjectedLtv;
}

[StructLayout(LayoutKind.Sequential)]
public struct GsdScheduleResult
{
    public double AvgCpuUtilization;
    public double IdleCapacityPct;
    public int Scheduled;
    public int Queued;
}

/// <summary>
/// Raw P/Invoke declarations for GameStudioNative.dll. The library name has no
/// extension so the runtime resolves the platform file (.dll/.so/.dylib).
/// </summary>
internal static class NativeInterop
{
    public const int ExpectedAbi = 1;
    private const string Lib = "GameStudioNative";
    private const CallingConvention Conv = CallingConvention.Cdecl;

    [DllImport(Lib, CallingConvention = Conv)]
    public static extern int gsd_abi_version();

    [DllImport(Lib, CallingConvention = Conv)]
    public static extern double gsd_ewma(double[] values, int n, double alpha);

    [DllImport(Lib, CallingConvention = Conv)]
    public static extern void gsd_analyze_frame_budget(
        double[] frameTimesMs, int n, int targetFps, out GsdFrameBudgetReport outReport);

    [DllImport(Lib, CallingConvention = Conv)]
    public static extern double gsd_build_health_score(in GsdBuildInput input, double durationBaseline);

    [DllImport(Lib, CallingConvention = Conv)]
    public static extern void gsd_schedule(
        int[] jobCores, int[] jobRam, int[] jobGpu, double[] jobDurationMin, int jobCount,
        int[] machCores, int[] machRam, int[] machGpu, int machCount,
        int[] outAssignedMachine, double[] outWaitMinutes, out GsdScheduleResult outSummary);

    [DllImport(Lib, CallingConvention = Conv)]
    public static extern void gsd_project_ltv(
        int[] days, double[] fractions, int n, double arpdau, int horizonDays, out GsdLtvResult outResult);
}
