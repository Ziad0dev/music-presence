#pragma once

#include <QObject>
#include <QString>
#include <QTimer>

/**
 * @brief Class to handle Discord Rich Presence integration
 * 
 * Uses Discord RPC to update the user's status with currently playing music
 */
class DiscordRpc : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Constructor
     */
    explicit DiscordRpc(QObject *parent = nullptr);
    
    /**
     * @brief Destructor
     */
    ~DiscordRpc();
    
    /**
     * @brief Initialize Discord RPC connection
     * @return Whether initialization was successful
     */
    bool initialize();
    
    /**
     * @brief Update the user's Discord presence
     * @param playerName Name of the media player
     * @param title Song title
     * @param artist Artist name
     * @param album Album name
     * @param coverArtPath Path to the cover art
     * @return Whether the update was successful
     */
    bool updatePresence(const QString &playerName,
                       const QString &title,
                       const QString &artist,
                       const QString &album,
                       const QString &coverArtPath);
    
    /**
     * @brief Clear the user's Discord presence
     * @return Whether the operation was successful
     */
    bool clearPresence();

private slots:
    /**
     * @brief Update Discord RPC connection
     * 
     * Called periodically to keep the connection alive
     */
    void updateConnection();
    void onClearPresenceTimerTimeout();
    void onUpdateDebounceTimerTimeout();

private:
    /**
     * @brief Set up the asset keys for Discord
     * @param playerName Name of the media player
     * @param coverArtPath Path to the cover art
     * @return Asset key for the cover art
     */
    QString setupAssets(const QString &playerName, const QString &coverArtPath);
    bool validateTrackData(const QString &title, const QString &artist);

private:
    QTimer m_updateTimer;
    QTimer m_clearPresenceTimer;
    QTimer m_updateDebounceTimer;
    QString m_applicationId;
    bool m_isInitialized;
    
    // Current state
    QString m_currentPlayerName;
    QString m_currentTitle;
    QString m_currentArtist;
    QString m_currentAlbum;
    qint64 m_startTimestamp;
    
    // Store current media info
    QString m_pendingTitle;
    QString m_pendingArtist;
    QString m_pendingAlbum;
    QString m_pendingPlayerName;
    QString m_pendingCoverArtPath;
    bool m_updatePending;
    int m_invalidUpdateCount;
    static const int MAX_INVALID_UPDATES = 3;
}; 