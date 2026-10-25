/*
    SPDX-FileCopyrightText: 2019 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <latte_debug.h>
#include "factory.h"

// local
#include "../layouts/importer.h"

// Qt
#include <QDebug>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QMessageBox>
#include <QProcess>
#include <QSharedPointer>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTimer>
#include <QLatin1String>
#include <QUrl>

// KDE
#include <KDirWatch>
#include <KLocalizedString>
#include <KMessageBox>
#include <KNotification>
#include <KPluginMetaData>
#include <KArchive/KTar>
#include <KArchive/KZip>
#include <KArchive/KArchiveEntry>
#include <KArchive/KArchiveDirectory>

namespace Latte {
namespace Indicator {

namespace {
KPluginMetaData readIndicatorMetaData(const QString &metadataFile)
{
    if (metadataFile.endsWith(QLatin1String(".json"), Qt::CaseInsensitive)) {
        return KPluginMetaData::fromJsonFile(metadataFile);
    }

    return KPluginMetaData(metadataFile);
}

QString kPackageToolExecutable()
{
    const QString kde6Tool = QStandardPaths::findExecutable(QStringLiteral("kpackagetool6"));

    if (!kde6Tool.isEmpty()) {
        return kde6Tool;
    }

    return QStandardPaths::findExecutable(QStringLiteral("kpackagetool5"));
}
}

Factory::Factory(QObject *parent)
    : QObject(parent)
{
    m_parentWidget = new QWidget();

    m_mainPaths = Latte::Layouts::Importer::standardPaths();

    for (int i = 0; i < m_mainPaths.count(); ++i) {
        m_mainPaths[i] = m_mainPaths[i] + QLatin1String("/latte/indicators");
        discoverNewIndicators(m_mainPaths[i]);
    }

    //! track paths for changes
    for (const auto &dir : m_mainPaths) {
        KDirWatch::self()->addDir(dir);
    }

    connect(KDirWatch::self(), &KDirWatch::dirty, this, [ & ](const QString & path) {
        if (m_indicatorsPaths.contains(path)) {
            //! indicator updated
            reload(path);
        } else if (m_mainPaths.contains(path)) {
            //! consider indicator addition
            discoverNewIndicators(path);
        }
    });

    connect(KDirWatch::self(), &KDirWatch::deleted, this, [ & ](const QString & path) {
        if (m_indicatorsPaths.contains(path)) {
            //! indicator removed
            removeIndicatorRecords(path);
        }
    });

    qCDebug(latteIndicator) << m_plugins[QStringLiteral("org.kde.latte.default")].name();
}

Factory::~Factory()
{
    m_parentWidget->deleteLater();
}

bool Factory::pluginExists(QString id) const
{
    return m_plugins.contains(id);
}

int Factory::customPluginsCount()
{
    return m_customPluginIds.count();
}

QStringList Factory::customPluginIds()
{
    return m_customPluginIds;
}

QStringList Factory::customPluginNames()
{
    return m_customPluginNames;
}

QStringList Factory::customLocalPluginIds()
{
    return m_customLocalPluginIds;
}

KPluginMetaData Factory::metadata(QString pluginId)
{
    if (m_plugins.contains(pluginId)) {
        return m_plugins[pluginId];
    }

    return KPluginMetaData();
}

void Factory::reload(const QString &indicatorPath)
{
    QString pluginChangedId;

    if (!indicatorPath.isEmpty() && indicatorPath != QStringLiteral(".") && indicatorPath != QStringLiteral("..")) {
        QString metadataFile = metadataFileAbsolutePath(indicatorPath);

        if (QFileInfo(metadataFile).exists()) {
            KPluginMetaData metadata = readIndicatorMetaData(metadataFile);

            if (metadataAreValid(metadata)) {
                pluginChangedId = metadata.pluginId();
                QString uiFile = indicatorPath + QLatin1String("/package/") + metadata.value(QStringLiteral("X-Latte-MainScript"));

                // User-local installs (under $HOME) always take precedence over system-wide
                // installs so a development override at ~/.local/share/latte/indicators wins
                // even when /usr/share/latte/indicators ships the same plugin id.
                const bool isLocal = indicatorPath.startsWith(QDir::homePath());
                const bool override = isLocal || !m_plugins.contains(metadata.pluginId());

                if (override) {
                    m_plugins[metadata.pluginId()] = metadata;
                }

                if (QFileInfo(uiFile).exists()
                    && (override || !m_pluginUiPaths.contains(metadata.pluginId()))) {
                    m_pluginUiPaths[metadata.pluginId()] = QFileInfo(uiFile).absolutePath();
                }

                if ((metadata.pluginId() != QStringLiteral("org.kde.latte.default"))
                    && (metadata.pluginId() != QStringLiteral("org.kde.latte.plasma"))
                    && (metadata.pluginId() != QStringLiteral("org.kde.latte.plasmatabstyle"))) {

                    //! find correct alphabetical position
                    int newPos = -1;

                    if (!m_customPluginIds.contains(metadata.pluginId())) {
                        for (int i = 0; i < m_customPluginNames.count(); ++i) {
                            if (QString::compare(metadata.name(), m_customPluginNames[i], Qt::CaseInsensitive) <= 0) {
                                newPos = i;
                                break;
                            }
                        }
                    }

                    if (!m_customPluginIds.contains(metadata.pluginId())) {
                        if (newPos == -1) {
                            m_customPluginIds << metadata.pluginId();
                        } else {
                            m_customPluginIds.insert(newPos, metadata.pluginId());
                        }
                    }

                    if (!m_customPluginNames.contains(metadata.name())) {
                        if (newPos == -1) {
                            m_customPluginNames << metadata.name();
                        } else {
                            m_customPluginNames.insert(newPos, metadata.name());
                        }
                    }
                }

                if (indicatorPath.startsWith(QDir::homePath())) {
                    m_customLocalPluginIds << metadata.pluginId();
                }
            }

            qCDebug(latteIndicator) << " Indicator Package Loaded ::: " << metadata.name() << " [" << metadata.pluginId() << "]" << " - [" << indicatorPath << "]";

            /*qCDebug(latteIndicator) << " Indicator value ::: " << metadata.pluginId();
                            qCDebug(latteIndicator) << " Indicator value ::: " << metadata.fileName();
                            qCDebug(latteIndicator) << " Indicator value ::: " << metadata.value(QStringLiteral("X-Latte-MainScript"));
                            qCDebug(latteIndicator) << " Indicator value ::: " << metadata.value("X-Latte-ConfigUi");
                            qCDebug(latteIndicator) << " Indicator value ::: " << metadata.value("X-Latte-ConfigXml");*/
        }
    }

    if (!pluginChangedId.isEmpty()) {
        Q_EMIT indicatorChanged(pluginChangedId);
    }
}

