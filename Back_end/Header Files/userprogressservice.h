#ifndef USERPROGRESSSERVICE_H
#define USERPROGRESSSERVICE_H

#include <QtCore/QString>
#include <QtCore/QVector>
#include <QtSql/QSqlDatabase>

class UserProgressService{
public:
    struct RankRecord{
        qint64 userId;
        QString nickname;
        qint64 bestTimeMs;
    };

    UserProgressService();
    ~UserProgressService();

    bool Open(const QString& databaseFilePath,
              QString* errorMessage = NULL);
    void Close();

    bool EnsureSchema(QString* errorMessage = NULL);

    bool SaveModeProgress(qint64 userId,
                          const QString& modeType,
                          bool hasProgress,
                          const QString& snapshotJson,
                          QString* errorMessage = NULL);

    bool LoadModeProgress(qint64 userId,
                          const QString& modeType,
                          bool* outHasProgress,
                          QString* outSnapshotJson,
                          QString* errorMessage = NULL);

    bool SaveUnlockedLevel(qint64 userId,
                           int unlockedLevel,
                           QString* errorMessage = NULL);

    int LoadUnlockedLevel(qint64 userId,
                          int defaultUnlockedLevel,
                          QString* errorMessage = NULL);

    bool SaveBestTime(qint64 userId,
                      const QString& modeType,
                      int levelIndex,
                      qint64 elapsedMs,
                      QString* errorMessage = NULL);

    QVector<RankRecord> LoadModeLeaderboard(const QString& modeType,
                                            int levelIndex,
                                            int limit,
                                            QString* errorMessage = NULL);

private:
    QString m_connectionName;
    QSqlDatabase m_db;
};

#endif