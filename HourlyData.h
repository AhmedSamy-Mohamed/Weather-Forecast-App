#ifndef HOURLYDATA_H
#define HOURLYDATA_H

#include <QString>
#include <QDateTime>



class HourlyData
{
public:
    HourlyData() = default;

    QDateTime dateTime;
    double temperature = 0.0;
    int weatherCode = 0;   // Open-Meteo WMO weather code
    bool isDay = true;     // true = daytime, false = nighttime
};

#endif // HOURLYDATA_H