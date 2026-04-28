#include "registerdialog.h"

#include "dbconfighelper.h"

#include <QtCore/QRandomGenerator>
#include <QtGui/QBrush>
#include <QtGui/QColor>
#include <QtGui/QFont>
#include <QtGui/QFontMetrics>
#include <QtGui/QImage>
#include <QtGui/QLinearGradient>
#include <QtGui/QPainter>
#include <QtGui/QPen>
#include <QtGui/QPainterPath>
#include <QtGui/QPixmap>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

namespace{
QLabel* MakeRequiredLabel(const QString& text){
    QLabel* label = new QLabel(QString("<span style=\"color:#d32f2f;\">*</span> %1").arg(text));
    label->setTextFormat(Qt::RichText);
    return label;
}

QPixmap BuildCaptchaPixmap(const QString& code, const QSize& size){
    QImage image(size, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    QLinearGradient gradient(0, 0, 0, size.height());
    gradient.setColorAt(0.0, QColor("#FFF5E9"));
    gradient.setColorAt(1.0, QColor("#F1D3A4"));
    painter.setPen(QPen(QColor("#A87437"), 2));
    painter.setBrush(gradient);
    painter.drawRoundedRect(image.rect().adjusted(1, 1, -2, -2), 10, 10);

    painter.setPen(QPen(QColor(183, 141, 78, 180), 1));
    for(int i = 0; i < 5; ++i){
        const int y = QRandomGenerator::global()->bounded(6, size.height() - 6);
        painter.drawLine(5,
                         y,
                         size.width() - 5,
                         y + QRandomGenerator::global()->bounded(-4, 5));
    }

    painter.setPen(QPen(QColor(130, 96, 48, 100), 1));
    for(int x = 0; x < size.width(); x += 14){
        painter.drawLine(x, 4, x + 16, size.height() - 4);
    }
    for(int x = -6; x < size.width(); x += 20){
        painter.drawLine(x, size.height() - 4, x + 16, 4);
    }

    painter.setPen(QPen(QColor(157, 112, 55, 150), 1));
    for(int i = 0; i < 3; ++i){
        const int startY = QRandomGenerator::global()->bounded(8, size.height() - 8);
        const int midY = QRandomGenerator::global()->bounded(4, size.height() - 4);
        const int endY = QRandomGenerator::global()->bounded(8, size.height() - 8);
        painter.drawLine(4, startY, size.width() / 2, midY);
        painter.drawLine(size.width() / 2, midY, size.width() - 4, endY);
    }

    for(int i = 0; i < 2; ++i){
        QPainterPath curve;
        const qreal top = QRandomGenerator::global()->bounded(6, size.height() - 18);
        const qreal amplitude = QRandomGenerator::global()->bounded(3, 7);
        const qreal phase = QRandomGenerator::global()->bounded(0, 360) / 57.2958;
        curve.moveTo(4, top);
        for(int x = 4; x <= size.width() - 4; x += 4){
            const qreal y = top + qSin((x / 9.0) + phase) * amplitude;
            curve.lineTo(x, y);
        }
        painter.setPen(QPen(QColor(122, 74, 32, 160), 1));
        painter.drawPath(curve);
    }

    for(int i = 0; i < 70; ++i){
        const QColor dotColor(145 + QRandomGenerator::global()->bounded(95),
                              90 + QRandomGenerator::global()->bounded(85),
                              35 + QRandomGenerator::global()->bounded(70),
                              90 + QRandomGenerator::global()->bounded(70));
        painter.setPen(Qt::NoPen);
        painter.setBrush(dotColor);
        const int x = QRandomGenerator::global()->bounded(4, size.width() - 4);
        const int y = QRandomGenerator::global()->bounded(3, size.height() - 3);
        const qreal radius = QRandomGenerator::global()->bounded(1, 3);
        painter.drawEllipse(QPointF(x, y), radius, radius);
    }

    QFont font("KaiTi", 24, QFont::Bold);
    painter.setFont(font);
    QFontMetrics metrics(font);
    const int contentWidth = metrics.horizontalAdvance(code) + 18;
    const int startX = qMax(8, (size.width() - contentWidth) / 2);
    const int baseline = size.height() / 2 + metrics.ascent() / 2 - 2;

    int currentX = startX;
    for(int i = 0; i < code.size(); ++i){
        const QString ch = code.mid(i, 1);
        const int charWidth = qMax(15, metrics.horizontalAdvance(ch));
        const int angle = QRandomGenerator::global()->bounded(-20, 21);
        const qreal scaleX = 0.92 + (QRandomGenerator::global()->bounded(0, 16) / 100.0);
        const qreal scaleY = 0.92 + (QRandomGenerator::global()->bounded(0, 16) / 100.0);
        const qreal shearX = QRandomGenerator::global()->bounded(-10, 11) / 100.0;
        const qreal shearY = QRandomGenerator::global()->bounded(-6, 7) / 100.0;
        const int yOffset = QRandomGenerator::global()->bounded(-6, 7);
        const int xOffset = QRandomGenerator::global()->bounded(-2, 3);
        const QColor chColor(65 + QRandomGenerator::global()->bounded(95),
                             28 + QRandomGenerator::global()->bounded(75),
                             8 + QRandomGenerator::global()->bounded(45));

        painter.save();
        painter.translate(currentX + charWidth / 2 + xOffset, baseline + yOffset);
        painter.shear(shearX, shearY);
        painter.rotate(angle);
        painter.scale(scaleX, scaleY);
        painter.setPen(chColor);
        painter.drawText(QPointF(-charWidth / 2, 0), ch);
        painter.restore();

        currentX += charWidth - 1 + QRandomGenerator::global()->bounded(0, 4);
    }

    painter.setPen(QPen(QColor(110, 68, 25, 160), 2));
    for(int i = 0; i < 5; ++i){
        const int y1 = QRandomGenerator::global()->bounded(3, size.height() - 3);
        const int y2 = QRandomGenerator::global()->bounded(3, size.height() - 3);
        const int bend = QRandomGenerator::global()->bounded(-10, 11);
        painter.drawLine(4, y1, size.width() / 2, y2 + bend);
        painter.drawLine(size.width() / 2, y2 + bend, size.width() - 4, QRandomGenerator::global()->bounded(3, size.height() - 3));
    }

    painter.setPen(QPen(QColor("#8B5A2B"), 1));
    painter.drawRoundedRect(image.rect().adjusted(1, 1, -2, -2), 10, 10);

    return QPixmap::fromImage(image);
}
}

RegisterDialog::RegisterDialog(QWidget* parent)
    : QDialog(parent),
      m_editUsername(NULL),
      m_editPassword(NULL),
      m_editConfirmPassword(NULL),
      m_editNickname(NULL),
      m_editCaptcha(NULL),
      m_labelCaptchaCode(NULL),
      m_labelStatus(NULL),
      m_btnRefreshCaptcha(NULL),
      m_btnRegister(NULL),
      m_btnCancel(NULL){
    InitUI();

    const bool dbReady = InitDatabase();
    if(!dbReady){
        m_btnRegister->setEnabled(false);
        m_btnRefreshCaptcha->setEnabled(false);
    }

    RefreshCaptcha();

    setWindowTitle("玩家注册");
    setModal(true);
    setFixedWidth(430);
}

QString RegisterDialog::registeredUsername() const{
    return m_registeredUsername;
}

void RegisterDialog::InitUI(){
    m_editUsername = new QLineEdit(this);
    m_editPassword = new QLineEdit(this);
    m_editConfirmPassword = new QLineEdit(this);
    m_editNickname = new QLineEdit(this);
    m_editCaptcha = new QLineEdit(this);
    m_labelCaptchaCode = new QLabel(this);
    m_labelStatus = new QLabel("带*为必填项", this);

    m_editUsername->setPlaceholderText("3-20位，只能使用英文字母、数字和下划线");
    m_editPassword->setPlaceholderText("6-15位，至少由英文字母+数字组成");
    m_editConfirmPassword->setPlaceholderText("请再次输入密码");
    m_editNickname->setPlaceholderText("可选，不填默认为用户名");
    m_editCaptcha->setPlaceholderText("请输入右侧验证码");

    m_editPassword->setEchoMode(QLineEdit::Password);
    m_editConfirmPassword->setEchoMode(QLineEdit::Password);

    m_labelCaptchaCode->setFixedSize(122, 46);
    m_labelCaptchaCode->setAlignment(Qt::AlignCenter);
    m_labelCaptchaCode->setCursor(Qt::PointingHandCursor);
    m_labelCaptchaCode->setStyleSheet("QLabel{background:transparent;}");

    m_btnRefreshCaptcha = new QPushButton("刷新验证码", this);
    m_btnRegister = new QPushButton("注册", this);
    m_btnCancel = new QPushButton("取消", this);

    m_btnRefreshCaptcha->setStyleSheet(
        "QPushButton{border:2px solid #A66A2E;border-radius:10px;background:#FFE8C8;color:#5B3714;font-weight:700;padding:5px 10px;}"
        "QPushButton:hover{background:#FFD39A;}"
        "QPushButton:pressed{background:#F3C27C;}"
    );
    m_btnRegister->setStyleSheet(
        "QPushButton{border:2px solid #8F4D17;border-radius:10px;background:#FFDCAA;color:#4A2A0D;font-weight:700;padding:5px 14px;}"
        "QPushButton:hover{background:#FFC57B;}"
        "QPushButton:pressed{background:#E7B86F;}"
    );
    m_btnCancel->setStyleSheet(
        "QPushButton{border:2px solid #B8A998;border-radius:10px;background:#ECE8E3;color:#93887A;font-weight:600;padding:5px 14px;}"
        "QPushButton:hover{background:#DDD7CF;}"
        "QPushButton:pressed{background:#D2CBC1;}"
    );

    connect(m_btnRefreshCaptcha, &QPushButton::clicked, this, &RegisterDialog::OnRefreshCaptchaClicked);
    connect(m_btnRegister, &QPushButton::clicked, this, &RegisterDialog::OnRegisterClicked);
    connect(m_btnCancel, &QPushButton::clicked, this, &QDialog::reject);

    QHBoxLayout* captchaLayout = new QHBoxLayout;
    captchaLayout->setContentsMargins(0, 0, 0, 0);
    captchaLayout->setSpacing(8);
    captchaLayout->addWidget(m_editCaptcha, 1);
    captchaLayout->addWidget(m_labelCaptchaCode);
    captchaLayout->addWidget(m_btnRefreshCaptcha);

    QFormLayout* formLayout = new QFormLayout;
    formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    formLayout->addRow(MakeRequiredLabel("用户名："), m_editUsername);
    formLayout->addRow(MakeRequiredLabel("密码："), m_editPassword);
    formLayout->addRow(MakeRequiredLabel("确认密码："), m_editConfirmPassword);
    formLayout->addRow("  昵称：", m_editNickname);

    QLabel* captchaLabel = MakeRequiredLabel("验证码：");
    QWidget* captchaLabelHost = new QWidget(this);
    QVBoxLayout* captchaLabelLayout = new QVBoxLayout(captchaLabelHost);
    captchaLabelLayout->setContentsMargins(0, 7, 0, 0);
    captchaLabelLayout->setSpacing(0);
    captchaLabelLayout->addWidget(captchaLabel);
    formLayout->addRow(captchaLabelHost, captchaLayout);

    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_btnRegister);
    buttonLayout->addWidget(m_btnCancel);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(m_labelStatus);
    mainLayout->addLayout(formLayout);
    mainLayout->addLayout(buttonLayout);
    setLayout(mainLayout);
}

