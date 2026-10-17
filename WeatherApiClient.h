#ifndef WEATHERAPICLIENT_H
#define WEATHERAPICLIENT_H

#include <QString>

class WeatherApiClient
{
public:
    WeatherApiClient();

    // Performs an HTTP GET request to the given URL.
    // Returns the raw response body as a string, or an empty
    // string if the request failed.
    QString get(const QString &url) const;
};

#endif // WEATHERAPICLIENT_H