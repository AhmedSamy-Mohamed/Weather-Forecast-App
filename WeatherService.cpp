#include "WeatherService.h"

#include <rapidjson/document.h>
#include <QUrl>
#include <QUrlQuery>

using namespace rapidjson;

WeatherService::WeatherService()
{
}

std::vector<CitySuggestion> WeatherService::searchCities(const QString &query, int count) const
{
    std::vector<CitySuggestion> result;

    QUrl url("https://geocoding-api.open-meteo.com/v1/search");
    QUrlQuery q;
    q.addQueryItem("name", query);
    q.addQueryItem("count", QString::number(count));
    q.addQueryItem("language", "en");
    q.addQueryItem("format", "json");
    url.setQuery(q);

    QString response = m_apiClient.get(url.toString());
    if (response.isEmpty()) {
        return result;
    }

    Document doc;
    doc.Parse(response.toUtf8().constData());
    if (doc.HasParseError() || !doc.IsObject() ||
        !doc.HasMember("results") || !doc["results"].IsArray()) {
        return result;
    }

    for (const Value &item : doc["results"].GetArray()) {
        if (!item.IsObject() ||
            !item.HasMember("name") || !item["name"].IsString() ||
            !item.HasMember("latitude") || !item["latitude"].IsNumber() ||
            !item.HasMember("longitude") || !item["longitude"].IsNumber()) {
            continue;
        }

        CitySuggestion c;
        c.name = QString::fromUtf8(item["name"].GetString());
        if (item.HasMember("admin1") && item["admin1"].IsString()) {
            c.region = QString::fromUtf8(item["admin1"].GetString());
        }
        if (item.HasMember("country") && item["country"].IsString()) {
            c.country = QString::fromUtf8(item["country"].GetString());
        }
        c.latitude = item["latitude"].GetDouble();
        c.longitude = item["longitude"].GetDouble();
        result.push_back(c);
    }
    return result;
}

bool WeatherService::fetchWeatherForLocation(const CitySuggestion &city,
                                              WeatherData &outCurrent,
                                              std::vector<ForecastData> &outForecast,
                                              std::vector<HourlyData> &outHourly) const
{
    QUrl url("https://api.open-meteo.com/v1/forecast");
    QUrlQuery query;
    query.addQueryItem("latitude", QString::number(city.latitude, 'f', 4));
    query.addQueryItem("longitude", QString::number(city.longitude, 'f', 4));
    query.addQueryItem("current", "temperature_2m,relative_humidity_2m,wind_speed_10m,weather_code,is_day");
    query.addQueryItem("daily", "temperature_2m_max,temperature_2m_min,weather_code,precipitation_probability_max");
    query.addQueryItem("hourly", "temperature_2m,weather_code,is_day");
    query.addQueryItem("timezone", "auto");
    url.setQuery(query);

    QString response = m_apiClient.get(url.toString());
    if (response.isEmpty()) {
        return false;
    }

    return m_parser.parse(response, city.shortLabel(), outCurrent, outForecast, outHourly);
}