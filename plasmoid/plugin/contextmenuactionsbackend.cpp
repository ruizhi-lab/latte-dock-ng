/*
    SPDX-FileCopyrightText: 2026 Latte Dock Contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "contextmenuactionsbackend.h"

// KDE
#include <KApplicationTrader>
#include <KDesktopFile>
#include <KFilePlacesModel>
#include <KNotificationJobUiDelegate>
#include <KService>
#include <KServiceAction>
#include <KIO/ApplicationLauncherJob>
#include <KIO/Global>
#include <PlasmaActivities/Stats/Cleaning>
#include <PlasmaActivities/Stats/ResultSet>
#include <PlasmaActivities/Stats/Terms>

// Qt
#include <QAction>
#include <QFileInfo>
#include <QIcon>
#include <QMenu>
#include <QStandardPaths>
#include <QUrl>

#include <memory>

namespace Latte
{
namespace Tasks
{

namespace KAStats = KActivities::Stats;
using namespace KAStats;
using namespace KAStats::Terms;

ContextMenuActionsBackend::ContextMenuActionsBackend(QObject *parent)
    : QObject(parent)
{
}

QVariantList ContextMenuActionsBackend::jumpListActions(const QVariant &launcherUrl, QObject *parent)
{
    QVariantList actions;

    if (!parent) {
        return actions;
    }

    const QUrl decodedLauncherUrl = tryDecodeApplicationsUrl(toUrl(launcherUrl));

    KService::Ptr service;

    if (decodedLauncherUrl.isValid() && decodedLauncherUrl.isLocalFile() && KDesktopFile::isDesktopFile(decodedLauncherUrl.toLocalFile())) {
        service = KService::serviceByDesktopPath(decodedLauncherUrl.toLocalFile());
    }

    if (!service) {
        service = serviceFromLauncherUrl(decodedLauncherUrl);
    }

    if (!service) {
        return actions;
    }

    const auto jumpListActions = service->actions();

    for (const KServiceAction &serviceAction : jumpListActions) {
        if (serviceAction.noDisplay()) {
            continue;
        }

        auto *action = new QAction(parent);
        action->setText(serviceAction.text());
        action->setIcon(QIcon::fromTheme(serviceAction.icon()));

        if (serviceAction.isSeparator()) {
            action->setSeparator(true);
        }

        connect(action, &QAction::triggered, this, [serviceAction]() {
            auto *job = new KIO::ApplicationLauncherJob(serviceAction);
            auto *delegate = new KNotificationJobUiDelegate;
            delegate->setAutoErrorHandlingEnabled(true);
            job->setUiDelegate(delegate);
            job->start();
        });

        actions << QVariant::fromValue<QAction *>(action);
    }

    return actions;
}

QVariantList ContextMenuActionsBackend::placesActions(const QVariant &launcherUrl, bool showAllPlaces, QObject *parent)
{
    QVariantList actions;

    if (!parent) {
        return actions;
    }

    const QUrl decodedLauncherUrl = tryDecodeApplicationsUrl(toUrl(launcherUrl));

    if (!decodedLauncherUrl.isValid() || !decodedLauncherUrl.isLocalFile() || !KDesktopFile::isDesktopFile(decodedLauncherUrl.toLocalFile())) {
        return actions;
    }

    if (!applicationCategories(decodedLauncherUrl).contains(QLatin1String("FileManager"))) {
        return actions;
    }

    QString previousGroup;
    QMenu *subMenu = nullptr;

    std::unique_ptr<KFilePlacesModel> placesModel(new KFilePlacesModel());

    for (int i = 0; i < placesModel->rowCount(); ++i) {
        const QModelIndex idx = placesModel->index(i, 0);

        if (placesModel->isHidden(idx)) {
            continue;
        }

        const QString title = idx.data(Qt::DisplayRole).toString();
        const QIcon icon = idx.data(Qt::DecorationRole).value<QIcon>();
        const QUrl url = idx.data(KFilePlacesModel::UrlRole).toUrl();

        auto *placeAction = new QAction(icon, title, parent);

        connect(placeAction, &QAction::triggered, this, [decodedLauncherUrl, url]() {
            KService::Ptr service = KService::serviceByDesktopPath(decodedLauncherUrl.toLocalFile());

            if (!service) {
                return;
            }

            auto *job = new KIO::ApplicationLauncherJob(service);
            auto *delegate = new KNotificationJobUiDelegate;
            delegate->setAutoErrorHandlingEnabled(true);
            job->setUiDelegate(delegate);
            job->setUrls({url});
            job->start();
        });

        const QString groupName = idx.data(KFilePlacesModel::GroupRole).toString();

        if (previousGroup.isEmpty()) {
            previousGroup = groupName;
        }

        if (previousGroup != groupName) {
            auto *subMenuAction = new QAction(groupName, parent);
            subMenu = new QMenu();
            subMenu->setAttribute(Qt::WA_TranslucentBackground);
            connect(parent, &QObject::destroyed, subMenu, &QObject::deleteLater);
            subMenuAction->setMenu(subMenu);
            actions << QVariant::fromValue(subMenuAction);

            previousGroup = groupName;
        }

        if (subMenu) {
            subMenu->addAction(placeAction);
        } else {
            actions << QVariant::fromValue(placeAction);
        }
    }

    if (!showAllPlaces && actions.count() > 7) {
        const int totalActionCount = actions.count();

        while (actions.count() > 5) {
            actions.removeLast();
        }

        auto *moreAction = new QAction(parent);
        moreAction->setIcon(QIcon::fromTheme(QStringLiteral("view-more-symbolic")));
        const int remainingPlaces = totalActionCount - actions.count();
        moreAction->setText(QStringLiteral("%1 more Places…").arg(remainingPlaces));
        connect(moreAction, &QAction::triggered, this, &ContextMenuActionsBackend::showAllPlaces);
        actions << QVariant::fromValue(moreAction);
    }

    return actions;
}

QVariantList ContextMenuActionsBackend::recentDocumentActions(const QVariant &launcherUrl, QObject *parent)
{
    QVariantList actions;

    if (!parent) {
        return actions;
    }

    const QUrl decodedLauncherUrl = tryDecodeApplicationsUrl(toUrl(launcherUrl));

    if (!decodedLauncherUrl.isValid() || !decodedLauncherUrl.isLocalFile() || !KDesktopFile::isDesktopFile(decodedLauncherUrl.toLocalFile())) {
        return actions;
    }

    QString desktopName = decodedLauncherUrl.fileName();
    QString storageId = desktopName;

    if (storageId.endsWith(QLatin1String(".desktop"))) {
        storageId.chop(8);
    }

    auto query = UsedResources | RecentlyUsedFirst | Agent(storageId) | Type::any() | Activity::current();
    ResultSet results(query);

    ResultSet::const_iterator resultIt = results.begin();

    int actionCount = 0;
    bool allFolders = true;
    bool allDownloads = true;
    bool allRemoteWithoutFileName = true;
    const QString downloadsPath = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);

    while (actionCount < 5 && resultIt != results.end()) {
        const QString mimetype = (*resultIt).mimetype();
        const QUrl url = (*resultIt).url();
        ++resultIt;

        if (!url.isValid()) {
            continue;
        }

        allFolders = allFolders && mimetype == QLatin1String("inode/directory");
        allDownloads = allDownloads && url.toLocalFile().startsWith(downloadsPath);
        allRemoteWithoutFileName = allRemoteWithoutFileName && !url.isLocalFile() && url.fileName().isEmpty();

        QString name;

        if (url.isLocalFile() && !url.fileName().isEmpty()) {
            name = url.fileName();
        } else {
            name = url.toDisplayString();
        }

        auto *action = new QAction(parent);
        action->setText(name);
        action->setIcon(QIcon::fromTheme(KIO::iconNameForUrl(url)));
        action->setProperty("agent", storageId);
        action->setProperty("entryPath", decodedLauncherUrl);
        action->setProperty("mimeType", mimetype);
        action->setData(url);
        connect(action, &QAction::triggered, this, &ContextMenuActionsBackend::handleRecentDocumentAction);

        actions << QVariant::fromValue<QAction *>(action);

        ++actionCount;
    }

    if (actionCount > 0) {
        if (allDownloads) {
            actions.prepend(QStringLiteral("Recent Downloads"));
        } else if (allRemoteWithoutFileName) {
            actions.prepend(QStringLiteral("Recent Connections"));
        } else if (allFolders) {
            actions.prepend(QStringLiteral("Recent Places"));
        }

        auto *separatorAction = new QAction(parent);
        separatorAction->setSeparator(true);
        actions << QVariant::fromValue<QAction *>(separatorAction);

        auto *forgetAction = new QAction(parent);

        if (allDownloads) {
            forgetAction->setText(QStringLiteral("Forget Recent Downloads"));
        } else if (allRemoteWithoutFileName) {
            forgetAction->setText(QStringLiteral("Forget Recent Connections"));
        } else if (allFolders) {
            forgetAction->setText(QStringLiteral("Forget Recent Places"));
        } else {
            forgetAction->setText(QStringLiteral("Forget Recent Files"));
        }

        forgetAction->setIcon(QIcon::fromTheme(QStringLiteral("edit-clear-history")));
        forgetAction->setProperty("agent", storageId);
        connect(forgetAction, &QAction::triggered, this, &ContextMenuActionsBackend::handleRecentDocumentAction);

        actions << QVariant::fromValue<QAction *>(forgetAction);
    }

    return actions;
}

void ContextMenuActionsBackend::handleRecentDocumentAction() const
{
    const auto *action = qobject_cast<QAction *>(sender());

    if (!action) {
        return;
    }

    const QString agent = action->property("agent").toString();

    if (agent.isEmpty()) {
        return;
    }

    const QString desktopPath = action->property("entryPath").toUrl().toLocalFile();
    const QUrl url = action->data().toUrl();

    if (desktopPath.isEmpty() || url.isEmpty()) {
        auto query = UsedResources | Agent(agent) | Type::any() | Activity::current();
        KAStats::forgetResources(query);
        return;
    }

    KService::Ptr service = KService::serviceByDesktopPath(desktopPath);

    if (!service) {
        return;
    }

    const QString mimetype = action->property("mimeType").toString();

    if (!mimetype.isEmpty() && mimetype != QLatin1String("application/octet-stream")) {
        if (!service->hasMimeType(mimetype)) {
            service = KApplicationTrader::preferredService(mimetype);

            if (!service) {
                return;
            }
        }
    }

    auto *job = new KIO::ApplicationLauncherJob(service);
    auto *delegate = new KNotificationJobUiDelegate;
    delegate->setAutoErrorHandlingEnabled(true);
    job->setUiDelegate(delegate);
    job->setUrls({url});
    job->start();
}

QStringList ContextMenuActionsBackend::applicationCategories(const QUrl &launcherUrl)
{
    const QUrl desktopEntryUrl = tryDecodeApplicationsUrl(launcherUrl);

    if (!desktopEntryUrl.isValid()) {
        return {};
    }

    KService::Ptr service = serviceFromLauncherUrl(desktopEntryUrl);

    if (!service && desktopEntryUrl.isLocalFile() && KDesktopFile::isDesktopFile(desktopEntryUrl.toLocalFile())) {
        service = KService::serviceByDesktopPath(desktopEntryUrl.toLocalFile());
    }

    if (!service) {
        return {};
    }

    return service->categories();
}

QUrl ContextMenuActionsBackend::toUrl(const QVariant &value)
{
    if (!value.isValid() || value.isNull()) {
        return {};
    }

    if (value.canConvert<QUrl>()) {
        const QUrl url = value.toUrl();
        if (url.isValid()) {
            return url;
        }
    }

    const QString text = value.toString();

    if (text.startsWith(QLatin1Char('/'))) {
        return QUrl::fromLocalFile(text);
    }

    return QUrl(text);
}

QUrl ContextMenuActionsBackend::tryDecodeApplicationsUrl(const QUrl &launcherUrl)
{
    if (!launcherUrl.isValid()) {
        return {};
    }

    if (launcherUrl.scheme() == QLatin1String("applications")) {
        QString menuId = launcherUrl.path();

        while (menuId.startsWith(QLatin1Char('/'))) {
            menuId.remove(0, 1);
        }

        if (menuId.isEmpty()) {
            menuId = launcherUrl.toString(QUrl::RemoveScheme | QUrl::RemoveQuery | QUrl::RemoveFragment);
            while (menuId.startsWith(QLatin1Char('/'))) {
                menuId.remove(0, 1);
            }
        }

        menuId = QUrl::fromPercentEncoding(menuId.toUtf8());

        if (!menuId.isEmpty()) {
            if (KService::Ptr service = KService::serviceByMenuId(menuId)) {
                return QUrl::fromLocalFile(service->entryPath());
            }

            if (KService::Ptr service = KService::serviceByStorageId(menuId)) {
                return QUrl::fromLocalFile(service->entryPath());
            }

            const QString desktopId = menuId.endsWith(QLatin1String(".desktop")) ? menuId : (menuId + QLatin1String(".desktop"));
            const QString desktopPath = QStandardPaths::locate(QStandardPaths::ApplicationsLocation, desktopId);

            if (!desktopPath.isEmpty()) {
                return QUrl::fromLocalFile(desktopPath);
            }
        }
    } else if (launcherUrl.scheme().isEmpty()) {
        const QString candidate = launcherUrl.toString(QUrl::RemoveQuery | QUrl::RemoveFragment);

        if (!candidate.isEmpty()) {
            if (candidate.startsWith(QLatin1Char('/'))) {
                return QUrl::fromLocalFile(candidate);
            }

            if (candidate.endsWith(QLatin1String(".desktop"))) {
                if (KService::Ptr service = KService::serviceByStorageId(candidate)) {
                    return QUrl::fromLocalFile(service->entryPath());
                }

                if (KService::Ptr service = KService::serviceByMenuId(candidate)) {
                    return QUrl::fromLocalFile(service->entryPath());
                }

                const QString desktopPath = QStandardPaths::locate(QStandardPaths::ApplicationsLocation, candidate);
                if (!desktopPath.isEmpty()) {
                    return QUrl::fromLocalFile(desktopPath);
                }
            }
        }
    }

    return launcherUrl;
}

KService::Ptr ContextMenuActionsBackend::serviceFromLauncherUrl(const QUrl &launcherUrl)
{
    if (!launcherUrl.isValid()) {
        return KService::Ptr();
    }

    if (launcherUrl.scheme() == QLatin1String("applications")) {
        QString menuId = launcherUrl.path();

        if (menuId.startsWith(QLatin1Char('/'))) {
            menuId.remove(0, 1);
        }

        if (menuId.isEmpty()) {
            menuId = launcherUrl.toString(QUrl::RemoveScheme | QUrl::RemoveQuery | QUrl::RemoveFragment);
        }

        if (menuId.startsWith(QLatin1String("//"))) {
            menuId.remove(0, 2);
        }

        if (menuId.startsWith(QLatin1Char('/'))) {
            menuId.remove(0, 1);
        }

        if (!menuId.isEmpty()) {
            if (KService::Ptr service = KService::serviceByMenuId(menuId)) {
                return service;
            }

            if (KService::Ptr service = KService::serviceByStorageId(menuId)) {
                return service;
            }
        }

        return KService::Ptr();
    }

    if (launcherUrl.isLocalFile()) {
        const QString desktopPath = launcherUrl.toLocalFile();

        if (!KDesktopFile::isDesktopFile(desktopPath)) {
            return KService::Ptr();
        }

        if (KService::Ptr service = KService::serviceByDesktopPath(desktopPath)) {
            return service;
        }

        const QString fileName = QFileInfo(desktopPath).fileName();
        if (!fileName.isEmpty()) {
            if (KService::Ptr service = KService::serviceByStorageId(fileName)) {
                return service;
            }

            if (KService::Ptr service = KService::serviceByMenuId(fileName)) {
                return service;
            }
        }

        return KService::Ptr();
    }

    if (launcherUrl.scheme().isEmpty()) {
        const QString desktopId = launcherUrl.toString(QUrl::RemoveQuery | QUrl::RemoveFragment);

        if (!desktopId.isEmpty()) {
            if (KService::Ptr service = KService::serviceByStorageId(desktopId)) {
                return service;
            }

            if (KService::Ptr service = KService::serviceByMenuId(desktopId)) {
                return service;
            }
        }
    }

    return KService::Ptr();
}

}
}