void Factory::discoverNewIndicators(const QString &main)
{
    if (!m_mainPaths.contains(main)) {
        return;
    }

    QDirIterator indicatorsDirs(main, QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot, QDirIterator::NoIteratorFlags);

    while (indicatorsDirs.hasNext()) {
        indicatorsDirs.next();
        QString iPath = indicatorsDirs.filePath();

        if (!m_indicatorsPaths.contains(iPath)) {
            m_indicatorsPaths << iPath;
            KDirWatch::self()->addDir(iPath);
            reload(iPath);
        }
    }
}

void Factory::removeIndicatorRecords(const QString &path)
{
    if (m_indicatorsPaths.contains(path)) {
        QString pluginId =  path.section(QLatin1Char('/'), -1);
        m_plugins.remove(pluginId);
        m_pluginUiPaths.remove(pluginId);

        const int pos = m_customPluginIds.indexOf(pluginId);

        if (pos >= 0) {
            m_customPluginIds.removeAt(pos);

            if (pos < m_customPluginNames.size()) {
                m_customPluginNames.removeAt(pos);
            }
        } else {
            qWarning() << "Indicator removal list mismatch, plugin id not found in custom ids:" << pluginId;
        }

        m_customLocalPluginIds.removeAll(pluginId);

        m_indicatorsPaths.removeAll(path);

        KDirWatch::self()->removeDir(path);

        //! delay informing the removal in case it is just an update
        QTimer::singleShot(1000, this, [this, pluginId]() {
            Q_EMIT indicatorRemoved(pluginId);
        });
    }
}

