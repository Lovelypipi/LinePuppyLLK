#include "helpdialog.h"

#include <QtMultimedia/QAudioOutput>
#include <QtMultimedia/QMediaPlayer>
#include <QtMultimediaWidgets/QVideoWidget>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

HelpDialog::HelpDialog(QWidget* parent)
    : QDialog(parent),
      m_titleLabel(NULL),
      m_introLabel(NULL),
      m_methodHintLabel(NULL),
      m_videoWidget(NULL),
      m_btnMethodOne(NULL),
      m_btnMethodTwo(NULL),
      m_btnMethodThree(NULL),
      m_player(NULL),
      m_audioOutput(NULL){
    InitUI();
    SetupMedia();
    SelectMethod(1);
}

void HelpDialog::InitUI(){
    setWindowTitle("游戏帮助");
    setModal(true);
    setMinimumSize(840, 660);

    m_titleLabel = new QLabel("《欢乐连连看》玩法说明", this);
    QFont titleFont("KaiTi", 18, QFont::Bold);
    m_titleLabel->setFont(titleFont);

    m_introLabel = new QLabel(this);
    m_introLabel->setWordWrap(true);
    m_introLabel->setText(
        "游戏目标：在尽量短的时间内消除全部图块。\n"
        "消除规则：选中两个相同图块，若二者可通过不穿过其他图块的折线路径连接（最多两次转折），则可消除。\n"
        "操作说明：\n"
        "1、开始游戏：开始或继续本局\n"
        "2、暂停游戏：暂停计时与操作\n"
        "3、提示：显示一组可消除图块\n"
        "4、重排：打乱剩余图块且保证地图可解\n"
        "5、退出：可选择是否保留本局进度\n"
    );

    QLabel* methodTitle = new QLabel("基础玩法演示（点击下方按钮切换）", this);
    QFont subtitleFont("KaiTi", 13, QFont::Bold);
    methodTitle->setFont(subtitleFont);

    m_videoWidget = new QVideoWidget(this);
    m_videoWidget->setMinimumHeight(300);

    m_methodHintLabel = new QLabel(this);
    m_methodHintLabel->setWordWrap(true);

    m_btnMethodOne = new QPushButton("方法一：直线连接", this);
    m_btnMethodTwo = new QPushButton("方法二：一次拐弯", this);
    m_btnMethodThree = new QPushButton("方法三：两次拐弯", this);

    connect(m_btnMethodOne, &QPushButton::clicked, this, [this](){ SelectMethod(1); });
    connect(m_btnMethodTwo, &QPushButton::clicked, this, [this](){ SelectMethod(2); });
    connect(m_btnMethodThree, &QPushButton::clicked, this, [this](){ SelectMethod(3); });

    QHBoxLayout* methodButtonLayout = new QHBoxLayout;
    methodButtonLayout->addWidget(m_btnMethodOne);
    methodButtonLayout->addWidget(m_btnMethodTwo);
    methodButtonLayout->addWidget(m_btnMethodThree);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, this);
    buttonBox->button(QDialogButtonBox::Ok)->setText("我知道了");
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(m_titleLabel);
    mainLayout->addWidget(m_introLabel);
    mainLayout->addWidget(methodTitle);
    mainLayout->addWidget(m_videoWidget, 1);
    mainLayout->addWidget(m_methodHintLabel);
    mainLayout->addLayout(methodButtonLayout);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);
}

void HelpDialog::SetupMedia(){
    m_audioOutput = new QAudioOutput(this);
    m_audioOutput->setVolume(0.0f);

    m_player = new QMediaPlayer(this);
    m_player->setAudioOutput(m_audioOutput);
    m_player->setVideoOutput(m_videoWidget);
    m_player->setLoops(QMediaPlayer::Infinite);

    connect(m_player, &QMediaPlayer::errorOccurred, this, [this](QMediaPlayer::Error, const QString& errorString){
        if(!errorString.isEmpty()){
            m_methodHintLabel->setText(QString("演示视频加载失败：%1").arg(errorString));
        }
    });
}

void HelpDialog::SelectMethod(int methodIndex){
    QString sourcePath;
    QString hintText;

    if(methodIndex == 1){
        sourcePath = "qrc:/res/Remove_MethodOne.mp4";
        hintText = "方法一：两个相同图块在同一行或同一列，中间无遮挡时可直接连线消除。";
    }else if(methodIndex == 2){
        sourcePath = "qrc:/res/Remove_MethodTwo.mp4";
        hintText = "方法二：两个相同图块可通过一个拐点连接时（一次拐弯）可以消除。";
    }else{
        sourcePath = "qrc:/res/Remove_MethodThree.mp4";
        hintText = "方法三：两个相同图块可通过两个拐点连接时（两次拐弯）可以消除。";
    }

    m_btnMethodOne->setEnabled(methodIndex != 1);
    m_btnMethodTwo->setEnabled(methodIndex != 2);
    m_btnMethodThree->setEnabled(methodIndex != 3);

    m_methodHintLabel->setText(hintText);
    m_player->setSource(QUrl(sourcePath));
    m_player->play();
}
