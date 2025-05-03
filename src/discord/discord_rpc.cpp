#include "discord_rpc.h"
#include <QDebug>
#include <QDateTime>
#include <QCoreApplication>
#include <QUrl>

#ifdef USE_DISCORD_RPC
#include <discord_rpc.h>
#endif

// IMPORTANT: This is a placeholder! Replace it with your actual Discord application ID
// To create your own Discord application:
// 1. Go to https://discord.com/developers/applications
// 2. Click "New Application" and give it a name (e.g., "Music Presence")
// 3. Copy the "Application ID" from the General Information page
// 4. Paste it here replacing the placeholder below
const QString DEFAULT_APP_ID = "1234567890123456789";

// Discord RPC callbacks
#ifdef USE_DISCORD_RPC
static void handleDiscordReady(const DiscordUser* request)
{
    qDebug() << "Discord: connected to user" << request->username << "#" << request->discriminator;
}

static void handleDiscordDisconnected(int errorCode, const char* message)
{
    qDebug() << "Discord: disconnected" << errorCode << message;
}

static void handleDiscordError(int errorCode, const char* message)
{
    qWarning() << "Discord: error" << errorCode << message;
}
#endif

DiscordRpc::DiscordRpc(QObject *parent)
    : QObject(parent)
    , m_applicationId(DEFAULT_APP_ID)
    , m_isInitialized(false)
    , m_startTimestamp(0)
    , m_updatePending(false)
    , m_invalidUpdateCount(0)
{
    // Load application ID from settings if available
    // For now, we're using the default

    // Set up update timer to keep Discord RPC connection alive
    connect(&m_updateTimer, &QTimer::timeout, this, &DiscordRpc::updateConnection);
    m_updateTimer.setInterval(5000); // 5 seconds
    
    // Set up clear presence timer with a delay
    m_clearPresenceTimer.setSingleShot(true);
    connect(&m_clearPresenceTimer, &QTimer::timeout, 
            this, &DiscordRpc::onClearPresenceTimerTimeout);
            
    // Set up debounce timer for update requests
    m_updateDebounceTimer.setSingleShot(true);
    connect(&m_updateDebounceTimer, &QTimer::timeout,
            this, &DiscordRpc::onUpdateDebounceTimerTimeout);
}

DiscordRpc::~DiscordRpc()
{
#ifdef USE_DISCORD_RPC
    if (m_isInitialized) {
        Discord_Shutdown();
    }
#endif
    
    m_updateTimer.stop();
    m_clearPresenceTimer.stop();
    m_updateDebounceTimer.stop();
}

bool DiscordRpc::initialize()
{
    qDebug() << "Initializing Discord RPC with app ID:" << m_applicationId;
    
#ifdef USE_DISCORD_RPC
    DiscordEventHandlers handlers;
    memset(&handlers, 0, sizeof(handlers));
    handlers.ready = handleDiscordReady;
    handlers.disconnected = handleDiscordDisconnected;
    handlers.errored = handleDiscordError;
    
    // Initialize Discord RPC
    Discord_Initialize(m_applicationId.toStdString().c_str(), &handlers, 1, nullptr);
    
    // Set a default presence to show the app is running
    DiscordRichPresence presence;
    memset(&presence, 0, sizeof(presence));
    presence.state = "Waiting for music";
    presence.details = "No media playing";
    presence.startTimestamp = QDateTime::currentSecsSinceEpoch();
    presence.largeImageKey = "app_icon";
    presence.largeImageText = "Music Presence";
    Discord_UpdatePresence(&presence);
    
    m_isInitialized = true;
    m_updateTimer.start();
#else
    qDebug() << "Discord RPC support not enabled at compile time";
    m_isInitialized = true; // Pretend we're initialized for the rest of the app
#endif
    
    return m_isInitialized;
}

bool DiscordRpc::validateTrackData(const QString &title, const QString &artist)
{
    // Check for empty data
    if (title.isEmpty()) {
        qDebug() << "Rejecting update: Empty title";
        return false;
    }
    
    // Simple check for placeholder/default values that some players might send
    if (title.toLower() == "unknown" || title.toLower() == "unknown title" || 
        title.toLower() == "untitled" || title == "?") {
        qDebug() << "Rejecting update: Generic placeholder title:" << title;
        return false;
    }
    
    // Check for overly short titles that might indicate partial data
    if (title.length() < 2) {
        qDebug() << "Rejecting update: Title too short:" << title;
        return false;
    }
    
    // Check for overly long titles that might indicate corrupted data
    if (title.length() > 128) {
        qDebug() << "Truncating very long title:" << title;
        // We'll still accept it but log a warning
    }
    
    return true;
}

