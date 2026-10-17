#ifndef CITYSUGGESTION_H
#define CITYSUGGESTION_H

#include <QString>
#include <QStringList>

class CitySuggestion
{
public:
    QString name;
    QString region;
    QString country;
    double latitude = 0.0;
    double longitude = 0.0;

    bool isValid() const { return !name.isEmpty(); }

    // "Cairo, Egypt" or "Cairo, Illinois, United States"
    QString label() const
    {
        QString text = name;
        if (!region.isEmpty() && region != name) {
            text += ", " + region;
        }
        if (!country.isEmpty()) {
            text += ", " + country;
        }
        return text;
    }

    // "Cairo, Egypt"
    QString shortLabel() const
    {
        return country.isEmpty() ? name : name + ", " + country;
    }

    QString toStorage() const
    {
        return QStringList{name, region, country,
                           QString::number(latitude, 'f', 4),
                           QString::number(longitude, 'f', 4)}.join('|');
    }

    static CitySuggestion fromStorage(const QString &text)
    {
        CitySuggestion c;
        const QStringList p = text.split('|');
        if (p.size() == 5) {
            c.name = p[0];
            c.region = p[1];
            c.country = p[2];
            c.latitude = p[3].toDouble();
            c.longitude = p[4].toDouble();
        }
        return c;
    }
};

#endif // CITYSUGGESTION_H