#include "gamedialog.h"
#include "llkappsettings.h"
#include <QtGui/QImage>
#include <QtWidgets/QMessageBox>
#include <QtMultimedia/QMediaPlayer>
#include <QtMultimedia/QAudioOutput>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QUrl>

static QPixmap RemoveBackgroundByColor(const QPixmap& source){
    if(source.isNull()) return source;

    QImage image = source.toImage().convertToFormat(QImage::Format_ARGB32);
    const QColor keyColor = image.pixelColor(0, 0);

    for(int y = 0; y < image.height(); ++y){
        QRgb* row = reinterpret_cast<QRgb*>(image.scanLine(y));
        for(int x = 0; x < image.width(); ++x){
            const QRgb pixel = row[x];
            if(qRed(pixel) == keyColor.red()
                && qGreen(pixel) == keyColor.green()
                && qBlue(pixel) == keyColor.blue()){
                row[x] = qRgba(qRed(pixel), qGreen(pixel), qBlue(pixel), 0);
            }
        }
    }

    return QPixmap::fromImage(image);
}

GameDialog::GameDialog(QWidget* parent,
                                             const QString& modeName,
                                             bool customMode,
                                             const GameModeConfig& modeConfig,
                                             bool allowCustomConfigDialog,
                                             int levelIndex)
    : QDialog(parent),
      m_pGameLogic(new GameLogic),
      m_bGameStart(false),
      m_bHasGameProgress(false),
      m_resetCount(0),
      m_tipCount(0),
      m_activeElapsedMs(0),
      m_bFirstSelected(false),
      m_hintIndex(-1),
      m_selectBlinkTimer(new QTimer(this)),
      m_linkAnimTimer(new QTimer(this)),
      m_linkClearTimer(new QTimer(this)),
      m_statsRefreshTimer(new QTimer(this)),
      m_hintAnimTimer(new QTimer(this)),
      m_noticeTimer(new QTimer(this)),
      m_bSelectBlinkVisible(true),
      m_bHintVisible(true),
      m_linkDrawProgress(0.0),
      m_modeName(modeName),
      m_bCustomMode(customMode),
      m_modeConfig(modeConfig),
            m_allowCustomConfigDialog(allowCustomConfigDialog),
            m_levelIndex(levelIndex),
            m_noticeProgress(0.0),
            m_removeSoundPlayer(NULL),
            m_removeSoundOutput(NULL){
    connect(m_selectBlinkTimer, &QTimer::timeout, this, [this](){
        if(!m_bFirstSelected){
            m_selectBlinkTimer->stop();
            return;
        }
        m_bSelectBlinkVisible = !m_bSelectBlinkVisible;
        update();
    });

    connect(m_linkAnimTimer, &QTimer::timeout, this, [this](){
        if(m_linkPath.size() < 2){
            m_linkAnimTimer->stop();
            return;
        }
        m_linkDrawProgress += 0.08;
        if(m_linkDrawProgress >= 1.0){
            m_linkDrawProgress = 1.0;
            m_linkAnimTimer->stop();
            if(!m_linkClearTimer->isActive()){
                m_linkClearTimer->start(500);
            }
        }
        update();
    });

    connect(m_linkClearTimer, &QTimer::timeout, this, [this](){
        m_linkClearTimer->stop();
        m_linkPath.clear();
        m_linkDrawProgress = 0.0;
        update();
    });

    connect(m_statsRefreshTimer, &QTimer::timeout, this, [this](){
        if(!m_bGameStart){
            return;
        }
        if(m_bCustomMode && CurrentRemainingMs() <= 0){
            HandleTimeUp();
            return;
        }
        update();
    });

    connect(m_hintAnimTimer, &QTimer::timeout, this, [this](){
        if(m_hintItems.isEmpty()){
            m_hintAnimTimer->stop();
            m_bHintVisible = true;
            return;
        }
        m_bHintVisible = !m_bHintVisible;
        update();
    });

    connect(m_noticeTimer, &QTimer::timeout, this, [this](){
        if(m_noticeText.isEmpty()){
            m_noticeTimer->stop();
            return;
        }
        m_noticeProgress += 0.016;
        if(m_noticeProgress >= 0.7){
            m_noticeProgress = 0.7;
            m_noticeTimer->stop();
        }
        update();
    });

    InitUI();
    LoadResource();

    m_removeSoundOutput = new QAudioOutput(this);
    m_removeSoundPlayer = new QMediaPlayer(this);
    m_removeSoundPlayer->setAudioOutput(m_removeSoundOutput);
    m_removeSoundPlayer->setSource(QUrl("qrc:/res/RemoveSoundEffects.mp3"));
    ApplyAppSettings();

    m_statsRefreshTimer->start(200);
    setFixedSize(800, 600);
    setWindowIcon(QIcon(":/res/llk.ico"));
    setWindowTitle(QString("欢乐连连看 - %1").arg(m_modeName));
}

