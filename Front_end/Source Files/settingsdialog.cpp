#include "settingsdialog.h"

#include <QColorDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QtMultimedia/QMediaPlayer>
#include <QtMultimedia/QAudioOutput>
#include <QUrl>

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent),
      m_sliderBgMusic(NULL),
      m_sliderRemoveSound(NULL),
      m_labelBgMusicValue(NULL),
      m_labelRemoveSoundValue(NULL),
      m_btnSelectionColor(NULL),
      m_btnLinkColor(NULL),
            m_spinLinkWidth(NULL),
            m_selectionColor(LLKAppSettings::defaultSelectionFrameColor()),
            m_linkColor(LLKAppSettings::defaultLinkLineColor()),
            m_initialBgMusicVolume(LLKAppSettings::defaultBgMusicVolume()),
            m_initialRemoveSoundVolume(LLKAppSettings::defaultRemoveSoundVolume()),
            m_bPreviewReady(false),
            m_removeSoundPreviewPlayer(NULL),
            m_removeSoundPreviewOutput(NULL){
    InitUI();
    LoadSettings();
        m_bPreviewReady = true;
    setWindowTitle("设置");
    setModal(true);
    setMinimumWidth(520);
}

void SettingsDialog::InitUI(){
    m_sliderBgMusic = new QSlider(Qt::Horizontal, this);
    m_sliderBgMusic->setRange(0, 100);
    m_sliderBgMusic->setTickPosition(QSlider::TicksBelow);
    m_sliderBgMusic->setTickInterval(10);

    m_sliderRemoveSound = new QSlider(Qt::Horizontal, this);
    m_sliderRemoveSound->setRange(0, 100);
    m_sliderRemoveSound->setTickPosition(QSlider::TicksBelow);
    m_sliderRemoveSound->setTickInterval(10);

    m_labelBgMusicValue = new QLabel(this);
    m_labelRemoveSoundValue = new QLabel(this);

    connect(m_sliderBgMusic, &QSlider::valueChanged, this, [this](int value){
        m_labelBgMusicValue->setText(QString("%1%").arg(value));
        ApplyAudioPreview();
    });
    connect(m_sliderRemoveSound, &QSlider::valueChanged, this, [this](int value){
        m_labelRemoveSoundValue->setText(QString("%1%").arg(value));
        ApplyAudioPreview();
        if(m_removeSoundPreviewOutput){
            m_removeSoundPreviewOutput->setVolume(value / 100.0f);
        }
    });
    connect(m_sliderRemoveSound, &QSlider::sliderPressed, this, &SettingsDialog::StopRemoveSoundPreview);
    connect(m_sliderRemoveSound, &QSlider::sliderReleased, this, &SettingsDialog::PlayRemoveSoundPreviewOnce);

    m_btnSelectionColor = new QPushButton(this);
    m_btnLinkColor = new QPushButton(this);
    connect(m_btnSelectionColor, &QPushButton::clicked, this, &SettingsDialog::OnPickSelectionColor);
    connect(m_btnLinkColor, &QPushButton::clicked, this, &SettingsDialog::OnPickLinkColor);

    m_spinLinkWidth = new QSpinBox(this);
    m_spinLinkWidth->setRange(1, 8);
    m_spinLinkWidth->setSuffix(" px");

    QGroupBox* audioGroup = new QGroupBox("音频", this);
    QFormLayout* audioForm = new QFormLayout(audioGroup);
    QHBoxLayout* bgMusicLayout = new QHBoxLayout;
    bgMusicLayout->addWidget(m_sliderBgMusic, 1);
    bgMusicLayout->addWidget(m_labelBgMusicValue);
    QHBoxLayout* removeSoundLayout = new QHBoxLayout;
    removeSoundLayout->addWidget(m_sliderRemoveSound, 1);
    removeSoundLayout->addWidget(m_labelRemoveSoundValue);
    audioForm->addRow("背景音乐音量：", bgMusicLayout);
    audioForm->addRow("消除音效音量：", removeSoundLayout);

    QGroupBox* visualGroup = new QGroupBox("画面与逻辑", this);
    QFormLayout* visualForm = new QFormLayout(visualGroup);

    m_comboEngine = new QComboBox(this);
    m_comboEngine->addItem("线性结构 (代数规则枚举)");
    m_comboEngine->addItem("非线性结构 (图深度优先搜索)");

    visualForm->addRow("后端寻路引擎：", m_comboEngine);
    visualForm->addRow("选中方框颜色：", m_btnSelectionColor);
    visualForm->addRow("消除连线颜色：", m_btnLinkColor);
    visualForm->addRow("连线粗细：", m_spinLinkWidth);

    QPushButton* btnResetDefaults = new QPushButton("恢复默认设置", this);
    connect(btnResetDefaults, &QPushButton::clicked, this, &SettingsDialog::OnResetDefaults);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->button(QDialogButtonBox::Ok)->setText("保存");
    buttonBox->button(QDialogButtonBox::Cancel)->setText("取消");
    connect(buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::OnAccepted);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &SettingsDialog::OnRejected);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(audioGroup);
    mainLayout->addWidget(visualGroup);
    mainLayout->addWidget(btnResetDefaults, 0, Qt::AlignLeft);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);
}

