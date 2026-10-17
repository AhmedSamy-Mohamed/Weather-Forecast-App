#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMessageBox>

#include <QVBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QDateTime>
#include <QPainter>
#include <QMouseEvent>
#include "Parser/WeatherCodeMapper.h"
#include <QtMath>
#include <QScrollBar>
#include <QButtonGroup>
#include <QSettings>
#include <QListWidget>
#include <QPushButton>

static const char *kOrg = "WeatherlyApp";
static const char *kApp = "WeatherForecastApp";

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setupHourlyStrip();
    setupDayCardClicks();
    setupCitySuggestions();
    setupSidebar();

    connect(ui->searchButton, &QPushButton::clicked, this, &MainWindow::onSearchClicked);
    connect(ui->refreshButton, &QPushButton::clicked, this, &MainWindow::onRefreshClicked);

    // Restore saved settings, then open the last used city (Cairo on first run)
    loadSettings();
    QSettings s(kOrg, kApp);
    CitySuggestion last = CitySuggestion::fromStorage(s.value("lastLocation").toString());
    if (last.isValid()) {
        fetchAndDisplay(last);
    } else {
        searchAndDisplay("Cairo");
    }
}

MainWindow::~MainWindow()
{
    saveSettings();
    delete ui;
}

void MainWindow::onSearchClicked()
{
    QString city = ui->cityLineEdit->text().trimmed();
    if (city.isEmpty()) {
        return;
    }
    searchAndDisplay(city);
}

void MainWindow::onRefreshClicked()
{
    if (!m_currentLocation.isValid()) {
        return;
    }
    fetchAndDisplay(m_currentLocation);
}

void MainWindow::fetchAndDisplay(const CitySuggestion &city)
{
    setLoadingState(true);

    WeatherData current;
    std::vector<ForecastData> forecast;
    std::vector<HourlyData> hourly;

    bool ok = m_weatherService.fetchWeatherForLocation(city, current, forecast, hourly);

    setLoadingState(false);

    if (!ok) {
        QMessageBox::warning(this, "Error",
                             "Could not fetch weather for \"" + city.label() +
                                 "\". Check your internet connection.");
        return;
    }

    m_currentLocation = city;
    updateUi(current, forecast, hourly);
    saveSettings();
}

void MainWindow::searchAndDisplay(const QString &typedName)
{
    const QString name = typedName.section(',', 0, 0).trimmed();

    setLoadingState(true);
    const std::vector<CitySuggestion> found = m_weatherService.searchCities(name, 1);
    setLoadingState(false);

    if (found.empty()) {
        QMessageBox::warning(this, "Error",
                             "Could not find a city named \"" + name +
                                 "\". Check the name or your internet connection.");
        return;
    }
    fetchAndDisplay(found.front());
}

void MainWindow::updateUi(const WeatherData &current, const std::vector<ForecastData> &forecast, const std::vector<HourlyData> &hourly)
{
    m_currentWeather = current;
    m_forecastData = forecast;
    m_hourlyData = hourly;

    ui->cityNameLabel->setText(current.cityName);
    ui->temperatureLabel->setText(QString::number(convertTemp(current.temperature), 'f', 1) + unitSymbol());
    ui->conditionLabel->setText(current.condition);
    ui->humidityLabel->setText("Humidity: " + QString::number(current.humidity) + "%");
    ui->windLabel->setText("Wind: " + windText(current.windSpeed));
    ui->lastUpdatedLabel->setText("Last updated: " + current.lastUpdated);

    QLabel *dayLabels[7] = {
        ui->dayLabel_1, ui->dayLabel_2, ui->dayLabel_3, ui->dayLabel_4,
        ui->dayLabel_5, ui->dayLabel_6, ui->dayLabel_7
    };
    QLabel *tempLabels[7] = {
        ui->dayTempLabel_1, ui->dayTempLabel_2, ui->dayTempLabel_3, ui->dayTempLabel_4,
        ui->dayTempLabel_5, ui->dayTempLabel_6, ui->dayTempLabel_7
    };
    QLabel *iconLabels[7] = {
        ui->dayIconLabel_1, ui->dayIconLabel_2, ui->dayIconLabel_3, ui->dayIconLabel_4,
        ui->dayIconLabel_5, ui->dayIconLabel_6, ui->dayIconLabel_7
    };

    for (int i = 0; i < 7; ++i) {
        if (i < static_cast<int>(forecast.size())) {
            dayLabels[i]->setText(forecast[i].dayName);
            tempLabels[i]->setText(
                QString::number(convertTemp(forecast[i].tempMax), 'f', 0) + "° / " +
                QString::number(convertTemp(forecast[i].tempMin), 'f', 0) + "°");

            QPixmap ic = createWeatherIcon(forecast[i].weatherCode, true, 32);
            if (!ic.isNull()) {
                iconLabels[i]->setPixmap(ic);
            } else {
                iconLabels[i]->setPixmap(QPixmap());
                iconLabels[i]->setText(WeatherCodeMapper::icon(forecast[i].weatherCode, true));
            }
        } else {
            dayLabels[i]->setText("-");
            tempLabels[i]->setText("- / -");
            iconLabels[i]->setPixmap(QPixmap());
            iconLabels[i]->setText("-");
        }
    }
    showRollingHours();
    updateForecastPage();
    updateAnalyticsPage();
}

