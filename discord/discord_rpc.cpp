#include "discord_rpc.h"
#include <QDebug>
#include <QDateTime>
#include <QCoreApplication>
#include <QUrl>

#ifdef USE_DISCORD_RPC
#include <discord_rpc.h>
#endif

// IMPORTANT: You must replace this with your actual Discord application ID
// To create your own Discord application:
// 1. Go to https://discord.com/developers/applications
// 2. Click "New Application" and give it a name (e.g., "Music Presence")
// 3. Copy the "Application ID" from the General Information page
// 4. Paste it here replacing the placeholder below
// Example: const QString DEFAULT_APP_ID = "977654321098765432";
const QString DEFAULT_APP_ID = "1367992071406878770"; 