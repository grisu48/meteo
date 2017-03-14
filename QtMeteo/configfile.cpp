#include "configfile.h"


ConfigFile::ConfigFile(const char* filename) : ConfigFile(QString(filename)) {}

ConfigFile::ConfigFile(const QString &filename) {
    QFile inputFile(filename);
    if (inputFile.open(QIODevice::ReadOnly))
    {
        QTextStream in(&inputFile);
        while (!in.atEnd())
        {
            QString line = in.readLine();
            line = line.trimmed();
            if(line.isEmpty()) continue;
            if(line.at(0) == '#' || line.at(0) == ':' || line.at(0) == ';') continue;     // Comment

            const int i = line.indexOf('=');
            if(i<0) continue;

            QString name = line.left(i).trimmed();
            QString value = line.mid(i+1).trimmed();

            if(name.isEmpty()) continue;
            if(value.isEmpty()) continue;

            this->values[name] = value;

        }
        inputFile.close();
        this->file_existed = true;
    } else
        this->file_existed = false;
}

ConfigFile::~ConfigFile() {

}

QString ConfigFile::operator[](const char* key) const {
    return (*this)[QString(key)];
}

QString ConfigFile::operator[](const QString &key) const {
    if(this->values.contains(key))
        return this->values[key];
    else
        return "";
}

bool ConfigFile::contains(const QString &key) const {
    return this->values.contains(key);
}
bool ConfigFile::contains(const char *key) const {
    return this->contains(QString(key));
}

bool ConfigFile::fileExists(void) const {
    return this->file_existed;
}