void MainWindow::setLoadingState(bool loading)
{
    ui->searchButton->setEnabled(!loading);
    ui->refreshButton->setEnabled(!loading);
    ui->refreshButton->setText(loading ? "Loading..." : "Refresh");
}

void MainWindow::setupHourlyStrip()
{
    QLayout *parentLayout = ui->temperatureChartWidget->parentWidget()->layout();

    m_hourlyScrollArea = new QScrollArea();
    m_hourlyScrollArea->setWidgetResizable(true);
    m_hourlyScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_hourlyScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_hourlyScrollArea->setFrameShape(QFrame::NoFrame);
    m_hourlyScrollArea->setStyleSheet("background: transparent;");

    QWidget *container = new QWidget();
    container->setStyleSheet("background: transparent;");
    m_hourlyLayout = new QHBoxLayout(container);
    m_hourlyLayout->setContentsMargins(12, 12, 12, 12);
    m_hourlyLayout->setSpacing(10);
    m_hourlyLayout->addStretch();

    m_hourlyScrollArea->setWidget(container);

    if (parentLayout) {
        parentLayout->replaceWidget(ui->temperatureChartWidget, m_hourlyScrollArea);
    }
    ui->temperatureChartWidget->hide();
}

void MainWindow::renderHourlyCards(const std::vector<HourlyData> &hours, bool highlightNow)
{
    if (!m_hourlyLayout) {
        return;
    }

    QLayoutItem *item;
    while ((item = m_hourlyLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }

    for (int i = 0; i < static_cast<int>(hours.size()); ++i) {
        const HourlyData &h = hours[i];

        QWidget *card = new QWidget();
        card->setFixedSize(64, 100);
        card->setStyleSheet("background-color: white; border-radius: 12px;");

        QVBoxLayout *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(4, 8, 4, 8);
        cardLayout->setSpacing(4);

        QLabel *timeLabel = new QLabel((highlightNow && i == 0) ? "Now" : h.dateTime.toString("h ap"));
        timeLabel->setAlignment(Qt::AlignCenter);
        timeLabel->setStyleSheet("color: #5f6368; font-size: 11px;");

        QLabel *iconLabel = new QLabel();
        iconLabel->setAlignment(Qt::AlignCenter);
        QPixmap icon = createWeatherIcon(h.weatherCode, h.isDay, 34);
        if (!icon.isNull()) {
            iconLabel->setPixmap(icon);
        } else {
            iconLabel->setText(WeatherCodeMapper::icon(h.weatherCode, h.isDay));
            iconLabel->setStyleSheet("font-size: 22px;");
        }

        QLabel *tempLabel = new QLabel(QString::number(convertTemp(h.temperature), 'f', 0) + "°");
        tempLabel->setAlignment(Qt::AlignCenter);
        tempLabel->setStyleSheet("color: #202124; font-weight: bold; font-size: 13px;");

        cardLayout->addWidget(timeLabel);
        cardLayout->addWidget(iconLabel);
        cardLayout->addWidget(tempLabel);

        m_hourlyLayout->addWidget(card);
    }

    m_hourlyLayout->addStretch();
    m_hourlyScrollArea->horizontalScrollBar()->setValue(0);
}

QPixmap MainWindow::createWeatherIcon(int code, bool isDay, int s) const
{
    // Only clear/cloudy codes are drawn; others fall back to emoji
    if (code < 0 || code > 3) {
        return QPixmap();
    }

    QPixmap pm(s, s);
    pm.fill(Qt::transparent);

    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);

    bool withCloud = (code == 2 || code == 3);
    QPointF c = withCloud ? QPointF(s * 0.38, s * 0.36) : QPointF(s * 0.5, s * 0.5);
    double r = withCloud ? s * 0.19 : s * 0.25;

    if (isDay) {
        p.setPen(QPen(QColor("#F9AB00"), s * 0.06, Qt::SolidLine, Qt::RoundCap));
        for (int i = 0; i < 8; ++i) {
            double a = qDegreesToRadians(i * 45.0);
            p.drawLine(QPointF(c.x() + qCos(a) * r * 1.4, c.y() + qSin(a) * r * 1.4),
                       QPointF(c.x() + qCos(a) * r * 1.85, c.y() + qSin(a) * r * 1.85));
        }
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#FBBC04"));
        p.drawEllipse(c, r, r);
    } else {
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#5DADE2"));
        p.drawEllipse(c, r * 1.2, r * 1.2);
        p.setCompositionMode(QPainter::CompositionMode_Clear);
        p.drawEllipse(QPointF(c.x() + r * 0.75, c.y() - r * 0.4), r, r);
        p.setCompositionMode(QPainter::CompositionMode_SourceOver);
    }

    if (withCloud) {
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#B0BEC5"));
        p.drawEllipse(QPointF(s * 0.40, s * 0.72), s * 0.16, s * 0.16);
        p.drawEllipse(QPointF(s * 0.60, s * 0.64), s * 0.20, s * 0.20);
        p.drawEllipse(QPointF(s * 0.78, s * 0.74), s * 0.14, s * 0.14);
        p.drawRoundedRect(QRectF(s * 0.30, s * 0.72, s * 0.56, s * 0.16), s * 0.08, s * 0.08);
    }

    p.end();
    return pm;
}

