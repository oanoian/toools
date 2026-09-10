using System.Collections.ObjectModel;
using GameStudioDashboard.Infrastructure;
using GameStudioDashboard.Models;
using GameStudioDashboard.Services;

namespace GameStudioDashboard.ViewModels;

public class MilestoneForecast : ObservableObject
{
    public string Name { get; init; } = "";
    public DateTime TargetDate { get; init; }
    public double Progress { get; init; }

    private DateTime _projectedDate;
    public DateTime ProjectedDate { get => _projectedDate; set => SetProperty(ref _projectedDate, value); }

    private bool _atRisk;
    public bool AtRisk { get => _atRisk; set => SetProperty(ref _atRisk, value); }

    private double _velocityPerDay;
    public double VelocityPerDay { get => _velocityPerDay; set => SetProperty(ref _velocityPerDay, value); }

    public string ProjectedText => ProjectedDate == DateTime.MaxValue ? "no velocity" : ProjectedDate.ToString("dd MMM yyyy");
    public string StatusText => AtRisk ? "AT RISK" : "ON TRACK";
    public void Refresh() { OnPropertyChanged(nameof(ProjectedText)); OnPropertyChanged(nameof(StatusText)); }
}

public class ProjectViewModel : ViewModelBase
{
    public override string Title => "Project & Team";
    public override string Glyph => ""; // People

    public ObservableCollection<SprintTask> Todo { get; } = new();
    public ObservableCollection<SprintTask> InProgress { get; } = new();
    public ObservableCollection<SprintTask> Done { get; } = new();
    public ObservableCollection<TeamMember> Team { get; } = new();
    public ObservableCollection<MilestoneForecast> Milestones { get; } = new();

    private double _velocity;
    public double Velocity { get => _velocity; private set => SetProperty(ref _velocity, value); }

    private int _committedPoints;
    public int CommittedPoints { get => _committedPoints; private set => SetProperty(ref _committedPoints, value); }

    private int _donePoints;
    public int DonePoints { get => _donePoints; private set => SetProperty(ref _donePoints, value); }

    public RelayCommand AdvanceCommand { get; }

    public ProjectViewModel(StudioContext studio) : base(studio)
    {
        foreach (var t in Studio.Team) Team.Add(t);
        foreach (var m in Studio.Milestones)
            Milestones.Add(new MilestoneForecast { Name = m.Name, TargetDate = m.TargetDate, Progress = m.Progress });

        AdvanceCommand = new RelayCommand(p =>
        {
            if (p is SprintTask t)
            {
                t.State = t.State switch
                {
                    TaskState.Todo => TaskState.InProgress,
                    TaskState.InProgress => TaskState.Done,
                    _ => TaskState.Todo,
                };
                Rebuild();
            }
        });

        Rebuild();
    }

    private void Rebuild()
    {
        Todo.Clear(); InProgress.Clear(); Done.Clear();
        foreach (var t in Studio.Tasks)
        {
            switch (t.State)
            {
                case TaskState.Todo: Todo.Add(t); break;
                case TaskState.InProgress: InProgress.Add(t); break;
                case TaskState.Done: Done.Add(t); break;
            }
        }

        CommittedPoints = Studio.Tasks.Sum(t => t.Points);
        DonePoints = Studio.Tasks.Where(t => t.State == TaskState.Done).Sum(t => t.Points);

        // Blend historical velocities with this sprint's live completed points so
        // toggling a task actually moves the forecast.
        var velInputs = Studio.Velocities.Append((double)DonePoints).ToList();
        Velocity = Math.Round(Algorithms.ExponentialMovingAverage(velInputs, 0.4), 1);

        foreach (var (mf, m) in Milestones.Zip(Studio.Milestones))
        {
            var f = Algorithms.ForecastMilestone(m, velInputs, 14, Studio.Today);
            mf.ProjectedDate = f.ProjectedDate;
            mf.AtRisk = f.AtRisk;
            mf.VelocityPerDay = f.VelocityPerDay;
            mf.Refresh();
        }
    }
}
