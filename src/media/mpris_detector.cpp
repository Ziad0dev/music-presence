#include "mpris_detector.h"
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusReply>
#include <QDBusMessage>
#include <QDBusArgument>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>
#include <QImage>
#include <QUrl>

// MPRIS D-Bus constants
const QString MPRIS_PREFIX = "org.mpris.MediaPlayer2.";
const QString MPRIS_INTERFACE = "org.mpris.MediaPlayer2.Player";
const QString MPRIS_PATH = "/org/mpris/MediaPlayer2";
const QString PROPERTIES_INTERFACE = "org.freedesktop.DBus.Properties";

MprisDetector::MprisDetector(QObject *parent)
    : QObject(parent)
    , m_serviceWatcher(nullptr)
    , m_isDetecting(false)
{
}

MprisDetector::~MprisDetector()
{
    stopDetection();
}

void MprisDetector::startDetection()
{
    if (m_isDetecting) {
        return;
    }
    
    m_isDetecting = true;
    
    // Create a D-Bus service watcher
    m_serviceWatcher = new QDBusServiceWatcher(this);
    m_serviceWatcher->setConnection(QDBusConnection::sessionBus());
    m_serviceWatcher->setWatchMode(QDBusServiceWatcher::WatchForOwnerChange);
    
    // Connect signals
    connect(m_serviceWatcher, &QDBusServiceWatcher::serviceRegistered,
            this, &MprisDetector::handleServiceRegistered);
    connect(m_serviceWatcher, &QDBusServiceWatcher::serviceUnregistered,
            this, &MprisDetector::handleServiceUnregistered);
    
    // Find existing media players
    findMediaPlayers();
}

void MprisDetector::stopDetection()
{
    if (!m_isDetecting) {
        return;
    }
    
    m_isDetecting = false;
    
    // Delete all interfaces
    for (auto interface : m_playerInterfaces) {
        delete interface;
    }
    m_playerInterfaces.clear();
    
    // Delete the service watcher
    if (m_serviceWatcher) {
        delete m_serviceWatcher;
        m_serviceWatcher = nullptr;
    }
}

void MprisDetector::findMediaPlayers()
{
    // Get all services on the session bus
    QDBusConnectionInterface *interface = QDBusConnection::sessionBus().interface();
    QDBusReply<QStringList> reply = interface->registeredServiceNames();
    
    if (!reply.isValid()) {
        qWarning() << "Failed to get D-Bus services:" << reply.error().message();
        return;
    }
    
    // Filter for MPRIS services
    QStringList services = reply.value();
    for (const QString &service : services) {
        if (service.startsWith(MPRIS_PREFIX)) {
            connectToMediaPlayer(service);
            
            // Add the service to the watcher
            m_serviceWatcher->addWatchedService(service);
        }
    }
}

