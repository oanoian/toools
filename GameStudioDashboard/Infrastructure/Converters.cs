using System.Globalization;
using System.Windows;
using System.Windows.Data;
using System.Windows.Media;
using GameStudioDashboard.Models;

namespace GameStudioDashboard.Infrastructure;

// 0..1 fraction -> percentage width fraction of a given container (via ConverterParameter width).
public class FractionToWidthConverter : IValueConverter
{
    public object Convert(object value, Type t, object parameter, CultureInfo c)
    {
        double frac = System.Convert.ToDouble(value);
        double max = parameter is null ? 100 : System.Convert.ToDouble(parameter, CultureInfo.InvariantCulture);
        return Math.Clamp(frac, 0, 1) * max;
    }
    public object ConvertBack(object v, Type t, object p, CultureInfo c) => throw new NotSupportedException();
}

// percent value already 0..100 -> width fraction of parameter.
public class PercentToWidthConverter : IValueConverter
{
    public object Convert(object value, Type t, object parameter, CultureInfo c)
    {
        double pct = System.Convert.ToDouble(value);
        double max = parameter is null ? 100 : System.Convert.ToDouble(parameter, CultureInfo.InvariantCulture);
        return Math.Clamp(pct, 0, 100) / 100.0 * max;
    }
    public object ConvertBack(object v, Type t, object p, CultureInfo c) => throw new NotSupportedException();
}

public class BuildStatusToBrushConverter : IValueConverter
{
    public object Convert(object value, Type t, object parameter, CultureInfo c) => value switch
    {
        BuildStatus.Passed => new SolidColorBrush(Color.FromRgb(0x3D, 0xD6, 0x8C)),
        BuildStatus.Failed => new SolidColorBrush(Color.FromRgb(0xF5, 0x6C, 0x6C)),
        BuildStatus.Running => new SolidColorBrush(Color.FromRgb(0x4E, 0xA1, 0xFF)),
        BuildStatus.Queued => new SolidColorBrush(Color.FromRgb(0xF5, 0xB1, 0x4C)),
        _ => new SolidColorBrush(Color.FromRgb(0x8A, 0x93, 0xA3)),
    };
    public object ConvertBack(object v, Type t, object p, CultureInfo c) => throw new NotSupportedException();
}

// Health score (0..100) -> traffic-light brush.
public class HealthToBrushConverter : IValueConverter
{
    public object Convert(object value, Type t, object parameter, CultureInfo c)
    {
        double h = System.Convert.ToDouble(value);
        return h >= 80 ? new SolidColorBrush(Color.FromRgb(0x3D, 0xD6, 0x8C))
             : h >= 55 ? new SolidColorBrush(Color.FromRgb(0xF5, 0xB1, 0x4C))
             : new SolidColorBrush(Color.FromRgb(0xF5, 0x6C, 0x6C));
    }
    public object ConvertBack(object v, Type t, object p, CultureInfo c) => throw new NotSupportedException();
}

// Utilization fraction (0..1) -> brush, red when hot.
public class UtilizationToBrushConverter : IValueConverter
{
    public object Convert(object value, Type t, object parameter, CultureInfo c)
    {
        double f = System.Convert.ToDouble(value);
        return f >= 0.85 ? new SolidColorBrush(Color.FromRgb(0xF5, 0x6C, 0x6C))
             : f >= 0.6 ? new SolidColorBrush(Color.FromRgb(0xF5, 0xB1, 0x4C))
             : new SolidColorBrush(Color.FromRgb(0x4E, 0xA1, 0xFF));
    }
    public object ConvertBack(object v, Type t, object p, CultureInfo c) => throw new NotSupportedException();
}

public class BoolToRiskBrushConverter : IValueConverter
{
    public object Convert(object value, Type t, object parameter, CultureInfo c)
        => (value is bool b && b)
            ? new SolidColorBrush(Color.FromRgb(0xF5, 0x6C, 0x6C))
            : new SolidColorBrush(Color.FromRgb(0x3D, 0xD6, 0x8C));
    public object ConvertBack(object v, Type t, object p, CultureInfo c) => throw new NotSupportedException();
}

// equality -> Visibility, used for filter chip highlight etc.
public class EqualityToBoolConverter : IValueConverter
{
    public object Convert(object value, Type t, object parameter, CultureInfo c)
        => Equals(value?.ToString(), parameter?.ToString());
    public object ConvertBack(object value, Type t, object parameter, CultureInfo c)
        => (value is bool b && b) ? parameter! : Binding.DoNothing;
}
