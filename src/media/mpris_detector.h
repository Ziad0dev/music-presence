#pragma once

#include <QObject>
#include <QMap>
#include <QDBusInterface>
#include <QDBusServiceWatcher>
#include <QDBusMessage>
#include <memory>

/**
 * @brief Class to detect and interact with MPRIS-compatible media players
 * 
 * Uses the D-Bus MPRIS interface to detect media players and track their status
 */
class MprisDetector : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Constructor
     */
    explicit MprisDetector(QObject *parent = nullptr);
    
    /**
     * @brief Destructor
     */
    ~MprisDetector();
    
    /**
     * @brief Start detection of media players
     */
    void startDetection();
    
    /**
     * @brief Stop detection of media players
     */
    void stopDetection();

signals:
    /**
     * @brief Signal emitted when a media player's state changes
     * @param playerName Name of the media player
     * @param title Song title
     * @param artist Artist name
     * @param album Album name
     * @param isPlaying Whether the player is playing
     * @param coverArtPath Path to the cover art
     */
    void mediaPlayerUpdated(const QString &playerName, 
                          const QString &title,
                          const QString &artist,
                          const QString &album,
                          bool isPlaying,
                          const QString &coverArtPath);

private slots:
    /**
     * @brief Handle a new D-Bus service appearing
     * @param serviceName Name of the D-Bus service
     */
    void handleServiceRegistered(const QString &serviceName);
    
    /**
     * @brief Handle a D-Bus service disappearing
     * @param serviceName Name of the D-Bus service
     */
    void handleServiceUnregistered(const QString &serviceName);
    
    /**
     * @brief Handle property changes from a media player
     * @param changedProperties Map of changed properties
     */
    void handlePropertiesChanged(const QDBusMessage &message);

private:
    /**
     * @brief Find MPRIS-compatible media players
     */
    void findMediaPlayers();
    
    /**
     * @brief Connect to a media player
     * @param serviceName Name of the D-Bus service
     */
    void connectToMediaPlayer(const QString &serviceName);
    
    /**
     * @brief Extract player name from MPRIS service name
     * @param serviceName MPRIS service name
     * @return Player name
     */
    QString extractPlayerName(const QString &serviceName) const;
    
    /**
     * @brief Get media metadata from a player
     * @param interface D-Bus interface to the player
     * @return Map of metadata
     */
    QVariantMap getMediaMetadata(QDBusInterface *interface) const;
    
    /**
     * @brief Save cover art from player metadata
     * @param metadata Media metadata
     * @return Path to saved cover art
     */
    QString saveCoverArt(const QVariantMap &metadata) const;

    /**
     * @brief Get interface for a service name
     * @param serviceName Name of the D-Bus service
     * @return Pointer to the interface or nullptr if not found
     */
    QDBusInterface* getInterface(const QString &serviceName) const;
    
    /**
     * @brief Sanitize text from media player
     * 
     * Removes problematic characters and truncates if necessary
     * 
     * @param text Text to sanitize
     * @param maxLength Maximum length of text
     * @return Sanitized text
     */
    QString sanitizeText(const QString &text, int maxLength = 128) const;

private:
    QDBusServiceWatcher *m_serviceWatcher;
    QMap<QString, QDBusInterface*> m_playerInterfaces;
    QMap<QString, QString> m_playerCoverPaths;
    QMap<QString, QVariantMap> m_playerMetadata; // Store metadata for each player
    bool m_isDetecting;
}; 