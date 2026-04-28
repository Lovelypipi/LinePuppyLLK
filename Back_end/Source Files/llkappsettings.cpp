#include "llkappsettings.h"
#include <algorithm>

namespace{
const char* const kGroupName = "Settings";
const char* const kBgMusicVolumeKey = "BgMusicVolume";
const char* const kRemoveSoundVolumeKey = "RemoveSoundVolume";
const char* const kSelectionFrameColorKey = "SelectionFrameColor";
const char* const kLinkLineColorKey = "LinkLineColor";
const char* const kLinkLineWidthKey = "LinkLineWidth";
const char* const kPathEngineKey = "PathEngine";
}

LLKAppSettings& LLKAppSettings::instance(){
    static LLKAppSettings settings;
    return settings;
}

LLKAppSettings::LLKAppSettings()
    : m_settings(QSettings::IniFormat, QSettings::UserScope, "LLK", "LinePuppyLLK"),
      m_bgMusicVolume(defaultBgMusicVolume()),
      m_removeSoundVolume(defaultRemoveSoundVolume()),
      m_selectionFrameColor(defaultSelectionFrameColor()),
      m_linkLineColor(defaultLinkLineColor()),
      m_linkLineWidth(defaultLinkLineWidth()),
      m_pathEngine(defaultPathEngine()),
      m_activeUserId(0){
    load();
}

void LLKAppSettings::load(){
    auto readValue = [this](const char* key, const QVariant& defaultValue) -> QVariant {
        const QString scopedKey = ScopedSettingKey(key);
        if(m_activeUserId > 0 && !m_settings.contains(scopedKey)){
            return m_settings.value(GlobalSettingKey(key), defaultValue);
        }
        return m_settings.value(scopedKey, defaultValue);
    };

    m_bgMusicVolume = ClampVolume(readValue(kBgMusicVolumeKey, defaultBgMusicVolume()).toInt());
    m_removeSoundVolume = ClampVolume(readValue(kRemoveSoundVolumeKey, defaultRemoveSoundVolume()).toInt());
    m_selectionFrameColor = readValue(kSelectionFrameColorKey, defaultSelectionFrameColor()).value<QColor>();
    if(!m_selectionFrameColor.isValid()){
        m_selectionFrameColor = defaultSelectionFrameColor();
    }
    m_linkLineColor = readValue(kLinkLineColorKey, defaultLinkLineColor()).value<QColor>();
    if(!m_linkLineColor.isValid()){
        m_linkLineColor = defaultLinkLineColor();
    }
    m_linkLineWidth = ClampLineWidth(readValue(kLinkLineWidthKey, defaultLinkLineWidth()).toInt());
    const int storedEngine = readValue(kPathEngineKey, defaultPathEngine()).toInt();
    if(storedEngine == static_cast<int>(EngineGraph)){
        m_pathEngine = EngineGraph;
    }else{
        m_pathEngine = EngineLinear;
    }
}

void LLKAppSettings::save(){
    m_settings.setValue(ScopedSettingKey(kBgMusicVolumeKey), m_bgMusicVolume);
    m_settings.setValue(ScopedSettingKey(kRemoveSoundVolumeKey), m_removeSoundVolume);
    m_settings.setValue(ScopedSettingKey(kSelectionFrameColorKey), m_selectionFrameColor);
    m_settings.setValue(ScopedSettingKey(kLinkLineColorKey), m_linkLineColor);
    m_settings.setValue(ScopedSettingKey(kLinkLineWidthKey), m_linkLineWidth);
    m_settings.setValue(ScopedSettingKey(kPathEngineKey), static_cast<int>(m_pathEngine));
    m_settings.sync();
}

void LLKAppSettings::setActiveUser(qint64 userId){
    const qint64 normalizedUserId = userId > 0 ? userId : 0;
    if(m_activeUserId == normalizedUserId){
        return;
    }
    m_activeUserId = normalizedUserId;
    load();
}

qint64 LLKAppSettings::activeUserId() const{
    return m_activeUserId;
}

void LLKAppSettings::resetToDefaults(){
    m_bgMusicVolume = defaultBgMusicVolume();
    m_removeSoundVolume = defaultRemoveSoundVolume();
    m_selectionFrameColor = defaultSelectionFrameColor();
    m_linkLineColor = defaultLinkLineColor();
    m_linkLineWidth = defaultLinkLineWidth();
    m_pathEngine = defaultPathEngine();
}

int LLKAppSettings::defaultBgMusicVolume(){
    return 35;
}

int LLKAppSettings::defaultRemoveSoundVolume(){
    return 50;
}

QColor LLKAppSettings::defaultSelectionFrameColor(){
    return QColor(255, 66, 66);
}

QColor LLKAppSettings::defaultLinkLineColor(){
    return QColor(234, 63, 247);
}

int LLKAppSettings::defaultLinkLineWidth(){
    return 2;
}

LLKAppSettings::PathEngine LLKAppSettings::defaultPathEngine(){
    return EngineLinear;    //默认使用线性结构
}

int LLKAppSettings::bgMusicVolume() const{
    return m_bgMusicVolume;
}

void LLKAppSettings::setBgMusicVolume(int volume){
    m_bgMusicVolume = ClampVolume(volume);
}

int LLKAppSettings::removeSoundVolume() const{
    return m_removeSoundVolume;
}

void LLKAppSettings::setRemoveSoundVolume(int volume){
    m_removeSoundVolume = ClampVolume(volume);
}

QColor LLKAppSettings::selectionFrameColor() const{
    return m_selectionFrameColor;
}

void LLKAppSettings::setSelectionFrameColor(const QColor& color){
    if(color.isValid()){
        m_selectionFrameColor = color;
    }
}

QColor LLKAppSettings::linkLineColor() const{
    return m_linkLineColor;
}

void LLKAppSettings::setLinkLineColor(const QColor& color){
    if(color.isValid()){
        m_linkLineColor = color;
    }
}

int LLKAppSettings::linkLineWidth() const{
    return m_linkLineWidth;
}

void LLKAppSettings::setLinkLineWidth(int width){
    m_linkLineWidth = ClampLineWidth(width);
}

LLKAppSettings::PathEngine LLKAppSettings::pathEngine() const{
    return m_pathEngine;
}

void LLKAppSettings::setPathEngine(PathEngine engine){
    if(engine == EngineGraph){
        m_pathEngine = EngineGraph;
    }else{
        m_pathEngine = EngineLinear;
    }
}

int LLKAppSettings::ClampVolume(int value){
    return std::max(0, std::min(value, 100));
}

int LLKAppSettings::ClampLineWidth(int value){
    return std::max(1, std::min(value, 8));
}

QString LLKAppSettings::ScopedSettingKey(const char* key) const{
    if(m_activeUserId > 0){
        return QString("Users/%1/%2/%3").arg(m_activeUserId).arg(kGroupName).arg(key);
    }
    return GlobalSettingKey(key);
}

QString LLKAppSettings::GlobalSettingKey(const char* key){
    return QString("%1/%2").arg(kGroupName).arg(key);
}