void SettingsDialog::LoadSettings(){
    const LLKAppSettings& settings = LLKAppSettings::instance();
    m_initialBgMusicVolume = settings.bgMusicVolume();
    m_initialRemoveSoundVolume = settings.removeSoundVolume();
    m_sliderBgMusic->setValue(settings.bgMusicVolume());
    m_sliderRemoveSound->setValue(settings.removeSoundVolume());
    m_labelBgMusicValue->setText(QString("%1%").arg(settings.bgMusicVolume()));
    m_labelRemoveSoundValue->setText(QString("%1%").arg(settings.removeSoundVolume()));
    m_selectionColor = settings.selectionFrameColor();
    m_linkColor = settings.linkLineColor();
    ApplyColorPreview(m_btnSelectionColor, m_selectionColor, "选择颜色");
    ApplyColorPreview(m_btnLinkColor, m_linkColor, "选择颜色");
    m_spinLinkWidth->setValue(settings.linkLineWidth());
    m_initialEngine = settings.pathEngine();
    m_comboEngine->setCurrentIndex(static_cast<int>(m_initialEngine));
}

void SettingsDialog::ApplyColorPreview(QPushButton* button, const QColor& color, const QString& text){
    if(!button){
        return;
    }

    QColor previewColor = color.isValid() ? color : Qt::white;
    const int brightness = previewColor.red() * 299 + previewColor.green() * 587 + previewColor.blue() * 114;
    const QString textColor = brightness > 128000 ? "#222222" : "#FFFFFF";
    button->setText(QString("%1  %2").arg(text).arg(previewColor.name(QColor::HexRgb)));
    button->setStyleSheet(QString(
        "QPushButton {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: 1px solid #666666;"
        "  border-radius: 8px;"
        "  padding: 6px 12px;"
        "}"
        "QPushButton:hover {"
        "  border: 1px solid #333333;"
        "}")
        .arg(previewColor.name(QColor::HexRgb))
        .arg(textColor));
}

void SettingsDialog::SetControlsToDefaults(){
    m_sliderBgMusic->setValue(LLKAppSettings::defaultBgMusicVolume());
    m_sliderRemoveSound->setValue(LLKAppSettings::defaultRemoveSoundVolume());
    m_selectionColor = LLKAppSettings::defaultSelectionFrameColor();
    m_linkColor = LLKAppSettings::defaultLinkLineColor();
    ApplyColorPreview(m_btnSelectionColor, m_selectionColor, "选择颜色");
    ApplyColorPreview(m_btnLinkColor, m_linkColor, "选择颜色");
    m_spinLinkWidth->setValue(LLKAppSettings::defaultLinkLineWidth());
    m_comboEngine->setCurrentIndex(static_cast<int>(LLKAppSettings::defaultPathEngine()));
}

void SettingsDialog::ApplyAudioPreview(){
    if(!m_bPreviewReady){
        return;
    }

    LLKAppSettings& settings = LLKAppSettings::instance();
    settings.setBgMusicVolume(m_sliderBgMusic->value());
    settings.setRemoveSoundVolume(m_sliderRemoveSound->value());
    emit SigPreviewAudioChanged();
}

void SettingsDialog::PlayRemoveSoundPreviewOnce(){
    if(!m_removeSoundPreviewOutput){
        m_removeSoundPreviewOutput = new QAudioOutput(this);
    }
    if(!m_removeSoundPreviewPlayer){
        m_removeSoundPreviewPlayer = new QMediaPlayer(this);
        m_removeSoundPreviewPlayer->setAudioOutput(m_removeSoundPreviewOutput);
        m_removeSoundPreviewPlayer->setSource(QUrl("qrc:/res/RemoveSoundEffects.mp3"));
    }

    m_removeSoundPreviewOutput->setVolume(m_sliderRemoveSound->value() / 100.0f);
    m_removeSoundPreviewPlayer->setLoops(1);
    m_removeSoundPreviewPlayer->setPosition(0);
    m_removeSoundPreviewPlayer->play();
}

void SettingsDialog::StopRemoveSoundPreview(){
    if(!m_removeSoundPreviewPlayer){
        return;
    }
    m_removeSoundPreviewPlayer->stop();
}

void SettingsDialog::OnPickSelectionColor(){
    const QColor color = QColorDialog::getColor(m_selectionColor, this, "选择选中方框颜色");
    if(color.isValid()){
        m_selectionColor = color;
        ApplyColorPreview(m_btnSelectionColor, color, "选择颜色");
    }
}

void SettingsDialog::OnPickLinkColor(){
    const QColor color = QColorDialog::getColor(m_linkColor, this, "选择消除连线颜色");
    if(color.isValid()){
        m_linkColor = color;
        ApplyColorPreview(m_btnLinkColor, color, "选择颜色");
    }
}

void SettingsDialog::OnResetDefaults(){
    SetControlsToDefaults();
}

void SettingsDialog::OnAccepted(){
    StopRemoveSoundPreview();
    LLKAppSettings& settings = LLKAppSettings::instance();
    settings.setBgMusicVolume(m_sliderBgMusic->value());
    settings.setRemoveSoundVolume(m_sliderRemoveSound->value());
    settings.setSelectionFrameColor(m_selectionColor);
    settings.setLinkLineColor(m_linkColor);
    settings.setLinkLineWidth(m_spinLinkWidth->value());
    settings.setPathEngine(static_cast<LLKAppSettings::PathEngine>(m_comboEngine->currentIndex()));
    settings.save();
    accept();
}

void SettingsDialog::OnRejected(){
    StopRemoveSoundPreview();
    LLKAppSettings& settings = LLKAppSettings::instance();
    settings.setBgMusicVolume(m_initialBgMusicVolume);
    settings.setRemoveSoundVolume(m_initialRemoveSoundVolume);
    settings.setPathEngine(m_initialEngine);
    emit SigPreviewAudioChanged();
    reject();
}
