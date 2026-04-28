#include "llkdialog.h"
#include "settingsdialog.h"
#include "llkappsettings.h"
#include "helpdialog.h"
#include "levelselectdialog.h"
#include "rankdialog.h"
#include "dbconfighelper.h"
#include <QtMultimedia/QMediaPlayer>
#include <QtMultimedia/QAudioOutput>
#include <QtWidgets/QMessageBox>
#include <QUrl>

LLKDialog::LLKDialog(QWidget* parent)
        : QDialog(parent),
            m_basicMode(NULL),
            m_customMode(NULL),
        m_levelMode(NULL),
        m_unlockedLevel(1),
            m_currentUserId(0),
            m_bProgressServiceReady(false),
            m_bgMusicPlayer(NULL),
            m_bgMusicOutput(NULL){
    InitUI();
    LoadBackground();
    InitBackgroundMusic();
    setFixedSize(800, 600);
    setWindowIcon(QIcon(":/res/llk.ico"));
    setWindowTitle("欢乐连连看");
}

bool LLKDialog::InitProgressService(QString* errorMessage){
    DbConnectionConfig config;
    if(!DbConfigHelper::LoadConfig(&config, errorMessage)){
        return false;
    }

    QString err;
    if(!m_progressService.Open(config.databaseFilePath,
                               &err)){
        if(errorMessage){
            *errorMessage = err;
        }
        return false;
    }
    if(!m_progressService.EnsureSchema(&err)){
        if(errorMessage){
            *errorMessage = err;
        }
        return false;
    }

    return true;
}

void LLKDialog::DestroyModeDialog(GameDialog*& modeDialog){
    if(!modeDialog){
        return;
    }
    modeDialog->hide();
    modeDialog->deleteLater();
    modeDialog = NULL;
}

void LLKDialog::DestroyAllModeDialogs(){
    DestroyModeDialog(m_basicMode);
    DestroyModeDialog(m_customMode);
    DestroyModeDialog(m_levelMode);
}

void LLKDialog::PersistModeProgress(const QString& modeKey, GameDialog* modeDialog, bool keepProgress){
    if(!m_bProgressServiceReady || m_currentUserId <= 0 || !modeDialog || modeKey.trimmed().isEmpty()){
        return;
    }

    const bool hasProgress = keepProgress && modeDialog->hasGameProgress();
    const QString snapshot = hasProgress ? modeDialog->ExportProgressSnapshot() : QString();
    QString err;
    if(!m_progressService.SaveModeProgress(m_currentUserId, modeKey, hasProgress, snapshot, &err)){
        QMessageBox::warning(this, "进度保存失败", err);
    }
}

void LLKDialog::RestoreModeProgress(const QString& modeKey, GameDialog* modeDialog){
    if(!m_bProgressServiceReady || m_currentUserId <= 0 || !modeDialog || modeDialog->hasGameProgress() || modeKey.trimmed().isEmpty()){
        return;
    }

    bool hasProgress = false;
    QString snapshot;
    QString err;
    if(!m_progressService.LoadModeProgress(m_currentUserId, modeKey, &hasProgress, &snapshot, &err)){
        QMessageBox::warning(this, "进度加载失败", err);
        return;
    }
    if(!hasProgress){
        return;
    }
    if(snapshot.trimmed().isEmpty()){
        m_progressService.SaveModeProgress(m_currentUserId, modeKey, false, QString(), NULL);
        return;
    }

    if(!modeDialog->ImportProgressSnapshot(snapshot, &err)){
        QMessageBox::warning(this, "进度恢复失败", err);
        m_progressService.SaveModeProgress(m_currentUserId, modeKey, false, QString(), NULL);
    }
}

LLKDialog::~LLKDialog(){
    delete m_basicMode;
    delete m_customMode;
    delete m_levelMode;
}

void LLKDialog::SetCurrentUser(qint64 userId, const QString& nickname){
    if(m_currentUserId > 0 && m_currentUserId != userId){
        DestroyAllModeDialogs();
    }

    m_currentUserId = userId;
    m_currentNickname = nickname.trimmed();

    LLKAppSettings::instance().setActiveUser(m_currentUserId);
    ApplyAppSettings();
    ApplySettingsToAllModes();

    QString err;
    m_bProgressServiceReady = InitProgressService(&err);
    if(!m_bProgressServiceReady){
        QMessageBox::warning(this, "进度服务不可用", err);
        m_unlockedLevel = 1;
        return;
    }

    m_unlockedLevel = qBound(1,
                             m_progressService.LoadUnlockedLevel(m_currentUserId, 1, &err),
                             LevelSelectDialog::LevelCount());
}

qint64 LLKDialog::currentUserId() const{
    return m_currentUserId;
}

QString LLKDialog::currentNickname() const{
    return m_currentNickname;
}

