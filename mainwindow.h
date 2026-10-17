#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <vector>

#include "Models/WeatherData.h"
#include "Models/ForecastData.h"
#include "Models/HourlyData.h"
#include "Services/WeatherService.h"
#include "Models/CitySuggestion.h"

#include <QScrollArea>
#include <QHBoxLayout>
#include <QDate>
#include <QPixmap>

#include <QTimer>
#include <QCompleter>
#include <QStringListModel>

#include <QStackedWidget>

#include <QVBoxLayout>
#include <QChartView>
#include <QChart>
#include <QLineSeries>
#include <QBarCategoryAxis>
#include <QValueAxis>
#include <QRadioButton>
#include <QListWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onSearchClicked();
    void onRefreshClicked();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    Ui::MainWindow *ui;
    WeatherService m_weatherService;
    CitySuggestion m_currentLocation;
    std::vector<CitySuggestion> m_suggestions;

    std::vector<ForecastData> m_forecastData;
    std::vector<HourlyData> m_hourlyData;

    QScrollArea *m_hourlyScrollArea = nullptr;
    QHBoxLayout *m_hourlyLayout = nullptr;

    QTimer *m_suggestTimer = nullptr;
    QCompleter *m_completer = nullptr;
    QStringListModel *m_suggestModel = nullptr;
    QStackedWidget *m_pages = nullptr;
    QVBoxLayout *m_forecastListLayout = nullptr;
    QChartView *m_chartView = nullptr;
    QLineSeries *m_maxSeries = nullptr;
    QLineSeries *m_minSeries = nullptr;
    QBarCategoryAxis *m_axisX = nullptr;
    QValueAxis *m_axisY = nullptr;
    bool m_useFahrenheit = false;
    bool m_useMph = false;
    QListWidget *m_favoritesList = nullptr;
    QRadioButton *m_celsiusBtn = nullptr;
    QRadioButton *m_fahrenheitBtn = nullptr;
    QRadioButton *m_kmhBtn = nullptr;
    QRadioButton *m_mphBtn = nullptr;
    WeatherData m_currentWeather;

    void setupSettingsPage(QWidget *page);
    double convertTemp(double celsius) const;
    QString unitSymbol() const;
    void updateForecastPage();
    void setupSidebar();
    void setupCitySuggestions();
    void updateSuggestions();
    void setupAnalyticsChart();
    void updateAnalyticsPage();
    void loadSettings();
    void saveSettings();
    void addCurrentToFavorites();
    void removeSelectedFavorite();
    QString windText(double kmh) const;

    void fetchAndDisplay(const CitySuggestion &city);
    void searchAndDisplay(const QString &typedName);
    void updateUi(const WeatherData &current, const std::vector<ForecastData> &forecast, const std::vector<HourlyData> &hourly);
    void setupHourlyStrip();
    void setupDayCardClicks();
    void showRollingHours();
    void showHoursForDate(const QDate &date);
    void renderHourlyCards(const std::vector<HourlyData> &hours, bool highlightNow);
    QPixmap createWeatherIcon(int code, bool isDay, int sizePx) const;
    void setLoadingState(bool loading);
};
#endif // MAINWINDOW_H