using GameStudioDashboard.Infrastructure;
using GameStudioDashboard.Services;

namespace GameStudioDashboard.ViewModels;

public abstract class ViewModelBase : ObservableObject
{
    protected readonly StudioContext Studio;
    public abstract string Title { get; }
    public abstract string Glyph { get; }   // Segoe MDL2 Assets glyph

    protected ViewModelBase(StudioContext studio) => Studio = studio;

    // Called when the view becomes active / inactive so timers can start / stop.
    public virtual void OnActivated() { }
    public virtual void OnDeactivated() { }
}