bool RegisterDialog::InitDatabase(){
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

    m_labelStatus->setText("  带*为必填项");
    return true;
}

QString RegisterDialog::GenerateCaptchaCode() const{
    static const QString chars("ABCDEFGHJKLMNPQRSTUVWXYZ23456789");
    QString code;
    code.reserve(5);
    for(int i = 0; i < 5; ++i){
        const int index = QRandomGenerator::global()->bounded(chars.size());
        code.append(chars.at(index));
    }
    return code;
}

void RegisterDialog::RefreshCaptcha(){
    m_currentCaptchaCode = GenerateCaptchaCode();
    m_labelCaptchaCode->setPixmap(BuildCaptchaPixmap(m_currentCaptchaCode, m_labelCaptchaCode->size()));
    m_labelCaptchaCode->setAlignment(Qt::AlignCenter);
}

void RegisterDialog::OnRefreshCaptchaClicked(){
    RefreshCaptcha();
}

void RegisterDialog::OnRegisterClicked(){
    const QString username = m_editUsername->text().trimmed();
    const QString password = m_editPassword->text();
    const QString confirmPassword = m_editConfirmPassword->text();
    const QString nickname = m_editNickname->text().trimmed();
    const QString captchaInput = m_editCaptcha->text().trimmed().toUpper();

    if(password != confirmPassword){
        m_labelStatus->setText("注册失败：两次输入的密码不一致");
        QMessageBox::warning(this, "注册失败", "两次输入的密码不一致，请重新输入。");
        m_editConfirmPassword->clear();
        m_editConfirmPassword->setFocus();
        return;
    }

    if(captchaInput.isEmpty() || captchaInput != m_currentCaptchaCode){
        m_labelStatus->setText("注册失败：验证码错误");
        QMessageBox::warning(this, "注册失败", "验证码错误，请重试。");
        m_editCaptcha->clear();
        RefreshCaptcha();
        m_editCaptcha->setFocus();
        return;
    }

    QString err;
    if(!m_authService.RegisterUser(username, password, nickname, &err)){
        m_labelStatus->setText("注册失败：" + err);
        QMessageBox::warning(this, "注册失败", err);
        m_editCaptcha->clear();
        RefreshCaptcha();
        return;
    }

    m_registeredUsername = username;
    m_labelStatus->setText("注册成功，请返回登录。");
    QMessageBox::information(this, "注册成功", "用户已创建，请返回登录。");
    accept();
}
