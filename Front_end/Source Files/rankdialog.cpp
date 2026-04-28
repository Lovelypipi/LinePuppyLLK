#include "rankdialog.h"

#include "levelselectdialog.h"
#include <QtWidgets/QAbstractItemView>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QVBoxLayout>

RankDialog::RankDialog(UserProgressService& progressService, QWidget* parent)
    : QDialog(parent),
      m_progressService(progressService),
      m_basicTable(NULL),
      m_basicErrorLabel(NULL),
      m_levelSelector(NULL),
      m_levelTable(NULL),
      m_levelErrorLabel(NULL){
    InitUI();

    FillRankTable(m_basicTable, "basic", 0, m_basicErrorLabel);
    FillRankTable(m_levelTable,
                  "level",
                  m_levelSelector->currentData().toInt(),
                  m_levelErrorLabel);

    connect(m_levelSelector, &QComboBox::currentIndexChanged, this, [this](int){
        FillRankTable(m_levelTable,
                      "level",
                      m_levelSelector->currentData().toInt(),
                      m_levelErrorLabel);
    });
}

void RankDialog::InitUI(){
    setWindowTitle("排行榜");
    resize(620, 460);

    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    QTabWidget* tabs = new QTabWidget(this);
    rootLayout->addWidget(tabs);

    QWidget* basicTab = new QWidget(tabs);
    QVBoxLayout* basicLayout = new QVBoxLayout(basicTab);
    QLabel* basicTitle = new QLabel("基本模式最短用时排行榜（Top 20）", basicTab);
    m_basicTable = new QTableWidget(basicTab);
    m_basicErrorLabel = new QLabel(basicTab);
    m_basicErrorLabel->setStyleSheet("color:#C62828;");
    m_basicErrorLabel->setVisible(false);
    SetupRankTable(m_basicTable);
    basicLayout->addWidget(basicTitle);
    basicLayout->addWidget(m_basicTable);
    basicLayout->addWidget(m_basicErrorLabel);
    tabs->addTab(basicTab, "基本模式");

    QWidget* levelTab = new QWidget(tabs);
    QVBoxLayout* levelLayout = new QVBoxLayout(levelTab);
    QLabel* levelTitle = new QLabel("关卡模式最短用时（按关卡查看，Top 20）", levelTab);
    m_levelSelector = new QComboBox(levelTab);
    for(int i = 1; i <= LevelSelectDialog::LevelCount(); ++i){
        if(i == 11){
            m_levelSelector->addItem("终极挑战", i);
        }else{
            m_levelSelector->addItem(QString("第%1关").arg(i), i);
        }
    }
    m_levelTable = new QTableWidget(levelTab);
    m_levelErrorLabel = new QLabel(levelTab);
    m_levelErrorLabel->setStyleSheet("color:#C62828;");
    m_levelErrorLabel->setVisible(false);
    SetupRankTable(m_levelTable);
    levelLayout->addWidget(levelTitle);
    levelLayout->addWidget(m_levelSelector);
    levelLayout->addWidget(m_levelTable);
    levelLayout->addWidget(m_levelErrorLabel);
    tabs->addTab(levelTab, "关卡模式");
}

void RankDialog::SetupRankTable(QTableWidget* table){
    table->setColumnCount(3);
    table->setHorizontalHeaderLabels(QStringList() << "排名" << "玩家" << "最短用时");
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionMode(QAbstractItemView::NoSelection);
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setStretchLastSection(true);
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
}

void RankDialog::FillRankTable(QTableWidget* table,
                               const QString& modeType,
                               int levelIndex,
                               QLabel* errorLabel){
    QString err;
    const QVector<UserProgressService::RankRecord> rows =
        m_progressService.LoadModeLeaderboard(modeType, levelIndex, 20, &err);
    table->setRowCount(rows.size());

    for(int i = 0; i < rows.size(); ++i){
        const UserProgressService::RankRecord& row = rows[i];
        table->setItem(i, 0, new QTableWidgetItem(QString::number(i + 1)));
        table->setItem(i, 1, new QTableWidgetItem(row.nickname));
        table->setItem(i, 2, new QTableWidgetItem(FormatElapsedMs(row.bestTimeMs)));
    }

    if(errorLabel){
        errorLabel->setText(err.isEmpty() ? QString() : QString("数据加载失败：%1").arg(err));
        errorLabel->setVisible(!err.isEmpty());
    }
}

QString RankDialog::FormatElapsedMs(qint64 elapsedMs){
    if(elapsedMs <= 0){
        return "--";
    }

    const qint64 totalSeconds = elapsedMs / 1000;
    const qint64 minutes = totalSeconds / 60;
    const qint64 seconds = totalSeconds % 60;
    const qint64 milli = elapsedMs % 1000;
    return QString("%1:%2.%3")
        .arg(minutes, 2, 10, QLatin1Char('0'))
        .arg(seconds, 2, 10, QLatin1Char('0'))
        .arg(milli, 3, 10, QLatin1Char('0'));
}