void MprisDetector::connectToMediaPlayer(const QString &serviceName)
{
    // Create a D-Bus interface to the player
    QDBusInterface *interface = new QDBusInterface(
        serviceName,
        MPRIS_PATH,
        MPRIS_INTERFACE,
        QDBusConnection::sessionBus()
    );
    
    if (!interface->isValid()) {
        qWarning() << "Failed to connect to" << serviceName << ":" 
                  << QDBusConnection::sessionBus().lastError().message();
        delete interface;
        return;
    }
    
    // Get player name
    QString playerName = extractPlayerName(serviceName);
    
    // Connect to the PropertiesChanged signal
    QDBusConnection::sessionBus().connect(
        serviceName,
        MPRIS_PATH,
        PROPERTIES_INTERFACE,
        "PropertiesChanged",
        this,
        SLOT(handlePropertiesChanged(QDBusMessage))
    );
    
    // Store the interface
    m_playerInterfaces[serviceName] = interface;
    
    qDebug() << "Connected to media player:" << playerName;
    
    // Get initial metadata
    QVariantMap metadata = getMediaMetadata(interface);
    
    // Check if playing
    QVariant playbackStatus = interface->property("PlaybackStatus");
    bool isPlaying = (playbackStatus.toString() == "Playing");
    
    // Get metadata
    QString title = sanitizeText(metadata["xesam:title"].toString().trimmed());
    
    // Extract and validate artist - handle various formats
    QString artist;
    if (metadata.contains("xesam:artist")) {
        QVariant artistVariant = metadata["xesam:artist"];
        if (artistVariant.type() == QVariant::StringList) {
            artist = artistVariant.toStringList().join(", ");
        } else if (artistVariant.type() == QVariant::String) {
            artist = artistVariant.toString();
        } 
    }
    artist = sanitizeText(artist.trimmed());
    
    // Default artist if empty
    if (artist.isEmpty()) {
        artist = "Unknown Artist";
    }
    
    QString album = sanitizeText(metadata["xesam:album"].toString().trimmed());
    
    // Validate data - don't send incomplete updates
    if (title.isEmpty()) {
        if (isPlaying) {
            qDebug() << "Not sending initial update for" << playerName << "- title is empty but state is playing";
        }
        return;
    }
    
    // Check length - some players might report very short titles during transitions
    if (title.length() < 2 && isPlaying) {
        qDebug() << "Not sending initial update from" << playerName << "- title is too short:" << title;
        return;
    }
    
    // Store current metadata
    QVariantMap currentMetadata;
    currentMetadata["title"] = title;
    currentMetadata["artist"] = artist;
    currentMetadata["album"] = album;
    m_playerMetadata[serviceName] = currentMetadata;
    
    // Save cover art
    QString coverArtPath = saveCoverArt(metadata);
    
    // Emit the signal
    emit mediaPlayerUpdated(playerName, title, artist, album, isPlaying, coverArtPath);
}

void MprisDetector::handleServiceRegistered(const QString &serviceName)
{
    if (serviceName.startsWith(MPRIS_PREFIX)) {
        qDebug() << "New media player detected:" << serviceName;
        connectToMediaPlayer(serviceName);
    }
}

void MprisDetector::handleServiceUnregistered(const QString &serviceName)
{
    if (m_playerInterfaces.contains(serviceName)) {
        QString playerName = extractPlayerName(serviceName);
        qDebug() << "Media player disconnected:" << playerName;
        
        // Delete the interface
        delete m_playerInterfaces[serviceName];
        m_playerInterfaces.remove(serviceName);
        
        // Clear the cover art path
        m_playerCoverPaths.remove(serviceName);
        
        // Clear stored metadata
        m_playerMetadata.remove(serviceName);
        
        // Emit signal with empty metadata to clear the presence
        emit mediaPlayerUpdated(playerName, "", "", "", false, "");
    }
}

void MprisDetector::handlePropertiesChanged(const QDBusMessage &message)
{
    if (message.arguments().size() < 3) {
        return;
    }
    
    QString interface = message.arguments().at(0).toString();
    if (interface != MPRIS_INTERFACE) {
        return;
    }
    
    // Get the service name
    QString serviceName = message.service();
    if (!m_playerInterfaces.contains(serviceName)) {
        return;
    }
    
    // Get the player name
    QString playerName = extractPlayerName(serviceName);
    
    // Get the properties
    QDBusInterface *pInterface = m_playerInterfaces[serviceName];
    QVariantMap metadata = getMediaMetadata(pInterface);
    
    // Check if playing
    QVariant playbackStatus = pInterface->property("PlaybackStatus");
    bool isPlaying = (playbackStatus.toString() == "Playing");
    
    // Get metadata
    QString title = sanitizeText(metadata["xesam:title"].toString().trimmed());
    
    // Extract and validate artist - handle various formats
    QString artist;
    if (metadata.contains("xesam:artist")) {
        QVariant artistVariant = metadata["xesam:artist"];
        if (artistVariant.type() == QVariant::StringList) {
            artist = artistVariant.toStringList().join(", ");
        } else if (artistVariant.type() == QVariant::String) {
            artist = artistVariant.toString();
        } 
    }
    artist = sanitizeText(artist.trimmed());
    
    // Default artist if empty
    if (artist.isEmpty()) {
        artist = "Unknown Artist";
    }
    
    QString album = sanitizeText(metadata["xesam:album"].toString().trimmed());
    
    // Validate data - skip corrupt/incomplete updates
    if (title.isEmpty() && isPlaying) {
        qDebug() << "Skipping update from" << playerName << "- title is empty but state is playing";
        // We'll still emit the playing state but keep previous title if available
        QString currentTitle;
        QString currentArtist;
        QString currentAlbum;
        
        // Check if we have previous data for this player
        if (m_playerMetadata.contains(serviceName)) {
            currentTitle = m_playerMetadata[serviceName]["title"].toString();
            currentArtist = m_playerMetadata[serviceName]["artist"].toString();
            currentAlbum = m_playerMetadata[serviceName]["album"].toString();
        }
        
        if (!currentTitle.isEmpty()) {
            // Use previous metadata with current play state
            title = currentTitle;
            artist = currentArtist;
            album = currentAlbum;
            qDebug() << "Using previous metadata for" << playerName;
        } else {
            qDebug() << "No previous metadata for" << playerName << ", skipping update";
            return;
        }
    }
    
    // Check length - some players might report very short titles during transitions
    if (title.length() < 2 && isPlaying) {
        qDebug() << "Skipping update from" << playerName << "- title is too short:" << title;
        return;
    }
    
    // Store current metadata
    QVariantMap currentMetadata;
    currentMetadata["title"] = title;
    currentMetadata["artist"] = artist;
    currentMetadata["album"] = album;
    m_playerMetadata[serviceName] = currentMetadata;
    
    // Save cover art
    QString coverArtPath = saveCoverArt(metadata);
    
    // Emit the signal
    emit mediaPlayerUpdated(playerName, title, artist, album, isPlaying, coverArtPath);
}

