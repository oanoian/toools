using GameStudioDashboard.Infrastructure;

namespace GameStudioDashboard.Models;

public enum BuildStatus { Passed, Failed, Running, Queued }

public class BuildJob : ObservableObject
{
    public string Id { get; init; } = "";
    public string Branch { get; init; } = "";
    public string Author { get; init; } = "";
    public string Platform { get; init; } = "";      // Windows / PS5 / Xbox / Switch
    public string Commit { get; init; } = "";

    private BuildStatus _status;
    public BuildStatus Status { get => _status; set => SetProperty(ref _status, value); }

    public double DurationMinutes { get; init; }       // wall-clock build time
    public int TotalTests { get; init; }
    public int FailedTests { get; init; }
    public int FlakyTests { get; init; }
    public double CrashRatePct { get; init; }          // % sessions crashing on this build
    public DateTime StartedAt { get; init; }

    // resource demand for the scheduler
    public int RequiredCores { get; init; }
    public int RequiredRamGb { get; init; }
    public bool RequiresGpu { get; init; }

    private double _healthScore;
    public double HealthScore { get => _healthScore; set => SetProperty(ref _healthScore, value); }

    private string? _assignedMachine;
    public string? AssignedMachine { get => _assignedMachine; set => SetProperty(ref _assignedMachine, value); }

    private double _estWaitMinutes;
    public double EstWaitMinutes { get => _estWaitMinutes; set => SetProperty(ref _estWaitMinutes, value); }

    public double PassRate => TotalTests == 0 ? 1 : (double)(TotalTests - FailedTests) / TotalTests;
}

public class Machine : ObservableObject
{
    public string Name { get; init; } = "";
    public int Cores { get; init; }
    public int RamGb { get; init; }
    public bool HasGpu { get; init; }

    private int _usedCores;
    public int UsedCores { get => _usedCores; set { if (SetProperty(ref _usedCores, value)) OnPropertyChanged(nameof(CpuUtilization)); } }

    private int _usedRamGb;
    public int UsedRamGb { get => _usedRamGb; set { if (SetProperty(ref _usedRamGb, value)) OnPropertyChanged(nameof(RamUtilization)); } }

    private int _assignedJobs;
    public int AssignedJobs { get => _assignedJobs; set => SetProperty(ref _assignedJobs, value); }

    public double CpuUtilization => Cores == 0 ? 0 : (double)UsedCores / Cores;
    public double RamUtilization => RamGb == 0 ? 0 : (double)UsedRamGb / RamGb;

    public void Reset() { UsedCores = 0; UsedRamGb = 0; AssignedJobs = 0; }
}

public class TeamMember
{
    public string Name { get; init; } = "";
    public string Role { get; init; } = "";      // Gameplay / Rendering / Art / Design / QA / Audio
    public int AssignedPoints { get; init; }
    public int CapacityPoints { get; init; }
    public double Load => CapacityPoints == 0 ? 0 : (double)AssignedPoints / CapacityPoints;
}

public enum TaskState { Todo, InProgress, Done }

public class SprintTask : ObservableObject
{
    public string Id { get; init; } = "";
    public string Title { get; init; } = "";
    public string Assignee { get; init; } = "";
    public int Points { get; init; }
    public string Discipline { get; init; } = "";

    private TaskState _state;
    public TaskState State { get => _state; set => SetProperty(ref _state, value); }
}

public class Milestone
{
    public string Name { get; init; } = "";
    public DateTime TargetDate { get; init; }
    public int TotalPoints { get; init; }
    public int DonePoints { get; init; }
    public double Progress => TotalPoints == 0 ? 0 : (double)DonePoints / TotalPoints;
}

public class LiveOpsSample
{
    public DateTime Date { get; init; }
    public int Dau { get; init; }
    public double RetentionD1 { get; init; }   // 0..1
    public double Arpdau { get; init; }        // revenue per DAU
    public double ServerLoadPct { get; init; } // 0..100
}

// Cohort retention observation used by the LTV projector.
public record RetentionPoint(int Day, double Fraction);
