#include "gamedialog.h"
#include "custommodesettingsdialog.h"
#include "settingsdialog.h"
#include "llkdialog.h"
#include "helpdialog.h"
#include <QtWidgets/QMessageBox>

void GameDialog::mousePressEvent(QMouseEvent* event){
    if(!m_bGameStart){
        QDialog::mousePressEvent(event);
        return;
    }

    //计算地图偏移与单元格尺寸，和绘制逻辑对齐
    const int cellW = m_icons[0].width();
    const int cellH = m_icons[0].height();
    const int gapX = 5;
    const int gapY = 3;
    const int rows = MapRows();
    const int cols = MapCols();
    const int gridW = cols * cellW + (cols - 1) * gapX;
    const int gridH = rows * cellH + (rows - 1) * gapY;
    int xOffset = (width() - gridW) / 2;
    int yOffset = (height() - gridH) / 2 - 47;

    //将点击坐标转换为地图行列坐标
    int clickX = static_cast<int>(event->position().x()) - xOffset;
    int clickY = static_cast<int>(event->position().y()) - yOffset;
    int col = clickX / (cellW + gapX);
    int row = clickY / (cellH + gapY);

    if(row < 0 || row >= rows || col < 0 || col >= cols){
        QDialog::mousePressEvent(event);
        return;
    }
    int val = m_pGameLogic->GetMapVal(row, col);
    //空白单元格点击无效
    if(val <= 0){ 
        QDialog::mousePressEvent(event);
        return;
    }

    Vertex currentPoint(row, col);
    if(!m_bFirstSelected){
        //第一次点击：记录选中点
        m_firstPoint = currentPoint;
        m_bFirstSelected = true;
        StartSelectBlink();
        m_linkPath.clear();
        StopLinkAnimation();
    }else{
        //第二次点击：判断是否可以消除，当重复点击同一个元素时，不做任何处理
        if(currentPoint.row == m_firstPoint.row && currentPoint.col == m_firstPoint.col){
            QDialog::mousePressEvent(event);
            return;
        }
        int firstVal = m_pGameLogic->GetMapVal(m_firstPoint.row, m_firstPoint.col);
        //图片相同，尝试连接
        if(firstVal == val){
            QVector<Vertex> path;
            if(m_pGameLogic->Link(m_firstPoint, currentPoint, path)){
                //可消，清空两个单元格
                m_pGameLogic->SetMapVal(m_firstPoint.row, m_firstPoint.col, 0);
                m_pGameLogic->SetMapVal(row, col, 0);
                m_linkPath = path;
                StopSelectBlink();
                m_bFirstSelected = false;
                StartLinkAnimation();
                ClearHintState();
                PlayRemoveSoundEffect();

                //检查是否满足游戏胜利条件
                CheckGameWin(); 
                if(m_bGameStart){
                    EnsureSolvableMap();
                }
            }else{
                //无法连接，更新选中点为当前点
                m_firstPoint = currentPoint;
                m_linkPath.clear();
                m_bSelectBlinkVisible = true;
            }
        }else{
            m_firstPoint = currentPoint;
            m_linkPath.clear();
            m_bSelectBlinkVisible = true;
        }
    }
    update();//触发重绘以更新界面显示
    QDialog::mousePressEvent(event);
}

void GameDialog::OnStartGame(){
    if(!m_bHasGameProgress){
        if(m_bCustomMode && m_allowCustomConfigDialog){
            CustomModeSettingsDialog settingsDialog(m_modeConfig, this);
            if(settingsDialog.exec() != QDialog::Accepted){
                return;
            }
            m_modeConfig = settingsDialog.config();
        }
        StartCurrentRound();
        return;
    }
    m_activeTimer.start();
    m_bGameStart = true;
    m_btnStart->setEnabled(false);  //开始游戏后禁用开始按钮
    m_btnReset->setEnabled(!IsResetDisabled());
    m_btnTip->setEnabled(!IsTipDisabled());
    m_btnPause->setEnabled(true);
    update();
}