void MainWindow::showRollingHours()
{
    QDateTime now = QDateTime::currentDateTime();

    int startIndex = 0;
    for (int i = 0; i < static_cast<int>(m_hourlyData.size()); ++i) {
        if (m_hourlyData[i].dateTime.addSecs(3600) > now) {
            startIndex = i;
            break;
        }
    }

    std::vector<HourlyData> slice;
    for (int i = startIndex; i < static_cast<int>(m_hourlyData.size()) && slice.size() < 24; ++i) {
        slice.push_back(m_hourlyData[i]);
    }

    renderHourlyCards(slice, true);
}

void MainWindow::showHoursForDate(const QDate &date)
{
    if (date == QDate::currentDate()) {
        showRollingHours();
        return;
    }

    std::vector<HourlyData> dayHours;
    for (const HourlyData &h : m_hourlyData) {
        if (h.dateTime.date() == date) {
            dayHours.push_back(h);
        }
    }

    renderHourlyCards(dayHours, false);
}

void MainWindow::setupDayCardClicks()
{
    QWidget *cards[7] = {
        ui->dayCard_1, ui->dayCard_2, ui->dayCard_3, ui->dayCard_4,
        ui->dayCard_5, ui->dayCard_6, ui->dayCard_7
    };

    for (QWidget *card : cards) {
        card->setCursor(Qt::PointingHandCursor);
        card->installEventFilter(this);
    }
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        QWidget *cards[7] = {
            ui->dayCard_1, ui->dayCard_2, ui->dayCard_3, ui->dayCard_4,
            ui->dayCard_5, ui->dayCard_6, ui->dayCard_7
        };

        for (int i = 0; i < 7; ++i) {
            if (watched == cards[i]) {
                if (i < static_cast<int>(m_forecastData.size()) && m_forecastData[i].date.isValid()) {
                    showHoursForDate(m_forecastData[i].date);
                }
                return true;
            }
        }
    }

    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::setupCitySuggestions()
{
    m_suggestModel = new QStringListModel(this);
    m_completer = new QCompleter(m_suggestModel, this);
    m_completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
    ui->cityLineEdit->setCompleter(m_completer);

    m_suggestTimer = new QTimer(this);
    m_suggestTimer->setSingleShot(true);
    m_suggestTimer->setInterval(400);
    connect(m_suggestTimer, &QTimer::timeout, this, &MainWindow::updateSuggestions);

    connect(ui->cityLineEdit, &QLineEdit::textEdited, this, [this](const QString &text) {
        if (text.trimmed().length() >= 2) {
            m_suggestTimer->start();
        } else {
            m_suggestTimer->stop();
        }
    });

    connect(m_completer, QOverload<const QString &>::of(&QCompleter::activated),
            this, [this](const QString &text) {
                for (const CitySuggestion &c : m_suggestions) {
                    if (c.label() == text) {
                        ui->cityLineEdit->setText(c.name);
                        fetchAndDisplay(c);
                        return;
                    }
                }
                searchAndDisplay(text);
            });

    connect(ui->cityLineEdit, &QLineEdit::returnPressed, this, &MainWindow::onSearchClicked);
}

