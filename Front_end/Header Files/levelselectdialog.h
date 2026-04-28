#ifndef LEVELSELECTDIALOG_H
#define LEVELSELECTDIALOG_H

#include "stdafx.h"
#include "gamecommon.h"
#include <QVector>

class QLabel;
class QPushButton;

class LevelSelectDialog : public QDialog{
    Q_OBJECT
public:
    explicit LevelSelectDialog(int unlockedLevel, QWidget* parent = NULL);

    int selectedLevelIndex() const;

    static int LevelCount();
    static const QVector<GameModeConfig>& LevelConfigs();

private:
    void InitUI();
    QString BuildLevelDescription(int levelIndex, const GameModeConfig& config) const;

    int m_unlockedLevel;
    int m_selectedLevelIndex;
    QLabel* m_titleLabel;
    QLabel* m_descLabel;
    QVector<QPushButton*> m_levelButtons;
};

#endif
