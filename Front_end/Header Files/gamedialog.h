#ifndef GAMEDIALOG_H
#define GAMEDIALOG_H

#include "stdafx.h"
#include "gamecommon.h"
#include "gamelogic.h"
#include <QVector>
#include <QMouseEvent>
#include <QPen>
#include <QTimer>
#include <QColor>
#include <QElapsedTimer>
#include <QtCore/QString>

class QMediaPlayer;
class QAudioOutput;

class GameDialog : public QDialog{
    Q_OBJECT
public:
    GameDialog(QWidget* parent = NULL,
               const QString& modeName = "基本模式",
               bool customMode = false,
               const GameModeConfig& modeConfig = GameModeConfig(),
               bool allowCustomConfigDialog = true,
               int levelIndex = -1);
    ~GameDialog();
    void ApplyAppSettings();
    bool hasGameProgress() const;
    QString ExportProgressSnapshot() const;
    bool ImportProgressSnapshot(const QString& snapshotJson, QString* errorMessage = NULL);
    
protected:
    void paintEvent(QPaintEvent* event) override;              //重写绘制事件（绘制游戏界面）
    virtual void mousePressEvent(QMouseEvent* event) override; //重写鼠标点击事件
    void DrawTipLine(QPainter& painter, int xOffset,int yOffset, 
         int cellW, int cellH, int gapX, int gapY);            //绘制提示连线
    void DrawHintOverlay(QPainter& painter, int xOffset, int yOffset,
         int cellW, int cellH, int gapX, int gapY);            //绘制提示高亮
        void DrawNoticeOverlay(QPainter& painter);             //绘制通知提示

signals:
    void SigReturnToMain(bool keepProgress);
    void SigLevelPassed(int levelIndex);
    void SigReturnToLevelSelect();
    void SigGameWon(const QString& modeType, int levelIndex, qint64 elapsedMs);

private slots:
    //按钮槽函数
    void OnStartGame();
    void OnPauseGame();
    void OnTip();
    void OnReset();
    void OnSetting();
    void OnHelp();
    void OnExitToMain();

private:
    void InitUI();                                      //初始化界面
    void LoadResource();                                //加载资源
    void SelectBasicModeIcons();                        //随机选择基础模式图标
    void RebuildIconsFromIndices();                     //按已选图标索引重建图标映射
    void UpdateHintCandidates();                        //刷新可提示的消除对
    void EnsureSolvableMap();                           //兜底重排直到当前地图可解
    void ClearTransientVisualState();                   //清空临时显示状态
    void ResetSessionStats();                           //重置本局统计
    qint64 CurrentActiveElapsedMs() const;              //获取当前有效用时
    qint64 CurrentRemainingMs() const;                  //获取当前剩余用时
    QString FormatElapsedText(qint64 elapsedMs) const;  //格式化用时文本
    int MapRows() const;                                //当前地图行数
    int MapCols() const;                                //当前地图列数
    bool IsTipDisabled() const;                         //提示是否被禁用
    bool IsResetDisabled() const;                       //重排是否被禁用
    void StartHintAnimation();                          //开始提示动画
    void StopHintAnimation();                           //停止提示动画
    void ClearHintState();                              //清空提示状态
    void StartSelectBlink();                            //开始选中闪烁
    void StopSelectBlink();                             //停止选中闪烁
    void StartLinkAnimation();                          //开始连线动画
    void StopLinkAnimation();                           //停止连线动画
    void CheckGameWin();                                //检查游戏胜利条件
    void StartNoticeAnimation(const QString& message);  //开始中心提示动画
    void StopNoticeAnimation();                         //停止中心提示动画
    void ResetCurrentRound();                           //重置本局到初始状态
    void StartCurrentRound();                           //按当前配置开始本局
    void HandleTimeUp();                                //处理自定义模式超时
    void PlayRemoveSoundEffect();                       //播放消除音效

    struct HintItem{
        Vertex first;
        Vertex second;
        QVector<Vertex> path;
    };

    //按钮
    QPushButton* m_btnStart;
    QPushButton* m_btnPause;
    QPushButton* m_btnTip;
    QPushButton* m_btnReset;
    QPushButton* m_btnSetting;
    QPushButton* m_btnHelp;
    QPushButton* m_btnExit;

    //资源图
    QPixmap m_bgGame;
    QPixmap m_limitTimeIcon;
    QVector<QPixmap> m_allIcons;
    QVector<QPixmap> m_icons;
    QVector<int> m_selectedIconIndices;

    //游戏逻辑
    GameLogic* m_pGameLogic;                          
    bool m_bGameStart;
    bool m_bHasGameProgress;                            //是否已有游戏进度（用于控制继续游戏时是否需要重新开始）
    int m_resetCount;
    int m_tipCount;
    qint64 m_activeElapsedMs;
    QElapsedTimer m_activeTimer;

    Vertex m_firstPoint;                                //第一个选中的点
    bool m_bFirstSelected;                              //是否已经选中第一个点
    QVector<Vertex> m_linkPath;                         //连线的路径点
    QVector<HintItem> m_hintItems;                      //可提示的元素组

    int m_hintIndex;                                    //当前提示的元素组索引，-1表示未提示
    QTimer* m_selectBlinkTimer;                         //选中闪烁计时器
    QTimer* m_linkAnimTimer;                            //连线动画计时器
    QTimer* m_linkClearTimer;                           //连线清除计时器
    QTimer* m_statsRefreshTimer;                        //统计刷新计时器
    QTimer* m_hintAnimTimer;                            //提示动画计时器
    QTimer* m_noticeTimer;                              //通知提示计时器
    bool m_bSelectBlinkVisible;                         //选中闪烁当前是否可见
    bool m_bHintVisible;                                //提示高亮当前是否可见
    qreal m_linkDrawProgress;                           //连线动画当前进度，范围0.0-1.0
    QString m_modeName;                                 //模式名称（用于显示在界面标题和通知中）
    bool m_bCustomMode;                                 //是否为自定义模式
    GameModeConfig m_modeConfig;                        //当前模式配置（仅自定义模式使用）
    bool m_allowCustomConfigDialog;                     //是否允许开局弹出自定义配置窗口
    int m_levelIndex;                                   //关卡序号（非关卡模式为-1）
    QString m_noticeText;                               //当前中心提示文本
    qreal m_noticeProgress;                             //当前中心提示动画进度，范围0.0-1.0
    QMediaPlayer* m_removeSoundPlayer;                  //消除音效播放器
    QAudioOutput* m_removeSoundOutput;                  //消除音效输出
};

#endif