void LLKDialog::SaveUnlockedLevel(){
    if(!m_bProgressServiceReady || m_currentUserId <= 0){
        return;
    }
    QString err;
    m_progressService.SaveUnlockedLevel(m_currentUserId,
                                        qBound(1, m_unlockedLevel, LevelSelectDialog::LevelCount()),
                                        &err);
}

void LLKDialog::HandleGameWon(const QString& modeType, int levelIndex, qint64 elapsedMs){
    if(!m_bProgressServiceReady || m_currentUserId <= 0){
        return;
    }
    QString err;
    if(!m_progressService.SaveBestTime(m_currentUserId, modeType, levelIndex, elapsedMs, &err)){
        QMessageBox::warning(this, "成绩保存失败", err);
    }
}

void LLKDialog::ApplySettingsToAllModes(){
    if(m_basicMode){
        m_basicMode->ApplyAppSettings();
    }
    if(m_customMode){
        m_customMode->ApplyAppSettings();
    }
    if(m_levelMode){
        m_levelMode->ApplyAppSettings();
    }
}

void LLKDialog::BindCommonModeSignals(GameDialog*& modeDialog, bool trackBestTime){
    connect(modeDialog, &GameDialog::SigReturnToMain, this, [this, &modeDialog](bool keepProgress){
        HandleModeReturn(modeDialog, keepProgress);
    });

    if(trackBestTime){
        connect(modeDialog, &GameDialog::SigGameWon, this, [this](const QString& modeType, int levelIndex, qint64 elapsedMs){
            HandleGameWon(modeType, levelIndex, elapsedMs);
        });
    }
}

void LLKDialog::ShowModeDialog(GameDialog* modeDialog){
    if(!modeDialog){
        return;
    }

    HideMainButtons();
    ApplyAppSettings();
    modeDialog->setGeometry(rect());
    modeDialog->ApplyAppSettings();
    modeDialog->show();
    modeDialog->raise();
    modeDialog->activateWindow();
}

void LLKDialog::BindLevelModeSignals(){
    BindCommonModeSignals(m_levelMode, true);

    connect(m_levelMode, &GameDialog::SigReturnToLevelSelect, this, [this](){
        HandleModeReturn(m_levelMode, false);
        ShowLevelMode();
    });

    connect(m_levelMode, &GameDialog::SigLevelPassed, this, [this](int levelIndex){
        if(levelIndex == m_unlockedLevel && m_unlockedLevel < LevelSelectDialog::LevelCount()){
            ++m_unlockedLevel;
            SaveUnlockedLevel();
            QMessageBox::information(this,
                                     "关卡解锁",
                                     QString("恭喜通过第%1关，已解锁第%2关！").arg(levelIndex).arg(m_unlockedLevel));
        }
    });
}

void LLKDialog::RecreateLevelModeDialog(const QString& title,
                                        const GameModeConfig& config,
                                        int levelIndex){
    DestroyModeDialog(m_levelMode);

    m_levelMode = new GameDialog(this, title, true, config, false, levelIndex);
    m_levelMode->setWindowFlags(Qt::Widget);
    m_levelMode->setGeometry(rect());
    BindLevelModeSignals();
}

bool LLKDialog::LoadLevelModeProgressFromDb(bool* hasDbProgress, QString* outSnapshot){
    if(hasDbProgress){
        *hasDbProgress = false;
    }
    if(outSnapshot){
        outSnapshot->clear();
    }

    if(!m_bProgressServiceReady || m_currentUserId <= 0){
        return true;
    }

    bool hasProgress = false;
    QString snapshot;
    QString err;
    if(!m_progressService.LoadModeProgress(m_currentUserId, "level", &hasProgress, &snapshot, &err)){
        QMessageBox::warning(this, "进度加载失败", err);
        return false;
    }

    if(hasProgress && snapshot.trimmed().isEmpty()){
        m_progressService.SaveModeProgress(m_currentUserId, "level", false, QString(), NULL);
        hasProgress = false;
        snapshot.clear();
    }

    if(hasDbProgress){
        *hasDbProgress = hasProgress;
    }
    if(outSnapshot){
        *outSnapshot = snapshot;
    }
    return true;
}

