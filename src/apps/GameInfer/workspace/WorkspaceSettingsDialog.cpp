#include "WorkspaceSettingsDialog.h"

#include "WorkspaceManager.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QStringList>
#include <QVBoxLayout>

#include <utility>

namespace
{
    QString formattedBytes(const quint64 bytes) {
        if (bytes >= 1024ULL * 1024ULL * 1024ULL) {
            return QStringLiteral("%1 GB").arg(static_cast<double>(bytes) / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
        }
        if (bytes >= 1024ULL * 1024ULL) {
            return QStringLiteral("%1 MB").arg(static_cast<double>(bytes) / (1024.0 * 1024.0), 0, 'f', 1);
        }
        if (bytes >= 1024ULL) {
            return QStringLiteral("%1 KB").arg(static_cast<double>(bytes) / 1024.0, 0, 'f', 1);
        }
        return QStringLiteral("%1 B").arg(bytes);
    }
}

WorkspaceSettingsDialog::WorkspaceSettingsDialog(QSettings *settings, QSet<QString> protectedPaths, QWidget *parent)
    : QDialog(parent), m_settings(settings), m_protectedPaths(std::move(protectedPaths)) {
    setWindowTitle(tr("Workspace paths"));
    resize(720, 260);

    auto *mainLayout = new QVBoxLayout(this);
    auto *description = new QLabel(
        tr("GameInfer stores temporary separated and manually sliced audio here. Successful tasks are cleaned "
           "automatically unless intermediate files are kept."),
        this);
    description->setWordWrap(true);
    mainLayout->addWidget(description);

    auto *form = new QFormLayout();
    const auto addDirectoryRow = [this, form](const QString &label, const QString &value,
                                               const QString &browseTitle, QLineEdit *&edit) {
        auto *row = new QWidget(this);
        auto *layout = new QHBoxLayout(row);
        layout->setContentsMargins(0, 0, 0, 0);
        edit = new QLineEdit(value, row);
        auto *browse = new QPushButton(tr("Browse..."), row);
        layout->addWidget(edit, 1);
        layout->addWidget(browse);
        form->addRow(label, row);
        connect(browse, &QPushButton::clicked, this, [this, edit, browseTitle] {
            browseForDirectory(edit, browseTitle);
        });
    };
    addDirectoryRow(tr("Manual slices:"), WorkspaceManager::sliceDirectory(settings),
                    tr("Select the manual slice workspace"), m_sliceDirectoryEdit);
    addDirectoryRow(tr("Separated audio:"), WorkspaceManager::separatorDirectory(settings),
                    tr("Select the separator workspace"), m_separatorDirectoryEdit);
    mainLayout->addLayout(form);

    m_keepIntermediatesCheck = new QCheckBox(tr("Keep intermediate files after successful tasks"), this);
    m_keepIntermediatesCheck->setChecked(WorkspaceManager::keepIntermediates(settings));
    mainLayout->addWidget(m_keepIntermediatesCheck);

    auto *cleanupLayout = new QHBoxLayout();
    auto *cleanupButton = new QPushButton(tr("Clear old workspace files"), this);
    m_cleanupResultLabel = new QLabel(this);
    cleanupLayout->addWidget(cleanupButton);
    cleanupLayout->addWidget(m_cleanupResultLabel, 1);
    mainLayout->addLayout(cleanupLayout);
    connect(cleanupButton, &QPushButton::clicked, this, &WorkspaceSettingsDialog::clearOldWorkspaceFiles);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &WorkspaceSettingsDialog::saveSettings);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void WorkspaceSettingsDialog::browseForDirectory(QLineEdit *edit, const QString &title) {
    const QString selected = QFileDialog::getExistingDirectory(this, title, edit->text());
    if (!selected.isEmpty()) {
        edit->setText(selected);
    }
}

void WorkspaceSettingsDialog::clearOldWorkspaceFiles() {
    const auto answer = QMessageBox::question(
        this, tr("Clear workspace"),
        tr("Delete unused GameInfer workspace files? Files required by current queue tasks will be kept."),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes) {
        return;
    }

    QString error;
    QStringList roots = m_settings->value(QStringLiteral("Workspace/knownDirectories")).toStringList();
    roots << WorkspaceManager::defaultSliceDirectory() << WorkspaceManager::defaultSeparatorDirectory()
          << WorkspaceManager::sliceDirectory(m_settings) << WorkspaceManager::separatorDirectory(m_settings)
          << m_sliceDirectoryEdit->text() << m_separatorDirectoryEdit->text();
    roots.removeDuplicates();
    const WorkspaceCleanupResult result = WorkspaceManager::clearUnused(roots, m_protectedPaths, error);
    if (!error.isEmpty()) {
        QMessageBox::critical(this, tr("Clear workspace"), error);
        return;
    }
    m_cleanupResultLabel->setText(
        tr("Removed %1 files (%2); kept %3 active task folders.")
            .arg(result.filesRemoved)
            .arg(formattedBytes(result.bytesRemoved))
            .arg(result.protectedDirectories));
}

void WorkspaceSettingsDialog::saveSettings() {
    if (m_sliceDirectoryEdit->text().trimmed().isEmpty() || m_separatorDirectoryEdit->text().trimmed().isEmpty()) {
        QMessageBox::critical(this, tr("Workspace paths"), tr("Workspace directories cannot be empty."));
        return;
    }
    QStringList knownDirectories = m_settings->value(QStringLiteral("Workspace/knownDirectories")).toStringList();
    knownDirectories << WorkspaceManager::defaultSliceDirectory() << WorkspaceManager::defaultSeparatorDirectory()
                     << WorkspaceManager::sliceDirectory(m_settings) << WorkspaceManager::separatorDirectory(m_settings)
                     << m_sliceDirectoryEdit->text().trimmed() << m_separatorDirectoryEdit->text().trimmed();
    knownDirectories.removeDuplicates();
    m_settings->setValue(QStringLiteral("Workspace/knownDirectories"), knownDirectories);
    m_settings->setValue(QStringLiteral("Workspace/sliceDirectory"), m_sliceDirectoryEdit->text().trimmed());
    m_settings->setValue(QStringLiteral("Workspace/separatorDirectory"),
                         m_separatorDirectoryEdit->text().trimmed());
    m_settings->setValue(QStringLiteral("Workspace/keepIntermediates"), m_keepIntermediatesCheck->isChecked());
    accept();
}
