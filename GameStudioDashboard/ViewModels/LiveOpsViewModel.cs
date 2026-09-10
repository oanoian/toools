using System.Windows.Threading;
using LiveChartsCore;
using LiveChartsCore.SkiaSharpView;
using LiveChartsCore.SkiaSharpView.Painting;
using SkiaSharp;
using GameStudioDashboard.Infrastructure;
using GameStudioDashboard.Models;
using GameStudioDashboard.Services;

namespace GameStudioDashboard.ViewModels;

public class LiveOpsViewModel : ViewModelBase
{
    public override string Title => "Live Ops";
    public override string Glyph => ""; // Globe

    public string[] Metrics { get; } = { "DAU", "Revenue", "Server Load" };

    private string _selectedMetric = "DAU";
    public string SelectedMetric
    {
        get => _selectedMetric;
        set { if (SetProperty(ref _selectedMetric, value)) BuildSeries(); }
    }

    private ISeries[] _series = Array.Empty<ISeries>();
    public ISeries[] Series { get => _series; private set => SetProperty(ref _series, value); }

    public ISeries[] RetentionSeries { get; }
    public Axis[] XAxes { get; }
    public Axis[] YAxes { get; }
    public Axis[] RetentionXAxes { get; }
    public Axis[] RetentionYAxes { get; }

    public int CurrentDau { get; private set; }
    public double CurrentServerLoad { get; private set; }
    public double DailyRevenue { get; private set; }
    public double ProjectedLtv { get; }
    public double DecayLambda { get; }
    public double ProjectedD30 { get; }

    private readonly SolidColorPaint _muted = new(new SKColor(0x8A, 0x93, 0xA3));
    private readonly SolidColorPaint _grid = new(new SKColor(0x1F, 0x25, 0x30));
    private readonly DispatcherTimer _timer;

    public LiveOpsViewModel(StudioContext studio) : base(studio)
    {
        var ltv = Algorithms.ProjectLtv(Studio.RetentionCurve.ToList(), Studio.LiveOps[^1].Arpdau);
        ProjectedLtv = ltv.ProjectedLtv;
        DecayLambda = ltv.DecayLambda;
        ProjectedD30 = Math.Round(ltv.ProjectedD30Retention * 100, 1);

        XAxes = new[] { new Axis { Labels = Studio.LiveOps.Select(x => x.Date.ToString("dd/MM")).ToArray(), LabelsPaint = _muted, TextSize = 10 } };
        YAxes = new[] { new Axis { LabelsPaint = _muted, TextSize = 11, SeparatorsPaint = _grid } };

        // Fitted retention curve overlaid on observed points.
        var fitted = Enumerable.Range(1, 30)
            .Select(d => Math.Round(Math.Clamp(ProjectRetention(ltv, d), 0, 1) * 100, 2)).ToArray();
        RetentionSeries = new ISeries[]
        {
            new LineSeries<double>
            {
                Values = fitted, GeometrySize = 0, LineSmoothness = 0.4,
                Stroke = new SolidColorPaint(new SKColor(0x3D, 0xD6, 0x8C)) { StrokeThickness = 2.5f },
                Fill = new SolidColorPaint(new SKColor(0x3D, 0xD6, 0x8C, 28)),
                Name = "Fitted retention %",
            }
        };
        RetentionXAxes = new[] { new Axis { Name = "Day", LabelsPaint = _muted, TextSize = 10, NamePaint = _muted } };
        RetentionYAxes = new[] { new Axis { LabelsPaint = _muted, TextSize = 10, SeparatorsPaint = _grid } };

        _timer = new DispatcherTimer();
        _timer.Tick += (_, _) => Tick();

        UpdateHeadline();
        BuildSeries();
    }

    private static double ProjectRetention(Algorithms.LtvProjection ltv, int day)
        => Math.Pow(day, -ltv.DecayLambda) * (ltv.ProjectedD30Retention / Math.Pow(30, -ltv.DecayLambda));

    private void UpdateHeadline()
    {
        var last = Studio.LiveOps[^1];
        CurrentDau = last.Dau;
        CurrentServerLoad = last.ServerLoadPct;
        DailyRevenue = Math.Round(last.Dau * last.Arpdau, 0);
        OnPropertyChanged(nameof(CurrentDau));
        OnPropertyChanged(nameof(CurrentServerLoad));
        OnPropertyChanged(nameof(DailyRevenue));
    }

    private void BuildSeries()
    {
        SKColor color = SelectedMetric switch
        {
            "Revenue" => new SKColor(0x3D, 0xD6, 0x8C),
            "Server Load" => new SKColor(0xF5, 0xB1, 0x4C),
            _ => new SKColor(0x4E, 0xA1, 0xFF),
        };
        double[] values = SelectedMetric switch
        {
            "Revenue" => Studio.LiveOps.Select(x => Math.Round(x.Dau * x.Arpdau, 0)).ToArray(),
            "Server Load" => Studio.LiveOps.Select(x => x.ServerLoadPct).ToArray(),
            _ => Studio.LiveOps.Select(x => (double)x.Dau).ToArray(),
        };
        Series = new ISeries[]
        {
            new LineSeries<double>
            {
                Values = values, GeometrySize = 0, LineSmoothness = 0.55,
                Stroke = new SolidColorPaint(color) { StrokeThickness = 2.5f },
                Fill = new SolidColorPaint(color.WithAlpha(30)),
                Name = SelectedMetric,
            }
        };
    }

    private void Tick()
    {
        var last = Studio.LiveOps[^1];
        int newDau = Math.Max(20000, last.Dau + Random.Shared.Next(-1200, 1400));
        Studio.LiveOps.Add(new LiveOpsSample
        {
            Date = last.Date.AddDays(1),
            Dau = newDau,
            RetentionD1 = last.RetentionD1,
            Arpdau = Math.Round(last.Arpdau + (Random.Shared.NextDouble() - 0.5) * 0.01, 3),
            ServerLoadPct = Math.Round(Math.Clamp(last.ServerLoadPct + (Random.Shared.NextDouble() - 0.5) * 8, 20, 99), 1),
        });
        if (Studio.LiveOps.Count > 30) Studio.LiveOps.RemoveAt(0);

        XAxes[0].Labels = Studio.LiveOps.Select(x => x.Date.ToString("dd/MM")).ToArray();
        UpdateHeadline();
        BuildSeries();
    }

    public override void OnActivated()
    {
        if (Studio.Settings.LiveTelemetry)
        {
            _timer.Interval = TimeSpan.FromSeconds(Math.Max(1, Studio.Settings.RefreshIntervalSeconds));
            _timer.Start();
        }
    }

    public override void OnDeactivated() => _timer.Stop();
}