bool LLKDialog::HandleExistingLevelProgress(bool hasDbProgress, const QString& dbSnapshot){
    if(!m_levelMode && !hasDbProgress){
        return false;
    }

    QMessageBox resumeBox(this);
    resumeBox.setWindowTitle("关卡模式");
    resumeBox.setText("检测到你有未完成的关卡进度");
    resumeBox.setInformativeText("请选择继续当前进度，或重新选择关卡。");
    resumeBox.setIcon(QMessageBox::Question);

    QPushButton* btnResume = resumeBox.addButton("继续当前进度", QMessageBox::AcceptRole);
    QPushButton* btnSelectNew = resumeBox.addButton("重新选择关卡", QMessageBox::ActionRole);
    QPushButton* btnCancel = resumeBox.addButton("取消", QMessageBox::RejectRole);
    resumeBox.exec();

    if(resumeBox.clickedButton() == btnCancel){
        return true;
    }

    if(resumeBox.clickedButton() == btnResume){
        if(m_levelMode){
            ShowModeDialog(m_levelMode);
            return true;
        }

        QString err;
        RecreateLevelModeDialog("关卡模式", GameModeConfig(), -1);
        if(!m_levelMode->ImportProgressSnapshot(dbSnapshot, &err)){
            QMessageBox::warning(this, "进度恢复失败", err);
            m_progressService.SaveModeProgress(m_currentUserId, "level", false, QString(), NULL);
            DestroyModeDialog(m_levelMode);
            return false;
        }

        ShowModeDialog(m_levelMode);
        return true;
    }

    if(resumeBox.clickedButton() == btnSelectNew){
        DestroyModeDialog(m_levelMode);
        if(m_bProgressServiceReady && m_currentUserId > 0){
            m_progressService.SaveModeProgress(m_currentUserId, "level", false, QString(), NULL);
        }
    }

    return false;
}

void LLKDialog::HandleModeReturn(GameDialog*& modeDialog, bool keepProgress){
    //modeDialog为当前正在显示的游戏对话框指针
    if(!modeDialog){
        ShowMainButtons();
        update();
        return;
    }
    QString modeKey;
    if(modeDialog == m_basicMode){
        modeKey = "basic";
    }else if(modeDialog == m_customMode){
        modeKey = "custom";
    }else if(modeDialog == m_levelMode){
        modeKey = "level";
    }

    if(keepProgress){
        PersistModeProgress(modeKey, modeDialog, true);
        modeDialog->hide();
        modeDialog->setGeometry(rect());    //将游戏对话框设置为与主窗口相同的几何位置
    }else{
        PersistModeProgress(modeKey, modeDialog, false);
        DestroyModeDialog(modeDialog);
    }

    ShowMainButtons();
    update();
}

void LLKDialog::InitUI(){
    //6个主功能按钮
    m_btnBasic = new QPushButton("基本模式", this);
    m_btnRest = new QPushButton("自定义模式", this);
    m_btnLevel = new QPushButton("关卡模式", this);
    m_btnRank = new QPushButton("排行榜", this);
    m_btnSetting = new QPushButton("设置", this);
    m_btnHelp = new QPushButton("帮助", this);

    //按钮样式
    const QString btnStyle =
        "QPushButton {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #FFF4E3, stop:1 #FFE0B8);"
        "  border: 2px solid #D9A86C;"
        "  border-radius: 10px;"
        "  color: #6D4C2C;"
        "  font-size: 18px;"
        "  font-weight: 600;"
        "  padding: 7px 14px;"
        "}"
        "QPushButton:hover {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #FFECCF, stop:1 #FFD39B);"
        "  border-color: #C27933;"
        "  color: #5A3B1E;"
        "  padding-left: 18px;"
        "}"
        "QPushButton:pressed {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #FFDDB0, stop:1 #F4C185);"
        "  padding-top: 7px;"
        "}";

    m_btnBasic->setStyleSheet(btnStyle);
    m_btnRest->setStyleSheet(btnStyle);
    m_btnLevel->setStyleSheet(btnStyle);
    m_btnRank->setStyleSheet(btnStyle);
    m_btnSetting->setStyleSheet(btnStyle);
    m_btnHelp->setStyleSheet(btnStyle);

    //按钮位置：左侧竖排
    const int x = 30;
    const int y0 = 160;
    const int w = 135;
    const int h = 40;
    const int gap = 25;//按钮间距
    m_btnBasic->setGeometry(x, y0 + (h + gap) * 0, w, h);
    m_btnRest->setGeometry(x, y0 + (h + gap) * 1, w, h);
    m_btnLevel->setGeometry(x, y0 + (h + gap) * 2, w, h);
    m_btnRank->setGeometry(x, y0 + (h + gap) * 3, w, h);
    m_btnSetting->setGeometry(x, y0 + (h + gap) * 4, w, h);
    m_btnHelp->setGeometry(x, y0 + (h + gap) * 5, w, h);

    //绑定事件
    connect(m_btnBasic, &QPushButton::clicked, this, &LLKDialog::OnBasicMode);
    connect(m_btnRest, &QPushButton::clicked, this, &LLKDialog::OnRestMode);
    connect(m_btnLevel, &QPushButton::clicked, this, &LLKDialog::OnLevelMode);
    connect(m_btnRank, &QPushButton::clicked, this, &LLKDialog::OnRank);
    connect(m_btnSetting, &QPushButton::clicked, this, &LLKDialog::OnSetting);
    connect(m_btnHelp, &QPushButton::clicked, this, &LLKDialog::OnHelp);
}

