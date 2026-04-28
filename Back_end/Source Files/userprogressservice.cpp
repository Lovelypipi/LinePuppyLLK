#include "userprogressservice.h"

#include <QtCore/QUuid>
#include <QtCore/QDir>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

UserProgressService::UserProgressService()
    : m_connectionName(QUuid::createUuid().toString(QUuid::WithoutBraces)){
}

UserProgressService::~UserProgressService(){
    Close();
}

bool UserProgressService::Open(const QString& databaseFilePath,
                               QString* errorMessage){
    if(m_db.isValid() && m_db.isOpen()){
        m_db.close();
    }

    if(!QSqlDatabase::isDriverAvailable("QSQLITE")){
        if(errorMessage){
            const QString available = QSqlDatabase::drivers().join(", ");
            *errorMessage = QString("QSQLITE 驱动不可用。当前可用驱动：%1").arg(available.isEmpty() ? "<none>" : available);
        }
        return false;
    }

    if(QSqlDatabase::contains(m_connectionName)){
        m_db = QSqlDatabase::database(m_connectionName);
    }else{
        m_db = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
    }

    m_db.setDatabaseName(databaseFilePath);

    if(!m_db.open()){
        if(errorMessage){
            *errorMessage = m_db.lastError().text();
        }
        return false;
    }

    QSqlQuery pragmaQuery(m_db);
    if(!pragmaQuery.exec("PRAGMA foreign_keys = ON")){
        if(errorMessage){
            *errorMessage = pragmaQuery.lastError().text();
        }
        return false;
    }

    return true;
}

void UserProgressService::Close(){
    if(m_db.isValid()){
        if(m_db.isOpen()){
            m_db.close();
        }
        m_db = QSqlDatabase();
    }
    if(!m_connectionName.isEmpty() && QSqlDatabase::contains(m_connectionName)){
        QSqlDatabase::removeDatabase(m_connectionName);
    }
}

bool UserProgressService::EnsureSchema(QString* errorMessage){
    if(!m_db.isValid() || !m_db.isOpen()){
        if(errorMessage){
            *errorMessage = "数据库未连接";
        }
        return false;
    }

    QSqlQuery query(m_db);
    const QString createProgressSql =
        "CREATE TABLE IF NOT EXISTS user_mode_progress ("
        " user_id INTEGER NOT NULL,"
        " mode_type VARCHAR(16) NOT NULL,"
        " has_progress TINYINT(1) NOT NULL DEFAULT 0,"
        " snapshot_json LONGTEXT NULL,"
        " updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        " PRIMARY KEY (user_id, mode_type),"
        " FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE"
        ")";

    if(!query.exec(createProgressSql)){
        if(errorMessage){
            *errorMessage = query.lastError().text();
        }
        return false;
    }

    const QString createLevelSql =
        "CREATE TABLE IF NOT EXISTS user_level_state ("
        " user_id INTEGER NOT NULL PRIMARY KEY,"
        " unlocked_level INT NOT NULL DEFAULT 1,"
        " updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        " FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE"
        ")";

    if(!query.exec(createLevelSql)){
        if(errorMessage){
            *errorMessage = query.lastError().text();
        }
        return false;
    }

    const QString createBestTimeSql =
        "CREATE TABLE IF NOT EXISTS user_best_times ("
        " user_id INTEGER NOT NULL,"
        " mode_type VARCHAR(16) NOT NULL,"
        " level_index INT NOT NULL DEFAULT 0,"
        " best_time_ms BIGINT NOT NULL,"
        " updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        " PRIMARY KEY (user_id, mode_type, level_index),"
        " FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE"
        ")";

    if(!query.exec(createBestTimeSql)){
        if(errorMessage){
            *errorMessage = query.lastError().text();
        }
        return false;
    }

    return true;
}