GameDialog::~GameDialog(){
    delete m_pGameLogic;
}

void GameDialog::ApplyAppSettings(){
    const LLKAppSettings& settings = LLKAppSettings::instance();
    if(m_removeSoundOutput){
        m_removeSoundOutput->setVolume(settings.removeSoundVolume() / 100.0f);
    }
    update();
}

bool GameDialog::hasGameProgress() const{
    return m_bHasGameProgress;
}

QString GameDialog::ExportProgressSnapshot() const{
    QJsonObject root;
    root.insert("modeName", m_modeName);
    root.insert("customMode", m_bCustomMode);
    root.insert("levelIndex", m_levelIndex);
    root.insert("hasGameProgress", m_bHasGameProgress);
    root.insert("gameStart", m_bGameStart);
    root.insert("resetCount", m_resetCount);
    root.insert("tipCount", m_tipCount);
    root.insert("activeElapsedMs", static_cast<qint64>(CurrentActiveElapsedMs()));

    QJsonObject modeCfg;
    modeCfg.insert("iconTypeCount", m_modeConfig.iconTypeCount);
    modeCfg.insert("mapRows", m_modeConfig.mapRows);
    modeCfg.insert("mapCols", m_modeConfig.mapCols);
    modeCfg.insert("maxTipCount", m_modeConfig.maxTipCount);
    modeCfg.insert("maxResetCount", m_modeConfig.maxResetCount);
    modeCfg.insert("timeLimitSeconds", m_modeConfig.timeLimitSeconds);
    root.insert("modeConfig", modeCfg);

    QJsonArray iconIndices;
    for(int i = 0; i < m_selectedIconIndices.size(); ++i){
        iconIndices.append(m_selectedIconIndices[i]);
    }
    root.insert("selectedIconIndices", iconIndices);

    QJsonArray mapValues;
    const QVector<int> values = m_pGameLogic ? m_pGameLogic->ExportMapValues() : QVector<int>();
    for(int i = 0; i < values.size(); ++i){
        mapValues.append(values[i]);
    }
    root.insert("mapValues", mapValues);

    return QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Compact));
}

