#include "WorkspaceManager.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include <iostream>

namespace
{
    bool createFile(const QString &path, const QByteArray &contents = QByteArrayLiteral("test")) {
        QFile file(path);
        return file.open(QIODevice::WriteOnly) && file.write(contents) == contents.size();
    }
}

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    QTemporaryDir temporary;
    if (!temporary.isValid()) {
        std::cerr << "temporary directory unavailable" << std::endl;
        return 1;
    }

    const QString root = QDir(temporary.path()).filePath(QStringLiteral("workspace"));
    QString error;
    const QString protectedDirectory = WorkspaceManager::createManagedDirectory(root, QStringLiteral("protected"), error);
    const QString staleDirectory = WorkspaceManager::createManagedDirectory(root, QStringLiteral("stale"), error);
    const QString protectedFile = QDir(protectedDirectory).filePath(QStringLiteral("vocals.wav"));
    const QString staleFile = QDir(staleDirectory).filePath(QStringLiteral("slice.wav"));
    const QString unrelatedDirectory = QDir(root).filePath(QStringLiteral("unrelated"));
    QDir().mkpath(unrelatedDirectory);
    const QString unrelatedFile = QDir(unrelatedDirectory).filePath(QStringLiteral("keep.txt"));
    if (protectedDirectory.isEmpty() || staleDirectory.isEmpty() || !createFile(protectedFile) ||
        !createFile(staleFile, QByteArray(1024, 'x')) || !createFile(unrelatedFile)) {
        std::cerr << "workspace fixture creation failed: " << error.toStdString() << std::endl;
        return 1;
    }

    const WorkspaceCleanupResult result =
        WorkspaceManager::clearUnused({root}, QSet<QString>{protectedFile}, error);
    if (!error.isEmpty() || result.filesRemoved != 1 || result.bytesRemoved != 1024 ||
        result.protectedDirectories != 1 || !QFile::exists(protectedFile) || QFile::exists(staleFile) ||
        !QFile::exists(unrelatedFile)) {
        std::cerr << "safe cleanup test failed: error=" << error.toStdString()
                  << ", filesRemoved=" << result.filesRemoved << ", bytesRemoved=" << result.bytesRemoved
                  << ", protectedDirectories=" << result.protectedDirectories
                  << ", protectedFileExists=" << QFile::exists(protectedFile)
                  << ", staleFileExists=" << QFile::exists(staleFile)
                  << ", unrelatedFileExists=" << QFile::exists(unrelatedFile) << std::endl;
        return 1;
    }

    WorkspaceManager::removeManagedArtifacts({protectedFile});
    if (QDir(protectedDirectory).exists() || !QFile::exists(unrelatedFile)) {
        std::cerr << "managed artifact removal test failed" << std::endl;
        return 1;
    }

    std::cout << "GameInfer workspace tests passed" << std::endl;
    return 0;
}
