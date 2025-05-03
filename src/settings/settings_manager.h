#pragma once

#include <QObject>
#include <QString>
#include <QVariant>
#include <QMap>

/**
 * @brief Class to manage application settings
 * 
 * Handles saving and loading settings to/from disk
 */
class SettingsManager : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Constructor
     */
    explicit SettingsManager(QObject *parent = nullptr);
    
    /**
     * @brief Destructor
     */
    ~SettingsManager();
    
    /**
     * @brief Get a setting value
     * @param key Setting key
     * @param defaultValue Default value if setting is not found
     * @return Setting value
     */
    QVariant getValue(const QString &key, const QVariant &defaultValue = QVariant()) const;
    
    /**
     * @brief Set a setting value
     * @param key Setting key
     * @param value Setting value
     * @return Whether the operation was successful
     */
    bool setValue(const QString &key, const QVariant &value);
    
    /**
     * @brief Check if a player is enabled
     * @param playerName Name of the player
     * @return Whether the player is enabled
     */
    bool isPlayerEnabled(const QString &playerName) const;
    
    /**
     * @brief Set whether a player is enabled
     * @param playerName Name of the player
     * @param enabled Whether the player is enabled
     * @return Whether the operation was successful
     */
    bool setPlayerEnabled(const QString &playerName, bool enabled);
    
    /**
     * @brief Get the list of known players
     * @return List of player names
     */
    QStringList getKnownPlayers() const;
    
    /**
     * @brief Check if autostart is enabled
     * @return Whether autostart is enabled
     */
    bool isAutostartEnabled() const;
    
    /**
     * @brief Set whether autostart is enabled
     * @param enabled Whether autostart is enabled
     * @return Whether the operation was successful
     */
    bool setAutostartEnabled(bool enabled);

signals:
    /**
     * @brief Signal emitted when settings are changed
     */
    void settingsChanged();

private:
    /**
     * @brief Load settings from disk
     * @return Whether the operation was successful
     */
    bool loadSettings();
    
    /**
     * @brief Save settings to disk
     * @return Whether the operation was successful
     */
    bool saveSettings();
    
    /**
     * @brief Get the path to the settings file
     * @return Path to the settings file
     */
    QString getSettingsFilePath() const;
    
    /**
     * @brief Set up autostart
     * @param enabled Whether autostart is enabled
     * @return Whether the operation was successful
     */
    bool setupAutostart(bool enabled);

private:
    QMap<QString, QVariant> m_settings;
    QMap<QString, bool> m_playerStates;
    bool m_autostartEnabled;
}; 