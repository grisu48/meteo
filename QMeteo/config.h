#ifndef CONFIG_H
#define CONFIG_H

#include <QString>
#include <QMap>
#include <QFile>
#include <QTextStream>

class Config
{
private:
    QString filename;
    QMap<QString,QString> values;
public:
    Config(const QString filename);
    QString operator[](const QString name) { return this->values[name]; }
    QString get(const QString key, const QString defaultValue = "") {
        if(this->values.contains(key))
            return this->values[key];
        else
            return defaultValue;
    }
};

#endif // CONFIG_H
