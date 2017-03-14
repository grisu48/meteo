#ifndef CONFIGFILE_H
#define CONFIGFILE_H

#include <QMap>
#include <QString>
#include <QFile>
#include <QTextStream>

/** Instance for reading and accessing a config file*/
class ConfigFile
{
protected:
    /** Read values */
    QMap< QString, QString> values;

    bool file_existed = false;
public:
    ConfigFile(const char* filename);
    ConfigFile(const QString &filename);
    virtual ~ConfigFile();

    QString operator[](const char* key) const;
    QString operator[](const QString &key) const;

    bool contains(const QString &key) const;
    bool contains(const char *key) const;

    bool fileExists(void) const;
};

#endif // CONFIGFILE_H
