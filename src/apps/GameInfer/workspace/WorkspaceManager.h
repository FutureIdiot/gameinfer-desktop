#pragma once

#include <QSet>
#include <QString>
#include <QStringList>

class QSettings;

struct WorkspaceCleanupResult {
    int filesRemoved = 0;
    quint64 bytesRemoved = 0;
    int protectedDirectories = 0;
};

class WorkspaceManager final {
public:
    static QString defaultSliceDirectory();
    static QString defaultSeparatorDirectory();
    static QString sliceDirectory(const QSettings *settings);
    static QString separatorDirectory(const QSettings *settings);
    static bool keepIntermediates(const QSettings *settings);

    static QString createManagedDirectory(const QString &root, const QString &label, QString &error);
    static bool isManagedArtifact(const QString &path);
    static void removeManagedArtifacts(const QStringList &paths);
    static void removeManagedDirectory(const QString &directory);
    static WorkspaceCleanupResult clearUnused(const QStringList &roots, const QSet<QString> &protectedPaths,
                                              QString &error);

private:
    static QString managedContainer(const QString &path);
};
