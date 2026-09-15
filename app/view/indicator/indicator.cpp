/*
    SPDX-FileCopyrightText: 2019 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <latte_debug.h>
#include "indicator.h"

// local
#include "pluginids.h"
#include <coretypes.h>
#include "indicatorinfo.h"
#include "../containmentinterface.h"
#include "../view.h"
#include "../../lattecorona.h"
#include "../../indicator/factory.h"

// Qt
#include <QFileDialog>
#include <QFileInfo>
#include <QLatin1String>

// KDE
#include <KLocalizedString>
#include <KPluginMetaData>
#include <KDeclarative/ConfigPropertyMap>
#include <KDeclarative/QmlObjectSharedEngine>


namespace Latte {
namespace ViewPart {

Indicator::Indicator(Latte::View *parent)
    : QObject(parent),
      m_view(parent),
      m_info(new IndicatorPart::Info(this)),
      m_resources(new IndicatorPart::Resources(this))
{
    m_corona = qobject_cast<Latte::Corona *>(m_view->corona());
    loadConfig();

    connect(this, &Indicator::enabledChanged, this, &Indicator::saveConfig);
    connect(this, &Indicator::pluginChanged, this, &Indicator::saveConfig);

    connect(m_view->extendedInterface(), &ContainmentInterface::hasLatteTasksChanged, this, &Indicator::latteTasksArePresentChanged);

    connect(m_view, &Latte::View::indicatorPluginChanged, [this](const QString & indicatorId) {
        if (m_corona && m_corona->indicatorFactory()->isCustomType(indicatorId)) {
            Q_EMIT customPluginsChanged();
        }
    });

    connect(m_view, &Latte::View::indicatorPluginRemoved, [this](const QString & indicatorId) {
        if (m_corona && m_type == indicatorId && !m_corona->indicatorFactory()->pluginExists(indicatorId)) {
            setType(QStringLiteral("org.kde.latte.default"));
        }

        if (m_corona && m_corona->indicatorFactory()->isCustomType(indicatorId)) {
            Q_EMIT customPluginsChanged();
        }
    });

    load(m_type);

    loadPlasmaComponent();
}

Indicator::~Indicator()
{
    unloadIndicators();
    saveConfig();

    if (m_component) {
        m_component->deleteLater();
    }

    if (m_configLoader) {
        m_configLoader->deleteLater();
    }

    if (m_configuration) {
        m_configuration->deleteLater();
    }

    if (m_info) {
        m_info->deleteLater();
    }
}

bool Indicator::enabled() const
{
    return m_enabled;
}

void Indicator::setEnabled(bool enabled)
{
    if (m_enabled == enabled) {
        return;
    }

    m_enabled = enabled;
    Q_EMIT enabledChanged();
}

bool Indicator::enabledForApplets() const
{
    return m_enabledForApplets;
}

void Indicator::setEnabledForApplets(bool enabled)
{
    if (m_enabledForApplets == enabled) {
        return;
    }

    m_enabledForApplets = enabled;
    Q_EMIT enabledForAppletsChanged();
}

bool Indicator::isCustomIndicator() const
{
    return m_corona->indicatorFactory()->isCustomType(type());
}

bool Indicator::latteTasksArePresent()
{
    return m_view->extendedInterface()->hasLatteTasks();
}

bool Indicator::pluginIsReady()
{
    return m_pluginIsReady;
}

bool Indicator::isModernDockStyle() const
{
    if (!m_view || !m_view->containment()) {
        return false;
    }

    const QString dockStyle = m_view->containment()->config().group(QStringLiteral("General"))
                                      .readEntry(QStringLiteral("dockStyle"), QStringLiteral("Classic"));
    return dockStyle == QLatin1String("Modern") || dockStyle == QLatin1String("1");
}

void Indicator::setPluginIsReady(bool ready)
{
    if (m_pluginIsReady == ready) {
        return;
    }

    m_pluginIsReady = ready;
    Q_EMIT pluginIsReadyChanged();
}

int Indicator::index(const QString &type)
{
    if (type == QLatin1String(Latte::PluginId::kDefaultIndicator)) {
        return 0;
    } else if (type == QLatin1String(Latte::PluginId::kPlasmaTabStyleIndicator)) {
        return 1;
    } else if (customPluginIds().contains(type)) {
        return 2 + customPluginIds().indexOf(type);
    }

    return -1;
}

QString Indicator::type() const
{
    return m_type;
}

void Indicator::setType(QString type)
{
    if (type == QLatin1String(Latte::PluginId::kPlasmaIndicator)) {
        type = QStringLiteral("org.kde.latte.default");
    }

    if (m_type == type) {
        return;
    }

    load(type);
}

QString Indicator::customType() const
{
    return m_customType;
}

void Indicator::setCustomType(QString type)
{
    if (m_customType == type) {
        return;
    }

    m_customType = type;
    Q_EMIT customPluginChanged();
}

int Indicator::customPluginsCount() const
{
    return m_corona->indicatorFactory()->customPluginsCount();
}

QString Indicator::uiPath() const
{
    return m_corona->indicatorFactory()->uiPath(m_type);
}

QStringList Indicator::customPluginIds() const
{
    return m_corona->indicatorFactory()->customPluginIds();
}

QStringList Indicator::customPluginNames() const
{
    return m_corona->indicatorFactory()->customPluginNames();
}

QStringList Indicator::customLocalPluginIds() const
{
    return m_corona->indicatorFactory()->customLocalPluginIds();
}

IndicatorPart::Info *Indicator::info() const
{
    return m_info;
}

IndicatorPart::Resources *Indicator::resources() const
{
    return m_resources;
}

QQmlComponent *Indicator::component() const
{
    //! Create the indicator component lazily on first access instead of at
    //! construction time. The org.kde.plasma.plasmoid QML module is only
    //! registered by PlasmaQuick once the first AppletQuickItem exists, so
    //! creating the component early (View construction happens before any
    //! applet item) fails on Plasma 6.3 with "module is not installed".
    //! By the time the containment QML instantiates the Indicators ability
    //! the module is registered and the import resolves.
    if (!m_component) {
        const QString uiPath = m_metadata.value(QStringLiteral("X-Latte-MainScript"));

        if (!uiPath.isEmpty()) {
            const QString fullPath = m_pluginPath + QLatin1String("/package/") + uiPath;
            m_component = new QQmlComponent(m_view->engine().get(), fullPath);
        }
    }

    return m_component;
}

QQmlComponent *Indicator::plasmaComponent() const
{
    //! Same lazy creation as component(): the plasma tab style indicator
    //! package runs in the same engine and must not be created before the
    //! plasmoid QML module is registered.
    if (!m_plasmaComponent) {
        KPluginMetaData metadata = m_corona->indicatorFactory()->metadata(QStringLiteral("org.kde.latte.plasmatabstyle"));
        QString uiPath = metadata.value(QStringLiteral("X-Latte-MainScript"));

        if (!uiPath.isEmpty()) {
            uiPath = QFileInfo(metadata.fileName()).absolutePath() + QLatin1String("/package/") + uiPath;
            m_plasmaComponent = new QQmlComponent(m_view->engine().get(), uiPath);
        }
    }

    return m_plasmaComponent;
}

QObject *Indicator::configuration() const
{
    return m_configuration;
}

void Indicator::load(QString type)
{
    KPluginMetaData metadata = m_corona->indicatorFactory()->metadata(type);

    if (metadata.isValid()) {
        //! remove all previous indicators
        setPluginIsReady(false);

        m_metadata = metadata;
        m_type = type;
        m_pluginPath = QFileInfo(m_metadata.fileName()).absolutePath();

        if (m_corona && m_corona->indicatorFactory()->isCustomType(type)) {
            setCustomType(type);
        }

        updateScheme();
        updateComponent();

        Q_EMIT pluginChanged();

        //! create all indicators with the new type
        setPluginIsReady(true);
    } else if (type != QStringLiteral("org.kde.latte.default")) {
        qCDebug(latteIndicator) << " Indicator metadata are not valid : " << type;
        setType(QStringLiteral("org.kde.latte.default"));
    }
}

void Indicator::updateComponent()
{
    auto prevComponent = m_component;
    //! drop the old component and let component() recreate it lazily
    m_component = nullptr;

    if (prevComponent) {
        prevComponent->deleteLater();
    }
}

void Indicator::loadPlasmaComponent()
{
    auto prevComponent = m_plasmaComponent;
    //! drop the old component and let plasmaComponent() recreate it lazily
    m_plasmaComponent = nullptr;

    if (prevComponent) {
        prevComponent->deleteLater();
    }

    Q_EMIT plasmaComponentChanged();
}

void Indicator::unloadIndicators()
{
    setPluginIsReady(false);
}

void Indicator::updateScheme()
{
    auto prevConfigLoader = m_configLoader;
    auto prevConfiguration = m_configuration;

    QString xmlPath = m_metadata.value(QStringLiteral("X-Latte-ConfigXml"));

    if (!xmlPath.isEmpty()) {
        QFile file(m_pluginPath + QLatin1String("/package/") + xmlPath);
        m_configLoader = new KConfigLoader(m_view->containment()->config().group(QStringLiteral("Indicator")).group(m_metadata.pluginId()), &file);
        m_configuration = new KDeclarative::ConfigPropertyMap(m_configLoader, this);

        // In KF6, KConfigPropertyMap dropped autosave on property assignment,
        // keeping changes in-memory until writeConfig() is explicitly called.
        // Without writeConfig(), m_configLoader never writes dirty items to the
        // underlying containment Indicator KConfigGroup, causing config.sync()
        // to flush an empty group and resetting all indicator settings on restart.
        connect(m_configuration, &QQmlPropertyMap::valueChanged, this, [this]() {
            if (m_configuration) {
                m_configuration->writeConfig();
            }

            if (m_view && m_view->containment()) {
                auto config = m_view->containment()->config().group(QStringLiteral("Indicator"));
                config.sync();
            }
        });
    } else {
        m_configLoader = nullptr;
        m_configuration = nullptr;
    }

    if (prevConfigLoader) {
        prevConfigLoader->deleteLater();
    }

    if (prevConfiguration) {
        prevConfiguration->writeConfig();
        prevConfiguration->deleteLater();
    }

    Q_EMIT configurationChanged();
}

void Indicator::loadConfig()
{
    auto config = m_view->containment()->config().group(QStringLiteral("Indicator"));
    m_customType = config.readEntry(QStringLiteral("customType"), QString());
    m_enabled = config.readEntry(QStringLiteral("enabled"), true);
    m_type = config.readEntry(QStringLiteral("type"), "org.kde.latte.default");

    if (m_type == QLatin1String(Latte::PluginId::kPlasmaIndicator)) {
        m_type = QStringLiteral("org.kde.latte.default");
    }
}

void Indicator::saveConfig()
{
    if (m_configuration) {
        m_configuration->writeConfig();
    }

    auto config = m_view->containment()->config().group(QStringLiteral("Indicator"));
    config.writeEntry(QStringLiteral("customType"), m_customType);
    config.writeEntry(QStringLiteral("enabled"), m_enabled);
    config.writeEntry(QStringLiteral("type"), m_type);
    config.sync();
}

}
}
