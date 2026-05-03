/*
    SPDX-FileCopyrightText: 2026 Latte Dock Contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef CONTEXTMENUACTIONSBACKEND_H
#define CONTEXTMENUACTIONSBACKEND_H

// Qt
#include <QObject>
#include <QUrl>
#include <QVariantList>

// KDE
#include <KService>

namespace Latte
{
namespace Tasks
{

class ContextMenuActionsBackend : public QObject
{
    Q_OBJECT

public:
    explicit ContextMenuActionsBackend(QObject *parent = nullptr);

    Q_INVOKABLE QVariantList jumpListActions(const QVariant &launcherUrl, QObject *parent);
    Q_INVOKABLE QVariantList placesActions(const QVariant &launcherUrl, bool showAllPlaces, QObject *parent);
    Q_INVOKABLE QVariantList recentDocumentActions(const QVariant &launcherUrl, QObject *parent);

Q_SIGNALS:
    void showAllPlaces();

private:
    void handleRecentDocumentAction() const;
    static QStringList applicationCategories(const QUrl &launcherUrl);
    static QUrl toUrl(const QVariant &value);
    static QUrl tryDecodeApplicationsUrl(const QUrl &launcherUrl);
    static KService::Ptr serviceFromLauncherUrl(const QUrl &launcherUrl);
};

}
}

#endif
