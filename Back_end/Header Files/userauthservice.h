#ifndef USERAUTHSERVICE_H
#define USERAUTHSERVICE_H

#include <QtCore/QString>
#include <QtSql/QSqlDatabase>

class UserAuthService{
public:
    UserAuthService();
    ~UserAuthService();

    bool Open(const QString& databaseFilePath,
              QString* errorMessage = NULL);
    void Close();

    bool EnsureSchema(QString* errorMessage = NULL);

    bool RegisterUser(const QString& username,
                      const QString& password,
                      const QString& nickname,
                      QString* errorMessage = NULL);

    bool VerifyLogin(const QString& username,
                     const QString& password,
                     qint64* outUserId = NULL,
                     QString* outNickname = NULL,
                     QString* errorMessage = NULL);

private:
    QString m_connectionName;
    QSqlDatabase m_db;

    static QString GenerateSaltHex();
    static QString BuildPasswordHash(const QString& password, const QString& saltHex);
};

#endif
