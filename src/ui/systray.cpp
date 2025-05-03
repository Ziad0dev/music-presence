#include "systray.h"
#include <QApplication>
#include <QMessageBox>
#include <QIcon>
#include <QFile>

SysTray::SysTray(QObject *parent)
    : QObject(parent)
    , m_isEnabled(true)
    , m_isActive(false)
{
    createMenu();
    updateIcon();
    
    connect(&m_trayIcon, &QSystemTrayIcon::activated, this, &SysTray::handleActivated);
}

SysTray::~SysTray()
{
}

void SysTray::show()
{
    m_trayIcon.show();
}

void SysTray::setActive(bool active)
{
    if (m_isActive != active) {
        m_isActive = active;
        updateIcon();
    }
}

void SysTray::handleActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger) {
        // Single click - show the menu
        m_trayMenu->popup(m_trayIcon.geometry().center());
    }
}

void SysTray::handleAbout()
{
    QMessageBox::about(nullptr, "About Music Presence",
                      "Music Presence for Linux\n\n"
                      "Shows your Discord friends what music you're listening to.\n"
                      "Version 1.0.0\n\n"
                      "Copyright (c) 2024");
}

void SysTray::handleSettings()
{
    // In a full implementation, this would open a settings dialog
    QMessageBox::information(nullptr, "Settings",
                            "Settings functionality not yet implemented.");
}

void SysTray::handleQuit()
{
    QApplication::quit();
}

void SysTray::handleEnableToggled(bool enabled)
{
    m_isEnabled = enabled;
    updateIcon();
    emit enabledChanged(enabled);
}

void SysTray::updateIcon()
{
    QString iconPath;
    if (!m_isEnabled) {
        iconPath = ":/images/app-icon-inactive.png";
    } else if (m_isActive) {
        iconPath = ":/images/app-icon-active.png";
    } else {
        iconPath = ":/images/app-icon.png";
    }
    
    // Fallback to default icon if resource not found
    if (!QFile::exists(iconPath)) {
        iconPath = ":/images/app-icon.png";
    }
    
    QIcon icon(iconPath);
    m_trayIcon.setIcon(icon);
    
    // Update tooltip
    QString tooltip = "Music Presence";
    if (m_isEnabled) {
        tooltip += m_isActive ? " (Active)" : " (Waiting)";
    } else {
        tooltip += " (Disabled)";
    }
    m_trayIcon.setToolTip(tooltip);
}

void SysTray::createMenu()
{
    m_trayMenu = std::make_unique<QMenu>();
    
    // Enable/disable action
    m_enableAction = m_trayMenu->addAction("Enable");
    m_enableAction->setCheckable(true);
    m_enableAction->setChecked(m_isEnabled);
    connect(m_enableAction, &QAction::toggled, this, &SysTray::handleEnableToggled);
    
    m_trayMenu->addSeparator();
    
    // Settings action
    QAction *settingsAction = m_trayMenu->addAction("Settings");
    connect(settingsAction, &QAction::triggered, this, &SysTray::handleSettings);
    
    // About action
    QAction *aboutAction = m_trayMenu->addAction("About");
    connect(aboutAction, &QAction::triggered, this, &SysTray::handleAbout);
    
    m_trayMenu->addSeparator();
    
    // Quit action
    QAction *quitAction = m_trayMenu->addAction("Quit");
    connect(quitAction, &QAction::triggered, this, &SysTray::handleQuit);
    
    // Set menu on tray icon
    m_trayIcon.setContextMenu(m_trayMenu.get());
} 
