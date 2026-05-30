/*
    SPDX-FileCopyrightText: 2019 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "abstractlayout.h"

// local
#include "../data/layoutdata.h"

// Qt
#include <QDir>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QLatin1String>

// KDE
#include <KSharedConfig>

namespace Latte {
namespace Layout {

AbstractLayout::AbstractLayout(QObject *parent, QString layoutFile, QString assignedName)
    : QObject(parent)
{
    qDebug() << "Layout file to create object: " << layoutFile << " with name: " << assignedName;

    if (QFile(layoutFile).exists()) {
        if (assignedName.isEmpty()) {
            assignedName =  layoutName(layoutFile);
        }

        //!this order is important because setFile initializes also the m_layoutGroup
        setFile(layoutFile);
        setName(assignedName);
        loadConfig();
        init();

        m_loadedCorrectly = true;
    }
}

AbstractLayout::~AbstractLayout()
{
}

void AbstractLayout::init()
{
    connect(this, &AbstractLayout::iconChanged, this, &AbstractLayout::saveConfig);
    connect(this, &AbstractLayout::lastUsedActivityChanged, this, &AbstractLayout::saveConfig);
    connect(this, &AbstractLayout::launchersChanged, this, &AbstractLayout::saveConfig);
    connect(this, &AbstractLayout::preferredForShortcutsTouchedChanged, this, &AbstractLayout::saveConfig);
    connect(this, &AbstractLayout::popUpMarginChanged, this, &AbstractLayout::saveConfig);
    connect(this, &AbstractLayout::schemeFileChanged, this, &AbstractLayout::saveConfig);
    connect(this, &AbstractLayout::versionChanged, this, &AbstractLayout::saveConfig);
}

int AbstractLayout::version() const
{
    return m_version;
}

void AbstractLayout::setVersion(int ver)
{
    if (m_version == ver) {
        return;
    }

    m_version = ver;

    Q_EMIT versionChanged();
}


bool AbstractLayout::preferredForShortcutsTouched() const
{
    return m_preferredForShortcutsTouched;
}

void AbstractLayout::setPreferredForShortcutsTouched(bool touched)
{
    if (m_preferredForShortcutsTouched == touched) {
        return;
    }

    m_preferredForShortcutsTouched = touched;
    Q_EMIT preferredForShortcutsTouchedChanged();
}

int AbstractLayout::popUpMargin() const
{
    return m_popUpMargin;
}

void AbstractLayout::setPopUpMargin(const int &margin)
{
    if (m_popUpMargin == margin) {
        return;
    }

    m_popUpMargin = margin;
    Q_EMIT popUpMarginChanged();
}

QString AbstractLayout::background() const
{
    return QString();
}

QString AbstractLayout::schemeFile() const
{
    return m_schemeFile;
}

void AbstractLayout::setSchemeFile(const QString &file)
{
    if (m_schemeFile == file) {
        return;
    }

    m_schemeFile = file;
    Q_EMIT schemeFileChanged();
}

QString AbstractLayout::textColor() const
{
    return m_customTextColor;
}

QString AbstractLayout::file() const
{
    return m_layoutFile;
}

void AbstractLayout::setFile(QString file)
{
    if (m_layoutFile == file) {
        return;
    }

    qDebug() << "Layout file:" << file;

    m_layoutFile = file;

    KSharedConfigPtr filePtr = KSharedConfig::openConfig(m_layoutFile);
    m_layoutGroup = KConfigGroup(filePtr, "LayoutSettings");

    Q_EMIT fileChanged();
}

QString AbstractLayout::name() const
{
    return m_layoutName;
}

void AbstractLayout::setName(QString name)
{
    if (m_layoutName == name) {
        return;
    }

    qDebug() << "Layout name:" << name;

    m_layoutName = name;

    Q_EMIT nameChanged();
}

QString AbstractLayout::icon() const
{
    return m_icon;
}

void AbstractLayout::setIcon(const QString &icon)
{
    if (m_icon == icon) {
        return;
    }

    m_icon = icon;
    Q_EMIT iconChanged();
}

QString AbstractLayout::lastUsedActivity() const
{
    return m_lastUsedActivity;
}

void AbstractLayout::clearLastUsedActivity()
{
    m_lastUsedActivity = "";
    Q_EMIT lastUsedActivityChanged();
}

QString AbstractLayout::customTextColor() const
{
    return m_customTextColor;
}

void AbstractLayout::setCustomTextColor(const QString &customColor)
{
    if (m_customTextColor == customColor) {
        return;
    }

    m_customTextColor = customColor;
    Q_EMIT textColorChanged();
}

QStringList AbstractLayout::launchers() const
{
    return m_launchers;
}

void AbstractLayout::setLaunchers(QStringList launcherList)
{
    if (m_launchers == launcherList)
        return;

    m_launchers = launcherList;

    Q_EMIT launchersChanged();
}

Type AbstractLayout::type() const
{
    return Type::Abstract;
}

QList<Plasma::Types::Location> combinedFreeEdges(const QList<Plasma::Types::Location> &edges1, const QList<Plasma::Types::Location> &edges2)
{
    QList<Plasma::Types::Location> validFreeEdges;

    for (int i=0; i<edges1.count(); ++i) {
        if (edges2.contains(edges1[i])) {
            validFreeEdges << edges1[i];
        }
    }

    return validFreeEdges;
}

QString AbstractLayout::layoutName(const QString &fileName)
{
    int lastSlash = fileName.lastIndexOf("/");
    QString tempLayoutFile = fileName;
    QString layoutName = tempLayoutFile.remove(0, lastSlash + 1);

    int ext = layoutName.lastIndexOf(".layout.latte");
    layoutName = layoutName.remove(ext, 13);

    return layoutName;
}

void AbstractLayout::syncSettings()
{
    if (QFile(file()).exists()) {
        m_layoutGroup.sync();
    }
}

void AbstractLayout::loadConfig()
{
    m_version = m_layoutGroup.readEntry("version", 2);
    m_launchers = m_layoutGroup.readEntry("launchers", QStringList());
    m_lastUsedActivity = m_layoutGroup.readEntry("lastUsedActivity", QString());
    m_preferredForShortcutsTouched = m_layoutGroup.readEntry("preferredForShortcutsTouched", false);
    m_popUpMargin = m_layoutGroup.readEntry("popUpMargin", -1);

    m_schemeFile = m_layoutGroup.readEntry("schemeFile", QString(Data::Layout::DEFAULTSCHEMEFILE));

    if (m_schemeFile.startsWith("~")) {
        m_schemeFile.remove(0, 1);
        m_schemeFile = QDir::homePath() + m_schemeFile;
    }

    m_schemeFile = m_schemeFile.isEmpty() || !QFileInfo(m_schemeFile).exists() ? Data::Layout::DEFAULTSCHEMEFILE : m_schemeFile;

    Q_EMIT schemeFileChanged();

    m_customTextColor = m_layoutGroup.readEntry("customTextColor", QString());
    m_icon = m_layoutGroup.readEntry("icon", QString());
}

void AbstractLayout::saveConfig()
{
    qDebug() << "abstract layout is saving... for layout:" << m_layoutName;
    m_layoutGroup.writeEntry("version", m_version);
    m_layoutGroup.writeEntry("launchers", m_launchers);
    m_layoutGroup.writeEntry("icon", m_icon);
    m_layoutGroup.writeEntry("lastUsedActivity", m_lastUsedActivity);
    m_layoutGroup.writeEntry("popUpMargin", m_popUpMargin);
    m_layoutGroup.writeEntry("preferredForShortcutsTouched", m_preferredForShortcutsTouched);

    QString scmfile = m_schemeFile;

    if (scmfile.startsWith(QDir::homePath())) {
        scmfile.remove(0, QDir::homePath().size());
        scmfile = "~" + scmfile;
    }
    m_layoutGroup.writeEntry("schemeFile", scmfile == Data::Layout::DEFAULTSCHEMEFILE ? "" : scmfile);

    m_layoutGroup.sync();
}

}
}
