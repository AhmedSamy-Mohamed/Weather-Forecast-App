#ifndef WEATHERCODEMAPPER_H
#define WEATHERCODEMAPPER_H

#include <QString>

class WeatherCodeMapper
{
public:
    // Returns a short human-readable description for an Open-Meteo
    // WMO weather code (e.g. "Partly Cloudy", "Light Rain").
    static QString description(int code);

    // Returns an emoji icon representing the weather code.
    // isDay controls sun vs moon for clear/partly-cloudy codes.
    static QString icon(int code, bool isDay);
};

#endif // WEATHERCODEMAPPER_H