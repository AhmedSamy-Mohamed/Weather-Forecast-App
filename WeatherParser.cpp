#include "WeatherParser.h"
#include "WeatherCodeMapper.h"
#include <rapidjson/document.h>
#include <QDateTime>
#include "WeatherCodeMapper.h"

using namespace rapidjson;

WeatherParser::WeatherParser()
{
}

bool WeatherParser::parse(const QString &json,
                           const QString &cityName,
                           WeatherData &outCurrent,
                           std::vector<ForecastData> &outForecast,
                           std::vector<HourlyData> &outHourly) const
{
    Document doc;
    std::string jsonStd = json.toStdString();
    doc.Parse(jsonStd.c_str());

    if (doc.HasParseError() || !doc.IsObject()) {
        return false;
    }

    if (!doc.HasMember("current") || !doc["current"].IsObject()) {
        return false;
    }
    if (!doc.HasMember("daily") || !doc["daily"].IsObject()) {
        return false;
    }

    const Value &current = doc["current"];
    const Value &daily = doc["daily"];

    // ---- Current weather ----
    WeatherData weather;
    weather.cityName = cityName;
    weather.temperature = current.HasMember("temperature_2m") && current["temperature_2m"].IsNumber()
                               ? current["temperature_2m"].GetDouble() : 0.0;
    weather.humidity = current.HasMember("relative_humidity_2m") && current["relative_humidity_2m"].IsNumber()
                            ? current["relative_humidity_2m"].GetInt() : 0;
    weather.windSpeed = current.HasMember("wind_speed_10m") && current["wind_speed_10m"].IsNumber()
                             ? current["wind_speed_10m"].GetDouble() : 0.0;
    int currentCode = current.HasMember("weather_code") && current["weather_code"].IsInt()
                          ? current["weather_code"].GetInt() : 0;
    bool currentIsDay = current.HasMember("is_day") && current["is_day"].IsInt()
                            ? (current["is_day"].GetInt() != 0) : true;
    weather.condition = WeatherCodeMapper::description(currentCode);
    weather.lastUpdated = QDateTime::currentDateTime().toString("hh:mm");

    // ---- 7-day forecast ----
    std::vector<ForecastData> forecast;

    if (!daily.HasMember("time") || !daily["time"].IsArray() ||
        !daily.HasMember("temperature_2m_max") || !daily["temperature_2m_max"].IsArray() ||
        !daily.HasMember("temperature_2m_min") || !daily["temperature_2m_min"].IsArray()) {
        return false;
    }

    const Value &times = daily["time"];
    const Value &maxTemps = daily["temperature_2m_max"];
    const Value &minTemps = daily["temperature_2m_min"];
    const Value *codesPtr = (daily.HasMember("weather_code") && daily["weather_code"].IsArray())
                                ? &daily["weather_code"] : nullptr;
    const Value *probsPtr = (daily.HasMember("precipitation_probability_max") && daily["precipitation_probability_max"].IsArray())
                                ? &daily["precipitation_probability_max"] : nullptr;

    SizeType count = times.Size();
    for (SizeType i = 0; i < count && i < 7; ++i) {
        ForecastData day;

        QString dateStr = times[i].IsString() ? QString::fromUtf8(times[i].GetString()) : QString();
        QDate date = QDate::fromString(dateStr, "yyyy-MM-dd");
        day.dayName = date.isValid() ? date.toString("ddd") : dateStr;
        day.date = date;

        day.tempMax = maxTemps[i].IsNumber() ? maxTemps[i].GetDouble() : 0.0;
        day.tempMin = minTemps[i].IsNumber() ? minTemps[i].GetDouble() : 0.0;
        int dayCode = (codesPtr && i < codesPtr->Size() && (*codesPtr)[i].IsInt())
                          ? (*codesPtr)[i].GetInt() : 0;
        day.condition = WeatherCodeMapper::description(dayCode);
        day.weatherCode = dayCode;
        day.precipitationProbability = (probsPtr && i < probsPtr->Size() && (*probsPtr)[i].IsNumber())
                                           ? static_cast<int>((*probsPtr)[i].GetDouble()) : 0;
        forecast.push_back(day);
    }

    // ---- Hourly forecast ----
    std::vector<HourlyData> hourly;

    if (doc.HasMember("hourly") && doc["hourly"].IsObject()) {
        const Value &hourlyObj = doc["hourly"];

        if (hourlyObj.HasMember("time") && hourlyObj["time"].IsArray() &&
            hourlyObj.HasMember("temperature_2m") && hourlyObj["temperature_2m"].IsArray() &&
            hourlyObj.HasMember("weather_code") && hourlyObj["weather_code"].IsArray() &&
            hourlyObj.HasMember("is_day") && hourlyObj["is_day"].IsArray()) {

            const Value &hTimes = hourlyObj["time"];
            const Value &hTemps = hourlyObj["temperature_2m"];
            const Value &hCodes = hourlyObj["weather_code"];
            const Value &hIsDay = hourlyObj["is_day"];

            SizeType hCount = hTimes.Size();
            for (SizeType i = 0; i < hCount; ++i) {
                HourlyData h;

                QString dtStr = hTimes[i].IsString() ? QString::fromUtf8(hTimes[i].GetString()) : QString();
                h.dateTime = QDateTime::fromString(dtStr, "yyyy-MM-ddTHH:mm");
                h.temperature = hTemps[i].IsNumber() ? hTemps[i].GetDouble() : 0.0;
                h.weatherCode = hCodes[i].IsInt() ? hCodes[i].GetInt() : 0;
                h.isDay = hIsDay[i].IsInt() ? (hIsDay[i].GetInt() != 0) : true;

                hourly.push_back(h);
            }
        }
    }

    outCurrent = weather;
    outForecast = forecast;
    outHourly = hourly;
    return true;
}