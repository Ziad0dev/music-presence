#pragma once

#include <QObject>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <memory>

/**
 * @brief Class to handle the system tray icon and menu
 */
class SysTray : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Constructor
     */
    explicit SysTray(QObject *parent = nullptr);
    
    /**
     * @brief Destructor
     */
    ~SysTray();
    
    /**
     * @brief Show the system tray icon
     */
    void show();
    
    /**
     * @brief Set whether the application is actively sharing music
     * @param active Whether the application is active
     */
    void setActive(bool active);

signals:
    /**
     * @brief Signal emitted when the application is enabled/disabled
     * @param enabled Whether the application is enabled
     */
    void enabledChanged(bool enabled);

private slots:
    /**
     * @brief Handle system tray icon activation
     * @param reason Reason for activation
     */
    void handleActivated(QSystemTrayIcon::ActivationReason reason);
    
    /**
     * @brief Handle when the "About" action is triggered
     */
    void handleAbout();
    
    /**
     * @brief Handle when the "Settings" action is triggered
     */
    void handleSettings();
    
    /**
     * @brief Handle when the "Quit" action is triggered
     */
    void handleQuit();
    
    /**
     * @brief Handle when the "Enable" action is toggled
     * @param enabled Whether the application is enabled
     */
    void handleEnableToggled(bool enabled);

private:
    /**
     * @brief Update the system tray icon based on the current state
     */
    void updateIcon();
    
    /**
     * @brief Create the system tray menu
     */
    void createMenu();

private:
    QSystemTrayIcon m_trayIcon;
    std::unique_ptr<QMenu> m_trayMenu;
    QAction *m_enableAction;
    bool m_isEnabled;
    bool m_isActive;
}; 