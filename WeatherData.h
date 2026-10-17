#ifndef WEATHERDATA_H
#define WEATHERDATA_H

#include <QString>

class WeatherData
{
public:
    WeatherData() = default;

    QString cityName;
    double temperature = 0.0;
    QString condition;
    int humidity = 0;
    double windSpeed = 0.0;
    QString lastUpdated;

    bool isValid() const { return !cityName.isEmpty(); }
};

#endif // WEATHERDATA_H