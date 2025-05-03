#include "settings_manager.h"
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QFileInfo>

SettingsManager::SettingsManager(QObject *parent)
    : QObject(parent)
    , m_autostartEnabled(false)
{
    // Load settings
    loadSettings();
}

SettingsManager::~SettingsManager()
{
    // Save settings
    saveSettings();
}

QVariant SettingsManager::getValue(const QString &key, const QVariant &defaultValue) const
{
    if (m_settings.contains(key)) {
        return m_settings[key];
    }
    return defaultValue;
}

bool SettingsManager::setValue(const QString &key, const QVariant &value)
{
    m_settings[key] = value;
    emit settingsChanged();
    return saveSettings();
}

bool SettingsManager::isPlayerEnabled(const QString &playerName) const
{
    if (m_playerStates.contains(playerName)) {
        return m_playerStates[playerName];
    }
    return true; // Default to enabled
}

bool SettingsManager::setPlayerEnabled(const QString &playerName, bool enabled)
{
    m_playerStates[playerName] = enabled;
    emit settingsChanged();
    return saveSettings();
}

QStringList SettingsManager::getKnownPlayers() const
{
    return m_playerStates.keys();
}

bool SettingsManager::isAutostartEnabled() const
{
    return m_autostartEnabled;
}

bool SettingsManager::setAutostartEnabled(bool enabled)
{
    if (m_autostartEnabled != enabled) {
        m_autostartEnabled = enabled;
        setupAutostart(enabled);
        emit settingsChanged();
        return saveSettings();
    }
    return true;
}

bool SettingsManager::loadSettings()
{
    QString filePath = getSettingsFilePath();
    QFile file(filePath);
    
    if (!file.exists()) {
        qDebug() << "Settings file does not exist, using defaults";
        return false;
    }
    
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open settings file:" << file.errorString();
        return false;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "Invalid settings file format";
        return false;
    }
    
    QJsonObject root = doc.object();
    
    // Load general settings
    if (root.contains("settings") && root["settings"].isObject()) {
        QJsonObject settingsObj = root["settings"].toObject();
        for (auto it = settingsObj.begin(); it != settingsObj.end(); ++it) {
            m_settings[it.key()] = it.value().toVariant();
        }
    }
    
    // Load player states
    if (root.contains("players") && root["players"].isObject()) {
        QJsonObject playersObj = root["players"].toObject();
        for (auto it = playersObj.begin(); it != playersObj.end(); ++it) {
            m_playerStates[it.key()] = it.value().toBool();
        }
    }
    
    // Load autostart setting
    if (root.contains("autostart")) {
        m_autostartEnabled = root["autostart"].toBool();
    }
    
    qDebug() << "Settings loaded from" << filePath;
    return true;
}

bool SettingsManager::saveSettings()
{
    QString filePath = getSettingsFilePath();
    
    // Create the directory if it doesn't exist
    QFileInfo fileInfo(filePath);
    QDir dir = fileInfo.dir();
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            qWarning() << "Failed to create settings directory";
            return false;
        }
    }
    
    QJsonObject root;
    
    // Save general settings
    QJsonObject settingsObj;
    for (auto it = m_settings.begin(); it != m_settings.end(); ++it) {
        settingsObj[it.key()] = QJsonValue::fromVariant(it.value());
    }
    root["settings"] = settingsObj;
    
    // Save player states
    QJsonObject playersObj;
    for (auto it = m_playerStates.begin(); it != m_playerStates.end(); ++it) {
        playersObj[it.key()] = it.value();
    }
    root["players"] = playersObj;
    
    // Save autostart setting
    root["autostart"] = m_autostartEnabled;
    
    QJsonDocument doc(root);
    QByteArray data = doc.toJson(QJsonDocument::Indented);
    
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "Failed to open settings file for writing:" << file.errorString();
        return false;
    }
    
    if (file.write(data) == -1) {
        qWarning() << "Failed to write settings:" << file.errorString();
        file.close();
        return false;
    }
    
    file.close();
    qDebug() << "Settings saved to" << filePath;
    return true;
}

QString SettingsManager::getSettingsFilePath() const
{
    // Use standard configuration location
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    return QDir(configDir).filePath("music-presence/config.json");
}

bool SettingsManager::setupAutostart(bool enabled)
{
    // Get path to the autostart file
    QString autoStartPath = QDir(QStandardPaths::writableLocation(
        QStandardPaths::ConfigLocation)).filePath("autostart/music-presence.desktop");
    
    if (enabled) {
        // Create autostart directory if it doesn't exist
        QFileInfo fileInfo(autoStartPath);
        QDir dir = fileInfo.dir();
        if (!dir.exists() && !dir.mkpath(".")) {
            qWarning() << "Failed to create autostart directory";
            return false;
        }
        
        // Create desktop file
        QFile file(autoStartPath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            qWarning() << "Failed to create autostart file:" << file.errorString();
            return false;
        }
        
        // Write desktop file content
        QByteArray content = 
            "[Desktop Entry]\n"
            "Type=Application\n"
            "Name=Music Presence\n"
            "Exec=music-presence --minimized\n"
            "Terminal=false\n"
            "X-GNOME-Autostart-enabled=true\n";
        
        if (file.write(content) == -1) {
            qWarning() << "Failed to write autostart file:" << file.errorString();
            file.close();
            return false;
        }
        
        file.close();
    } else {
        // Remove autostart file
        QFile file(autoStartPath);
        if (file.exists() && !file.remove()) {
            qWarning() << "Failed to remove autostart file:" << file.errorString();
            return false;
        }
    }
    
    return true;
} 