bool UserProgressService::SaveModeProgress(qint64 userId,
                                           const QString& modeType,
                                           bool hasProgress,
                                           const QString& snapshotJson,
                                           QString* errorMessage){
    if(userId <= 0 || modeType.trimmed().isEmpty()){
        if(errorMessage){
            *errorMessage = "参数非法";
        }
        return false;
    }

    if(!m_db.isValid() || !m_db.isOpen()){
        if(errorMessage){
            *errorMessage = "数据库未连接";
        }
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare("INSERT INTO user_mode_progress(user_id, mode_type, has_progress, snapshot_json) "
                  "VALUES(:user_id, :mode_type, :has_progress, :snapshot_json) "
                  "ON CONFLICT(user_id, mode_type) DO UPDATE SET "
                  "has_progress = excluded.has_progress, "
                  "snapshot_json = excluded.snapshot_json, "
                  "updated_at = CURRENT_TIMESTAMP");
    query.bindValue(":user_id", userId);
    query.bindValue(":mode_type", modeType.trimmed());
    query.bindValue(":has_progress", hasProgress ? 1 : 0);
    query.bindValue(":snapshot_json", snapshotJson);

    if(!query.exec()){
        if(errorMessage){
            *errorMessage = query.lastError().text();
        }
        return false;
    }

    return true;
}

bool UserProgressService::LoadModeProgress(qint64 userId,
                                           const QString& modeType,
                                           bool* outHasProgress,
                                           QString* outSnapshotJson,
                                           QString* errorMessage){
    if(userId <= 0 || modeType.trimmed().isEmpty()){
        if(errorMessage){
            *errorMessage = "参数非法";
        }
        return false;
    }

    if(!m_db.isValid() || !m_db.isOpen()){
        if(errorMessage){
            *errorMessage = "数据库未连接";
        }
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare("SELECT has_progress, snapshot_json FROM user_mode_progress WHERE user_id = :user_id AND mode_type = :mode_type");
    query.bindValue(":user_id", userId);
    query.bindValue(":mode_type", modeType.trimmed());
    if(!query.exec()){
        if(errorMessage){
            *errorMessage = query.lastError().text();
        }
        return false;
    }

    if(!query.next()){
        if(outHasProgress){
            *outHasProgress = false;
        }
        if(outSnapshotJson){
            outSnapshotJson->clear();
        }
        return true;
    }

    if(outHasProgress){
        *outHasProgress = query.value(0).toBool();
    }
    if(outSnapshotJson){
        *outSnapshotJson = query.value(1).toString();
    }

    return true;
}

bool UserProgressService::SaveUnlockedLevel(qint64 userId,
                                            int unlockedLevel,
                                            QString* errorMessage){
    if(userId <= 0 || unlockedLevel <= 0){
        if(errorMessage){
            *errorMessage = "参数非法";
        }
        return false;
    }

    if(!m_db.isValid() || !m_db.isOpen()){
        if(errorMessage){
            *errorMessage = "数据库未连接";
        }
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare("INSERT INTO user_level_state(user_id, unlocked_level) VALUES(:user_id, :unlocked_level) "
                  "ON CONFLICT(user_id) DO UPDATE SET "
                  "unlocked_level = excluded.unlocked_level, "
                  "updated_at = CURRENT_TIMESTAMP");
    query.bindValue(":user_id", userId);
    query.bindValue(":unlocked_level", unlockedLevel);

    if(!query.exec()){
        if(errorMessage){
            *errorMessage = query.lastError().text();
        }
        return false;
    }

    return true;
}

int UserProgressService::LoadUnlockedLevel(qint64 userId,
                                           int defaultUnlockedLevel,
                                           QString* errorMessage){
    if(userId <= 0){
        if(errorMessage){
            *errorMessage = "参数非法";
        }
        return defaultUnlockedLevel;
    }

    if(!m_db.isValid() || !m_db.isOpen()){
        if(errorMessage){
            *errorMessage = "数据库未连接";
        }
        return defaultUnlockedLevel;
    }

    QSqlQuery query(m_db);
    query.prepare("SELECT unlocked_level FROM user_level_state WHERE user_id = :user_id");
    query.bindValue(":user_id", userId);
    if(!query.exec()){
        if(errorMessage){
            *errorMessage = query.lastError().text();
        }
        return defaultUnlockedLevel;
    }

    if(!query.next()){
        return defaultUnlockedLevel;
    }

    return query.value(0).toInt();
}

bool UserProgressService::SaveBestTime(qint64 userId,
                                       const QString& modeType,
                                       int levelIndex,
                                       qint64 elapsedMs,
                                       QString* errorMessage){
    const QString safeModeType = modeType.trimmed();
    if(userId <= 0 || safeModeType.isEmpty() || elapsedMs <= 0){
        if(errorMessage){
            *errorMessage = "参数非法";
        }
        return false;
    }

    if(!m_db.isValid() || !m_db.isOpen()){
        if(errorMessage){
            *errorMessage = "数据库未连接";
        }
        return false;
    }

    const int safeLevelIndex = levelIndex > 0 ? levelIndex : 0;

    QSqlQuery query(m_db);
    query.prepare("INSERT INTO user_best_times(user_id, mode_type, level_index, best_time_ms) "
                  "VALUES(:user_id, :mode_type, :level_index, :best_time_ms) "
                  "ON CONFLICT(user_id, mode_type, level_index) DO UPDATE SET "
                  "best_time_ms = CASE WHEN excluded.best_time_ms < best_time_ms THEN excluded.best_time_ms ELSE best_time_ms END, "
                  "updated_at = CURRENT_TIMESTAMP");
    query.bindValue(":user_id", userId);
    query.bindValue(":mode_type", safeModeType);
    query.bindValue(":level_index", safeLevelIndex);
    query.bindValue(":best_time_ms", elapsedMs);

    if(!query.exec()){
        if(errorMessage){
            *errorMessage = query.lastError().text();
        }
        return false;
    }

    return true;
}

QVector<UserProgressService::RankRecord> UserProgressService::LoadModeLeaderboard(const QString& modeType,
                                                                                   int levelIndex,
                                                                                   int limit,
                                                                                   QString* errorMessage){
    QVector<RankRecord> rows;
    const QString safeModeType = modeType.trimmed();
    if(safeModeType.isEmpty() || limit <= 0){
        return rows;
    }

    if(!m_db.isValid() || !m_db.isOpen()){
        if(errorMessage){
            *errorMessage = "数据库未连接";
        }
        return rows;
    }

    const int safeLevelIndex = levelIndex > 0 ? levelIndex : 0;

    QSqlQuery query(m_db);
    query.prepare("SELECT ubt.user_id, u.nickname, ubt.best_time_ms "
                  "FROM user_best_times ubt "
                  "INNER JOIN users u ON u.id = ubt.user_id "
                  "WHERE ubt.mode_type = :mode_type AND ubt.level_index = :level_index "
                  "ORDER BY ubt.best_time_ms ASC, ubt.updated_at ASC "
                  "LIMIT :limit_count");
    query.bindValue(":mode_type", safeModeType);
    query.bindValue(":level_index", safeLevelIndex);
    query.bindValue(":limit_count", limit);

    if(!query.exec()){
        if(errorMessage){
            *errorMessage = query.lastError().text();
        }
        return rows;
    }

    while(query.next()){
        RankRecord row;
        row.userId = query.value(0).toLongLong();
        row.nickname = query.value(1).toString();
        row.bestTimeMs = query.value(2).toLongLong();
        rows.push_back(row);
    }

    return rows;
}