void LLKDialog::LoadBackground(){
    const bool loadedBmp = m_bgMain.load(":/res/llk_main.bmp");
    const bool loadedPng = loadedBmp ? false : m_bgMain.load(":/res/QQ20260329-213711.png");

    (void)loadedBmp;
    (void)loadedPng;
}

void LLKDialog::InitBackgroundMusic(){
    m_bgMusicOutput = new QAudioOutput(this);

    m_bgMusicPlayer = new QMediaPlayer(this);
    m_bgMusicPlayer->setAudioOutput(m_bgMusicOutput);
    m_bgMusicPlayer->setSource(QUrl("qrc:/res/BackgroundMusic.mp3"));
    m_bgMusicPlayer->setLoops(QMediaPlayer::Infinite);
    ApplyAppSettings();
    m_bgMusicPlayer->play();
}

void LLKDialog::ApplyAppSettings(){
    if(m_bgMusicOutput){
        m_bgMusicOutput->setVolume(LLKAppSettings::instance().bgMusicVolume() / 100.0f);
    }
}

void LLKDialog::paintEvent(QPaintEvent*){
    QPainter painter(this);
    if (!m_bgMain.isNull())
        painter.drawPixmap(rect(), m_bgMain);
}

void LLKDialog::HideMainButtons(){
    m_btnBasic->hide();
    m_btnRest->hide();
    m_btnLevel->hide();
    m_btnRank->hide();
    m_btnSetting->hide();
    m_btnHelp->hide();
}

void LLKDialog::ShowMainButtons(){
    m_btnBasic->show();
    m_btnRest->show();
    m_btnLevel->show();
    m_btnRank->show();
    m_btnSetting->show();
    m_btnHelp->show();
}

//主界面按钮实现
void LLKDialog::OnBasicMode() { ShowBasicMode(); }
void LLKDialog::OnRestMode() { ShowCustomMode(); }
void LLKDialog::OnLevelMode() { ShowLevelMode(); }
void LLKDialog::OnRank(){
    if(!m_bProgressServiceReady || m_currentUserId <= 0){
        QMessageBox::information(this, "排行榜", "请先登录并确保数据库服务可用。");
        return;
    }

    RankDialog rankDialog(m_progressService, this);
    rankDialog.exec();
}
void LLKDialog::OnSetting(){
    SettingsDialog dialog(this);
    connect(&dialog, &SettingsDialog::SigPreviewAudioChanged, this, [this](){
        ApplyAppSettings();
        ApplySettingsToAllModes();
    });
    if(dialog.exec() != QDialog::Accepted){
        return;
    }

    ApplyAppSettings();
    ApplySettingsToAllModes();
}
void LLKDialog::OnHelp(){
    HelpDialog helpDialog(this);
    helpDialog.exec();
}

void LLKDialog::ShowBasicMode(){
    if(!m_basicMode){
        m_basicMode = new GameDialog(this, "基本模式", false);
        m_basicMode->setWindowFlags(Qt::Widget);
        m_basicMode->setGeometry(rect());
        BindCommonModeSignals(m_basicMode, true);
    }
    RestoreModeProgress("basic", m_basicMode);
    ShowModeDialog(m_basicMode);
}

void LLKDialog::ShowCustomMode(){
    if(!m_customMode){
        m_customMode = new GameDialog(this, "自定义模式", true);
        m_customMode->setWindowFlags(Qt::Widget);
        m_customMode->setGeometry(rect());
        BindCommonModeSignals(m_customMode, false);
    }
    RestoreModeProgress("custom", m_customMode);
    ShowModeDialog(m_customMode);
}

void LLKDialog::ShowLevelMode(){
    bool hasDbProgress = false;
    QString dbSnapshot;
    if(!LoadLevelModeProgressFromDb(&hasDbProgress, &dbSnapshot)){
        return;
    }

    if(HandleExistingLevelProgress(hasDbProgress, dbSnapshot)){
        return;
    }

    LevelSelectDialog levelSelectDialog(m_unlockedLevel, this);
    if(levelSelectDialog.exec() != QDialog::Accepted){
        return;
    }

    const int selectedLevel = levelSelectDialog.selectedLevelIndex();
    if(selectedLevel < 1 || selectedLevel > LevelSelectDialog::LevelCount()){
        return;
    }

    const GameModeConfig levelConfig = LevelSelectDialog::LevelConfigs().at(selectedLevel - 1);
    const QString levelTitle = (selectedLevel == LevelSelectDialog::LevelCount())
        ? "关卡模式 - 终极挑战"
        : QString("关卡模式 - 第%1关").arg(selectedLevel);
    RecreateLevelModeDialog(levelTitle, levelConfig, selectedLevel);
    ShowModeDialog(m_levelMode);
}