QString MprisDetector::extractPlayerName(const QString &serviceName) const
{
    return serviceName.mid(MPRIS_PREFIX.length());
}

QVariantMap MprisDetector::getMediaMetadata(QDBusInterface *interface) const
{
    QVariant metadataVariant = interface->property("Metadata");
    QDBusArgument argument = metadataVariant.value<QDBusArgument>();
    QVariantMap metadata;
    
    argument >> metadata;
    return metadata;
}

QString MprisDetector::saveCoverArt(const QVariantMap &metadata) const
{
    // Check if the metadata has an art URL
    QString artUrlStr;
    if (metadata.contains("mpris:artUrl")) {
        artUrlStr = metadata["mpris:artUrl"].toString();
    }
    
    if (artUrlStr.isEmpty()) {
        return QString();
    }
    
    QUrl artUrl(artUrlStr);
    
    // Create the cache directory if it doesn't exist
    QDir cacheDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
    if (!cacheDir.exists()) {
        cacheDir.mkpath(".");
    }
    
    // Create a file path for the cover art
    QString title = metadata["xesam:title"].toString();
    QString artist = metadata["xesam:artist"].toStringList().join(", ");
    QString filePath = cacheDir.filePath(QString("%1-%2.png").arg(artist, title));
    
    // If the file already exists, return its path
    if (QFile::exists(filePath)) {
        return filePath;
    }
    
    // If the art URL is a local file, copy it
    if (artUrl.isLocalFile()) {
        QFile::copy(artUrl.toLocalFile(), filePath);
        return filePath;
    }
    
    // Remote file handling would go here (using QNetworkAccessManager)
    // For simplicity, we're not implementing that in this example
    
    return QString();
}

QDBusInterface* MprisDetector::getInterface(const QString &serviceName) const
{
    if (m_playerInterfaces.contains(serviceName)) {
        return m_playerInterfaces[serviceName];
    }
    return nullptr;
}

QString MprisDetector::sanitizeText(const QString &text, int maxLength) const
{
    if (text.isEmpty()) {
        return text;
    }
    
    // Make a copy we can modify
    QString result = text;
    
    // Remove any control characters
    for (int i = 0; i < result.length(); i++) {
        if (result[i].category() == QChar::Other_Control) {
            result[i] = ' ';
        }
    }
    
    // Remove any excessive whitespace
    result = result.simplified();
    
    // Truncate if longer than maxLength
    if (result.length() > maxLength) {
        result = result.left(maxLength - 3) + "...";
    }
    
    return result;
} 