#ifndef HELPDIALOG_H
#define HELPDIALOG_H

#include <QtWidgets/QDialog>

class QLabel;
class QPushButton;
class QMediaPlayer;
class QAudioOutput;
class QVideoWidget;

class HelpDialog : public QDialog{
public:
    explicit HelpDialog(QWidget* parent = NULL);

private:
    void InitUI();
    void SetupMedia();
    void SelectMethod(int methodIndex);

    QLabel* m_titleLabel;
    QLabel* m_introLabel;
    QLabel* m_methodHintLabel;
    QVideoWidget* m_videoWidget;
    QPushButton* m_btnMethodOne;
    QPushButton* m_btnMethodTwo;
    QPushButton* m_btnMethodThree;

    QMediaPlayer* m_player;
    QAudioOutput* m_audioOutput;
};

#endif