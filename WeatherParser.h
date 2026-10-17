#ifndef WEATHERPARSER_H
#define WEATHERPARSER_H

#include <QString>
#include <vector>

#include "../Models/WeatherData.h"
#include "../Models/ForecastData.h"
#include "../Models/HourlyData.h"

class WeatherParser
{
public:
    WeatherParser();

    bool parse(const QString &json,
               const QString &cityName,
               WeatherData &outCurrent,
               std::vector<ForecastData> &outForecast,
               std::vector<HourlyData> &outHourly) const;
};

#endif // WEATHERPARSER_H


