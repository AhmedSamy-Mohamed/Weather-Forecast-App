#ifndef WEATHERSERVICE_H
#define WEATHERSERVICE_H

#include <QStringList>
#include <QString>
#include <vector>

#include "../Models/WeatherData.h"
#include "../Models/ForecastData.h"
#include "../Models/HourlyData.h"
#include "../Models/CitySuggestion.h"
#include "../Network/WeatherApiClient.h"
#include "../Parser/WeatherParser.h"

class WeatherService
{
public:
    WeatherService();

    // Returns up to `count` matching cities (name, region, country, lat/lon)
    std::vector<CitySuggestion> searchCities(const QString &query, int count = 8) const;

    // Fetches weather using the exact coordinates of the chosen city
    bool fetchWeatherForLocation(const CitySuggestion &city,
                                 WeatherData &outCurrent,
                                 std::vector<ForecastData> &outForecast,
                                 std::vector<HourlyData> &outHourly) const;

private:
    WeatherApiClient m_apiClient;
    WeatherParser m_parser;
};

#endif // WEATHERSERVICE_H