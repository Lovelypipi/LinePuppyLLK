#include "dbconfighelper.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QSettings>
#include <QtCore/QStandardPaths>

namespace{
QString DefaultDatabaseFilePath(){
    QString basePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if(basePath.isEmpty()){
        basePath = QDir::homePath();
    }

    QDir baseDir(basePath);
    if(!baseDir.exists()){
        baseDir.mkpath(".");
    }

    return baseDir.filePath("llk_local.sqlite");
}
}

QString DbConfigHelper::ResolveConfigPath(){
    const QString appDirConfig = QDir(QCoreApplication::applicationDirPath()).filePath("dbconfig.ini");
    if(QFileInfo::exists(appDirConfig)){
        return appDirConfig;
    }

    const QString appParentConfig = QDir(QCoreApplication::applicationDirPath()).filePath("../dbconfig.ini");
    if(QFileInfo::exists(appParentConfig)){
        return appParentConfig;
    }

    const QString currentDirConfig = QDir::current().filePath("dbconfig.ini");
    if(QFileInfo::exists(currentDirConfig)){
        return currentDirConfig;
    }

    const QString currentParentConfig = QDir::current().filePath("../dbconfig.ini");
    if(QFileInfo::exists(currentParentConfig)){
        return currentParentConfig;
    }

    return appDirConfig;
}

bool DbConfigHelper::LoadConfig(DbConnectionConfig* outConfig, QString* errorMessage){
    if(!outConfig){
        if(errorMessage){
            *errorMessage = "配置输出对象不能为空";
        }
        return false;
    }

    const QString configPath = ResolveConfigPath();
    QSettings settings(configPath, QSettings::IniFormat);

    const QString configuredFile = settings.value("database/file", QString()).toString().trimmed();
    if(configuredFile.isEmpty()){
        outConfig->databaseFilePath = DefaultDatabaseFilePath();
    }else{
        QFileInfo fileInfo(configuredFile);
        if(fileInfo.isAbsolute()){
            outConfig->databaseFilePath = configuredFile;
        }else{
            outConfig->databaseFilePath = QDir(QFileInfo(configPath).absolutePath()).filePath(configuredFile);
        }
    }
    outConfig->configPath = configPath;

    QFileInfo dbFileInfo(outConfig->databaseFilePath);
    QDir dbDir = dbFileInfo.dir();
    if(!dbDir.exists() && !dbDir.mkpath(".")){
        if(errorMessage){
            *errorMessage = QString("无法创建本地数据库目录：%1").arg(dbDir.absolutePath());
        }
        return false;
    }

    return true;
}