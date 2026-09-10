using GameStudioDashboard.Models;

namespace GameStudioDashboard.Services;

/// <summary>
/// Deterministic seeded studio data (project "Aetherbound"). Numbers are
/// plausible rather than random so the dashboard reads like a real snapshot.
/// </summary>
public class MockDataService
{
    private readonly Random _rng = new(20260910);
    public DateTime Today { get; } = new(2026, 9, 10);

    public List<BuildJob> CreateBuilds()
    {
        var authors = new[] { "M. Okafor", "L. Tanaka", "S. Petrov", "A. Reyes", "J. Halden", "N. Bergström" };
        var branches = new[] { "main", "feature/nav-mesh", "feature/ray-lighting", "hotfix/save-corruption", "release/1.4" };
        var platforms = new[] { "Windows", "PS5", "Xbox Series X", "Switch 2" };
        var list = new List<BuildJob>();

        for (int i = 0; i < 14; i++)
        {
            int total = 1800 + _rng.Next(0, 900);
            int failed = i % 5 == 0 ? _rng.Next(3, 40) : _rng.Next(0, 6);
            var status = i switch
            {
                0 => BuildStatus.Running,
                1 => BuildStatus.Queued,
                2 => BuildStatus.Queued,
                _ => failed > 20 ? BuildStatus.Failed : BuildStatus.Passed
            };

            list.Add(new BuildJob
            {
                Id = $"#{4820 - i}",
                Branch = branches[i % branches.Length],
                Author = authors[i % authors.Length],
                Platform = platforms[i % platforms.Length],
                Commit = Guid.NewGuid().ToString("N")[..7],
                Status = status,
                DurationMinutes = Math.Round(14 + _rng.NextDouble() * 22, 1),
                TotalTests = total,
                FailedTests = failed,
                FlakyTests = _rng.Next(0, 12),
                CrashRatePct = Math.Round(_rng.NextDouble() * 3.0, 2),
                StartedAt = Today.AddHours(-i * 1.7),
                RequiredCores = new[] { 4, 8, 12, 16 }[_rng.Next(4)],
                RequiredRamGb = new[] { 8, 16, 32 }[_rng.Next(3)],
                RequiresGpu = platforms[i % platforms.Length] != "Switch 2" && i % 3 == 0,
            });
        }
        return list;
    }

    public List<Machine> CreateMachines() => new()
    {
        new Machine { Name = "farm-01 (Ryzen 9)", Cores = 32, RamGb = 128, HasGpu = true },
        new Machine { Name = "farm-02 (Ryzen 9)", Cores = 32, RamGb = 128, HasGpu = true },
        new Machine { Name = "farm-03 (Threadripper)", Cores = 64, RamGb = 256, HasGpu = true },
        new Machine { Name = "farm-04 (Xeon)", Cores = 24, RamGb = 96, HasGpu = false },
        new Machine { Name = "farm-05 (M3 Ultra)", Cores = 24, RamGb = 128, HasGpu = true },
    };

    public List<TeamMember> CreateTeam() => new()
    {
        new TeamMember { Name = "Maya Okafor", Role = "Gameplay", AssignedPoints = 21, CapacityPoints = 24 },
        new TeamMember { Name = "Liang Tanaka", Role = "Rendering", AssignedPoints = 26, CapacityPoints = 24 },
        new TeamMember { Name = "Sofia Petrov", Role = "Art", AssignedPoints = 18, CapacityPoints = 22 },
        new TeamMember { Name = "Andre Reyes", Role = "Design", AssignedPoints = 14, CapacityPoints = 20 },
        new TeamMember { Name = "Jae Halden", Role = "QA", AssignedPoints = 20, CapacityPoints = 22 },
        new TeamMember { Name = "Nils Bergström", Role = "Audio", AssignedPoints = 9, CapacityPoints = 16 },
    };

    public List<SprintTask> CreateTasks()
    {
        var data = new (string title, string who, int pts, string disc, TaskState st)[]
        {
            ("Nav-mesh regeneration on streamed chunks", "Maya Okafor", 8, "Gameplay", TaskState.InProgress),
            ("Ray-traced GI denoiser pass", "Liang Tanaka", 13, "Rendering", TaskState.InProgress),
            ("Boss arena set dressing", "Sofia Petrov", 5, "Art", TaskState.Done),
            ("Difficulty curve rebalance", "Andre Reyes", 5, "Design", TaskState.Todo),
            ("Save-corruption repro harness", "Jae Halden", 8, "QA", TaskState.InProgress),
            ("Adaptive combat music layers", "Nils Bergström", 3, "Audio", TaskState.Todo),
            ("Inventory drag-drop polish", "Maya Okafor", 3, "Gameplay", TaskState.Done),
            ("Foliage LOD popping fix", "Liang Tanaka", 5, "Rendering", TaskState.Todo),
            ("Localization string audit", "Andre Reyes", 2, "Design", TaskState.Done),
            ("Controller haptics mapping", "Jae Halden", 3, "QA", TaskState.Todo),
        };
        return data.Select((d, i) => new SprintTask
        {
            Id = $"AET-{318 + i}", Title = d.title, Assignee = d.who,
            Points = d.pts, Discipline = d.disc, State = d.st
        }).ToList();
    }

    public List<Milestone> CreateMilestones() => new()
    {
        new Milestone { Name = "Vertical Slice", TargetDate = new(2026, 10, 15), TotalPoints = 180, DonePoints = 132 },
        new Milestone { Name = "Content Complete", TargetDate = new(2026, 12, 20), TotalPoints = 420, DonePoints = 96 },
        new Milestone { Name = "Gold Master", TargetDate = new(2027, 3, 1), TotalPoints = 640, DonePoints = 40 },
    };

    public List<double> RecentVelocities() => new() { 42, 48, 45, 51, 47, 53 };

    public List<double> CreateFrameTimes(int n = 600)
    {
        // ~90fps baseline with occasional spikes (streaming hitches).
        var list = new List<double>(n);
        for (int i = 0; i < n; i++)
        {
            double baseMs = 11.1 + _rng.NextDouble() * 3.5;
            if (_rng.NextDouble() < 0.06) baseMs += 8 + _rng.NextDouble() * 22; // hitch
            list.Add(Math.Round(baseMs, 2));
        }
        return list;
    }

    public List<LiveOpsSample> CreateLiveOps(int days = 30)
    {
        var list = new List<LiveOpsSample>();
        int dau = 42000;
        for (int i = days - 1; i >= 0; i--)
        {
            dau += _rng.Next(-1500, 2600);
            list.Add(new LiveOpsSample
            {
                Date = Today.AddDays(-i),
                Dau = Math.Max(20000, dau),
                RetentionD1 = Math.Round(0.38 + _rng.NextDouble() * 0.06, 3),
                Arpdau = Math.Round(0.22 + _rng.NextDouble() * 0.08, 3),
                ServerLoadPct = Math.Round(45 + _rng.NextDouble() * 40, 1),
            });
        }
        return list;
    }

    public List<RetentionPoint> CreateRetentionCurve() => new()
    {
        new(1, 0.41), new(3, 0.28), new(7, 0.19), new(14, 0.13), new(21, 0.10),
    };
}
