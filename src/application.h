#pragma once

#include <QObject>
#include <memory>

class SettingsManager;
class SysTray;
class MprisDetector;
class DiscordRpc;
class QTimer;

/**
 * @brief Main application class that coordinates all components
 */
class Application : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param startMinimized Whether to start the application minimized
     */
    explicit Application(bool startMinimized = false, QObject *parent = nullptr);
    
    /**
     * @brief Destructor
     */
    ~Application();

private slots:
    /**
     * @brief Handle media player state changes
     * @param playerName Name of the media player
     * @param title Song title
     * @param artist Artist name
     * @param album Album name
     * @param isPlaying Whether the player is playing
     * @param coverArtPath Path to the cover art
     */
    void handleMediaPlayerUpdate(const QString &playerName, 
                               const QString &title,
                               const QString &artist,
                               const QString &album,
                               bool isPlaying,
                               const QString &coverArtPath);
    void handlePauseTimeout();

private:
    std::unique_ptr<SettingsManager> m_settingsManager;
    std::unique_ptr<SysTray> m_sysTray;
    std::unique_ptr<MprisDetector> m_mprisDetector;
    std::unique_ptr<DiscordRpc> m_discordRpc;
    
    // Track the last active player
    QString m_lastPlayerName;
    
    // Timer for handling paused state
    QTimer* m_pauseHandlingTimer;
}; 