#include "levelselectdialog.h"

#include <QDialogButtonBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

int LevelSelectDialog::LevelCount(){
    return LevelConfigs().size();
}

const QVector<GameModeConfig>& LevelSelectDialog::LevelConfigs(){
    //关卡模式共11关。
    static const QVector<GameModeConfig> kLevelConfigs{
        GameModeConfig(6, 6, 6, -1, -1, 60),
        GameModeConfig(8, 6, 8, -1, -1, 90),
        GameModeConfig(10, 8, 8, 5, 5, 120),
        GameModeConfig(12, 8, 10, 5, 5, 100),
        GameModeConfig(14, 8, 12, 5, 5, 150),
        GameModeConfig(16, 10, 11, 3, 3, 180),
        GameModeConfig(17, 10, 12, 3, 3, 170),
        GameModeConfig(18, 10, 14, 2, 2, 230),
        GameModeConfig(19, 10, 15, 1, 1, 290),
        GameModeConfig(20, 10, 16, 0, 1, 360),
        GameModeConfig(20, 10, 16, 0, 0, 500)
    };
    return kLevelConfigs;
}

LevelSelectDialog::LevelSelectDialog(int unlockedLevel, QWidget* parent)
    : QDialog(parent),
      m_unlockedLevel(qBound(1, unlockedLevel, LevelCount())),
      m_selectedLevelIndex(-1),
      m_titleLabel(NULL),
      m_descLabel(NULL){
    InitUI();
    setWindowTitle("关卡选择");
    setModal(true);
    setFixedSize(620, 420);
}

int LevelSelectDialog::selectedLevelIndex() const{
    return m_selectedLevelIndex;
}

QString LevelSelectDialog::BuildLevelDescription(int levelIndex, const GameModeConfig& config) const{
    const QString tipText = (config.maxTipCount < 0)
        ? "提示不限"
        : (config.maxTipCount == 0 ? "禁用提示" : QString("提示上限%1次").arg(config.maxTipCount));

    const QString resetText = (config.maxResetCount < 0)
        ? "重排不限"
        : (config.maxResetCount == 0 ? "禁用重排" : QString("重排上限%1次").arg(config.maxResetCount));

    const QString levelName = (levelIndex == LevelCount())
        ? "第11关（终极挑战）"
        : QString("第%1关").arg(levelIndex);

    return QString("%1：%2种元素，地图%3x%4，%5，%6，限时%7秒")
        .arg(levelName)
        .arg(config.iconTypeCount)
        .arg(config.mapCols)
        .arg(config.mapRows)
        .arg(tipText)
        .arg(resetText)
        .arg(config.timeLimitSeconds);
}

void LevelSelectDialog::InitUI(){
    m_titleLabel = new QLabel(QString("已解锁：第1关 - 第%1关").arg(m_unlockedLevel), this);
    QFont titleFont("KaiTi", 26, QFont::Bold);
    m_titleLabel->setFont(titleFont);
    m_titleLabel->setAlignment(Qt::AlignCenter);

    m_descLabel = new QLabel("请选择要挑战的关卡", this);
    QFont descFont("KaiTi", 24, QFont::Bold);
    m_descLabel->setFont(descFont);
    m_descLabel->setWordWrap(true);
    m_descLabel->setAlignment(Qt::AlignCenter);
    m_descLabel->setFixedHeight(100);
    m_descLabel->setStyleSheet("QLabel{color:#3E2A13;background:#FFF5E8;border:1px solid #D9B68A;border-radius:10px;padding:8px 14px;}");

    QGridLayout* gridLayout = new QGridLayout;
    gridLayout->setHorizontalSpacing(10);
    gridLayout->setVerticalSpacing(10);
    QHBoxLayout* finalRowLayout = new QHBoxLayout;
    finalRowLayout->setContentsMargins(0, 0, 0, 0);
    finalRowLayout->setSpacing(0);

    m_levelButtons.clear();
    const QVector<GameModeConfig>& configs = LevelConfigs();
    QPushButton* finalChallengeButton = NULL;
    for(int i = 0; i < configs.size(); ++i){
        const int levelIndex = i + 1;
        QPushButton* button = new QPushButton(this);
        if(levelIndex == LevelCount()){
            button->setText("终极挑战");
        }else{
            button->setText(QString("第%1关").arg(levelIndex));
        }
        button->setMinimumSize(100, 42);
        button->setEnabled(levelIndex <= m_unlockedLevel);
        if(levelIndex <= m_unlockedLevel){
            if(levelIndex == LevelCount()){
                button->setStyleSheet(
                    "QPushButton{border:2px solid #8F4D17;border-radius:10px;background:#FFDCAA;color:#4A2A0D;font-weight:700;}"
                    "QPushButton:hover{background:#FFC57B;}"
                );
            }else{
                button->setStyleSheet(
                    "QPushButton{border:2px solid #A66A2E;border-radius:8px;background:#FFE8C8;color:#5B3714;font-weight:600;}"
                    "QPushButton:hover{background:#FFD39A;}"
                );
            }
        }else{
            if(levelIndex == LevelCount()){
                button->setStyleSheet(
                    "QPushButton{border:2px solid #B8A998;border-radius:10px;background:#ECE8E3;color:#93887A;font-weight:600;}"
                );
                button->setText("终极挑战(未解锁)");
            }else{
                button->setStyleSheet(
                    "QPushButton{border:2px solid #B9B0A5;border-radius:8px;background:#ECE8E3;color:#93887A;}"
                );
                button->setText(QString("第%1关(未解锁)").arg(levelIndex));
            }
        }

        connect(button, &QPushButton::clicked, this, [this, levelIndex, configs](){
            m_selectedLevelIndex = levelIndex;
            m_descLabel->setText(BuildLevelDescription(levelIndex, configs[levelIndex - 1]));
            accept();
        });

        if(levelIndex == LevelCount()){
            finalChallengeButton = button;
        }else{
            const int regularIndex = levelIndex - 1;
            gridLayout->addWidget(button, regularIndex / 5, regularIndex % 5);
        }
        m_levelButtons.push_back(button);
    }

    if(finalChallengeButton){
        finalChallengeButton->setMinimumSize(210, 56);
        QFont finalFont = finalChallengeButton->font();
        finalFont.setPointSize(finalFont.pointSize() + 2);
        finalFont.setBold(true);
        finalChallengeButton->setFont(finalFont);

        finalRowLayout->addStretch();
        finalRowLayout->addWidget(finalChallengeButton);
        finalRowLayout->addStretch();
    }

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel, this);
    buttonBox->button(QDialogButtonBox::Cancel)->setText("返回");
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(m_titleLabel);
    mainLayout->addWidget(m_descLabel);
    mainLayout->addLayout(gridLayout);
    mainLayout->addLayout(finalRowLayout);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);
}