void MainWindow::updateSuggestions()
{
    QString text = ui->cityLineEdit->text().trimmed();
    if (text.length() < 2) {
        return;
    }

    m_suggestions = m_weatherService.searchCities(text, 8);
    QStringList labels;
    for (const CitySuggestion &c : m_suggestions) {
        labels << c.label();
    }
    m_suggestModel->setStringList(labels);
    m_completer->complete();
}

void MainWindow::setupSidebar()
{
    m_pages = new QStackedWidget();

    // Existing dashboard becomes page 0 (Overview)
    ui->horizontalLayout->replaceWidget(ui->contentArea, m_pages);
    m_pages->addWidget(ui->contentArea);
    ui->horizontalLayout->setStretchFactor(m_pages, 1);

    auto makePage = [](const QString &title) {
        QWidget *page = new QWidget();
        page->setObjectName("page");
        page->setStyleSheet("#page { background-color: #F1F3F4; }");

        QVBoxLayout *layout = new QVBoxLayout(page);
        layout->setContentsMargins(24, 24, 24, 24);

        QLabel *titleLabel = new QLabel(title);
        titleLabel->setStyleSheet("font-size: 28px; font-weight: bold; color: #202124;");
        layout->addWidget(titleLabel);
        layout->addStretch();
        return page;
    };

    QWidget *forecastPage = makePage("Forecast");
    m_forecastListLayout = new QVBoxLayout();
    m_forecastListLayout->setSpacing(10);
    static_cast<QVBoxLayout *>(forecastPage->layout())->insertLayout(1, m_forecastListLayout);
    m_pages->addWidget(forecastPage);           // index 1
    QWidget *analyticsPage = makePage("Analytics");
    setupAnalyticsChart();
    static_cast<QVBoxLayout *>(analyticsPage->layout())->insertWidget(1, m_chartView, 1);
    m_pages->addWidget(analyticsPage);          // index 2
    QWidget *settingsPage = makePage("Settings");
    setupSettingsPage(settingsPage);
    m_pages->addWidget(settingsPage);           // index 3

    QPushButton *buttons[4] = {
        ui->overviewButton, ui->forecastButton,
        ui->analyticsButton, ui->settingsButton
    };

    QButtonGroup *group = new QButtonGroup(this);
    group->setExclusive(true);

    for (int i = 0; i < 4; ++i) {
        QString css = buttons[i]->styleSheet();
        if (!css.contains('{')) {
            css = QString("QPushButton { %1 }").arg(css);
        }
        buttons[i]->setStyleSheet(css +
                                  " QPushButton:checked { background-color: #1F2937; color: white; }");

        buttons[i]->setCheckable(true);
        buttons[i]->setCursor(Qt::PointingHandCursor);
        group->addButton(buttons[i], i);
    }

    ui->overviewButton->setChecked(true);
    connect(group, &QButtonGroup::idClicked, m_pages, &QStackedWidget::setCurrentIndex);
}

