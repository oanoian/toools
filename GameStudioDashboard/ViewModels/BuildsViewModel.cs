using System.Collections.ObjectModel;
using LiveChartsCore;
using LiveChartsCore.SkiaSharpView;
using LiveChartsCore.SkiaSharpView.Painting;
using SkiaSharp;
using GameStudioDashboard.Infrastructure;
using GameStudioDashboard.Models;
using GameStudioDashboard.Services;

namespace GameStudioDashboard.ViewModels;

public class BuildsViewModel : ViewModelBase
{
    public override string Title => "Build Pipeline";
    public override string Glyph => ""; // Repair / build

    public ObservableCollection<BuildJob> Builds { get; } = new();
    public string[] Filters { get; } = { "All", "Passed", "Failed", "Running", "Queued" };

    private string _selectedFilter = "All";
    public string SelectedFilter
    {
        get => _selectedFilter;
        set { if (SetProperty(ref _selectedFilter, value)) ApplyFilter(); }
    }

    private bool _sortByHealth = true;
    public bool SortByHealth
    {
        get => _sortByHealth;
        set { if (SetProperty(ref _sortByHealth, value)) ApplyFilter(); }
    }

    public double PassRatePct { get; }
    public double AvgHealth { get; }
    public double DurationBaseline { get; }

    public ISeries[] DurationSeries { get; }
    public Axis[] XAxes { get; }
    public Axis[] YAxes { get; }

    public RelayCommand ToggleSortCommand { get; }

    public BuildsViewModel(StudioContext studio) : base(studio)
    {
        DurationBaseline = Math.Round(Studio.DurationBaseline, 1);
        var completed = Studio.Builds.Where(b => b.Status is BuildStatus.Passed or BuildStatus.Failed).ToList();
        PassRatePct = completed.Count == 0 ? 0
            : 100.0 * completed.Count(b => b.Status == BuildStatus.Passed) / completed.Count;
        AvgHealth = Studio.Builds.Count == 0 ? 0 : Studio.Builds.Average(b => b.HealthScore);

        var recent = Studio.Builds.OrderBy(b => b.StartedAt).ToList();
        DurationSeries = new ISeries[]
        {
            new ColumnSeries<double>
            {
                Values = recent.Select(b => b.DurationMinutes).ToArray(),
                Fill = new SolidColorPaint(new SKColor(0x4E, 0xA1, 0xFF)),
                Name = "Build minutes",
            }
        };
        XAxes = new[] { new Axis { Labels = recent.Select(b => b.Id).ToArray(), LabelsPaint = new SolidColorPaint(new SKColor(0x8A, 0x93, 0xA3)), TextSize = 11 } };
        YAxes = new[] { new Axis { LabelsPaint = new SolidColorPaint(new SKColor(0x8A, 0x93, 0xA3)), TextSize = 11, SeparatorsPaint = new SolidColorPaint(new SKColor(0x1F, 0x25, 0x30)) } };

        ToggleSortCommand = new RelayCommand(() => SortByHealth = !SortByHealth);
        ApplyFilter();
    }

    private void ApplyFilter()
    {
        IEnumerable<BuildJob> q = Studio.Builds;
        if (SelectedFilter != "All" && Enum.TryParse<BuildStatus>(SelectedFilter, out var st))
            q = q.Where(b => b.Status == st);

        q = SortByHealth ? q.OrderBy(b => b.HealthScore) : q.OrderByDescending(b => b.StartedAt);

        Builds.Clear();
        foreach (var b in q) Builds.Add(b);
    }
}
