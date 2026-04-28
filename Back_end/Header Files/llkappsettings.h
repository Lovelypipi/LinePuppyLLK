#ifndef LLKAPPSETTINGS_H
#define LLKAPPSETTINGS_H

#include <QtCore/QSettings>
#include <QtCore/QString>
#include <QtCore/QtGlobal>
#include <QtGui/QColor>

class LLKAppSettings{
public:
    enum PathEngine {EngineLinear = 0, EngineGraph = 1};

    static LLKAppSettings& instance();

    void load();
    void save();
    void resetToDefaults();
    void setActiveUser(qint64 userId);
    qint64 activeUserId() const;

    static int defaultBgMusicVolume();
    static int defaultRemoveSoundVolume();
    static QColor defaultSelectionFrameColor();
    static QColor defaultLinkLineColor();
    static int defaultLinkLineWidth();
    static PathEngine defaultPathEngine();

    int bgMusicVolume() const;
    void setBgMusicVolume(int volume);

    int removeSoundVolume() const;
    void setRemoveSoundVolume(int volume);

    QColor selectionFrameColor() const;
    void setSelectionFrameColor(const QColor& color);

    QColor linkLineColor() const;
    void setLinkLineColor(const QColor& color);

    int linkLineWidth() const;
    void setLinkLineWidth(int width);

    PathEngine pathEngine() const;                 
    void setPathEngine(PathEngine engine);         

private:
    LLKAppSettings();

    static int ClampVolume(int value);
    static int ClampLineWidth(int value);
    QString ScopedSettingKey(const char* key) const;
    static QString GlobalSettingKey(const char* key);

    QSettings m_settings;
    int m_bgMusicVolume;
    int m_removeSoundVolume;
    QColor m_selectionFrameColor;
    QColor m_linkLineColor;
    int m_linkLineWidth;
    PathEngine m_pathEngine;
    qint64 m_activeUserId;
};

#endif