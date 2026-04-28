#ifndef LLKDIALOG_H
#define LLKDIALOG_H

#include "stdafx.h"
#include "gamedialog.h"
#include "userprogressservice.h"

class QMediaPlayer;
class QAudioOutput;

class LLKDialog : public QDialog{
    Q_OBJECT
public:
    LLKDialog(QWidget* parent = NULL);
    ~LLKDialog();
    void ApplyAppSettings();
    void SetCurrentUser(qint64 userId, const QString& nickname);
    qint64 currentUserId() const;
    QString currentNickname() const;
protected:
    void paintEvent(QPaintEvent* event) override;
private slots:
    //主界面按钮
    void OnBasicMode();
    void OnRestMode();
    void OnLevelMode();
    void OnRank();
    void OnSetting();
    void OnHelp();
private:
    void InitUI();
    void LoadBackground();
    void InitBackgroundMusic();
    bool InitProgressService(QString* errorMessage = NULL);
    void DestroyModeDialog(GameDialog*& modeDialog);
    void DestroyAllModeDialogs();
    void PersistModeProgress(const QString& modeKey, GameDialog* modeDialog, bool keepProgress);
    void RestoreModeProgress(const QString& modeKey, GameDialog* modeDialog);
    void ShowMainButtons();
    void HideMainButtons();
    void ShowBasicMode();
    void ShowCustomMode();
    void ShowLevelMode();
    bool LoadLevelModeProgressFromDb(bool* hasDbProgress, QString* outSnapshot);
    bool HandleExistingLevelProgress(bool hasDbProgress, const QString& dbSnapshot);
    void BindLevelModeSignals();
    void RecreateLevelModeDialog(const QString& title, const GameModeConfig& config, int levelIndex);
    void ShowModeDialog(GameDialog* modeDialog);
    void HandleModeReturn(GameDialog*& modeDialog, bool keepProgress);
    void BindCommonModeSignals(GameDialog*& modeDialog, bool trackBestTime);
    void HandleGameWon(const QString& modeType, int levelIndex, qint64 elapsedMs);
    void ApplySettingsToAllModes();
    void SaveUnlockedLevel();

    //按钮
    QPushButton* m_btnBasic;
    QPushButton* m_btnRest;
    QPushButton* m_btnLevel;
    QPushButton* m_btnRank;
    QPushButton* m_btnSetting;
    QPushButton* m_btnHelp;

    //主界面背景
    QPixmap m_bgMain;
    GameDialog* m_basicMode;
    GameDialog* m_customMode;
    GameDialog* m_levelMode;
    int m_unlockedLevel;
    qint64 m_currentUserId;
    QString m_currentNickname;
    bool m_bProgressServiceReady;
    UserProgressService m_progressService;
    QMediaPlayer* m_bgMusicPlayer;
    QAudioOutput* m_bgMusicOutput;
};

#endif