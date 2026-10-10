/*
    SPDX-FileCopyrightText: 2020 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <latte_debug.h>
#include "environment.h"

// Qt
#include <KConfigGroup>
#include <KDirWatch>
#include <KIconThemes/KIconLoader>
#include <KIconThemes/KIconTheme>
#include <KSharedConfig>
#include <QIcon>
#include <QDebug>
#include <QGuiApplication>
#include <QPixmapCache>
#include <QQuickItem>
#include <QStandardPaths>

#define LONGDURATION 240
#define SHORTDURATION 40

namespace Latte {

const int Environment::SeparatorLength;

Environment::Environment(QObject *parent)
    : QObject(parent)
{
    const QString initialTheme = currentIconTheme();

    if (!initialTheme.isEmpty()) {
        QIcon::setThemeName(initialTheme);
    }

    // KF6: iconLoaderSettingsChanged is emitted whenever any icon setting
    // changes (theme, size, effects).  Previously iconChanged(int) was also
    // connected, but that signal fires once per icon group (Desktop, Toolbar,
    // MainToolbar …) and during theme switches caused a flood of QML binding
    // re-evaluations that could crash when Svg objects were being recreated.
    // KIconLoader initializes desktop integration synchronously; that is not
    // available in the offscreen/minimal platforms used by QML smoke tests.
    const bool desktopIntegrationAvailable = QGuiApplication::platformName() != QLatin1String("offscreen")
                                             && QGuiApplication::platformName() != QLatin1String("minimal");
    if (desktopIntegrationAvailable) {
        connect(KIconLoader::global(), &KIconLoader::iconLoaderSettingsChanged,
                this, &Environment::markIconThemeChanged);
    }

    const QString kdeGlobalsFile = desktopIntegrationAvailable
        ? QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QStringLiteral("/kdeglobals")
        : QString();

    if (!kdeGlobalsFile.isEmpty()) {
        KDirWatch::self()->addFile(kdeGlobalsFile);

        auto handleKdeGlobalsChange = [this, kdeGlobalsFile](const QString & path) {
            if (path != kdeGlobalsFile) {
                return;
            }

            QIcon::setThemeName(currentIconTheme());

            QPixmapCache::clear();
            // File-backed widget icons use KIconLoader rather than Qt's theme
            // engine. Clear KIconTheme's cached name before rebuilding the
            // loader, otherwise kdeglobals is new but resolved paths stay old.
            KIconTheme::reconfigure();
            qCDebug(latteQml) << "Environment::kdeglobals changed => reconfigure icon loader" << QIcon::themeName();
            KIconLoader::global()->reconfigure(QString());
            markIconThemeChanged();
        };

        connect(KDirWatch::self(), &KDirWatch::dirty, this, handleKdeGlobalsChange);
        connect(KDirWatch::self(), &KDirWatch::created, this, handleKdeGlobalsChange);
        connect(KDirWatch::self(), &KDirWatch::deleted, this, handleKdeGlobalsChange);

        m_iconThemeChangedTimer.setSingleShot(true);
        connect(&m_iconThemeChangedTimer, &QTimer::timeout, this, &Environment::emitIconThemeVersionChanged);
    }
}

int Environment::separatorLength() const
{
    return SeparatorLength;
}

uint Environment::shortDuration() const
{
    return SHORTDURATION;
}

uint Environment::longDuration() const
{
    return LONGDURATION;
}

uint Environment::iconThemeVersion() const
{
    return m_iconThemeVersion;
}

uint Environment::makeVersion(uint major, uint minor, uint release) const
{
    return (((major) << 16) | ((minor) << 8) | (release));
}

QVariant Environment::iconSourceForTheme(const QVariant &source) const
{
    if (source.canConvert<QIcon>()) {
        const QIcon icon = source.value<QIcon>();
        const QString iconName = icon.name();

        if (!iconName.isEmpty()) {
            return iconName;
        }
    }

    return source;
}

QString Environment::iconDescriptor(const QVariant &source) const
{
    QString descriptor = QString::fromLatin1(source.typeName() ? source.typeName() : "<unknown>");

    if (source.canConvert<QIcon>()) {
        const QIcon icon = source.value<QIcon>();
        descriptor += QStringLiteral(" iconName=\"%1\" isNull=%2")
                      .arg(icon.name(), icon.isNull() ? QStringLiteral("true") : QStringLiteral("false"));
    }

    if (source.canConvert<QString>()) {
        descriptor += QStringLiteral(" string=\"%1\"").arg(source.toString());
    }

    return descriptor;
}

void
Environment::refreshAppletIcons(QQuickItem *root) const
{
    if (!root) {
        return;
    }

    // Native Kirigami/KSvg icons retain rendered pixels when their bound
    // source name does not change. After the coalesced theme notification,
    // request polish without writing source, preserving each widget's own
    // configuration/state binding. Walk the visual subtree because Plasma 6
    // compact representations need not match legacy IconItem discovery.
    QList<QQuickItem *> pending{ root };
    while (!pending.isEmpty()) {
        QQuickItem *item = pending.takeLast();
        for (const QMetaObject *meta = item->metaObject(); meta; meta = meta->superClass()) {
            const QByteArray name(meta->className());
            if (name == "Icon" || name == "IconItem" || name.endsWith("::Icon") || name.endsWith("::IconItem")) {
                item->polish();
                qCDebug(latteQml) << "[widget-icon-refresh]" << item << item->property("source");
                break;
            }
        }
        pending.append(item->childItems());
    }
}

QString Environment::currentIconTheme() const
{
    KSharedConfigPtr kdeGlobals = KSharedConfig::openConfig(QStringLiteral("kdeglobals"));
    kdeGlobals->reparseConfiguration();
    KConfigGroup iconsGroup(kdeGlobals, QStringLiteral("Icons"));
    return iconsGroup.readEntry(QStringLiteral("Theme"), QStringLiteral("breeze"));
}

void Environment::markIconThemeChanged()
{
    // Debounce: rapid theme-change signals (e.g. from multiple icon groups
    // during a Plasma global-theme switch) can cause QML binding storms
    // that race with Svg object recreation.  Coalesce them into a single
    // notification per event-loop iteration.
    if (!m_iconThemeChangedTimer.isActive()) {
        m_iconThemeChangedTimer.start(50);
    }
}

void Environment::emitIconThemeVersionChanged()
{
    ++m_iconThemeVersion;
    qCDebug(latteQml) << "Environment::iconThemeVersionChanged" << m_iconThemeVersion;
    Q_EMIT iconThemeVersionChanged();
}

}