bool GameDialog::ImportProgressSnapshot(const QString& snapshotJson, QString* errorMessage){
    const QJsonDocument doc = QJsonDocument::fromJson(snapshotJson.toUtf8());
    if(!doc.isObject()){
        if(errorMessage){
            *errorMessage = "快照JSON格式非法";
        }
        return false;
    }

    const QJsonObject root = doc.object();
    m_modeName = root.value("modeName").toString(m_modeName);
    m_bCustomMode = root.value("customMode").toBool(m_bCustomMode);
    m_levelIndex = root.value("levelIndex").toInt(m_levelIndex);
    m_bHasGameProgress = root.value("hasGameProgress").toBool(false);
    m_bGameStart = false;
    m_resetCount = root.value("resetCount").toInt(0);
    m_tipCount = root.value("tipCount").toInt(0);
    m_activeElapsedMs = root.value("activeElapsedMs").toInteger(0);
    m_activeTimer.invalidate();

    const QJsonObject modeCfg = root.value("modeConfig").toObject();
    m_modeConfig.iconTypeCount = modeCfg.value("iconTypeCount").toInt(m_modeConfig.iconTypeCount);
    m_modeConfig.mapRows = modeCfg.value("mapRows").toInt(m_modeConfig.mapRows);
    m_modeConfig.mapCols = modeCfg.value("mapCols").toInt(m_modeConfig.mapCols);
    m_modeConfig.maxTipCount = modeCfg.value("maxTipCount").toInt(m_modeConfig.maxTipCount);
    m_modeConfig.maxResetCount = modeCfg.value("maxResetCount").toInt(m_modeConfig.maxResetCount);
    m_modeConfig.timeLimitSeconds = modeCfg.value("timeLimitSeconds").toInt(m_modeConfig.timeLimitSeconds);

    m_selectedIconIndices.clear();
    const QJsonArray iconIndices = root.value("selectedIconIndices").toArray();
    for(int i = 0; i < iconIndices.size(); ++i){
        m_selectedIconIndices.push_back(iconIndices[i].toInt());
    }
    RebuildIconsFromIndices();

    const QJsonArray mapValues = root.value("mapValues").toArray();
    QVector<int> values;
    values.reserve(mapValues.size());
    for(int i = 0; i < mapValues.size(); ++i){
        values.push_back(mapValues[i].toInt());
    }

    if(m_bHasGameProgress){
        if(!m_pGameLogic->ImportMapValues(m_modeConfig.mapRows, m_modeConfig.mapCols, values)){
            if(errorMessage){
                *errorMessage = "地图快照恢复失败";
            }
            return false;
        }
    }

    m_btnStart->setEnabled(true);
    m_btnPause->setEnabled(false);
    m_btnReset->setEnabled(false);
    m_btnTip->setEnabled(false);
    setWindowTitle(QString("欢乐连连看 - %1").arg(m_modeName));
    update();
    return true;
}

void GameDialog::ResetSessionStats(){
    m_resetCount = 0;
    m_tipCount = 0;
    m_activeElapsedMs = 0;
    m_activeTimer.invalidate();
}

qint64 GameDialog::CurrentActiveElapsedMs() const{
    qint64 elapsedMs = m_activeElapsedMs;
    if(m_bGameStart && m_activeTimer.isValid()){
        elapsedMs += m_activeTimer.elapsed();
    }
    return elapsedMs;
}

qint64 GameDialog::CurrentRemainingMs() const{
    if(!m_bCustomMode){
        return CurrentActiveElapsedMs();
    }

    const qint64 limitMs = static_cast<qint64>(m_modeConfig.timeLimitSeconds) * 1000;
    const qint64 remainingMs = limitMs - CurrentActiveElapsedMs();
    return remainingMs > 0 ? remainingMs : 0;
}

QString GameDialog::FormatElapsedText(qint64 elapsedMs) const{
    const qint64 totalSeconds = elapsedMs / 1000;
    const qint64 minutes = totalSeconds / 60;
    const qint64 seconds = totalSeconds % 60;
    if(totalSeconds > 60 * 60){
        const qint64 hours = totalSeconds / 3600;
        const qint64 remainingMinutes = (totalSeconds % 3600) / 60;
        return QString("%1小时%2分%3秒")
            .arg(hours)
            .arg(remainingMinutes, 2, 10, QLatin1Char('0'))
            .arg(seconds, 2, 10, QLatin1Char('0'));
    }
    return QString("%1分%2秒").arg(minutes).arg(seconds, 2, 10, QLatin1Char('0'));
}

int GameDialog::MapRows() const{
    return m_modeConfig.mapRows;
}

int GameDialog::MapCols() const{
    return m_modeConfig.mapCols;
}

bool GameDialog::IsTipDisabled() const{
    return m_bCustomMode && m_modeConfig.maxTipCount == 0;
}

bool GameDialog::IsResetDisabled() const{
    return m_bCustomMode && m_modeConfig.maxResetCount == 0;
}

