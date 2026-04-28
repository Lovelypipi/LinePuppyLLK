#ifndef CUSTOMMODESETTINGSDIALOG_H
#define CUSTOMMODESETTINGSDIALOG_H

#include "stdafx.h"
#include "gamecommon.h"
#include <QSpinBox>

class CustomModeSettingsDialog : public QDialog{
    Q_OBJECT
public:
    explicit CustomModeSettingsDialog(const GameModeConfig& initialConfig, QWidget* parent = NULL);

    GameModeConfig config() const;

private:
    void InitUI();
    void LoadConfig(const GameModeConfig& initialConfig);
    void OnAcceptClicked();

    QSpinBox* m_spinIconTypeCount;
    QSpinBox* m_spinMapRows;
    QSpinBox* m_spinMapCols;
    QSpinBox* m_spinMaxUseCount;
    QSpinBox* m_spinResetCount;
    QSpinBox* m_spinTimeLimit;
};

#endif