void MainWindow::updateForecastPage()
{
    if (!m_forecastListLayout) {
        return;
    }

    QLayoutItem *item;
    while ((item = m_forecastListLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }

    for (const ForecastData &d : m_forecastData) {
        QWidget *row = new QWidget();
        row->setObjectName("forecastRow");
        row->setStyleSheet("#forecastRow { background-color: white; border-radius: 12px; }");
        row->setFixedHeight(64);

        QHBoxLayout *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(20, 8, 20, 8);
        rowLayout->setSpacing(16);

        QLabel *dayLabel = new QLabel(d.date.isValid() ? d.date.toString("dddd, MMM d") : d.dayName);
        dayLabel->setMinimumWidth(170);
        dayLabel->setStyleSheet("font-size: 15px; font-weight: bold; color: #202124;");

        QLabel *iconLabel = new QLabel();
        QPixmap icon = createWeatherIcon(d.weatherCode, true, 36);
        if (!icon.isNull()) {
            iconLabel->setPixmap(icon);
        } else {
            iconLabel->setText(WeatherCodeMapper::icon(d.weatherCode, true));
            iconLabel->setStyleSheet("font-size: 24px;");
        }

        QLabel *condLabel = new QLabel(d.condition);
        condLabel->setStyleSheet("font-size: 14px; color: #5f6368;");

        QLabel *rainLabel = new QLabel("💧 " + QString::number(d.precipitationProbability) + "%");
        rainLabel->setMinimumWidth(70);
        rainLabel->setStyleSheet("font-size: 13px; color: #5f6368;");

        QLabel *tempLabel = new QLabel(QString::number(convertTemp(d.tempMax), 'f', 0) + "° / " +
                                       QString::number(convertTemp(d.tempMin), 'f', 0) + "°");
        tempLabel->setStyleSheet("font-size: 15px; font-weight: bold; color: #1A73E8;");

        rowLayout->addWidget(dayLabel);
        rowLayout->addWidget(iconLabel);
        rowLayout->addWidget(condLabel, 1);
        rowLayout->addWidget(rainLabel);
        rowLayout->addWidget(tempLabel);

        m_forecastListLayout->addWidget(row);
    }
}

void MainWindow::setupAnalyticsChart()
{
    m_maxSeries = new QLineSeries();
    m_maxSeries->setName("Max");
    QPen maxPen(QColor("#EA4335"));
    maxPen.setWidth(3);
    m_maxSeries->setPen(maxPen);

    m_minSeries = new QLineSeries();
    m_minSeries->setName("Min");
    QPen minPen(QColor("#1A73E8"));
    minPen.setWidth(3);
    m_minSeries->setPen(minPen);

    for (QLineSeries *s : {m_maxSeries, m_minSeries}) {
        s->setPointsVisible(true);
        s->setPointLabelsVisible(true);
        s->setPointLabelsFormat("@yPoint°");
        s->setPointLabelsFont(QFont("Segoe UI", 9, QFont::Bold));
        s->setPointLabelsColor(QColor("#202124"));
    }

    QChart *chart = new QChart();
    chart->addSeries(m_maxSeries);
    chart->addSeries(m_minSeries);
    chart->setBackgroundBrush(QBrush(Qt::white));
    chart->setBackgroundPen(Qt::NoPen);
    chart->setBackgroundRoundness(12);
    chart->setTitleFont(QFont("Segoe UI", 12, QFont::Bold));
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->legend()->setAlignment(Qt::AlignBottom);

    m_axisX = new QBarCategoryAxis();
    m_axisY = new QValueAxis();
    m_axisY->setLabelFormat(QString::fromLatin1("%.0f\xC2\xB0"));
    m_axisY->setMinorTickCount(0);

    chart->addAxis(m_axisX, Qt::AlignBottom);
    chart->addAxis(m_axisY, Qt::AlignLeft);
    for (QLineSeries *s : {m_maxSeries, m_minSeries}) {
        s->attachAxis(m_axisX);
        s->attachAxis(m_axisY);
    }

    m_chartView = new QChartView(chart);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setStyleSheet("background: transparent;");
}

void MainWindow::updateAnalyticsPage()
{
    if (!m_maxSeries || m_forecastData.empty()) {
        return;
    }

    m_maxSeries->clear();
    m_minSeries->clear();

    QStringList categories;
    double lo = 1000.0;
    double hi = -1000.0;

    for (int i = 0; i < static_cast<int>(m_forecastData.size()); ++i) {
        const ForecastData &d = m_forecastData[i];
        const double mx = qRound(convertTemp(d.tempMax));
        const double mn = qRound(convertTemp(d.tempMin));

        m_maxSeries->append(i, mx);
        m_minSeries->append(i, mn);
        categories << (d.date.isValid() ? d.date.toString("ddd d") : d.dayName);

        lo = qMin(lo, mn);
        hi = qMax(hi, mx);
    }

    m_axisX->clear();
    m_axisX->append(categories);
    m_axisY->setRange(lo - 2, hi + 2);
    m_axisY->applyNiceNumbers();

    m_chartView->chart()->setTitle(m_currentLocation.shortLabel() + " - 7-Day Temperature (" + unitSymbol() + ")");
}

double MainWindow::convertTemp(double celsius) const
{
    return m_useFahrenheit ? celsius * 9.0 / 5.0 + 32.0 : celsius;
}

QString MainWindow::unitSymbol() const
{
    return m_useFahrenheit ? "°F" : "°C";
}

void MainWindow::setupSettingsPage(QWidget *page)
{
    auto makeCard = [](const QString &name, const QString &heading) {
        QFrame *card = new QFrame();
        card->setObjectName(name);
        card->setMaximumWidth(460);
        card->setStyleSheet(QString("#%1 { background-color: white; border-radius: 12px; }").arg(name));

        QVBoxLayout *layout = new QVBoxLayout(card);
        layout->setContentsMargins(20, 16, 20, 16);
        layout->setSpacing(10);

        QLabel *title = new QLabel(heading);
        title->setStyleSheet("font-size: 16px; font-weight: bold; color: #202124;");
        layout->addWidget(title);
        return card;
    };
    const QString radioStyle = "font-size: 14px; color: #202124;";

    // ---- Temperature unit ----
    QFrame *tempCard = makeCard("tempCard", "Temperature unit");
    m_celsiusBtn = new QRadioButton("Celsius (°C)");
    m_fahrenheitBtn = new QRadioButton("Fahrenheit (°F)");
    m_celsiusBtn->setChecked(true);
    m_celsiusBtn->setStyleSheet(radioStyle);
    m_fahrenheitBtn->setStyleSheet(radioStyle);
    tempCard->layout()->addWidget(m_celsiusBtn);
    tempCard->layout()->addWidget(m_fahrenheitBtn);

    // ---- Wind speed unit ----
    QFrame *windCard = makeCard("windCard", "Wind speed unit");
    m_kmhBtn = new QRadioButton("Kilometers per hour (km/h)");
    m_mphBtn = new QRadioButton("Miles per hour (mph)");
    m_kmhBtn->setChecked(true);
    m_kmhBtn->setStyleSheet(radioStyle);
    m_mphBtn->setStyleSheet(radioStyle);
    windCard->layout()->addWidget(m_kmhBtn);
    windCard->layout()->addWidget(m_mphBtn);

    // ---- Favorite cities ----
    QFrame *favCard = makeCard("favCard", "Favorite cities");
    QLabel *hint = new QLabel("Double-click a city to open it.");
    hint->setStyleSheet("font-size: 12px; color: #5f6368;");

    m_favoritesList = new QListWidget();
    m_favoritesList->setFixedHeight(96);
    m_favoritesList->setFrameShape(QFrame::NoFrame);
    m_favoritesList->setStyleSheet(
        "QListWidget { background-color: #F8F9FA; border-radius: 8px; font-size: 13px; color: #202124; outline: none; }"
        "QListWidget::item { padding: 5px 12px; font-size: 12px; }"
        "QListWidget::item:selected { background-color: #E8F0FE; color: #1A73E8; }");

    QPushButton *addBtn = new QPushButton("Add current city");
    addBtn->setCursor(Qt::PointingHandCursor);
    addBtn->setStyleSheet(
        "QPushButton { background-color: #1A73E8; color: white; border-radius: 8px; padding: 8px 14px; font-size: 13px; }"
        "QPushButton:hover { background-color: #1765CC; }");

    QPushButton *removeBtn = new QPushButton("Remove selected");
    removeBtn->setCursor(Qt::PointingHandCursor);
    removeBtn->setStyleSheet(
        "QPushButton { background-color: #F1F3F4; color: #202124; border-radius: 8px; padding: 8px 14px; font-size: 13px; }"
        "QPushButton:hover { background-color: #E0E0E0; }");

    QHBoxLayout *btnRow = new QHBoxLayout();
    btnRow->addWidget(addBtn);
    btnRow->addWidget(removeBtn);
    btnRow->addStretch();

    favCard->layout()->addWidget(hint);
    favCard->layout()->addWidget(m_favoritesList);
    static_cast<QVBoxLayout *>(favCard->layout())->addLayout(btnRow);

    // ---- Put the cards in the page (title is at index 0) ----
    QVBoxLayout *pageLayout = static_cast<QVBoxLayout *>(page->layout());
    pageLayout->insertWidget(1, tempCard);
    pageLayout->insertWidget(2, windCard);
    pageLayout->insertWidget(3, favCard);

    // ---- Connections ----
    connect(m_fahrenheitBtn, &QRadioButton::toggled, this, [this](bool on) {
        m_useFahrenheit = on;
        QSettings s(kOrg, kApp);
        s.setValue("useFahrenheit", on);
        if (!m_forecastData.empty()) {
            updateUi(m_currentWeather, m_forecastData, m_hourlyData);
        }
    });

    connect(m_mphBtn, &QRadioButton::toggled, this, [this](bool on) {
        m_useMph = on;
        QSettings s(kOrg, kApp);
        s.setValue("useMph", on);
        if (!m_forecastData.empty()) {
            updateUi(m_currentWeather, m_forecastData, m_hourlyData);
        }
    });

    connect(addBtn, &QPushButton::clicked, this, &MainWindow::addCurrentToFavorites);
    connect(removeBtn, &QPushButton::clicked, this, &MainWindow::removeSelectedFavorite);

    connect(m_favoritesList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) {
        CitySuggestion c = CitySuggestion::fromStorage(item->data(Qt::UserRole).toString());
        if (!c.isValid()) {
            return;
        }
        m_pages->setCurrentIndex(0);
        ui->overviewButton->setChecked(true);
        fetchAndDisplay(c);
    });
}