void GameDialog::InitUI(){
    m_btnStart = new QPushButton("开始游戏", this);
    m_btnPause = new QPushButton("暂停游戏", this);
    m_btnTip = new QPushButton("提示", this);
    m_btnReset = new QPushButton("重排", this);
    m_btnSetting = new QPushButton("设置", this);
    m_btnHelp = new QPushButton("帮助", this);
    m_btnExit = new QPushButton("退出", this);

    QFont btnFont_one("KaiTi", 16, QFont::Bold);
    m_btnStart->setFont(btnFont_one);
    m_btnPause->setFont(btnFont_one);
    m_btnTip->setFont(btnFont_one);
    m_btnReset->setFont(btnFont_one);
    m_btnSetting->setFont(btnFont_one);
    m_btnHelp->setFont(btnFont_one);

    QFont btnFont_two("KaiTi", 12, QFont::Bold);
    m_btnExit->setFont(btnFont_two);

    QString btnStyle = R"(
        QPushButton {
            border: 2px solid #8D7A60;
            border-radius: 12px;
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #FFF6E8, stop:1 #F2D9B4);
            color: #5B4022;
            padding: 4px 0;
        }
        QPushButton:hover {
            border: 2px solid #B27A3C;
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #FFE9C7, stop:1 #F8C983);
            color: #3F260F;
        }
        QPushButton:pressed {
            background: #E7B86F;
        }
    )";
    m_btnStart->setStyleSheet(btnStyle);
    m_btnPause->setStyleSheet(btnStyle);
    m_btnTip->setStyleSheet(btnStyle);
    m_btnReset->setStyleSheet(btnStyle);
    m_btnSetting->setStyleSheet(btnStyle);
    m_btnHelp->setStyleSheet(btnStyle);
    m_btnExit->setStyleSheet(
        "QPushButton {"
        "  border: 2px solid #B66B5F;"
        "  border-radius: 12px;"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #FFF0EE, stop:1 #F8C7BF);"
        "  color: #6D2C21;"
        "  padding: 4px 8px;"
        "}"
        "QPushButton:hover {"
        "  border: 2px solid #D94F3D;"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #FFD9D4, stop:1 #FFB4A7);"
        "  color: #4B140D;"
        "}"
        "QPushButton:pressed {"
        "  background: #F09C8E;"
        "}"
    );

    m_btnReset->setEnabled(false);
    m_btnTip->setEnabled(false);
    m_btnPause->setEnabled(false);

    m_btnExit->setGeometry(5, 5, 60, 30);

    const int btnW = 135;
    const int btnH = 40;
    const int btnGapX = 40;
    const int btnGapY = 16;
    const int leftMargin = 47;
    const int bottomMargin = 16;
    const int baseX = leftMargin;
    const int baseY = 600 - 2 * btnH - btnGapY - bottomMargin;

    m_btnStart->setGeometry(baseX, baseY, btnW, btnH);
    m_btnPause->setGeometry(baseX + (btnW + btnGapX) * 1, baseY, btnW, btnH);
    m_btnTip->setGeometry(baseX + (btnW + btnGapX) * 2, baseY, btnW, btnH);
    m_btnReset->setGeometry(baseX, baseY + btnH + btnGapY, btnW, btnH);
    m_btnSetting->setGeometry(baseX + (btnW + btnGapX) * 1, baseY + btnH + btnGapY, btnW, btnH);
    m_btnHelp->setGeometry(baseX + (btnW + btnGapX) * 2, baseY + btnH + btnGapY, btnW, btnH);

    connect(m_btnStart, &QPushButton::clicked, this, &GameDialog::OnStartGame);
    connect(m_btnPause, &QPushButton::clicked, this, &GameDialog::OnPauseGame);
    connect(m_btnTip, &QPushButton::clicked, this, &GameDialog::OnTip);
    connect(m_btnReset, &QPushButton::clicked, this, &GameDialog::OnReset);
    connect(m_btnSetting, &QPushButton::clicked, this, &GameDialog::OnSetting);
    connect(m_btnHelp, &QPushButton::clicked, this, &GameDialog::OnHelp);
    connect(m_btnExit, &QPushButton::clicked, this, &GameDialog::OnExitToMain);
}

