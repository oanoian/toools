using System.Collections.ObjectModel;
using GameStudioDashboard.Models;

namespace GameStudioDashboard.Services;

/// <summary>
/// Shared, in-memory studio state. Created once and handed to every ViewModel
/// so views operate on the same live collections and settings object.
/// </summary>
public class StudioContext
{
    private readonly MockDataService _data = new();

    public AppSettings Settings { get; }
    public DateTime Today => _data.Today;

    public ObservableCollection<BuildJob> Builds { get; }
    public ObservableCollection<Machine> Machines { get; }
    public ObservableCollection<TeamMember> Team { get; }
    public ObservableCollection<SprintTask> Tasks { get; }
    public ObservableCollection<Milestone> Milestones { get; }
    public ObservableCollection<LiveOpsSample> LiveOps { get; }

    public IReadOnlyList<double> Velocities { get; }
    public IReadOnlyList<double> FrameTimes { get; }
    public IReadOnlyList<RetentionPoint> RetentionCurve { get; }

    public StudioContext()
    {
        Settings = SettingsService.Load();
        Builds = new(_data.CreateBuilds());
        Machines = new(_data.CreateMachines());
        Team = new(_data.CreateTeam());
        Tasks = new(_data.CreateTasks());
        Milestones = new(_data.CreateMilestones());
        LiveOps = new(_data.CreateLiveOps());
        Velocities = _data.RecentVelocities();
        FrameTimes = _data.CreateFrameTimes();
        RetentionCurve = _data.CreateRetentionCurve();

        RecomputeBuildHealth();
    }

    public double DurationBaseline =>
        Algorithms.ExponentialMovingAverage(Builds.Select(b => b.DurationMinutes));

    public void RecomputeBuildHealth()
    {
        double baseline = DurationBaseline;
        foreach (var b in Builds)
            b.HealthScore = Algorithms.BuildHealthScore(b, baseline);
    }
}
