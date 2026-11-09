/*
    SPDX-FileCopyrightText: 2018 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef PLASMABACKGROUNDCACHE_H
#define PLASMABACKGROUNDCACHE_H

// local
#include "screenpool.h"

// Qt
#include <QHash>
#include <QObject>

// Plasma
#include <Plasma/Plasma>

// KDE
#include <KConfigGroup>
#include <KSharedConfig>

//! Unit test access to BackgroundCache::areaIsBusy(); see the friend declaration below.
class CoreComponentsTest;

namespace Latte {
namespace PlasmaExtended {

struct ImageHints
{
    bool busy{false};
    float brightness{-1000};
};

using EdgesHash = QHash<Plasma::Types::Location, ImageHints>;

class BackgroundCache: public QObject
{
    Q_OBJECT

public:
    static BackgroundCache *self();
    ~BackgroundCache() override;

    bool busyFor(QString activity, QString screen, Plasma::Types::Location location);
    float brightnessFor(QString activity, QString screen, Plasma::Types::Location location);

    QString background(QString activity, QString screen) const;

    void setBackgroundFromBroadcast(QString activity, QString screen, QString filename);
    void setBroadcastedBackgroundsEnabled(QString activity, QString screen, bool enabled);

Q_SIGNALS:
    void backgroundChanged(const QString &activity, const QString &screenName);

private Q_SLOTS:
    void reload();
    void settingsFileChanged(const QString &file);

private:
    //! areaIsBusy() is only reached from image tiles, and colorBrightness() keeps those
    //! in 0..255, so busyFor() cannot exercise its out-of-range branch. The test needs
    //! direct access to check that branch.
    friend class ::CoreComponentsTest;

    BackgroundCache(QObject *parent = nullptr);

    bool backgroundIsBroadcasted(QString activity, QString screenName) const;
    bool pluginExistsFor(QString activity, QString screenName) const;
    bool areaIsBusy(float bright1, float bright2) const;
    bool busyForFile(QString imageFile, Plasma::Types::Location location);
    bool isDesktopContainment(const KConfigGroup &containment) const;

    float brightnessForFile(QString imageFile, Plasma::Types::Location location);
    float brightnessFromArea(QImage &image, int firstRow, int firstColumn, int endRow, int endColumn);
    QString backgroundFromConfig(const KConfigGroup &config, QString wallpaperPlugin) const;

    void cleanupHashes();
    void updateImageCalculations(QString imageFile, Plasma::Types::Location location);

private:
    bool m_initialized{false};

    QString m_defaultWallpaperPath;

    ScreenPool *m_pool{nullptr};

    //! screen aware backgrounds: activity id, screen name, backgroundfile
    QHash<QString, QHash<QString, QString>> m_backgrounds;

    //! plugin names tracked: activity id, screen name, pluginName
    QHash<QString, QHash<QString, QString>> m_plugins;

    //! backgrounds that are broadcasted (not found through plasma files):
    //! and have higher priority: activity id, screen names
    QHash<QString, QList<QString>> m_broadcasted;

    //! image file and brightness per edge
    QHash<QString, EdgesHash> m_hintsCache;

    KSharedConfig::Ptr m_plasmaConfig;
};

}
}

#endif