void GameDialog::LoadResource(){
    m_bgGame.load(":/res/llk_newbackground.bmp");
    m_limitTimeIcon.load(":/res/LimitTime_Icon.png");
    m_allIcons.clear();
    m_allIcons.push_back(RemoveBackgroundByColor(QPixmap(":/res/pic1.bmp")));
    m_allIcons.push_back(RemoveBackgroundByColor(QPixmap(":/res/pic2.bmp")));
    m_allIcons.push_back(RemoveBackgroundByColor(QPixmap(":/res/pic3.bmp")));
    m_allIcons.push_back(RemoveBackgroundByColor(QPixmap(":/res/pic4.bmp")));
    m_allIcons.push_back(RemoveBackgroundByColor(QPixmap(":/res/pic5.bmp")));
    m_allIcons.push_back(RemoveBackgroundByColor(QPixmap(":/res/pic6.bmp")));
    m_allIcons.push_back(RemoveBackgroundByColor(QPixmap(":/res/pic7.bmp")));
    m_allIcons.push_back(RemoveBackgroundByColor(QPixmap(":/res/pic8.bmp")));
    m_allIcons.push_back(RemoveBackgroundByColor(QPixmap(":/res/pic9.bmp")));
    m_allIcons.push_back(RemoveBackgroundByColor(QPixmap(":/res/pic10.bmp")));
    m_allIcons.push_back(RemoveBackgroundByColor(QPixmap(":/res/pic11.bmp")));
    m_allIcons.push_back(RemoveBackgroundByColor(QPixmap(":/res/pic12.bmp")));
    m_allIcons.push_back(RemoveBackgroundByColor(QPixmap(":/res/pic13.bmp")));
    m_allIcons.push_back(RemoveBackgroundByColor(QPixmap(":/res/pic14.bmp")));
    m_allIcons.push_back(RemoveBackgroundByColor(QPixmap(":/res/pic15.bmp")));
    m_allIcons.push_back(RemoveBackgroundByColor(QPixmap(":/res/pic16.bmp")));
    m_allIcons.push_back(RemoveBackgroundByColor(QPixmap(":/res/pic17.bmp")));
    m_allIcons.push_back(RemoveBackgroundByColor(QPixmap(":/res/pic18.bmp")));
    m_allIcons.push_back(RemoveBackgroundByColor(QPixmap(":/res/pic19.bmp")));
    m_allIcons.push_back(RemoveBackgroundByColor(QPixmap(":/res/pic20.bmp")));
    SelectBasicModeIcons();
}

void GameDialog::UpdateHintCandidates(){
    m_hintItems.clear();
    if(!m_pGameLogic || m_icons.isEmpty()){
        m_hintIndex = -1;
        return;
    }

    const int rows = MapRows();
    const int cols = MapCols();

    for(int row1 = 0; row1 < rows; ++row1){
        for(int col1 = 0; col1 < cols; ++col1){
            const int value1 = m_pGameLogic->GetMapVal(row1, col1);
            if(value1 <= 0){
                continue;
            }

            const int startIndex = row1 * cols + col1 + 1;
            for(int index = startIndex; index < rows * cols; ++index){
                const int row2 = index / cols;
                const int col2 = index % cols;
                const int value2 = m_pGameLogic->GetMapVal(row2, col2);
                if(value2 <= 0 || value1 != value2){
                    continue;
                }

                QVector<Vertex> path = m_pGameLogic->GetPath(Vertex(row1, col1), Vertex(row2, col2));
                if(path.size() >= 2){
                    m_hintItems.push_back(HintItem{Vertex(row1, col1), Vertex(row2, col2), path});
                }
            }
        }
    }

    if(m_hintItems.isEmpty()){
        m_hintIndex = -1;
    }else if(m_hintIndex >= m_hintItems.size()){
        m_hintIndex = -1;
    }
}

