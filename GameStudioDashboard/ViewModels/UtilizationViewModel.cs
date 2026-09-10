using System.Collections.ObjectModel;
using System.Windows.Threading;
using LiveChartsCore;
using LiveChartsCore.SkiaSharpView;
using LiveChartsCore.SkiaSharpView.Painting;
using SkiaSharp;
using GameStudioDashboard.Infrastructure;
using GameStudioDashboard.Models;
using GameStudioDashboard.Services;

namespace GameStudioDashboard.ViewModels;

public class UtilizationViewModel : ViewModelBase
{
    public override string Title => "Resource Utilization";
    public override string Glyph => ""; // Processor

    public ObservableCollection<Machine> Machines { get; } = new();

    private double _avgCpu;
    public double AvgCpu { get => _avgCpu; private set => SetProperty(ref _avgCpu, value); }

    private double _idleCapacity;
    public double IdleCapacity { get => _idleCapacity; private set => SetProperty(ref _idleCapacity, value); }

    private int _scheduled;
    public int Scheduled { get => _scheduled; private set => SetProperty(ref _scheduled, value); }

    private int _queued;
    public int Queued { get => _queued; private set => SetProperty(ref _queued, value); }

    private string _alert = "";
    public string Alert { get => _alert; private set => SetProperty(ref _alert, value); }

    private ISeries[] _utilizationSeries = Array.Empty<ISeries>();
    public ISeries[] UtilizationSeries { get => _utilizationSeries; private set => SetProperty(ref _utilizationSeries, value); }

    public Axis[] XAxes { get; }
    public Axis[] YAxes { get; }

    public RelayCommand QueueJobCommand { get; }
    public RelayCommand RebalanceCommand { get; }

    private readonly DispatcherTimer _timer;
    private int _syntheticId = 9000;

    public UtilizationViewModel(StudioContext studio) : base(studio)
    {
        foreach (var m in Studio.Machines) Machines.Add(m);

        XAxes = new[] { new Axis { Labels = Machines.Select(m => m.Name.Split(' ')[0]).ToArray(), LabelsPaint = new SolidColorPaint(new SKColor(0x8A, 0x93, 0xA3)), TextSize = 11 } };
        YAxes = new[] { new Axis { MinLimit = 0, MaxLimit = 100, LabelsPaint = new SolidColorPaint(new SKColor(0x8A, 0x93, 0xA3)), TextSize = 11, SeparatorsPaint = new SolidColorPaint(new SKColor(0x1F, 0x25, 0x30)) } };

        QueueJobCommand = new RelayCommand(QueueJob);
        RebalanceCommand = new RelayCommand(Rebalance);

        _timer = new DispatcherTimer();
        _timer.Tick += (_, _) => Rebalance();

        Rebalance();
    }

    private void QueueJob()
    {
        Studio.Builds.Add(new BuildJob
        {
            Id = $"#{_syntheticId++}",
            Branch = "feature/adhoc",
            Author = "scheduler",
            Platform = "Windows",
            Commit = Guid.NewGuid().ToString("N")[..7],
            Status = BuildStatus.Queued,
            DurationMinutes = 18,
            TotalTests = 2000,
            RequiredCores = new[] { 8, 12, 16 }[Random.Shared.Next(3)],
            RequiredRamGb = new[] { 16, 32 }[Random.Shared.Next(2)],
            RequiresGpu = Random.Shared.Next(2) == 0,
            StartedAt = Studio.Today,
        });
        Rebalance();
    }

    private void Rebalance()
    {
        var result = Algorithms.Schedule(Studio.Builds.ToList(), Studio.Machines.ToList());
        AvgCpu = result.AvgCpuUtilization;
        IdleCapacity = result.IdleCapacityPct;
        Scheduled = result.Scheduled;
        Queued = result.Queued;

        var threshold = Studio.Settings.UtilizationAlertPct;
        var hot = Machines.Where(m => m.CpuUtilization * 100 >= threshold).Select(m => m.Name).ToList();
        Alert = hot.Count > 0
            ? $"{hot.Count} machine(s) over {threshold:0}% utilization"
            : "All machines within healthy utilization";

        UtilizationSeries = new ISeries[]
        {
            new ColumnSeries<double>
            {
                Values = Machines.Select(m => Math.Round(m.CpuUtilization * 100, 1)).ToArray(),
                Name = "CPU %",
                Fill = new SolidColorPaint(new SKColor(0x4E, 0xA1, 0xFF)),
            },
            new ColumnSeries<double>
            {
                Values = Machines.Select(m => Math.Round(m.RamUtilization * 100, 1)).ToArray(),
                Name = "RAM %",
                Fill = new SolidColorPaint(new SKColor(0xB0, 0x7C, 0xFF)),
            },
        };
        // Schedule() already updated each machine's usage counters, which raised
        // the CpuUtilization / RamUtilization change notifications for the bars.
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
