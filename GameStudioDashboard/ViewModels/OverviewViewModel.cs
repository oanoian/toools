using LiveChartsCore;
using LiveChartsCore.SkiaSharpView;
using LiveChartsCore.SkiaSharpView.Painting;
using SkiaSharp;
using GameStudioDashboard.Services;

namespace GameStudioDashboard.ViewModels;

public class OverviewViewModel : ViewModelBase
{
    public override string Title => "Overview";
    public override string Glyph => ""; // Home

    public OverviewViewModel(StudioContext studio) : base(studio)
    {
        var report = Algorithms.AnalyzeFrameBudget(Studio.FrameTimes.ToList(), Studio.Settings.TargetFps);
        var sched = Algorithms.Schedule(Studio.Builds.ToList(), Studio.Machines.ToList());
        var ltv = Algorithms.ProjectLtv(Studio.RetentionCurve.ToList(), Studio.LiveOps[^1].Arpdau);

        HealthyBuildsPct = Studio.Builds.Count == 0 ? 0
            : 100.0 * Studio.Builds.Count(b => b.HealthScore >= 70) / Studio.Builds.Count;
        AvgBuildMinutes = Studio.Builds.Average(b => b.DurationMinutes);
        FarmUtilization = sched.AvgCpuUtilization;
        AvgFps = report.AvgFps;
        Dau = Studio.LiveOps[^1].Dau;
        ProjectedLtv = ltv.ProjectedLtv;
        ProjectedD30 = ltv.ProjectedD30Retention * 100;

        DauSeries = new ISeries[]
        {
            new LineSeries<int>
            {
                Values = Studio.LiveOps.Select(x => x.Dau).ToArray(),
                GeometrySize = 0,
                Stroke = new SolidColorPaint(new SKColor(0x4E, 0xA1, 0xFF)) { StrokeThickness = 2.5f },
                Fill = new SolidColorPaint(new SKColor(0x4E, 0xA1, 0xFF, 32)),
                LineSmoothness = 0.6,
            }
        };

        var passed = Studio.Builds.Count(b => b.Status == Models.BuildStatus.Passed);
        var failed = Studio.Builds.Count(b => b.Status == Models.BuildStatus.Failed);
        var active = Studio.Builds.Count - passed - failed;
        BuildMixSeries = new ISeries[]
        {
            new PieSeries<int> { Values = new[] { passed }, Name = "Passed", Fill = new SolidColorPaint(new SKColor(0x3D, 0xD6, 0x8C)) },
            new PieSeries<int> { Values = new[] { failed }, Name = "Failed", Fill = new SolidColorPaint(new SKColor(0xF5, 0x6C, 0x6C)) },
            new PieSeries<int> { Values = new[] { active }, Name = "Active", Fill = new SolidColorPaint(new SKColor(0xF5, 0xB1, 0x4C)) },
        };
    }

    public double HealthyBuildsPct { get; }
    public double AvgBuildMinutes { get; }
    public double FarmUtilization { get; }
    public double AvgFps { get; }
    public int Dau { get; }
    public double ProjectedLtv { get; }
    public double ProjectedD30 { get; }

    public ISeries[] DauSeries { get; }
    public ISeries[] BuildMixSeries { get; }

    public Axis[] HiddenAxes { get; } =
    {
        new Axis { IsVisible = false }
    };
}