bool DiscordRpc::updatePresence(const QString &playerName,
                              const QString &title,
                              const QString &artist,
                              const QString &album,
                              const QString &coverArtPath)
{
    if (!m_isInitialized) {
        return false;
    }
    
    // Cancel any pending clear presence
    if (m_clearPresenceTimer.isActive()) {
        m_clearPresenceTimer.stop();
    }
    
    // If this is the same track that's already showing, no need to update
    if (m_currentTitle == title && m_currentArtist == artist && 
        m_currentPlayerName == playerName && m_currentAlbum == album) {
        qDebug() << "Ignoring duplicate update for same track";
        return true;
    }
    
    // Validate the track data
    if (!validateTrackData(title, artist)) {
        m_invalidUpdateCount++;
        
        if (m_invalidUpdateCount >= MAX_INVALID_UPDATES) {
            qWarning() << "Too many invalid updates in a row, clearing presence";
            clearPresence();
            m_invalidUpdateCount = 0;
        }
        
        return false;
    }
    
    // Reset invalid update counter
    m_invalidUpdateCount = 0;
    
    // Store pending update data
    m_pendingPlayerName = playerName;
    m_pendingTitle = title;
    m_pendingArtist = artist;
    m_pendingAlbum = album;
    m_pendingCoverArtPath = coverArtPath;
    m_updatePending = true;
    
    // Use debounce timer to avoid rapid updates
    if (!m_updateDebounceTimer.isActive()) {
        m_updateDebounceTimer.start(500); // 500ms debounce
    }
    
    return true;
}

void DiscordRpc::onUpdateDebounceTimerTimeout()
{
    if (!m_updatePending || !m_isInitialized) {
        return;
    }
    
    qDebug() << "Updating Discord presence:" << m_pendingPlayerName 
             << m_pendingTitle << m_pendingArtist << m_pendingAlbum;
    
    // Update current track info
    m_currentPlayerName = m_pendingPlayerName;
    m_currentTitle = m_pendingTitle;
    m_currentArtist = m_pendingArtist;
    m_currentAlbum = m_pendingAlbum;
    
    // Set start timestamp if not set
    if (m_startTimestamp == 0) {
        m_startTimestamp = QDateTime::currentSecsSinceEpoch();
    }
    
#ifdef USE_DISCORD_RPC
    // Create the presence object
    DiscordRichPresence presence;
    memset(&presence, 0, sizeof(presence));
    
    // Setup state (artist)
    std::string artistStr = m_currentArtist.toStdString();
    presence.state = artistStr.c_str();
    
    // Setup details (title)
    std::string titleStr = m_currentTitle.toStdString();
    presence.details = titleStr.c_str();
    
    // Set timestamps
    presence.startTimestamp = m_startTimestamp;
    
    // Set assets
    std::string assetKey = setupAssets(m_currentPlayerName, m_pendingCoverArtPath).toStdString();
    presence.largeImageKey = assetKey.c_str();
    
    std::string albumStr = m_currentAlbum.toStdString();
    presence.largeImageText = albumStr.c_str();
    
    // Small image (app icon)
    presence.smallImageKey = "app_icon";
    presence.smallImageText = "Music Presence";
    
    // Note: Buttons are not supported in this version of the library
    // We'll just use basic presence information
    
    // Update Discord
    Discord_UpdatePresence(&presence);
#endif

    m_updatePending = false;
    
    return;
}

bool DiscordRpc::clearPresence()
{
    if (!m_isInitialized) {
        return false;
    }
    
    m_clearPresenceTimer.stop();
    m_updateDebounceTimer.stop();
    m_updatePending = false;
    
    qDebug() << "Clearing Discord presence";
    
    // Reset current state
    m_currentPlayerName.clear();
    m_currentTitle.clear();
    m_currentArtist.clear();
    m_currentAlbum.clear();
    m_startTimestamp = 0;
    
#ifdef USE_DISCORD_RPC
    // Clear the presence
    Discord_ClearPresence();
#endif
    
    return true;
}

void DiscordRpc::onClearPresenceTimerTimeout()
{
    clearPresence();
}

void DiscordRpc::updateConnection()
{
    if (!m_isInitialized) {
        return;
    }
    
#ifdef USE_DISCORD_RPC
    // Run callbacks to keep the connection alive
    Discord_RunCallbacks();
#endif
}

QString DiscordRpc::setupAssets(const QString &playerName, const QString &coverArtPath)
{
    // Normalize the player name for use as an asset key
    QString assetKey = playerName.toLower().replace(' ', '_');
    
    // In a real implementation with a proper Discord application,
    // you would upload the player icon as an asset with this key
    // For now, we just return the key
    
    // In a more advanced implementation, you'd also upload the album art
    // and return a unique key for it
    
    return assetKey;
} 