void GameDialog::EnsureSolvableMap(){
    if(!m_pGameLogic || m_icons.isEmpty()){
        m_hintIndex = -1;
        return;
    }

    if(m_pGameLogic->IsBlank()){
        m_hintIndex = -1;
        return;
    }

    UpdateHintCandidates();
    while(m_hintItems.isEmpty()){
        m_pGameLogic->ShuffleRemainingMap();
        m_hintIndex = -1;
        UpdateHintCandidates();
    }
}

void GameDialog::StartHintAnimation(){
    m_bHintVisible = true;
    if(!m_hintAnimTimer->isActive()){
        m_hintAnimTimer->start(250);
    }
}

void GameDialog::StopHintAnimation(){
    if(m_hintAnimTimer->isActive()){
        m_hintAnimTimer->stop();
    }
    m_bHintVisible = true;
}

void GameDialog::ClearHintState(){
    StopHintAnimation();
    m_hintItems.clear();
    m_hintIndex = -1;
}

void GameDialog::ClearTransientVisualState(){
    m_bFirstSelected = false;
    if(m_linkClearTimer->isActive()){
        m_linkClearTimer->stop();
    }
    m_linkPath.clear();
    StopSelectBlink();
    StopLinkAnimation();
    ClearHintState();
    StopNoticeAnimation();
}

void GameDialog::SelectBasicModeIcons(){
    m_icons.clear();
    m_selectedIconIndices.clear();
    QVector<int> remainingIndices;
    remainingIndices.reserve(m_allIcons.size());
    for(int i = 0; i < m_allIcons.size(); ++i){
        remainingIndices.push_back(i);
    }

    const int iconTypeCount = m_bCustomMode ? m_modeConfig.iconTypeCount : GameControl::BASIC_ICON_COUNT;
    const int selectionCount = std::min(iconTypeCount, static_cast<int>(m_allIcons.size()));
    for(int i = 0; i < selectionCount; ++i){
        const int pickIndex = QRandomGenerator::global()->bounded(remainingIndices.size());
        const int sourceIndex = remainingIndices.takeAt(pickIndex);
        m_selectedIconIndices.push_back(sourceIndex);
        m_icons.push_back(m_allIcons[sourceIndex]);
    }
}

void GameDialog::RebuildIconsFromIndices(){
    m_icons.clear();
    for(int i = 0; i < m_selectedIconIndices.size(); ++i){
        const int sourceIndex = m_selectedIconIndices[i];
        if(sourceIndex >= 0 && sourceIndex < m_allIcons.size()){
            m_icons.push_back(m_allIcons[sourceIndex]);
        }
    }
    if(m_icons.isEmpty()){
        SelectBasicModeIcons();
    }
}

void GameDialog::StartNoticeAnimation(const QString& message){
    m_noticeText = message;
    m_noticeProgress = 0.0;
    if(!m_noticeTimer->isActive()){
        m_noticeTimer->start(16);
    }
    update();
}

void GameDialog::StopNoticeAnimation(){
    if(m_noticeTimer->isActive()){
        m_noticeTimer->stop();
    }
    m_noticeText.clear();
    m_noticeProgress = 0.0;
}

void GameDialog::StartSelectBlink(){
    m_bSelectBlinkVisible = true;
    if(!m_selectBlinkTimer->isActive()){
        m_selectBlinkTimer->start(400);
    }
}

void GameDialog::StopSelectBlink(){
    if(m_selectBlinkTimer->isActive()){
        m_selectBlinkTimer->stop();
    }
    m_bSelectBlinkVisible = true;
}

void GameDialog::StartLinkAnimation(){
    m_linkDrawProgress = 0.0;
    if(m_linkClearTimer->isActive()){
        m_linkClearTimer->stop();
    }
    if(!m_linkAnimTimer->isActive()){
        m_linkAnimTimer->start(16);
    }
}

void GameDialog::StopLinkAnimation(){
    if(m_linkAnimTimer->isActive()){
        m_linkAnimTimer->stop();
    }
    m_linkDrawProgress = 0.0;
}

