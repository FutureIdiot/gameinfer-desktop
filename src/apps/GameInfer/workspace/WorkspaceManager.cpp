#include "WorkspaceManager.h"

#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSettings>
#include <QUuid>

#include <algorithm>
#include <utility>

#include "utils/AppPaths.h"

namespace
{
    const QString MarkerName = QStringLiteral(".gameinfer-managed");
    const QByteArray MarkerContents = QByteArrayLiteral("GameInfer managed workspace\n");

    QString normalizedPath(const QString &path) {
        QString result = QDir::fromNativeSeparators(QDir::cleanPath(QFileInfo(path).absoluteFilePath()));
#ifdef Q_OS_WIN
        result = result.toLower();
#endif
        return result;
    }

    bool containsPath(const QString &directory, const QString &path) {
        const QString root = normalizedPath(directory) + QLatin1Char('/');
        return normalizedPath(path).startsWith(root);
    }

    QPair<int, quint64> directoryStats(const QString &directory) {
        int files = 0;
        quint64 bytes = 0;
        QDirIterator iterator(directory, QDir::Files, QDirIterator::Subdirectories);
        while (iterator.hasNext()) {
            const QFileInfo info(iterator.next());
            if (info.fileName() == MarkerName) {
                continue;
            }
            ++files;
            bytes += static_cast<quint64>(std::max<qint64>(0, info.size()));
        }
        return {files, bytes};
    }
}

QString WorkspaceManager::defaultSliceDirectory() {
    return QDir(AppPaths::workspaceDirectory()).filePath(QStringLiteral("slice"));
}

QString WorkspaceManager::defaultSeparatorDirectory() {
    return QDir(AppPaths::workspaceDirectory()).filePath(QStringLiteral("separators"));
}

QString WorkspaceManager::sliceDirectory(const QSettings *settings) {
    return settings->value(QStringLiteral("Workspace/sliceDirectory"), defaultSliceDirectory()).toString();
}

QString WorkspaceManager::separatorDirectory(const QSettings *settings) {
    return settings->value(QStringLiteral("Workspace/separatorDirectory"), defaultSeparatorDirectory()).toString();
}

bool WorkspaceManager::keepIntermediates(const QSettings *settings) {
    return settings->value(QStringLiteral("Workspace/keepIntermediates"), false).toBool();
}

QString WorkspaceManager::createManagedDirectory(const QString &root, const QString &label, QString &error) {
    const QString trimmedRoot = root.trimmed();
    if (trimmedRoot.isEmpty()) {
        error = QObject::tr("The workspace directory is empty.");
        return {};
    }
    if (!QDir().mkpath(trimmedRoot)) {
        error = QObject::tr("Failed to create the workspace directory: %1").arg(trimmedRoot);
        return {};
    }

    QString safeLabel = label;
    safeLabel.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9._-]+")), QStringLiteral("_"));
    safeLabel = safeLabel.left(48);
    if (safeLabel.isEmpty()) {
        safeLabel = QStringLiteral("audio");
    }
    const QString uniqueName = QStringLiteral("%1_%2_%3")
                                   .arg(safeLabel, QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMdd-HHmmss")),
                                        QUuid::createUuid().toString(QUuid::Id128).left(8));
    const QString directory = QDir(trimmedRoot).filePath(uniqueName);
    if (!QDir().mkpath(directory)) {
        error = QObject::tr("Failed to create the task workspace: %1").arg(directory);
        return {};
    }

    QFile marker(QDir(directory).filePath(MarkerName));
    if (!marker.open(QIODevice::WriteOnly | QIODevice::Truncate) || marker.write(MarkerContents) != MarkerContents.size()) {
        QDir(directory).removeRecursively();
        error = QObject::tr("Failed to initialize the task workspace: %1").arg(directory);
        return {};
    }
    return directory;
}

QString WorkspaceManager::managedContainer(const QString &path) {
    QFileInfo info(path);
    QDir current(info.isDir() ? info.absoluteFilePath() : info.absolutePath());
    for (int depth = 0; depth < 8 && current.exists(); ++depth) {
        QFile marker(current.filePath(MarkerName));
        if (marker.open(QIODevice::ReadOnly) && marker.readAll() == MarkerContents) {
            return current.absolutePath();
        }
        if (!current.cdUp()) {
            break;
        }
    }
    return {};
}

bool WorkspaceManager::isManagedArtifact(const QString &path) { return !managedContainer(path).isEmpty(); }

void WorkspaceManager::removeManagedArtifacts(const QStringList &paths) {
    QSet<QString> containers;
    for (const QString &path : paths) {
        const QString container = managedContainer(path);
        if (container.isEmpty() || !containsPath(container, path)) {
            continue;
        }
        QFile::remove(path);
        containers.insert(container);
    }

    for (const QString &container : std::as_const(containers)) {
        const QStringList remaining =
            QDir(container).entryList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
        if (remaining == QStringList{MarkerName}) {
            QDir(container).removeRecursively();
        }
    }
}

void WorkspaceManager::removeManagedDirectory(const QString &directory) {
    QFile marker(QDir(directory).filePath(MarkerName));
    const bool isManaged = marker.open(QIODevice::ReadOnly) && marker.readAll() == MarkerContents;
    marker.close();
    if (isManaged) {
        QDir(directory).removeRecursively();
    }
}

WorkspaceCleanupResult WorkspaceManager::clearUnused(const QStringList &roots,
                                                      const QSet<QString> &protectedPaths,
                                                      QString &error) {
    WorkspaceCleanupResult result;
    QSet<QString> normalizedProtected;
    for (const QString &path : protectedPaths) {
        if (!path.trimmed().isEmpty()) {
            normalizedProtected.insert(normalizedPath(path));
        }
    }

    for (const QString &root : roots) {
        QDir rootDirectory(root.trimmed());
        if (!rootDirectory.exists()) {
            continue;
        }
        const QFileInfoList children = rootDirectory.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QFileInfo &child : children) {
            QFile marker(QDir(child.absoluteFilePath()).filePath(MarkerName));
            const bool isManaged = marker.open(QIODevice::ReadOnly) && marker.readAll() == MarkerContents;
            marker.close();
            if (!isManaged) {
                continue;
            }

            bool isProtected = false;
            for (const QString &path : std::as_const(normalizedProtected)) {
                if (containsPath(child.absoluteFilePath(), path)) {
                    isProtected = true;
                    break;
                }
            }
            if (isProtected) {
                ++result.protectedDirectories;
                continue;
            }

            const auto [files, bytes] = directoryStats(child.absoluteFilePath());
            if (!QDir(child.absoluteFilePath()).removeRecursively()) {
                error = QObject::tr("Failed to remove a workspace directory: %1").arg(child.absoluteFilePath());
                return result;
            }
            result.filesRemoved += files;
            result.bytesRemoved += bytes;
        }
    }
    return result;
}
