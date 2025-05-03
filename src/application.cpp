#include "application.h"
#include "settings/settings_manager.h"
#include "ui/systray.h"
#include "media/mpris_detector.h"
#include "discord/discord_rpc.h"
#include <QDebug>
#include <QTimer>

Application::Application(bool startMinimized, QObject *parent)
    : QObject(parent)
    , m_lastPlayerName("")
    , m_pauseHandlingTimer(new QTimer(this))
{
    // Initialize settings
    m_settingsManager = std::make_unique<SettingsManager>();
    
    // Initialize Discord RPC
    m_discordRpc = std::make_unique<DiscordRpc>();
    m_discordRpc->initialize();
    
    // Initialize MPRIS detector
    m_mprisDetector = std::make_unique<MprisDetector>();
    connect(m_mprisDetector.get(), &MprisDetector::mediaPlayerUpdated,
            this, &Application::handleMediaPlayerUpdate);
    m_mprisDetector->startDetection();
    
    // Initialize system tray
    m_sysTray = std::make_unique<SysTray>();
    m_sysTray->show();
    
    // Setup pause handling timer
    m_pauseHandlingTimer->setSingleShot(true);
    connect(m_pauseHandlingTimer, &QTimer::timeout, this, &Application::handlePauseTimeout);
    
    qDebug() << "Application initialized";
}

Application::~Application()
{
    // Cancel pause timer
    if (m_pauseHandlingTimer->isActive()) {
        m_pauseHandlingTimer->stop();
    }
    
    // Shutdown in reverse order
    m_sysTray.reset();
    m_mprisDetector.reset();
    m_discordRpc.reset();
    m_settingsManager.reset();
    
    qDebug() << "Application shutdown complete";
}

void Application::handleMediaPlayerUpdate(const QString &playerName, 
                                       const QString &title,
                                       const QString &artist,
                                       const QString &album,
                                       bool isPlaying,
                                       const QString &coverArtPath)
{
    qDebug() << "Media update:" << playerName << title << artist << album 
             << (isPlaying ? "playing" : "paused") << coverArtPath;
    
    // Check for player changes
    bool playerChanged = !m_lastPlayerName.isEmpty() && 
                         m_lastPlayerName != playerName && 
                         !playerName.isEmpty();
    
    if (playerChanged) {
        qDebug() << "Player changed from" << m_lastPlayerName << "to" << playerName;
    }
    
    m_lastPlayerName = playerName.isEmpty() ? m_lastPlayerName : playerName;
    
    // Update Discord Rich Presence if playing
    if (isPlaying && !title.isEmpty()) {
        // Cancel any pending pause handling
        if (m_pauseHandlingTimer->isActive()) {
            m_pauseHandlingTimer->stop();
        }
        
        m_discordRpc->updatePresence(
            playerName,
            title,
            artist,
            album,
            coverArtPath
        );
        
        // Update system tray icon to show active state
        m_sysTray->setActive(true);
    } else if (!isPlaying) {
        // When paused, don't immediately clear presence 
        // Give some time in case it's just a brief pause
        if (!m_pauseHandlingTimer->isActive()) {
            // Wait 30 seconds before clearing presence when paused
            m_pauseHandlingTimer->start(30000);
        }
    } else if (title.isEmpty() && isPlaying) {
        // Handle the case where player reports playing but no track info
        // This is likely a bad update, so ignore it
        qDebug() << "Ignoring update with empty title but playing state";
    }
}

void Application::handlePauseTimeout()
{
    qDebug() << "Media has been paused for 30 seconds, clearing presence";
    
    // Clear presence when paused for more than timeout period
    m_discordRpc->clearPresence();
    
    // Update system tray icon to show inactive state
    m_sysTray->setActive(false);
} 