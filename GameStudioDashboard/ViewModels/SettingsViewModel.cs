using GameStudioDashboard.Infrastructure;
using GameStudioDashboard.Services;

namespace GameStudioDashboard.ViewModels;

public class SettingsViewModel : ViewModelBase
{
    public override string Title => "Settings";
    public override string Glyph => ""; // Settings gear

    public AppSettings Settings => Studio.Settings;

    public string BackendStatus => NativeBridge.Status;
    public bool NativeActive => NativeBridge.Available;
    public bool NativeInactive => !NativeBridge.Available;   // for the risk-brush dot (red when off)

    public int[] FpsTargets { get; } = { 30, 60, 120, 144 };

    private string _saveStatus = "";
    public string SaveStatus { get => _saveStatus; private set => SetProperty(ref _saveStatus, value); }

    // Live preview of the render-budget analyzer against the chosen FPS target,
    // proving settings actually drive the algorithms.
    public double PctOverBudget { get; private set; }
    public double P95 { get; private set; }
    public double P99 { get; private set; }
    public double TargetMs { get; private set; }

    public RelayCommand SaveCommand { get; }

    public SettingsViewModel(StudioContext studio) : base(studio)
    {
        SaveCommand = new RelayCommand(() =>
        {
            SettingsService.Save(Settings);
            SaveStatus = $"Saved to %AppData% at {DateTime.Now:HH:mm:ss}";
        });

        Settings.PropertyChanged += (_, e) =>
        {
            if (e.PropertyName == nameof(AppSettings.TargetFps)) RecomputePreview();
            SaveStatus = "Unsaved changes";
        };

        RecomputePreview();
    }

    private void RecomputePreview()
    {
        var report = Algorithms.AnalyzeFrameBudget(Studio.FrameTimes.ToList(), Settings.TargetFps);
        PctOverBudget = Math.Round(report.PctOverBudget, 1);
        P95 = report.P95;
        P99 = report.P99;
        TargetMs = Math.Round(report.TargetMs, 2);
        OnPropertyChanged(nameof(PctOverBudget));
        OnPropertyChanged(nameof(P95));
        OnPropertyChanged(nameof(P99));
        OnPropertyChanged(nameof(TargetMs));
    }
}
