#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include "stdafx.h"
#include "userauthservice.h"

class QLabel;
class QLineEdit;
class QPushButton;

class LoginDialog : public QDialog{
    Q_OBJECT
public:
    explicit LoginDialog(QWidget* parent = NULL);

    qint64 loggedInUserId() const;
    QString loggedInNickname() const;

private slots:
    void OnLoginClicked();
    void OnOpenRegisterClicked();

private:
    void InitUI();
    bool InitDatabase();

    QLineEdit* m_editUsername;
    QLineEdit* m_editPassword;
    QLabel* m_labelStatus;
    QPushButton* m_btnLogin;
    QPushButton* m_btnOpenRegister;
    QPushButton* m_btnCancel;

    UserAuthService m_authService;
    qint64 m_loggedInUserId;
    QString m_loggedInNickname;
};

#endif
