#include "logindialog.h"
#include "registerdialog.h"
#include "userprogressservice.h"
#include "dbconfighelper.h"

#include <QtWidgets/QFormLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

/*namespace内的函数仅在LoginDialog.cpp中使用，且不依赖于LoginDialog类成员，
放在匿名namespace中实现，避免与其他文件中的同名函数冲突*/
namespace{
QLabel* MakeRequiredLabel(const QString& text){
    QLabel* label = new QLabel(QString("<span style=\"color:#d32f2f;\">*</span> %1").arg(text));
    label->setTextFormat(Qt::RichText);
    return label;
}
}

LoginDialog::LoginDialog(QWidget* parent)
    : QDialog(parent),
      m_editUsername(NULL),
      m_editPassword(NULL),
      m_labelStatus(NULL),
      m_btnLogin(NULL),
            m_btnOpenRegister(NULL),
    m_btnCancel(NULL),
    m_loggedInUserId(0){
    InitUI();

    const bool dbReady = InitDatabase();
    if(!dbReady){
        m_btnLogin->setEnabled(false);
        m_btnOpenRegister->setEnabled(false);
    }

    setWindowTitle("玩家登录");
    setModal(true);
    setFixedWidth(420);
}

qint64 LoginDialog::loggedInUserId() const{
    return m_loggedInUserId;
}

QString LoginDialog::loggedInNickname() const{
    return m_loggedInNickname;
}

void LoginDialog::InitUI(){
    m_editUsername = new QLineEdit(this);
    m_editPassword = new QLineEdit(this);
    m_labelStatus = new QLabel("带*为必填项", this);

    m_editUsername->setPlaceholderText("3-20位，只能使用英文字母、数字和下划线");
    m_editPassword->setPlaceholderText("6-15位，至少由英文字母+数字组成");
    m_editPassword->setEchoMode(QLineEdit::Password);

    m_btnLogin = new QPushButton("登录", this);
    m_btnOpenRegister = new QPushButton("去注册", this);
    m_btnCancel = new QPushButton("取消", this);

    connect(m_btnLogin, &QPushButton::clicked, this, &LoginDialog::OnLoginClicked);
    connect(m_btnOpenRegister, &QPushButton::clicked, this, &LoginDialog::OnOpenRegisterClicked);
    connect(m_btnCancel, &QPushButton::clicked, this, &QDialog::reject);

    QFormLayout* formLayout = new QFormLayout;
    formLayout->addRow(MakeRequiredLabel("用户名："), m_editUsername);
    formLayout->addRow(MakeRequiredLabel("密码："), m_editPassword);

    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_btnOpenRegister);
    buttonLayout->addWidget(m_btnLogin);
    buttonLayout->addWidget(m_btnCancel);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(m_labelStatus);
    mainLayout->addLayout(formLayout);
    mainLayout->addLayout(buttonLayout);
    setLayout(mainLayout);
}

bool LoginDialog::InitDatabase(){
    DbConnectionConfig config;
    QString err;
    if(!DbConfigHelper::LoadConfig(&config, &err)){
        m_labelStatus->setText(err);
        QMessageBox::critical(this, "数据库连接失败", err);
        return false;
    }

    if(!m_authService.Open(config.databaseFilePath, &err)){
        m_labelStatus->setText("数据库连接失败：" + err);
        QMessageBox::critical(this, "数据库连接失败", err);
        return false;
    }

    if(!m_authService.EnsureSchema(&err)){
        m_labelStatus->setText("数据库初始化失败：" + err);
        QMessageBox::critical(this, "数据库初始化失败", err);
        return false;
    }

    UserProgressService progressService;
    if(!progressService.Open(config.databaseFilePath, &err)){
        m_labelStatus->setText("进度服务连接失败：" + err);
        QMessageBox::critical(this, "数据库初始化失败", err);
        return false;
    }
    if(!progressService.EnsureSchema(&err)){
        m_labelStatus->setText("进度表初始化失败：" + err);
        QMessageBox::critical(this, "数据库初始化失败", err);
        return false;
    }

    m_labelStatus->setText("  带*为必填项");
    return true;
}

void LoginDialog::OnLoginClicked(){
    const QString username = m_editUsername->text().trimmed();
    const QString password = m_editPassword->text();

    QString err;
    qint64 userId = 0;
    QString nickname;
    if(!m_authService.VerifyLogin(username, password, &userId, &nickname, &err)){
        m_labelStatus->setText("登录失败：" + err);
        QMessageBox::warning(this, "登录失败", err);
        return;
    }

    m_loggedInUserId = userId;
    m_loggedInNickname = nickname.trimmed().isEmpty() ? username : nickname;
    m_labelStatus->setText(QString("登录成功，欢迎你：%1").arg(m_loggedInNickname));
    accept();
}

void LoginDialog::OnOpenRegisterClicked(){
    RegisterDialog registerDialog(this);
    if(registerDialog.exec() == QDialog::Accepted){
        const QString username = registerDialog.registeredUsername().trimmed();
        if(!username.isEmpty()){
            m_editUsername->setText(username);
        }
        m_editPassword->clear();
        m_editPassword->setFocus();
        m_labelStatus->setText("注册成功，请使用新账号登录。");
    }
}