void GameDialog::OnPauseGame(){
    if(!m_bGameStart){
        return;
    }
    if(m_activeTimer.isValid()){
        m_activeElapsedMs += m_activeTimer.elapsed();
        m_activeTimer.invalidate();
    }
    m_bGameStart = false;
    m_btnStart->setEnabled(true);
    m_btnReset->setEnabled(false);
    m_btnTip->setEnabled(false);
    m_btnPause->setEnabled(false);
    ClearTransientVisualState();
    update();
}

void GameDialog::OnTip(){
    if(!m_bGameStart){
        return;
    }
    if(IsTipDisabled()){
        StartNoticeAnimation("本关禁用提示");
        return;
    }
    if(m_bCustomMode && m_modeConfig.maxTipCount >= 0 && m_tipCount >= m_modeConfig.maxTipCount){
        StartNoticeAnimation("提示次数已达到上限");
        return;
    }
    ++m_tipCount;
    UpdateHintCandidates();
    if(m_hintItems.isEmpty()){
        QMessageBox::information(this, "提示", "当前没有可以消除的元素组了。");
        StopHintAnimation();
        update();
        return;
    }

    m_hintIndex = (m_hintIndex + 1) % m_hintItems.size();
    StartHintAnimation();
    update();
}

void GameDialog::OnReset(){
    if(!m_bGameStart){
        return;
    }
    if(IsResetDisabled()){
        StartNoticeAnimation("本关禁用重排");
        return;
    }
    if(m_bCustomMode && m_modeConfig.maxResetCount >= 0 && m_resetCount >= m_modeConfig.maxResetCount){
        StartNoticeAnimation("重排次数已达到上限");
        return;
    }
    ++m_resetCount;
    m_bFirstSelected = false;
    m_linkPath.clear();
    StopSelectBlink();
    StopLinkAnimation();
    ClearHintState();
    m_pGameLogic->ShuffleRemainingMap();
    EnsureSolvableMap();
    StartNoticeAnimation("重排成功");
    update();
}

void GameDialog::OnSetting(){
    if(m_bGameStart){
        OnPauseGame();
    }

    SettingsDialog dialog(this);
    connect(&dialog, &SettingsDialog::SigPreviewAudioChanged, this, [this](){
        ApplyAppSettings();
        LLKDialog* rootDialog = qobject_cast<LLKDialog*>(parentWidget());
        if(rootDialog){
            rootDialog->ApplyAppSettings();
        }
    });
    if(dialog.exec() != QDialog::Accepted){
        return;
    }

    ApplyAppSettings();
    // 引擎切换后旧提示路径可能来自上一套逻辑，清理避免视觉与当前引擎不一致。
    ClearHintState();
    m_linkPath.clear();
    StopLinkAnimation();
    update();
}

void GameDialog::OnHelp(){
    if(m_bGameStart){
        OnPauseGame();
    }

    HelpDialog helpDialog(this);
    helpDialog.exec();
}

void GameDialog::OnExitToMain(){
    if(m_bGameStart){
        OnPauseGame();
    }

    QMessageBox box(this);
    box.setWindowTitle(QString("退出%1").arg(m_modeName));
    box.setText(QString("请选择退出%1的方式").arg(m_modeName));
    box.setIcon(QMessageBox::Question);

    box.setStandardButtons(QMessageBox::Cancel);
    QAbstractButton* btnContinue = box.button(QMessageBox::Cancel);
    btnContinue->setText("继续游戏");

    QPushButton* btnDiscard = NULL;
    QPushButton* btnKeep = NULL;
    if(m_bHasGameProgress){
        btnDiscard = box.addButton(QString("退出%1并回到主界面（不保留本局进度）").arg(m_modeName), QMessageBox::ActionRole);
        btnKeep = box.addButton(QString("退出%1并回到主界面（保留本局进度）").arg(m_modeName), QMessageBox::ActionRole);
    }else{
        btnDiscard = box.addButton(QString("退出%1并回到主界面").arg(m_modeName), QMessageBox::ActionRole);
    }

    box.exec();

    //根据用户选择执行相应操作
    if(box.clickedButton() == btnContinue){
        return;
    }

    const bool keepProgress = (btnKeep != NULL && box.clickedButton() == btnKeep);
    //使用void来显式地标明btnDiscard未被使用，以避免编译器发出未使用变量的警告
    (void)btnDiscard; 
    emit SigReturnToMain(keepProgress);
}
