#include "WeatherCodeMapper.h"

QString WeatherCodeMapper::description(int code)
{
    switch (code) {
    case 0: return "Clear Sky";
    case 1: return "Mainly Clear";
    case 2: return "Partly Cloudy";
    case 3: return "Overcast";
    case 45:
    case 48: return "Fog";
    case 51:
    case 53:
    case 55: return "Drizzle";
    case 56:
    case 57: return "Freezing Drizzle";
    case 61: return "Light Rain";
    case 63: return "Rain";
    case 65: return "Heavy Rain";
    case 66:
    case 67: return "Freezing Rain";
    case 71: return "Light Snow";
    case 73: return "Snow";
    case 75: return "Heavy Snow";
    case 77: return "Snow Grains";
    case 80: return "Light Showers";
    case 81: return "Showers";
    case 82: return "Heavy Showers";
    case 85:
    case 86: return "Snow Showers";
    case 95: return "Thunderstorm";
    case 96:
    case 99: return "Thunderstorm w/ Hail";
    default: return "N/A";
    }
}

QString WeatherCodeMapper::icon(int code, bool isDay)
{
    switch (code) {
    case 0:
    case 1:
        return isDay ? "☀️" : "🌙";
    case 2:
        return isDay ? "⛅" : "☁️🌙";
    case 3:
        return "☁️";
    case 45:
    case 48:
        return "🌫️";
    case 51:
    case 53:
    case 55:
    case 56:
    case 57:
        return "🌦️";
    case 61:
    case 63:
    case 65:
    case 66:
    case 67:
    case 80:
    case 81:
    case 82:
        return "🌧️";
    case 71:
    case 73:
    case 75:
    case 77:
    case 85:
    case 86:
        return "❄️";
    case 95:
    case 96:
    case 99:
        return "⛈️";
    default:
        return "❓";
    }
}