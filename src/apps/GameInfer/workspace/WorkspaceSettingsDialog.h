#pragma once

#include <QDialog>
#include <QSet>
#include <QString>

class QCheckBox;
class QLabel;
class QLineEdit;
class QSettings;

class WorkspaceSettingsDialog final : public QDialog {
    Q_OBJECT

public:
    WorkspaceSettingsDialog(QSettings *settings, QSet<QString> protectedPaths, QWidget *parent = nullptr);

private:
    void browseForDirectory(QLineEdit *edit, const QString &title);
    void clearOldWorkspaceFiles();
    void saveSettings();

    QSettings *m_settings;
    QSet<QString> m_protectedPaths;
    QLineEdit *m_sliceDirectoryEdit;
    QLineEdit *m_separatorDirectoryEdit;
    QCheckBox *m_keepIntermediatesCheck;
    QLabel *m_cleanupResultLabel;
};
