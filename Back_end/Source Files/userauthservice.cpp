#include "userauthservice.h"

#include <QtCore/QByteArray>
#include <QtCore/QCryptographicHash>
#include <QtCore/QDir>
#include <QtCore/QRegularExpression>
#include <QtCore/QRandomGenerator>
#include <QtCore/QUuid>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

namespace{
bool IsValidUsername(const QString& username){
    static const QRegularExpression pattern(QStringLiteral("^[A-Za-z0-9_]{3,20}$"));
    return pattern.match(username).hasMatch();
}

bool IsValidPassword(const QString& password){
    if(password.length() < 6 || password.length() > 15){
        return false;
    }

    static const QRegularExpression alpha(QStringLiteral("[A-Za-z]"));
    static const QRegularExpression digit(QStringLiteral("[0-9]"));
    return alpha.match(password).hasMatch() && digit.match(password).hasMatch();
}
}

UserAuthService::UserAuthService()
    : m_connectionName(QUuid::createUuid().toString(QUuid::WithoutBraces)){
}

UserAuthService::~UserAuthService(){
    Close();
}

bool UserAuthService::Open(const QString& databaseFilePath,
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

void UserAuthService::Close(){
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

bool UserAuthService::EnsureSchema(QString* errorMessage){
    if(!m_db.isValid() || !m_db.isOpen()){
        if(errorMessage){
            *errorMessage = "数据库未连接";
        }
        return false;
    }

    QSqlQuery query(m_db);
    const QString createUsersSql =
        "CREATE TABLE IF NOT EXISTS users ("
        " id INTEGER PRIMARY KEY AUTOINCREMENT,"
        " username VARCHAR(32) NOT NULL UNIQUE,"
        " password_hash VARCHAR(128) NOT NULL,"
        " password_salt VARCHAR(64) NOT NULL,"
        " nickname VARCHAR(64) NOT NULL,"
        " created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        " updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        " last_login_at TEXT NULL DEFAULT NULL"
        ")";

    if(!query.exec(createUsersSql)){
        if(errorMessage){
            *errorMessage = query.lastError().text();
        }
        return false;
    }

    const QString createLoginLogsSql =
        "CREATE TABLE IF NOT EXISTS login_logs ("
        " id INTEGER PRIMARY KEY AUTOINCREMENT,"
        " user_id INTEGER NOT NULL,"
        " login_time TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        " success TINYINT(1) NOT NULL,"
        " message VARCHAR(255) NULL,"
        " FOREIGN KEY (user_id) REFERENCES users(id)"
        ")";

    if(!query.exec(createLoginLogsSql)){
        if(errorMessage){
            *errorMessage = query.lastError().text();
        }
        return false;
    }

    return true;
}

bool UserAuthService::RegisterUser(const QString& username,
                                   const QString& password,
                                   const QString& nickname,
                                   QString* errorMessage){
    const QString trimmedUsername = username.trimmed();
    if(trimmedUsername.isEmpty() || password.isEmpty()){
        if(errorMessage){
            *errorMessage = "用户名和密码不能为空";
        }
        return false;
    }

    if(!IsValidUsername(trimmedUsername)){
        if(errorMessage){
            *errorMessage = "用户名只能使用英文字母、数字和下划线，长度为3-20位";
        }
        return false;
    }

    if(!IsValidPassword(password)){
        if(errorMessage){
            *errorMessage = "密码长度为6-15位，且至少包含英文字母和数字";
        }
        return false;
    }

    if(!m_db.isValid() || !m_db.isOpen()){
        if(errorMessage){
            *errorMessage = "数据库未连接";
        }
        return false;
    }

    const QString safeNickname = nickname.trimmed().isEmpty() ? trimmedUsername : nickname.trimmed();
    const QString saltHex = GenerateSaltHex();
    const QString passwordHash = BuildPasswordHash(password, saltHex);

    QSqlQuery query(m_db);
    query.prepare("INSERT INTO users(username, password_hash, password_salt, nickname) "
                  "VALUES(:username, :password_hash, :password_salt, :nickname)");
    query.bindValue(":username", trimmedUsername);
    query.bindValue(":password_hash", passwordHash);
    query.bindValue(":password_salt", saltHex);
    query.bindValue(":nickname", safeNickname);

    if(!query.exec()){
        if(errorMessage){
            *errorMessage = query.lastError().text();
        }
        return false;
    }

    return true;
}

bool UserAuthService::VerifyLogin(const QString& username,
                                  const QString& password,
                                  qint64* outUserId,
                                  QString* outNickname,
                                  QString* errorMessage){
    const QString trimmedUsername = username.trimmed();
    if(trimmedUsername.isEmpty() || password.isEmpty()){
        if(errorMessage){
            *errorMessage = "用户名和密码不能为空";
        }
        return false;
    }

    if(!IsValidUsername(trimmedUsername)){
        if(errorMessage){
            *errorMessage = "用户名只能使用英文字母、数字和下划线，长度为3-20位";
        }
        return false;
    }

    if(!IsValidPassword(password)){
        if(errorMessage){
            *errorMessage = "密码长度为6-15位，且至少包含英文字母和数字";
        }
        return false;
    }

    if(!m_db.isValid() || !m_db.isOpen()){
        if(errorMessage){
            *errorMessage = "数据库未连接";
        }
        return false;
    }

    QSqlQuery selectQuery(m_db);
    selectQuery.prepare("SELECT id, password_hash, password_salt, nickname FROM users WHERE username = :username");
    selectQuery.bindValue(":username", trimmedUsername);
    if(!selectQuery.exec()){
        if(errorMessage){
            *errorMessage = selectQuery.lastError().text();
        }
        return false;
    }

    if(!selectQuery.next()){
        if(errorMessage){
            *errorMessage = "用户不存在";
        }
        return false;
    }

    const qint64 userId = selectQuery.value(0).toLongLong();
    const QString dbHash = selectQuery.value(1).toString();
    const QString dbSalt = selectQuery.value(2).toString();
    const QString dbNickname = selectQuery.value(3).toString();
    const QString inputHash = BuildPasswordHash(password, dbSalt);
    const bool success = (dbHash == inputHash);

    QSqlQuery logQuery(m_db);
    logQuery.prepare("INSERT INTO login_logs(user_id, success, message) VALUES(:user_id, :success, :message)");
    logQuery.bindValue(":user_id", userId);
    logQuery.bindValue(":success", success ? 1 : 0);
    logQuery.bindValue(":message", success ? "login_success" : "password_incorrect");
    logQuery.exec();

    if(!success){
        if(errorMessage){
            *errorMessage = "密码错误";
        }
        return false;
    }

    QSqlQuery updateQuery(m_db);
    updateQuery.prepare("UPDATE users SET last_login_at = CURRENT_TIMESTAMP WHERE id = :id");
    updateQuery.bindValue(":id", userId);
    updateQuery.exec();

    if(outUserId){
        *outUserId = userId;
    }
    if(outNickname){
        *outNickname = dbNickname;
    }

    return true;
}

//生成一个随机的盐值，并以十六进制字符串形式返回。它使用QRandomGenerator生成16字节的随机数据，然后将其转换为十六进制字符串。
QString UserAuthService::GenerateSaltHex(){
    QByteArray saltBytes;
    saltBytes.reserve(16);
    for(int i = 0; i < 16; ++i){
        saltBytes.append(static_cast<char>(QRandomGenerator::global()->bounded(0, 256)));
    }
    return QString::fromLatin1(saltBytes.toHex());
}

QString UserAuthService::BuildPasswordHash(const QString& password, const QString& saltHex){
    QByteArray digest = (saltHex.toUtf8() + password.toUtf8());
    //使用多轮哈希提升安全性能
    for(int i = 0; i < 20000; i++){
        digest = QCryptographicHash::hash(digest, QCryptographicHash::Sha256);
    }
    return QString::fromLatin1(digest.toHex());
}