bool Factory::isCustomType(const QString &id) const
{
    return ((id != QStringLiteral("org.kde.latte.default")) && (id != QStringLiteral("org.kde.latte.plasma")) && (id != QStringLiteral("org.kde.latte.plasmatabstyle")));
}

bool Factory::metadataAreValid(KPluginMetaData &metadata)
{
    return metadata.isValid()
           && metadata.category() == QLatin1String("Latte Indicator")
           && !metadata.value(QStringLiteral("X-Latte-MainScript")).isEmpty();
}

bool Factory::metadataAreValid(QString &file)
{
    if (QFileInfo(file).exists()) {
        KPluginMetaData metadata = readIndicatorMetaData(file);
        return metadata.isValid();
    }

    return false;
}

QString Factory::uiPath(QString pluginName) const
{
    if (!m_pluginUiPaths.contains(pluginName)) {
        return QString();
    }

    return m_pluginUiPaths[pluginName];
}

QString Factory::metadataFileAbsolutePath(const QString &directoryPath)
{
    QString metadataFile = directoryPath + QLatin1String("/metadata.json");

    if (QFileInfo(metadataFile).exists()) {
        return metadataFile;
    }

    metadataFile = directoryPath + QLatin1String("/metadata.desktop");

    if (QFileInfo(metadataFile).exists()) {
        return metadataFile;
    }

    return QString();
}

Latte::ImportExport::State Factory::importIndicatorFile(QString compressedFile)
{
    auto showNotificationError = []() {
        auto notification = new KNotification(QStringLiteral("import-fail"), KNotification::CloseOnTimeout);
        notification->setText(i18n("Failed to import indicator"));
        notification->sendEvent();
    };

    auto showNotificationSucceed = [](QString name, bool updated) {
        auto notification = new KNotification(QStringLiteral("import-done"), KNotification::CloseOnTimeout);
        notification->setText(updated ? i18nc("indicator_name, imported updated", "%1 indicator updated successfully", name) :
                              i18nc("indicator_name, imported success", "%1 indicator installed successfully", name));
        notification->sendEvent();
    };

    QTemporaryDir archiveTempDir;
    {
        // RAII scope for archive extraction — KArchive is deleted once the
        // temporary directory has been populated.
        KArchive *archive = nullptr;

        KZip *zipArchive = new KZip(compressedFile);
        [[maybe_unused]] bool zipOpened = zipArchive->open(QIODevice::ReadOnly);

        if (!zipArchive->isOpen()) {
            delete zipArchive;

            KTar *tarArchive = new KTar(compressedFile, QStringLiteral("application/x-tar"));
            [[maybe_unused]] bool tarOpened = tarArchive->open(QIODevice::ReadOnly);

            if (!tarArchive->isOpen()) {
                delete tarArchive;
                showNotificationError();
                return Latte::ImportExport::FailedState;
            }

            archive = tarArchive;
        } else {
            archive = zipArchive;
        }

        archive->directory()->copyTo(archiveTempDir.path());
        delete archive;
    }

    //metadata file
    QString packagePath = archiveTempDir.path();
    QString metadataFile = metadataFileAbsolutePath(archiveTempDir.path());

    if (!QFileInfo(metadataFile).exists()) {
        QDirIterator iter(archiveTempDir.path(), QDir::Dirs | QDir::NoDotAndDotDot);

        while (iter.hasNext()) {
            QString currentPath = iter.next();

            QString tempMetadata = metadataFileAbsolutePath(currentPath);

            if (QFileInfo(tempMetadata).exists()) {
                metadataFile = tempMetadata;
                packagePath = currentPath;
            }
        }
    }

    KPluginMetaData metadata = readIndicatorMetaData(metadataFile);

    if (metadataAreValid(metadata)) {
        QStringList standardPaths = Latte::Layouts::Importer::standardPaths();
        QString installPath = standardPaths[0] + QLatin1String("/latte/indicators/") + metadata.pluginId();

        bool updated{QDir(installPath).exists()};

        if (QDir(installPath).exists()) {
            QDir(installPath).removeRecursively();
        }

        if (!QFile::rename(packagePath, installPath)) {
            showNotificationError();
            return Latte::ImportExport::FailedState;
        }

        showNotificationSucceed(metadata.name(), updated);
        return updated ? Latte::ImportExport::UpdatedState : Latte::ImportExport::InstalledState;
    }

    showNotificationError();
    return Latte::ImportExport::FailedState;
}

