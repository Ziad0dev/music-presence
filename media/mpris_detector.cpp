QVariantMap MprisDetector::getMediaMetadata(QDBusInterface *interface) const
{
    QVariantMap metadata;
    
    try {
        QVariant metadataVariant = interface->property("Metadata");
        if (!metadataVariant.isValid()) {
            qWarning() << "Invalid metadata property from" << interface->service();
            return metadata;
        }
        
        // Handle the case where we get a QDBusArgument
        if (metadataVariant.canConvert<QDBusArgument>()) {
            QDBusArgument argument = metadataVariant.value<QDBusArgument>();
            
            // Check if the argument is valid for dictionary extraction
            if (argument.currentType() == QDBusArgument::MapType) {
                argument >> metadata;
            } else {
                qWarning() << "Unexpected DBus argument type:" << argument.currentType();
            }
        } else {
            qWarning() << "Metadata is not a QDBusArgument as expected";
        }
    } catch (const std::exception &e) {
        qWarning() << "Exception while extracting metadata:" << e.what();
    } catch (...) {
        qWarning() << "Unknown exception while extracting metadata";
    }
    
    // Extract and validate artist - handle various formats
    QString artist;
    if (metadata.contains("xesam:artist")) {
        try {
            QVariant artistVariant = metadata["xesam:artist"];
            if (artistVariant.type() == QVariant::StringList) {
                artist = artistVariant.toStringList().join(", ");
            } else if (artistVariant.type() == QVariant::String) {
                artist = artistVariant.toString();
            } else if (artistVariant.canConvert<QString>()) {
                // Try to convert to string if possible
                artist = artistVariant.toString();
            } else {
                qWarning() << "Unusual artist data type:" << artistVariant.typeName() 
                           << "- using default";
            }
        } catch (...) {
            qWarning() << "Exception processing artist data - using default";
        }
    }
    
    return metadata;
}

void MprisDetector::handlePropertiesChanged(const QDBusMessage &message)
{
    try {
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
        QVariantMap metadata;
        
        try {
            metadata = getMediaMetadata(pInterface);
        } catch (const std::exception &e) {
            qWarning() << "Exception getting metadata from" << playerName << ":" << e.what();
            return;
        } catch (...) {
            qWarning() << "Unknown exception getting metadata from" << playerName;
            return;
        }
        
        // Check if playing
        bool isPlaying = false;
        try {
            QVariant playbackStatus = pInterface->property("PlaybackStatus");
            isPlaying = (playbackStatus.toString() == "Playing");
        } catch (...) {
            qWarning() << "Exception getting playback status - assuming not playing";
            isPlaying = false;
        }
        
        // Get metadata
        QString title = sanitizeText(metadata["xesam:title"].toString().trimmed());
        
        // Extract and validate artist - handle various formats
        QString artist;
        if (metadata.contains("xesam:artist")) {
            try {
                QVariant artistVariant = metadata["xesam:artist"];
                if (artistVariant.type() == QVariant::StringList) {
                    artist = artistVariant.toStringList().join(", ");
                } else if (artistVariant.type() == QVariant::String) {
                    artist = artistVariant.toString();
                } else if (artistVariant.canConvert<QString>()) {
                    // Try to convert to string if possible
                    artist = artistVariant.toString();
                } else {
                    qWarning() << "Unusual artist data type:" << artistVariant.typeName() 
                               << "for" << playerName << "- using default";
                }
            } catch (...) {
                qWarning() << "Exception processing artist data from" << playerName << "- using default";
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
        QString coverArtPath;
        try {
            coverArtPath = saveCoverArt(metadata);
        } catch (...) {
            qWarning() << "Exception saving cover art for" << playerName;
            coverArtPath = QString();
        }
        
        // Emit the signal
        emit mediaPlayerUpdated(playerName, title, artist, album, isPlaying, coverArtPath);
    } catch (const std::exception &e) {
        qWarning() << "Exception in handlePropertiesChanged:" << e.what();
    } catch (...) {
        qWarning() << "Unknown exception in handlePropertiesChanged";
    }
}

void MprisDetector::connectToMediaPlayer(const QString &serviceName)
{
    try {
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
        QVariantMap metadata;
        try {
            metadata = getMediaMetadata(interface);
        } catch (const std::exception &e) {
            qWarning() << "Exception getting initial metadata from" << playerName << ":" << e.what();
            return;
        } catch (...) {
            qWarning() << "Unknown exception getting initial metadata from" << playerName;
            return;
        }
        
        // Check if playing
        bool isPlaying = false;
        try {
            QVariant playbackStatus = interface->property("PlaybackStatus");
            isPlaying = (playbackStatus.toString() == "Playing");
        } catch (...) {
            qWarning() << "Exception getting initial playback status - assuming not playing";
            isPlaying = false;
        }
        
        // Get metadata
        QString title = sanitizeText(metadata["xesam:title"].toString().trimmed());
        
        // Extract and validate artist - handle various formats
        QString artist;
        if (metadata.contains("xesam:artist")) {
            try {
                QVariant artistVariant = metadata["xesam:artist"];
                if (artistVariant.type() == QVariant::StringList) {
                    artist = artistVariant.toStringList().join(", ");
                } else if (artistVariant.type() == QVariant::String) {
                    artist = artistVariant.toString();
                } else if (artistVariant.canConvert<QString>()) {
                    // Try to convert to string if possible
                    artist = artistVariant.toString();
                } else {
                    qWarning() << "Unusual artist data type:" << artistVariant.typeName() 
                               << "for" << playerName << "- using default";
                }
            } catch (...) {
                qWarning() << "Exception processing initial artist data - using default";
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
        QString coverArtPath;
        try {
            coverArtPath = saveCoverArt(metadata);
        } catch (...) {
            qWarning() << "Exception saving initial cover art for" << playerName;
            coverArtPath = QString();
        }
        
        // Emit the signal
        emit mediaPlayerUpdated(playerName, title, artist, album, isPlaying, coverArtPath);
    } catch (const std::exception &e) {
        qWarning() << "Exception in connectToMediaPlayer:" << e.what();
    } catch (...) {
        qWarning() << "Unknown exception in connectToMediaPlayer";
    }
} 