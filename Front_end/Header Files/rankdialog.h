#ifndef RANKDIALOG_H
#define RANKDIALOG_H

#include "stdafx.h"
#include "userprogressservice.h"

class QComboBox;
class QLabel;
class QTableWidget;

class RankDialog : public QDialog{
    Q_OBJECT
public:
    RankDialog(UserProgressService& progressService, QWidget* parent = NULL);

private:
    void InitUI();
    void SetupRankTable(QTableWidget* table);
    void FillRankTable(QTableWidget* table,
                       const QString& modeType,
                       int levelIndex,
                       QLabel* errorLabel);
    static QString FormatElapsedMs(qint64 elapsedMs);

    UserProgressService& m_progressService;
    QTableWidget* m_basicTable;
    QLabel* m_basicErrorLabel;
    QComboBox* m_levelSelector;
    QTableWidget* m_levelTable;
    QLabel* m_levelErrorLabel;
};

#endif
