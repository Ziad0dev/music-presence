#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>
#include "application.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Music Presence");
    app.setApplicationVersion("1.0.0");
    app.setQuitOnLastWindowClosed(false);
    
    QCommandLineParser parser;
    parser.setApplicationDescription("Discord Music Presence for Linux");
    parser.addHelpOption();
    parser.addVersionOption();
    
    QCommandLineOption startMinimizedOption({"m", "minimized"}, "Start minimized to tray");
    parser.addOption(startMinimizedOption);
    
    parser.process(app);
    
    Application musicPresence(parser.isSet(startMinimizedOption));
    
    qDebug() << "Music Presence started";
    
    return app.exec();
} 