void Factory::removeIndicator(QString id)
{
    if (m_plugins.contains(id)) {
        QString pluginName = m_plugins[id].name();

        QDialog* dialog = new QDialog(nullptr);
        dialog->setWindowTitle(i18n("Remove Indicator Confirmation"));
        dialog->setObjectName(QStringLiteral("warning"));
        dialog->setAttribute(Qt::WA_DeleteOnClose);

        auto buttonbox = new QDialogButtonBox(QDialogButtonBox::Yes | QDialogButtonBox::No);

        KMessageBox::createKMessageBox(dialog,
                                       buttonbox,
                                       QMessageBox::Question,
                                       i18n("Do you want to remove completely <b>%1</b> indicator from your system?", pluginName),
                                       QStringList(),
                                       QString(),
                                       nullptr,
                                       KMessageBox::Options{KMessageBox::NoExec},
                                       QString());

        connect(buttonbox, &QDialogButtonBox::accepted, this, [this, id, pluginName]() {
            auto showRemovedSucceed = [](QString name) {
                auto notification = new KNotification(QStringLiteral("remove-done"), KNotification::CloseOnTimeout);
                notification->setText(i18nc("indicator_name, removed success", "<b>%1</b> indicator removed successfully", name));
                notification->sendEvent();
            };

            auto showRemovedError = [](QString name) {
                auto notification = new KNotification(QStringLiteral("remove-failed"), KNotification::CloseOnTimeout);
                notification->setText(i18nc("indicator_name, removed failure", "Failed to remove <b>%1</b> indicator", name));
                notification->sendEvent();
            };

            qCDebug(latteIndicator) << "Trying to remove indicator :: " << id;

            const QString kpackagetool = kPackageToolExecutable();

            if (kpackagetool.isEmpty()) {
                qWarning() << "Could not find kpackagetool6 or kpackagetool5 executable";
                showRemovedError(pluginName);
                return;
            }

            auto *process = new QProcess(this);
            auto handled = QSharedPointer<bool>::create(false);

            connect(process, &QProcess::errorOccurred, this,
                    [process, handled, pluginName, showRemovedError](QProcess::ProcessError error) {
                if (*handled) {
                    return;
                }

                *handled = true;
                qWarning() << "Failed to start indicator removal process" << pluginName
                           << "error:" << error
                           << "stderr:" << process->readAllStandardError();
                showRemovedError(pluginName);
                process->deleteLater();
            });

            connect(process, &QProcess::finished, this,
                    [process, handled, id, pluginName, showRemovedSucceed, showRemovedError](int exitCode,
                                                                                               QProcess::ExitStatus exitStatus) {
                if (*handled) {
                    return;
                }

                *handled = true;
                if (exitStatus == QProcess::NormalExit && exitCode == 0) {
                    showRemovedSucceed(pluginName);
                } else {
                    qWarning() << "Failed to remove indicator" << id
                               << "exitCode:" << exitCode
                               << "stderr:" << process->readAllStandardError();
                    showRemovedError(pluginName);
                }

                process->deleteLater();
            });

            process->start(kpackagetool,
                           {QStringLiteral("-r"), id, QStringLiteral("-t"), QStringLiteral("Latte/Indicator")});
        });

        dialog->show();
    }
}

void Factory::downloadIndicator()
{
    QDesktopServices::openUrl(QUrl(QStringLiteral("https://store.kde.org/")));
}

}
}