QString MainWindow::windText(double kmh) const
{
    if (m_useMph) {
        return QString::number(kmh * 0.621371, 'f', 1) + " mph";
    }
    return QString::number(kmh, 'f', 1) + " km/h";
}

void MainWindow::loadSettings()
{
    QSettings s(kOrg, kApp);

    const QStringList saved = s.value("favorites").toStringList();
    for (const QString &entry : saved) {
        CitySuggestion c = CitySuggestion::fromStorage(entry);
        if (!c.isValid()) {
            continue;
        }
        QListWidgetItem *item = new QListWidgetItem(c.label());
        item->setData(Qt::UserRole, c.toStorage());
        m_favoritesList->addItem(item);
    }

    m_useFahrenheit = s.value("useFahrenheit", false).toBool();
    m_useMph = s.value("useMph", false).toBool();

    m_fahrenheitBtn->setChecked(m_useFahrenheit);
    m_celsiusBtn->setChecked(!m_useFahrenheit);
    m_mphBtn->setChecked(m_useMph);
    m_kmhBtn->setChecked(!m_useMph);
}

void MainWindow::saveSettings()
{
    QSettings s(kOrg, kApp);

    s.setValue("useFahrenheit", m_useFahrenheit);
    s.setValue("useMph", m_useMph);
    if (m_currentLocation.isValid()) {
        s.setValue("lastLocation", m_currentLocation.toStorage());
    }

    QStringList favorites;
    for (int i = 0; i < m_favoritesList->count(); ++i) {
        favorites << m_favoritesList->item(i)->data(Qt::UserRole).toString();
    }
    s.setValue("favorites", favorites);
}

void MainWindow::addCurrentToFavorites()
{
    if (!m_currentLocation.isValid()) {
        return;
    }
    if (!m_favoritesList->findItems(m_currentLocation.label(), Qt::MatchFixedString).isEmpty()) {
        return; // already in the list
    }
    QListWidgetItem *item = new QListWidgetItem(m_currentLocation.label());
    item->setData(Qt::UserRole, m_currentLocation.toStorage());
    m_favoritesList->addItem(item);
    saveSettings();
}

void MainWindow::removeSelectedFavorite()
{
    delete m_favoritesList->takeItem(m_favoritesList->currentRow());
    saveSettings();
}