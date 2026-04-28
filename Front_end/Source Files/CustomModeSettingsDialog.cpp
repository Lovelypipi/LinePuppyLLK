#include "CustomModeSettingsDialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QMessageBox>
#include <QVBoxLayout>

CustomModeSettingsDialog::CustomModeSettingsDialog(const GameModeConfig& initialConfig, QWidget* parent)
    : QDialog(parent),
            m_spinIconTypeCount(NULL),
    m_spinMapRows(NULL),
    m_spinMapCols(NULL),
      m_spinMaxUseCount(NULL),
            m_spinResetCount(NULL),
      m_spinTimeLimit(NULL){
    InitUI();
    LoadConfig(initialConfig);
    setWindowTitle("自定义模式设置");
    setModal(true);
}

void CustomModeSettingsDialog::InitUI(){
    m_spinIconTypeCount = new QSpinBox(this);
    m_spinIconTypeCount->setRange(1, GameControl::ICON_COUNT);

    m_spinMapRows = new QSpinBox(this);
    m_spinMapRows->setRange(2, GameControl::MAP_ROWS);

    m_spinMapCols = new QSpinBox(this);
    m_spinMapCols->setRange(2, GameControl::MAP_COLS);

    m_spinMaxUseCount = new QSpinBox(this);
    m_spinMaxUseCount->setRange(1, 99);

    m_spinResetCount = new QSpinBox(this);
    m_spinResetCount->setRange(1, 99);

    m_spinTimeLimit = new QSpinBox(this);
    m_spinTimeLimit->setRange(10, 3600);
    m_spinTimeLimit->setSuffix(" 秒");

    QFormLayout* formLayout = new QFormLayout;
    formLayout->addRow("消除元素种类数量：", m_spinIconTypeCount);
    formLayout->addRow("地图行数：", m_spinMapRows);
    formLayout->addRow("地图列数：", m_spinMapCols);
    formLayout->addRow("提示最大次数：", m_spinMaxUseCount);
    formLayout->addRow("重排最大次数：", m_spinResetCount);
    formLayout->addRow("限时：", m_spinTimeLimit);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->button(QDialogButtonBox::Ok)->setText("开始游戏");
    buttonBox->button(QDialogButtonBox::Cancel)->setText("取消");
    connect(buttonBox, &QDialogButtonBox::accepted, this, &CustomModeSettingsDialog::OnAcceptClicked);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);
}

void CustomModeSettingsDialog::LoadConfig(const GameModeConfig& initialConfig){
    m_spinIconTypeCount->setValue(initialConfig.iconTypeCount);
    m_spinMapRows->setValue(initialConfig.mapRows);
    m_spinMapCols->setValue(initialConfig.mapCols);
    m_spinMaxUseCount->setValue(initialConfig.maxTipCount);
    m_spinResetCount->setValue(initialConfig.maxResetCount);
    m_spinTimeLimit->setValue(initialConfig.timeLimitSeconds);
}

void CustomModeSettingsDialog::OnAcceptClicked(){
    const int rows = m_spinMapRows->value();
    const int cols = m_spinMapCols->value();
    const int total = rows * cols;
    const int maxTotal = GameControl::MAP_ROWS * GameControl::MAP_COLS;

    if((total % 2) != 0){
        QMessageBox::warning(this, "地图大小无效", "地图格子总数必须为偶数，请调整行列。");
        return;
    }

    if(total > maxTotal){
        QMessageBox::warning(this,
                             "地图大小无效",
                             QString("地图总格子数不能超过 %1x%2（%3）。\n请减小行数或列数。")
                                 .arg(GameControl::MAP_COLS)
                                 .arg(GameControl::MAP_ROWS)
                                 .arg(maxTotal));
        return;
    }

    accept();
}

GameModeConfig CustomModeSettingsDialog::config() const{
    return GameModeConfig(
        m_spinIconTypeCount->value(),
        m_spinMapRows->value(),
        m_spinMapCols->value(),
        m_spinMaxUseCount->value(),
        m_spinResetCount->value(),
        m_spinTimeLimit->value()
    );
}
