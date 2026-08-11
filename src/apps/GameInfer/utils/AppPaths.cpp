#include "AppPaths.h"

#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>

namespace AppPaths
{
    QString bundledDataDirectory() {
#ifdef Q_OS_MACOS
        return QDir::cleanPath(
            QDir(QCoreApplication::applicationDirPath()).absoluteFilePath(QStringLiteral("../Resources")));
#else
        return QCoreApplication::applicationDirPath();
#endif
    }

    QString configDirectory() {
#ifdef Q_OS_MACOS
        return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
#else
        return QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("config"));
#endif
    }

    QString workspaceDirectory() {
#ifdef Q_OS_MACOS
        return QDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation))
            .filePath(QStringLiteral("workspace"));
#else
        return QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("workspace"));
#endif
    }

    QString separatorModelDirectory() {
#ifdef Q_OS_MACOS
        return QDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation))
            .filePath(QStringLiteral("model/separator"));
#else
        return QDir(bundledDataDirectory()).filePath(QStringLiteral("model/separator"));
#endif
    }
}
