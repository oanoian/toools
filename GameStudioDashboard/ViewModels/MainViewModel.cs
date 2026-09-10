using System.Collections.ObjectModel;
using GameStudioDashboard.Infrastructure;
using GameStudioDashboard.Services;

namespace GameStudioDashboard.ViewModels;

public class MainViewModel : ObservableObject
{
    private readonly StudioContext _studio = new();

    public ObservableCollection<ViewModelBase> Views { get; }
    public AppSettings Settings => _studio.Settings;
    public string StudioName => _studio.Settings.StudioName;

    public string Today => _studio.Today.ToString("ddd, dd MMM yyyy");

    private ViewModelBase _current = null!;
    public ViewModelBase Current
    {
        get => _current;
        set
        {
            var old = _current;
            if (SetProperty(ref _current, value))
            {
                old?.OnDeactivated();
                value?.OnActivated();
            }
        }
    }

    public RelayCommand NavigateCommand { get; }

    public MainViewModel()
    {
        Views = new ObservableCollection<ViewModelBase>
        {
            new OverviewViewModel(_studio),
            new BuildsViewModel(_studio),
            new UtilizationViewModel(_studio),
            new ProjectViewModel(_studio),
            new LiveOpsViewModel(_studio),
            new SettingsViewModel(_studio),
        };

        NavigateCommand = new RelayCommand(p =>
        {
            if (p is ViewModelBase vm) Current = vm;
        });

        Current = Views[0];
    }
}
