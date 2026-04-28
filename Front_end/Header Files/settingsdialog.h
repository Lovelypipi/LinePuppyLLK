#ifndef LLKSETTINGSDIALOG_H
#define LLKSETTINGSDIALOG_H

#include "stdafx.h"
#include "llkappsettings.h"
#include <QColor>
#include <QtWidgets/QComboBox>

class QLabel;
class QPushButton;
class QSlider;
class QSpinBox;
class QMediaPlayer;
class QAudioOutput;

class SettingsDialog : public QDialog{
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget* parent = NULL);

signals:
    void SigPreviewAudioChanged();

private slots:
    void OnPickSelectionColor();
    void OnPickLinkColor();
    void OnResetDefaults();
    void OnAccepted();
    void OnRejected();

private:
    void InitUI();
    void LoadSettings();
    void ApplyColorPreview(QPushButton* button, const QColor& color, const QString& text);
    void SetControlsToDefaults();
    void ApplyAudioPreview();
    void PlayRemoveSoundPreviewOnce();
    void StopRemoveSoundPreview();

    QSlider* m_sliderBgMusic;
    QSlider* m_sliderRemoveSound;
    QLabel* m_labelBgMusicValue;
    QLabel* m_labelRemoveSoundValue;
    QPushButton* m_btnSelectionColor;
    QPushButton* m_btnLinkColor;
    QSpinBox* m_spinLinkWidth;
    QColor m_selectionColor;
    QColor m_linkColor;
    int m_initialBgMusicVolume;
    int m_initialRemoveSoundVolume;
    bool m_bPreviewReady;
    QMediaPlayer* m_removeSoundPreviewPlayer;
    QAudioOutput* m_removeSoundPreviewOutput;
    QComboBox* m_comboEngine;
    LLKAppSettings::PathEngine m_initialEngine;
};

#endif