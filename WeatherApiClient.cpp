#include "WeatherApiClient.h"

#include <curl/curl.h>
#include <string>

namespace {

size_t writeCallback(char *contents, size_t size, size_t nmemb, void *userp)
{
    size_t totalSize = size * nmemb;
    std::string *buffer = static_cast<std::string *>(userp);
    buffer->append(contents, totalSize);
    return totalSize;
}

} // namespace

WeatherApiClient::WeatherApiClient()
{
}

QString WeatherApiClient::get(const QString &url) const
{
    CURL *curl = curl_easy_init();
    if (!curl) {
        return QString();
    }

    std::string responseBuffer;
    std::string urlStd = url.toStdString();

    curl_easy_setopt(curl, CURLOPT_URL, urlStd.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBuffer);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);

    CURLcode res = curl_easy_perform(curl);

    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        return QString();
    }

    return QString::fromStdString(responseBuffer);
}