void GameDialog::ResetCurrentRound(){
    if(m_activeTimer.isValid()){
        m_activeElapsedMs += m_activeTimer.elapsed();
        m_activeTimer.invalidate();
    }
    m_pGameLogic->ReleaseMap();
    ClearTransientVisualState();
    ResetSessionStats();
    m_bGameStart = false;
    m_bHasGameProgress = false;
    m_btnStart->setEnabled(true);
    m_btnReset->setEnabled(false);
    m_btnTip->setEnabled(false);
    m_btnPause->setEnabled(false);
    update();
}

void GameDialog::StartCurrentRound(){
    ClearTransientVisualState();
    ResetSessionStats();
    SelectBasicModeIcons();
    m_pGameLogic->ReleaseMap();
    m_pGameLogic->InitMap(m_bCustomMode ? m_modeConfig.iconTypeCount : GameControl::BASIC_ICON_COUNT,
                          MapRows(),
                          MapCols());
    EnsureSolvableMap();
    m_bHasGameProgress = true;
    m_activeTimer.start();
    m_bGameStart = true;
    m_btnStart->setEnabled(false);
    m_btnReset->setEnabled(!IsResetDisabled());
    m_btnTip->setEnabled(!IsTipDisabled());
    m_btnPause->setEnabled(true);
    update();
}

void GameDialog::PlayRemoveSoundEffect(){
    if(!m_removeSoundPlayer){
        return;
    }
    m_removeSoundPlayer->setPosition(0);
    m_removeSoundPlayer->play();
}

void GameDialog::HandleTimeUp(){
    if(!m_bGameStart){
        return;
    }

    if(m_activeTimer.isValid()){
        m_activeElapsedMs += m_activeTimer.elapsed();
        m_activeTimer.invalidate();
    }

    m_bGameStart = false;
    m_bHasGameProgress = false;
    ClearTransientVisualState();
    m_btnStart->setEnabled(true);
    m_btnReset->setEnabled(false);
    m_btnTip->setEnabled(false);
    m_btnPause->setEnabled(false);

    QMessageBox box(this);
    box.setWindowTitle("游戏失败");
    box.setText("本局限时已到，挑战失败。");
    box.setIcon(QMessageBox::Warning);
    box.addButton(QString("退出%1").arg(m_modeName), QMessageBox::RejectRole);
    QPushButton* btnRetry = box.addButton("不服再战", QMessageBox::AcceptRole);
    box.exec();

    if(box.clickedButton() == btnRetry){
        m_pGameLogic->ReleaseMap();
        ClearTransientVisualState();
        ResetSessionStats();
        update();
        return;
    }

    emit SigReturnToMain(false);
}

void GameDialog::CheckGameWin(){
    if(m_pGameLogic->IsBlank()){
        const bool shouldReturnToLevelSelect = (m_levelIndex > 0);
        const bool shouldReturnToMain = !shouldReturnToLevelSelect;
        OnPauseGame();
        QString modeType;
        if(m_levelIndex > 0){
            modeType = "level";
        }else if(!m_bCustomMode){
            modeType = "basic";
        }
        if(!modeType.isEmpty()){
            emit SigGameWon(modeType, m_levelIndex > 0 ? m_levelIndex : 0, m_activeElapsedMs);
        }
        if(m_levelIndex > 0){
            emit SigLevelPassed(m_levelIndex);
        }
        QMessageBox::information(this, "游戏胜利",
                         QString("恭喜你，所有元素都已消除，你获胜了！\n\n"
                                 "重排次数：%1\n"
                                 "提示次数：%2\n"
                                 "本局用时：%3")
                                 .arg(m_resetCount)
                                 .arg(m_tipCount)
                                 .arg(FormatElapsedText(m_activeElapsedMs)));
        m_bHasGameProgress = false;
        if(shouldReturnToLevelSelect){
            emit SigReturnToLevelSelect();
        }else if(shouldReturnToMain){
            emit SigReturnToMain(false);
        }
    }
}
