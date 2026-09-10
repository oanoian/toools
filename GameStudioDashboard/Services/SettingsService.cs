using System.IO;
using System.Text.Json;
using GameStudioDashboard.Infrastructure;

namespace GameStudioDashboard.Services;

public class AppSettings : ObservableObject
{
    private int _targetFps = 60;
    public int TargetFps { get => _targetFps; set => SetProperty(ref _targetFps, value); }

    private int _refreshIntervalSeconds = 3;
    public int RefreshIntervalSeconds { get => _refreshIntervalSeconds; set => SetProperty(ref _refreshIntervalSeconds, value); }

    private bool _compactDensity;
    public bool CompactDensity { get => _compactDensity; set => SetProperty(ref _compactDensity, value); }

    private bool _liveTelemetry = true;
    public bool LiveTelemetry { get => _liveTelemetry; set => SetProperty(ref _liveTelemetry, value); }

    private double _crashAlertThresholdPct = 1.5;
    public double CrashAlertThresholdPct { get => _crashAlertThresholdPct; set => SetProperty(ref _crashAlertThresholdPct, value); }

    private double _utilizationAlertPct = 85;
    public double UtilizationAlertPct { get => _utilizationAlertPct; set => SetProperty(ref _utilizationAlertPct, value); }

    private string _studioName = "Nova Forge Studios";
    public string StudioName { get => _studioName; set => SetProperty(ref _studioName, value); }
}

public static class SettingsService
{
    private static readonly string Dir =
        Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), "GameStudioDashboard");

    private static readonly string FilePath = Path.Combine(Dir, "settings.json");

    private static readonly JsonSerializerOptions Options = new() { WriteIndented = true };

    public static AppSettings Load()
    {
        try
        {
            if (File.Exists(FilePath))
            {
                var json = File.ReadAllText(FilePath);
                var loaded = JsonSerializer.Deserialize<AppSettings>(json);
                if (loaded is not null) return loaded;
            }
        }
        catch
        {
            // Corrupt or unreadable settings fall back to defaults.
        }
        return new AppSettings();
    }

    public static void Save(AppSettings settings)
    {
        try
        {
            Directory.CreateDirectory(Dir);
            File.WriteAllText(FilePath, JsonSerializer.Serialize(settings, Options));
        }
        catch
        {
            // Persistence is best-effort; ignore IO failures.
        }
    }
}
