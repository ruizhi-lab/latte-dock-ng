/*
    SPDX-FileCopyrightText: 2020 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "environment.h"

// Qt
#include <KConfigGroup>
#include <KDirWatch>
#include <KIconThemes/KIconLoader>
#include <KSharedConfig>
#include <QIcon>
#include <QDebug>
#include <QStandardPaths>

#define LONGDURATION 240
#define SHORTDURATION 40

namespace Latte{

const int Environment::SeparatorLength;

Environment::Environment(QObject *parent)
    : QObject(parent)
{
    const QString initialTheme = currentIconTheme();

    if (!initialTheme.isEmpty()) {
        QIcon::setThemeName(initialTheme);
    }

    connect(KIconLoader::global(), &KIconLoader::iconLoaderSettingsChanged,
            this, &Environment::markIconThemeChanged);

    connect(KIconLoader::global(), &KIconLoader::iconChanged,
            this, [this](int) {
                markIconThemeChanged();
            });

    const QString kdeGlobalsFile = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QStringLiteral("/kdeglobals");

    if (!kdeGlobalsFile.isEmpty()) {
        KDirWatch::self()->addFile(kdeGlobalsFile);

        auto handleKdeGlobalsChange = [this, kdeGlobalsFile](const QString &path) {
            if (path != kdeGlobalsFile) {
                return;
            }

            const QString iconTheme = currentIconTheme();
            if (!iconTheme.isEmpty()) {
                QIcon::setThemeName(iconTheme);
            }

            qDebug() << "Environment::kdeglobals changed => reconfigure icon loader";
            KIconLoader::global()->reconfigure(QString());
            markIconThemeChanged();
        };

        connect(KDirWatch::self(), &KDirWatch::dirty, this, handleKdeGlobalsChange);
        connect(KDirWatch::self(), &KDirWatch::created, this, handleKdeGlobalsChange);
        connect(KDirWatch::self(), &KDirWatch::deleted, this, handleKdeGlobalsChange);
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

QString Environment::currentIconTheme() const
{
    KSharedConfigPtr kdeGlobals = KSharedConfig::openConfig(QStringLiteral("kdeglobals"));
    kdeGlobals->reparseConfiguration();
    KConfigGroup iconsGroup(kdeGlobals, QStringLiteral("Icons"));
    return iconsGroup.readEntry(QStringLiteral("Theme"), QString());
}

void Environment::markIconThemeChanged()
{
    ++m_iconThemeVersion;
    qDebug() << "Environment::iconThemeVersionChanged" << m_iconThemeVersion;
    Q_EMIT iconThemeVersionChanged();
}

}
