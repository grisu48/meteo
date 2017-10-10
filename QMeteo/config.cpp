#include "config.h"

Config::Config(const QString filename)
{
    this->filename = filename;

    // Read file, line by line
    QFile inputFile(filename);
    if (inputFile.open(QIODevice::ReadOnly))
    {
        QTextStream in(&inputFile);
        while (!in.atEnd())
        {
            QString line = in.readLine().trimmed();
            if(line.isEmpty()) continue;
            if(line.at(0) == '#') continue;

            int i = line.indexOf('=');
            if(i < 0) continue;
            QString key = line.left(i-1).trimmed();
            QString value = line.mid(i+1).trimmed();
            if(key.isEmpty()) continue;

            this->values[key] = value;
        }
        inputFile.close();
    }
}

