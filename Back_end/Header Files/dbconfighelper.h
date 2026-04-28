#ifndef DBCONFIGHELPER_H
#define DBCONFIGHELPER_H

#include <QtCore/QString>

struct DbConnectionConfig{
    QString databaseFilePath;
    QString configPath;
};

class DbConfigHelper{
public:
    static QString ResolveConfigPath();
    static bool LoadConfig(DbConnectionConfig* outConfig, QString* errorMessage = NULL);
};

#endif