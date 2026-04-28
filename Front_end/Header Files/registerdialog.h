#ifndef REGISTERDIALOG_H
#define REGISTERDIALOG_H

#include "stdafx.h"
#include "userauthservice.h"

class QLabel;
class QLineEdit;
class QPushButton;

class RegisterDialog : public QDialog{
    Q_OBJECT
public:
    explicit RegisterDialog(QWidget* parent = NULL);

    QString registeredUsername() const;

private slots:
    void OnRefreshCaptchaClicked();
    void OnRegisterClicked();

private:
    void InitUI();
    bool InitDatabase();
    QString GenerateCaptchaCode() const;
    void RefreshCaptcha();

    QLineEdit* m_editUsername;
    QLineEdit* m_editPassword;
    QLineEdit* m_editConfirmPassword;
    QLineEdit* m_editNickname;
    QLineEdit* m_editCaptcha;
    QLabel* m_labelCaptchaCode;
    QLabel* m_labelStatus;
    QPushButton* m_btnRefreshCaptcha;
    QPushButton* m_btnRegister;
    QPushButton* m_btnCancel;

    QString m_currentCaptchaCode;
    QString m_registeredUsername;
    UserAuthService m_authService;
};

#endif
