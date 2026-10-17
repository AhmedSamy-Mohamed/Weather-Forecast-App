#ifndef FORECASTDATA_H
#define FORECASTDATA_H

#include <QString>
#include <QDate>

class ForecastData
{
public:
    ForecastData() = default;

    QString dayName;
    QString condition;
    double tempMax = 0.0;
    double tempMin = 0.0;
    QDate date;
    int weatherCode = 0;
    int precipitationProbability = 0;
};
#endif // FORECASTDATA_H