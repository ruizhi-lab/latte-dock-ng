/*
    SPDX-FileCopyrightText: 2016 Smith AR <audoban@openmailbox.org>
    SPDX-FileCopyrightText: 2016 Michail Vourlakos <mvourlakos@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <latte_debug.h>
#include "view.h"

// local
#include "effects.h"
#include "positioner.h"
#include "visibilitymanager.h"
#include "../wm/schemecolors.h"
#include "../apptypes.h"
#include "settings/primaryconfigview.h"
#include "settings/viewsettingsfactory.h"
#include "settings/widgetexplorerview.h"
#include "../apptypes.h"
#include "../lattecorona.h"
#include "../data/layoutdata.h"
#include "../data/viewstable.h"
#include "../declarativeimports/interfaces.h"
#include "../indicator/factory.h"
#include "../layout/genericlayout.h"
#include "../layouts/manager.h"
#include "../layouts/storage.h"
#include "../plasma/extended/theme.h"
#include "../screenpool.h"
#include "../settings/universalsettings.h"
#include "../settings/exporttemplatedialog/exporttemplatedialog.h"
#include "../shortcuts/globalshortcuts.h"
#include "../shortcuts/shortcutstracker.h"

// Qt
#include <QAction>
#include <QCoreApplication>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QPointer>
#include <QSet>
#include <QTimer>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlProperty>
#include "../knscompat.h"
#include <QQuickItem>
#include <QSGRendererInterface>
#include <QMenu>

#include <algorithm>

// KDe
#include <KActionCollection>
#include <KActivities/Consumer>
#include <KWayland/Client/plasmashell.h>
#include <KWayland/Client/surface.h>

// Plasma
#include <Plasma/Containment>
#include <Plasma/ContainmentActions>
#include <PlasmaQuick/AppletPopup>
#include <PlasmaQuick/AppletQuickItem>

#define BLOCKHIDINGDRAGTYPE QStringLiteral("View::ContainsDrag()")
#define BLOCKHIDINGNEEDSATTENTIONTYPE QStringLiteral("View::Containment::NeedsAttentionState()")
#define BLOCKHIDINGREQUESTSINPUTTYPE "View::Containment::RequestsInputState()"

// Named timer intervals and layout constants
constexpr int kReleaseGrabIntervalMs = 400;
constexpr int kInitLayoutIntervalMs = 100;
constexpr int kWidgetExplorerShowDelayMs = 250;
constexpr int kEditThicknessSmallSpacing = 4;
constexpr int kVisibleHackTimer1Ms = 400;
constexpr int kVisibleHackTimer2Ms = 2500;

namespace {

QString actualQtQuickGraphicsApiName(QSGRendererInterface::GraphicsApi api)
{
    switch (api) {
        case QSGRendererInterface::Unknown:
            return QStringLiteral("Unknown");

        case QSGRendererInterface::Software:
            return QStringLiteral("Software");

        case QSGRendererInterface::OpenVG:
            return QStringLiteral("OpenVG");

        case QSGRendererInterface::OpenGL:
            return QStringLiteral("OpenGL");

        case QSGRendererInterface::Direct3D11:
            return QStringLiteral("Direct3D11");

        case QSGRendererInterface::Vulkan:
            return QStringLiteral("Vulkan");

        case QSGRendererInterface::Metal:
            return QStringLiteral("Metal");

        case QSGRendererInterface::Null:
            return QStringLiteral("Null");

        case QSGRendererInterface::Direct3D12:
            return QStringLiteral("Direct3D12");
    }

    return QStringLiteral("Unknown(%1)").arg(static_cast<int>(api));
}

bool isActualQtQuickGraphicsApiAccelerated(QSGRendererInterface::GraphicsApi api)
{
    return api != QSGRendererInterface::Unknown
           && api != QSGRendererInterface::Software
           && api != QSGRendererInterface::Null;
}

}

namespace Latte {

View::View(Plasma::Corona *corona, QScreen *targetScreen)
    : PlasmaQuick::ContainmentView(corona),
      m_effects(new ViewPart::Effects(this)),
      m_interface(new ViewPart::ContainmentInterface(this)),
      m_parabolic(new ViewPart::Parabolic(this)),
      m_sink(new ViewPart::EventsSink(this))
{
    //this is disabled because under wayland breaks Views positioning
    //setVisible(false);

    m_corona = qobject_cast<Latte::Corona *>(corona);

    connect(this, &QQuickWindow::sceneGraphInitialized, this, [this]() {
        const QSGRendererInterface *renderer = rendererInterface();
        const QSGRendererInterface::GraphicsApi api = renderer ? renderer->graphicsApi() : QSGRendererInterface::Unknown;
        qInfo().noquote() << QStringLiteral("Latte Dock actual Qt Quick scene graph graphics API: %1 (GPU accelerated: %2)")
                          .arg(actualQtQuickGraphicsApiName(api),
                               isActualQtQuickGraphicsApiAccelerated(api) ? QStringLiteral("true") : QStringLiteral("false"));
    }, Qt::DirectConnection);

    //! needs to be created after Effects because it catches some of its signals
    //! and avoid a crash from View::winId() at the same time
    m_positioner = new ViewPart::Positioner(this);

    setIcon(qGuiApp->windowIcon());
    setResizeMode(QuickViewSharedEngine::SizeRootObjectToView);
    setColor(QColor(Qt::transparent));

    //! Track pointer-leave on the dock's applet popup / menu windows so
    //! cascading submenus close when the pointer leaves the menu tree
    //! (event-driven; see eventFilter()).
    qApp->installEventFilter(this);

    const auto flags = Qt::FramelessWindowHint
                       | Qt::NoDropShadowWindowHint
                       | Qt::WindowDoesNotAcceptFocus;

    setFlags(flags);

    // Pin color scheme before screen assignment
    const QString appScheme = Latte::WindowSystem::SchemeColors::possibleSchemeFile(QStringLiteral("kdeglobals"));

    if (!appScheme.isEmpty()) {
        setProperty("KDE_COLOR_SCHEME_PATH", appScheme);
    }

    if (targetScreen) {
        m_positioner->setScreenToFollow(targetScreen);
    } else {
        qCDebug(latteView) << "org.kde.view :::: corona was found properly!!!";
        m_positioner->setScreenToFollow(m_corona->screenPool()->primaryScreen());
    }

    m_releaseGrabTimer.setInterval(kReleaseGrabIntervalMs);
    m_releaseGrabTimer.setSingleShot(true);
    connect(&m_releaseGrabTimer, &QTimer::timeout, this, &View::releaseGrab);

    // Plasmoid drag-drop timer: defers QML item access to next event loop iteration
    // to avoid breaking the Wayland drag event chain (Qt 6 + event() override issue).
    m_plasmoidDragTimer.setSingleShot(true);
    m_plasmoidDragTimer.setInterval(0);
    connect(&m_plasmoidDragTimer, &QTimer::timeout, this, &View::updatePlasmoidDrag);

    connect(m_interface, &ViewPart::ContainmentInterface::hasExpandedAppletChanged, this, &View::updateTransientWindowsTracking);

    connect(this, &View::containmentChanged, this, &View::groupIdChanged);
    connect(this, &View::containmentChanged
    , this, [&]() {
        qCDebug(latteView) << "dock view c++ containment changed 1...";

        if (!this->containment())
            return;

        qCDebug(latteView) << "dock view c++ containment changed 2...";

        setTitle(validTitle());

        //! First load default values from file
        restoreConfig();

        //! Check the screen assigned to this dock
        reconsiderScreen();

        //! needs to be created before visibility creation because visibility uses it
        if (!m_windowsTracker) {
            m_windowsTracker = new ViewPart::WindowsTracker(this);
            Q_EMIT windowsTrackerChanged();
        }

        if (!m_visibility) {
            m_visibility = new ViewPart::VisibilityManager(this);

            connect(m_visibility, &ViewPart::VisibilityManager::isHiddenChanged, this, [this]() {
                if (m_visibility->isHidden()) {
                    m_interface->deactivateApplets();
                }
            });

            connect(m_visibility, &ViewPart::VisibilityManager::containsMouseChanged,
                    this, &View::updateTransientWindowsTracking);

            Q_EMIT visibilityChanged();
        }

        if (!m_indicator) {
            m_indicator = new ViewPart::Indicator(this);
            Q_EMIT indicatorChanged();
        }

        if (m_positioner) {
            //! immediateSyncGeometry helps avoiding binding loops from containment qml side
            m_positioner->immediateSyncGeometry();
        }

        connect(this->containment(), &Plasma::Applet::statusChanged, this, &View::statusChanged);
        connect(this->containment(), &Plasma::Containment::showAddWidgetsInterface, this, &View::showWidgetExplorer);
        connect(this->containment(), &Plasma::Containment::userConfiguringChanged, this, [this]() {
            Q_EMIT inEditModeChanged();
        });

        connect(this->containment(), &Plasma::Containment::destroyedChanged, this, [this]() {
            m_inDelete = containment()->destroyed();
        });

        if (m_corona->viewSettingsFactory()->hasOrphanSettings()
            && m_corona->viewSettingsFactory()->hasVisibleSettings()
            && m_corona->viewSettingsFactory()->lastContainment() == containment()) {
            //! used mostly from view recreations in order to inform config windows that view has been updated
            m_primaryConfigView = m_corona->viewSettingsFactory()->primaryConfigView();
            m_primaryConfigView->setParentView(this, true);
        }

        Q_EMIT containmentActionsChanged();
    }, Qt::DirectConnection);

    if (m_corona) {
        connect(m_corona, &Latte::Corona::viewLocationChanged, this, &View::dockLocationChanged);
    }

    addLatteQmlImportPaths(engine().get());
}

View::~View()
{
    m_inDelete = true;

    qApp->removeEventFilter(this);

    //! Drop pointer-window tracking connections before member teardown. Each
    //! tracked popup/submenu window has a destroyed -> onPointerWindowDestroyed
    //! connection to this View (created in eventFilter on Enter). If such a window
    //! dies while the View is being destroyed, Qt activates the slot against the
    //! half-destroyed View and aborts on its internal qobject_cast assertion.
    //! disconnectSensitiveSignals() does not cover these per-window connections.
    for (QWindow *window : m_pointerWindows) {
        disconnect(window, &QObject::destroyed, this, &View::onPointerWindowDestroyed);
    }

    m_pointerWindows.clear();

    //! clear Layout connections
    m_visibleHackTimer1.stop();
    m_visibleHackTimer2.stop();

    for (auto &c : connectionsLayout) {
        disconnect(c);
    }

    //! unload indicators
    if (m_indicator) {
        m_indicator->unloadIndicators();
    }

    disconnectSensitiveSignals();
    disconnect(containment(), &Plasma::Applet::statusChanged, this, &View::statusChanged);

    qCDebug(latteView) << "dock view deleting...";

    //! this disconnect does not free up connections correctly when
    //! latteView is deleted. A crash for this example is the following:
    //! switch to Alternative Session and disable compositing,
    //! the signal creating the crash was probably from deleted
    //! windows.
    //! this->disconnect();

    if (m_primaryConfigView && m_corona->inQuit()) {
        //! delete only when application is quitting
        delete m_primaryConfigView;
    }

    if (m_appletConfigView) {
        delete m_appletConfigView;
    }

    //needs to be deleted before Effects because it catches some of its signals
    if (m_positioner) {
        delete m_positioner;
    }

    if (m_effects) {
        delete m_effects;
    }

    if (m_indicator) {
        delete m_indicator;
    }

    if (m_interface) {
        delete m_interface;
    }

    if (m_visibility) {
        delete m_visibility;
    }

    if (m_windowsTracker) {
        delete m_windowsTracker;
    }
}

void View::init(Plasma::Containment *plasma_containment)
{
    connect(this, &QQuickWindow::xChanged, this, &View::geometryChanged);
    connect(this, &QQuickWindow::yChanged, this, &View::geometryChanged);
    connect(this, &QQuickWindow::widthChanged, this, [this](int) { Q_EMIT geometryChanged(); });

    connect(this, &QQuickWindow::heightChanged, this, [this](int) { Q_EMIT geometryChanged(); });

    connect(this, &QQuickWindow::xChanged, this, &View::xChanged);
    connect(this, &QQuickWindow::xChanged, this, [this](int) { updateAbsoluteGeometry(); });

    connect(this, &QQuickWindow::yChanged, this, &View::yChanged);
    connect(this, &QQuickWindow::yChanged, this, [this](int) { updateAbsoluteGeometry(); });

    connect(this, &QQuickWindow::widthChanged, this, &View::widthChanged);
    connect(this, &QQuickWindow::widthChanged, this, [this](int) { updateAbsoluteGeometry(); });

    connect(this, &QQuickWindow::heightChanged, this, &View::heightChanged);
    connect(this, &QQuickWindow::heightChanged, this, [this](int) { updateAbsoluteGeometry(); });

    connect(this, &View::fontPixelSizeChanged, this, &View::editThicknessChanged);
    connect(this, &View::maxNormalThicknessChanged, this, &View::editThicknessChanged);

    connect(this, &View::activitiesChanged, this, &View::applyActivitiesToWindows);
    connect(m_positioner, &ViewPart::Positioner::winIdChanged, this, &View::applyActivitiesToWindows);
    connect(m_positioner, &ViewPart::Positioner::winIdChanged, this, [this]() {
        if (!m_visibility || !m_corona || !m_corona->wm()) {
            return;
        }

        const bool onFrontLayer = !m_visibility->isBelowLayer();
        const auto mode = onFrontLayer ? m_visibility->mode() : Types::WindowsAlwaysCover;
        m_corona->wm()->setViewExtraFlags(this, onFrontLayer, mode);
    });

    connect(this, &View::alignmentChanged, this, [this]() {
        // inform neighbour vertical docks to adjust their positioning
        if (m_inDelete || formFactor() == Plasma::Types::Vertical) {
            return;
        }

        Q_EMIT availableScreenRectChangedFrom(this);
        Q_EMIT availableScreenRegionChangedFrom(this);
    });

    connect(this, &View::maxLengthChanged, this, [this]() {
        if (m_inDelete) {
            return;
        }

        Q_EMIT availableScreenRectChangedFrom(this);
        Q_EMIT availableScreenRegionChangedFrom(this);
    });

    connect(this, &View::offsetChanged, this, [this]() {
        if (m_inDelete) {
            return;
        }

        Q_EMIT availableScreenRectChangedFrom(this);
        Q_EMIT availableScreenRegionChangedFrom(this);
    });

    connect(this, &View::localGeometryChanged, this, [this]() {
        updateAbsoluteGeometry();
    });

    connect(this, &View::screenEdgeMarginEnabledChanged, this, [this]() {
        updateAbsoluteGeometry();
    });

    //! used in order to disconnect it when it should NOT be called because it creates crashes
    connect(this, &View::availableScreenRectChangedFrom, m_corona, &Latte::Corona::availableScreenRectChangedFrom);
    connect(this, &View::availableScreenRegionChangedFrom, m_corona, &Latte::Corona::availableScreenRegionChangedFrom);
    connect(m_corona, &Latte::Corona::availableScreenRectChangedFrom, this, &View::availableScreenRectChangedFromSlot);
    connect(m_corona, &Latte::Corona::verticalUnityViewHasFocus, this, &View::topViewAlwaysOnTop);

    connect(this, &View::isPreferredForShortcutsChanged, this, &View::saveConfig);
    connect(this, &View::nameChanged, this, &View::saveConfig);
    connect(this, &View::onPrimaryChanged, this, &View::saveConfig);
    connect(this, &View::typeChanged, this, &View::saveConfig);

    connect(this, &View::normalThicknessChanged, this, [this]() {
        Q_EMIT availableScreenRectChangedFrom(this);
    });

    connect(m_effects, &ViewPart::Effects::innerShadowChanged, this, [this]() {
        Q_EMIT availableScreenRectChangedFrom(this);
    });

    connect(m_positioner, &ViewPart::Positioner::onHideWindowsForSlidingOut, this, &View::hideWindowsForSlidingOut);
    connect(m_positioner, &ViewPart::Positioner::screenGeometryChanged, this, &View::screenGeometryChanged);
    connect(m_positioner, &ViewPart::Positioner::windowSizeChanged, this, [this]() {
        Q_EMIT availableScreenRectChangedFrom(this);
    });

    connect(m_interface, &ViewPart::ContainmentInterface::hasExpandedAppletChanged, this, &View::verticalUnityViewHasFocus);

    //! View sends this signal in order to avoid crashes from ViewPart::Indicator when the view is recreated
    connect(m_corona->indicatorFactory(), &Latte::Indicator::Factory::indicatorChanged, this, [this](const QString & indicatorId) {
        Q_EMIT indicatorPluginChanged(indicatorId);
    });

    connect(this, &View::indicatorPluginChanged, this, [this](const QString & indicatorId) {
        if (m_indicator && m_indicator->isCustomIndicator() && m_indicator->type() == indicatorId) {
            reloadSource();
        }
    });

    connect(m_corona->indicatorFactory(), &Latte::Indicator::Factory::indicatorRemoved, this, &View::indicatorPluginRemoved);

    //! Assign app interfaces so both containment object and its graphic item can expose them.
    auto setLatteInterfaceProperties = [this](QObject * obj) {
        if (!obj) {
            return;
        }

        obj->setProperty("_latte_globalShortcuts_object", QVariant::fromValue(m_corona->globalShortcuts()->shortcutsTracker()));
        obj->setProperty("_latte_layoutsManager_object", QVariant::fromValue(m_corona->layoutsManager()));
        obj->setProperty("_latte_themeExtended_object", QVariant::fromValue(m_corona->themeExtended()));
        obj->setProperty("_latte_universalSettings_object", QVariant::fromValue(m_corona->universalSettings()));
        obj->setProperty("_latte_view_object", QVariant::fromValue(this));
    };

    setLatteInterfaceProperties(plasma_containment);

    QQuickItem *containmentGraphicItem = qobject_cast<QQuickItem *>(plasma_containment->property("_plasma_graphicObject").value<QObject *>());

    if (containmentGraphicItem) {
        setLatteInterfaceProperties(containmentGraphicItem);

        Latte::Interfaces *ifacesGraphicObject = qobject_cast<Latte::Interfaces *>(containmentGraphicItem->property("_latte_view_interfacesobject").value<QObject *>());

        if (ifacesGraphicObject) {
            ifacesGraphicObject->updateView();
            setInterfacesGraphicObj(ifacesGraphicObject);
        }
    }

    setSource(QUrl::fromLocalFile(corona()->kPackage().filePath("lattedockui")));

    //! immediateSyncGeometry helps avoiding binding loops from containment qml side
    m_positioner->immediateSyncGeometry();

    qCDebug(latteView) << "SOURCE:" << source();
}

void View::reloadSource()
{
    if (m_layout && containment()) {
        engine()->clearComponentCache();
        m_layout->recreateView(containment(), settingsWindowIsShown());
    }
}

bool View::inDelete() const
{
    return m_inDelete;
}

bool View::inReadyState() const
{
    return (m_layout != nullptr);
}

void View::disconnectSensitiveSignals()
{
    m_initLayoutTimer.stop();

    disconnect(this, &View::availableScreenRectChangedFrom, m_corona, &Latte::Corona::availableScreenRectChangedFrom);
    disconnect(this, &View::availableScreenRegionChangedFrom, m_corona, &Latte::Corona::availableScreenRegionChangedFrom);
    disconnect(m_corona, &Latte::Corona::availableScreenRectChangedFrom, this, &View::availableScreenRectChangedFromSlot);
    disconnect(m_corona, &Latte::Corona::verticalUnityViewHasFocus, this, &View::topViewAlwaysOnTop);

    setLayout(nullptr);
}

void View::availableScreenRectChangedFromSlot(View *origin)
{
    if (m_inDelete || origin == this || !origin) {
        return;
    }

    if (formFactor() == Plasma::Types::Vertical
        && origin->formFactor() == Plasma::Types::Horizontal //! accept only horizontal views
        && !(origin->location() == Plasma::Types::TopEdge && m_positioner->isStickedOnTopEdge()) //! ignore signals in such case
        && !(origin->location() == Plasma::Types::BottomEdge && m_positioner->isStickedOnBottomEdge()) //! ignore signals in such case
        && origin->layout()
        && m_layout
        && origin->layout()->lastUsedActivity() == m_layout->lastUsedActivity()) {
        //! must be in same activity
        m_positioner->syncGeometry();
    }
}

void View::setupWaylandIntegration()
{
    if (m_shellSurface)
        return;

    if (Latte::Corona *c = qobject_cast<Latte::Corona *>(corona())) {
        using namespace KWayland::Client;
        PlasmaShell *interface {c->waylandCoronaInterface()};

        if (!interface)
            return;

        Surface *s{Surface::fromWindow(this)};

        if (!s)
            return;

        m_shellSurface = interface->createSurface(s, this);
        qCDebug(latteView) << "WAYLAND dock window surface was created...";

        if (m_visibility) {
            m_visibility->initViewFlags();
        }

        if (m_positioner) {
            m_positioner->updateWaylandId();
        }
    }
}

KWayland::Client::PlasmaShellSurface *View::surface()
{
    return m_shellSurface;
}

//! the main function which decides if this dock is at the
//! correct screen
void View::reconsiderScreen()
{
    m_positioner->reconsiderScreen();
}

void View::duplicateView()
{
    QString storedTmpViewFilepath = m_layout->storedView(containment()->id());
    newView(storedTmpViewFilepath);
}

void View::exportTemplate()
{
    Latte::Settings::Dialog::ExportTemplateDialog *exportDlg = new Latte::Settings::Dialog::ExportTemplateDialog(this);
    exportDlg->show();
}

void View::newView(const QString &templateFile)
{
    if (templateFile.isEmpty() || !m_layout) {
        return;
    }

    Data::ViewsTable templateviews = Layouts::Storage::self()->views(templateFile);

    if (templateviews.rowCount() <= 0) {
        return;
    }

    Data::View nextdata = templateviews[0];
    int scrId = onPrimary() ? m_corona->screenPool()->primaryScreenId() : m_positioner->currentScreenId();

    QList<Plasma::Types::Location> freeedges = m_layout->freeEdges(scrId);

    if (!freeedges.contains(nextdata.edge)) {
        nextdata.edge = (freeedges.count() > 0 ? freeedges[0] : Plasma::Types::BottomEdge);
    }

    nextdata.setState(Data::View::OriginFromViewTemplate, templateFile);

    m_layout->newView(nextdata);
}

void View::removeView()
{
    if (m_layout) {
        QAction *removeAct = action(QStringLiteral("remove"));

        if (removeAct) {
            removeAct->trigger();
        }
    }
}

bool View::settingsWindowIsShown()
{
    return m_primaryConfigView && (m_primaryConfigView->parentView() == this) && m_primaryConfigView->isVisible();
}

void View::showSettingsWindow()
{
    if (!settingsWindowIsShown()) {
        Q_EMIT m_visibility->mustBeShown();
        showConfigurationInterface(containment());
        applyActivitiesToWindows();
    }
}

QQuickView *View::configView()
{
    return m_primaryConfigView.data();
}

void View::showConfigurationInterface(Plasma::Applet *applet)
{
    if (!applet || !applet->containment())
        return;

    Plasma::Containment *c = qobject_cast<Plasma::Containment *>(applet);

    if (m_primaryConfigView && c && c->isContainment() && c == this->containment()) {
        if (m_primaryConfigView->isVisible()) {
            m_primaryConfigView->hideConfigWindow();
        } else {
            m_primaryConfigView->showConfigWindow();
            applyActivitiesToWindows();
        }

        return;
    } else if (m_appletConfigView) {
        if (m_appletConfigView->applet() == applet) {
            m_appletConfigView->show();
            m_appletConfigView->requestActivate();
            return;
        } else {
            m_appletConfigView->hide();
        }
    }

    if (c && containment() && c->isContainment() && c->id() == containment()->id()) {
        m_primaryConfigView = m_corona->viewSettingsFactory()->primaryConfigView(this);
        applyActivitiesToWindows();
    } else {
        m_appletConfigView = new PlasmaQuick::ConfigView(applet);
        m_appletConfigView.data()->init();

        //! center applet config window
        m_appletConfigView->setScreen(screen());
        QRect scrgeometry = screenGeometry();
        QPoint position{scrgeometry.center().x() - m_appletConfigView->width() / 2, scrgeometry.center().y() - m_appletConfigView->height() / 2 };
        //!under wayland probably needs another workaround
        m_appletConfigView->setPosition(position);

        m_appletConfigView->show();
    }
}

void View::showWidgetExplorer(const QPointF &point)
{
    Q_UNUSED(point);

    auto widgetExplorerView = m_corona->viewSettingsFactory()->widgetExplorerView(this);

    if (!widgetExplorerView->isVisible()) {
        widgetExplorerView->showAfter(kWidgetExplorerShowDelayMs);
    }
}

QRect View::localGeometry() const
{
    return m_localGeometry;
}

void View::setLocalGeometry(const QRect &geometry)
{
    if (m_localGeometry == geometry) {
        return;
    }

    m_localGeometry = geometry;
    Q_EMIT localGeometryChanged();
}


QString View::name() const
{
    return m_name;
}

void View::setName(const QString &newname)
{
    if (m_name == newname) {
        return;
    }

    m_name = newname;
    Q_EMIT nameChanged();
}

QString View::validTitle() const
{
    if (!containment()) {
        return QString();
    }

    return QStringLiteral("#view#") + QString::number(containment()->id());
}

void View::updateAbsoluteGeometry(bool bypassChecks)
{
    //! there was a -1 in height and width here. The reason of this
    //! if I remember correctly was related to multi-screen but I cant
    //! remember exactly the reason, something related to right edge in
    //! multi screen environment. BUT this was breaking the entire AlwaysVisible
    //! experience with struts. Removing them in order to restore correct
    //! behavior and keeping this comment in order to check for
    //! multi-screen breakage
    QRect absGeometry = m_localGeometry;
    absGeometry.moveLeft(x() + m_localGeometry.x());
    absGeometry.moveTop(y() + m_localGeometry.y());

    if (m_absoluteGeometry == absGeometry && !bypassChecks) {
        return;
    }

    if (m_absoluteGeometry != absGeometry) {
        m_absoluteGeometry = absGeometry;
        Q_EMIT absoluteGeometryChanged(m_absoluteGeometry);
    }

    if ((m_absoluteGeometry != absGeometry) || bypassChecks) {
        //! inform others such as neighbour vertical views that new geometries are applied
        //! main use of BYPASSCKECKS is from Positioner when the view changes screens
        Q_EMIT availableScreenRectChangedFrom(this);
        Q_EMIT availableScreenRegionChangedFrom(this);
    }
}

void View::statusChanged(Plasma::Types::ItemStatus status)
{
    if (!containment()) {
        return;
    }

    //! Fix for #443236, following setFlags(...) need to be added at all three cases
    //! but initViewFlags() should be called afterwards because setFlags(...) breaks
    //! the dock window default behavior during status transitions.
    if (status == Plasma::Types::NeedsAttentionStatus) {
        //! The Application Menu widget (org.kde.plasma.appmenu) reports
        //! NeedsAttentionStatus while its menu bar menu is open.  As with
        //! RequiresAttentionStatus below, we must not reconfigure the window
        //! flags or the plasma shell surface at that point: on Wayland that
        //! invalidates the QMenu's child xdg_popup surfaces and the open menu
        //! (and its cascading submenus) closes as soon as the pointer reaches
        //! them.  Only block hiding so the dock cannot retract underneath the
        //! menu.  The no-focus flags are already applied in the normal-status
        //! branch and get re-applied there once the menu closes.
        m_visibility->addBlockHidingEvent(BLOCKHIDINGNEEDSATTENTIONTYPE);
    } else if (status == Plasma::Types::RequiresAttentionStatus) {
        // When an applet popup opens, we must not reconfigure the window flags
        // (e.g. WindowDoesNotAcceptFocus) because on Wayland this can invalidate
        // child xdg_popup surfaces, causing the popup to close immediately.
        // Only block hiding — the focus and layer flags stay as they were.
        m_visibility->addBlockHidingEvent(BLOCKHIDINGNEEDSATTENTIONTYPE);
    } else if (status == Plasma::Types::AcceptingInputStatus) {
        m_visibility->removeBlockHidingEvent(BLOCKHIDINGNEEDSATTENTIONTYPE);
        setFlags(flags() & ~Qt::WindowDoesNotAcceptFocus);
        m_visibility->initViewFlags();

        if (m_shellSurface) {
            m_shellSurface->setPanelTakesFocus(true);
        }
    } else {
        updateTransientWindowsTracking();
        m_visibility->removeBlockHidingEvent(BLOCKHIDINGNEEDSATTENTIONTYPE);
        setFlags(flags() | Qt::WindowDoesNotAcceptFocus);
        m_visibility->initViewFlags();

        if (m_shellSurface) {
            m_shellSurface->setPanelTakesFocus(false);
        }
    }
}

void View::addTransientWindow(QWindow *window)
{
    if (!m_transientWindows.contains(window) && !window->flags().testFlag(Qt::ToolTip) && !window->title().startsWith(QStringLiteral("#debugwindow#"))) {
        m_transientWindows.append(window);

        QString winPtrStr = QStringLiteral("0x") + QString::number((qulonglong)window, 16);
        m_visibility->addBlockHidingEvent(winPtrStr);

        if (m_visibility->hasBlockHidingEvent(Latte::GlobalShortcuts::SHORTCUTBLOCKHIDINGTYPE)) {
            m_visibility->removeBlockHidingEvent(Latte::GlobalShortcuts::SHORTCUTBLOCKHIDINGTYPE);
        }

        connect(window, &QWindow::visibleChanged, this, &View::removeTransientWindow);
    }
}

void View::removeTransientWindow(const bool &visible)
{
    QWindow *window = static_cast<QWindow *>(QObject::sender());

    if (window && !visible) {
        QString winPtrStr = QStringLiteral("0x") + QString::number((qulonglong)window, 16);
        m_visibility->removeBlockHidingEvent(winPtrStr);
        disconnect(window, &QWindow::visibleChanged, this, &View::removeTransientWindow);
        m_transientWindows.removeAll(window);

        if (m_visibility->hasBlockHidingEvent(Latte::GlobalShortcuts::SHORTCUTBLOCKHIDINGTYPE)) {
            m_visibility->removeBlockHidingEvent(Latte::GlobalShortcuts::SHORTCUTBLOCKHIDINGTYPE);
        }

        updateTransientWindowsTracking();
    }
}

void View::updateTransientWindowsTracking()
{
    for (QWindow *window : qApp->topLevelWindows()) {
        if (window->transientParent() == this && window->isVisible()) {
            addTransientWindow(window);
            break;
        }
    }
}

bool View::eventFilter(QObject *watched, QEvent *event)
{
    //! Event-driven cascading-menu close.  QCursor::pos() is stale on Wayland
    //! once the pointer leaves this process's surfaces, so the pointer window
    //! is tracked purely through Enter/Leave events.
    //
    //! Rules:
    //!  - the first-level menu (the applet popup) stays open on pointer-leave;
    //!  - a cascading submenu closes only when the pointer leaves the whole
    //!    menu tree (no popup window of the dock has the pointer anymore),
    //!    so moving from level N into level N+1 never closes level N.
    if (auto *window = qobject_cast<QWindow *>(watched)) {
        if (windowBelongsToThisDock(window)) {
            if (event->type() == QEvent::Enter) {
                if (!m_pointerWindows.contains(window)) {
                    m_pointerWindows.insert(window);
                    //! Keep the set free of dangling pointers: a popup /
                    //! submenu window can be destroyed while it is being
                    //! tracked (e.g. kicker closes its submenu), and the
                    //! deferred close check below must never dereference it.
                    connect(window, &QObject::destroyed, this, &View::onPointerWindowDestroyed, Qt::UniqueConnection);
                }
            } else if (event->type() == QEvent::Leave) {
                m_pointerWindows.remove(window);

                //! Mirror the Enter-side connect: a destroyed -> onPointerWindowDestroyed
                //! connection must never outlive its tracked-window membership. Leaving a
                //! window removes it from the set, so its connection is dropped here too;
                //! otherwise the dock's own window (which is also tracked while the pointer
                //! hovers it) keeps a self-connection alive that fires from ~QObject's
                //! destroyed() emission during teardown and aborts on Qt's internal
                //! qobject_cast assertion against the half-destroyed View.
                disconnect(window, &QObject::destroyed, this, &View::onPointerWindowDestroyed);

                //! Only submenu windows (transient children of an applet
                //! popup) can trigger the close; the first-level popup and
                //! the dock itself keep the menu open on pointer-leave.
                QWindow *popupWindow = window->transientParent();

                if (popupWindow && popupWindow != this) {
                    if (auto *popup = qobject_cast<PlasmaQuick::AppletPopup *>(popupWindow)) {
                        QQuickItem *appletInterface = popup->appletInterface();

                        if (appletInterface) {
                            //! Defer so a following Enter (moving into a deeper
                            //! level) can cancel the close.
                            QPointer<QObject> appletGuard(appletInterface);
                            QTimer::singleShot(0, this, [this, appletGuard]() {
                                const bool anyPopupUnderPointer = std::any_of(m_pointerWindows.cbegin(),
                                                                              m_pointerWindows.cend(),
                                                                              [this](QWindow *w) {
                                                                                  return w != this && windowBelongsToThisDock(w);
                                                                              });

                                if (!anyPopupUnderPointer && appletGuard) {
                                    QMetaObject::invokeMethod(appletGuard.data(), "reset", Qt::QueuedConnection);
                                }
                            });
                        }
                    }
                }
            }
        }
    }

    return QObject::eventFilter(watched, event);
}

bool View::windowBelongsToThisDock(QWindow *window) const
{
    QWindow *w = window;

    while (w) {
        if (w == this) {
            return true;
        }

        w = w->transientParent();
    }

    return false;
}

void View::onPointerWindowDestroyed(QObject *window)
{
    //! Remove a tracked window as soon as it is destroyed so the deferred
    //! close check never dereferences a dangling pointer.
    if (auto *w = qobject_cast<QWindow *>(window)) {
        m_pointerWindows.remove(w);
    }
}

Types::ViewType View::type() const
{
    return m_type;
}

void View::setType(Types::ViewType type)
{
    if (m_type == type) {
        return;
    }

    m_type = type;
    Q_EMIT typeChanged();
}

bool View::alternativesIsShown() const
{
    return m_alternativesIsShown;
}

void View::setAlternativesIsShown(bool show)
{
    if (m_alternativesIsShown == show) {
        return;
    }

    m_alternativesIsShown = show;

    Q_EMIT alternativesIsShownChanged();
}

bool View::containsDrag() const
{
    return m_containsDrag;
}

void View::setContainsDrag(bool contains)
{
    if (m_containsDrag == contains) {
        return;
    }

    m_containsDrag = contains;


    if (m_containsDrag) {
        m_visibility->addBlockHidingEvent(BLOCKHIDINGDRAGTYPE);
    } else {
        m_visibility->removeBlockHidingEvent(BLOCKHIDINGDRAGTYPE);
    }

    Q_EMIT containsDragChanged();
}

bool View::containsMouse() const
{
    return m_containsMouse;
}

int View::normalThickness() const
{
    return m_normalThickness;
}

void View::setNormalThickness(int thickness)
{
    if (m_normalThickness == thickness) {
        return;
    }

    m_normalThickness = thickness;
    Q_EMIT normalThicknessChanged();
}

int View::maxNormalThickness() const
{
    return m_maxNormalThickness;
}

void View::setMaxNormalThickness(int thickness)
{
    if (m_maxNormalThickness == thickness) {
        return;
    }

    m_maxNormalThickness = thickness;
    Q_EMIT maxNormalThicknessChanged();
}

int View::headThicknessGap() const
{
    return m_headThicknessGap;
}

void View::setHeadThicknessGap(int thickness)
{
    if (m_headThicknessGap == thickness) {
        return;
    }

    m_headThicknessGap = thickness;
    Q_EMIT headThicknessGapChanged();
}

bool View::inEditMode() const
{
    return containment() && containment()->isUserConfiguring();
}

bool View::isPreferredForShortcuts() const
{
    return m_isPreferredForShortcuts;
}

void View::setIsPreferredForShortcuts(bool preferred)
{
    if (m_isPreferredForShortcuts == preferred) {
        return;
    }

    m_isPreferredForShortcuts = preferred;

    Q_EMIT isPreferredForShortcutsChanged();

    if (m_isPreferredForShortcuts && m_layout) {
        Q_EMIT m_layout->preferredViewForShortcutsChanged(this);
    }
}

bool View::inSettingsAdvancedMode() const
{
    return m_primaryConfigView && m_corona->universalSettings()->inAdvancedModeForEditSettings();
}

bool View::isTouchingBottomViewAndIsBusy() const
{
    return m_isTouchingBottomViewAndIsBusy;
}

void View::setIsTouchingBottomViewAndIsBusy(bool touchAndBusy)
{
    if (m_isTouchingBottomViewAndIsBusy == touchAndBusy) {
        return;
    }

    m_isTouchingBottomViewAndIsBusy = touchAndBusy;

    Q_EMIT isTouchingBottomViewAndIsBusyChanged();
}

bool View::isTouchingTopViewAndIsBusy() const
{
    return m_isTouchingTopViewAndIsBusy;
}

void View::setIsTouchingTopViewAndIsBusy(bool touchAndBusy)
{
    if (m_isTouchingTopViewAndIsBusy == touchAndBusy) {
        return;
    }

    m_isTouchingTopViewAndIsBusy = touchAndBusy;
    Q_EMIT isTouchingTopViewAndIsBusyChanged();
}

void View::preferredViewForShortcutsChangedSlot(Latte::View *view)
{
    if (view != this) {
        setIsPreferredForShortcuts(false);
    }
}

bool View::onPrimary() const
{
    return m_onPrimary;
}

void View::setOnPrimary(bool flag)
{
    if (m_onPrimary == flag) {
        return;
    }

    m_onPrimary = flag;
    Q_EMIT onPrimaryChanged();
}

int View::groupId() const
{
    if (!containment()) {
        return -1;
    }

    return containment()->id();
}

float View::maxLength() const
{
    return m_maxLength;
}

void View::setMaxLength(float length)
{
    if (m_maxLength == length) {
        return;
    }

    m_maxLength = length;
    Q_EMIT maxLengthChanged();
}

int View::editThickness() const
{
    constexpr int kHeaderSpacingMultiplier = 2;
    constexpr int kTotalSpacingMultiplier = 6;

    int ruler_height{m_fontPixelSize};
    int header_height{m_fontPixelSize + kHeaderSpacingMultiplier * kEditThicknessSmallSpacing};

    return m_maxNormalThickness + ruler_height + header_height + kTotalSpacingMultiplier * kEditThicknessSmallSpacing;
}

int View::maxThickness() const
{
    return m_maxThickness;
}

void View::setMaxThickness(int thickness)
{
    if (m_maxThickness == thickness)
        return;

    m_maxThickness = thickness;
    Q_EMIT maxThicknessChanged();
}

int View::alignment() const
{
    return m_alignment;
}

void View::setAlignment(int alignment)
{
    Types::Alignment align = static_cast<Types::Alignment>(alignment);

    if (m_alignment == alignment) {
        return;
    }

    m_alignment = align;
    Q_EMIT alignmentChanged();
}

QRect View::absoluteGeometry() const
{
    return m_absoluteGeometry;
}

QRect View::screenGeometry() const
{
    if (this->screen()) {
        QRect geom = this->screen()->geometry();
        return geom;
    }

    return QRect();
}

float View::offset() const
{
    return m_offset;
}

void View::setOffset(float offset)
{
    if (m_offset == offset) {
        return;
    }

    m_offset = offset;
    Q_EMIT offsetChanged();
}

bool View::screenEdgeMarginEnabled() const
{
    return m_screenEdgeMarginEnabled;
}

void View::setScreenEdgeMarginEnabled(bool enabled)
{
    if (m_screenEdgeMarginEnabled == enabled) {
        return;
    }

    m_screenEdgeMarginEnabled = enabled;
    Q_EMIT screenEdgeMarginEnabledChanged();
}

int View::screenEdgeMargin() const
{
    return m_screenEdgeMargin;
}

void View::setScreenEdgeMargin(int margin)
{
    if (m_screenEdgeMargin == margin) {
        return;
    }



    m_screenEdgeMargin = margin;
    Q_EMIT screenEdgeMarginChanged();
}

int View::fontPixelSize() const
{
    return m_fontPixelSize;
}

void View::setFontPixelSize(int size)
{
    if (m_fontPixelSize == size) {
        return;
    }

    m_fontPixelSize = size;

    Q_EMIT fontPixelSizeChanged();
}

bool View::isOnAllActivities() const
{
    return m_activities.isEmpty() || m_activities[0] == Data::Layout::ALLACTIVITIESID;
}

bool View::isOnActivity(const QString &activity) const
{
    return isOnAllActivities() || m_activities.contains(activity);
}

QStringList View::activities() const
{
    QStringList running;

    QStringList runningAll = m_corona->activitiesConsumer()->activities();

    for (int i = 0; i < m_activities.count(); ++i) {
        if (runningAll.contains(m_activities[i])) {
            running << m_activities[i];
        }
    }

    return running;
}

void View::setActivities(const QStringList &ids)
{
    if (m_activities == ids) {
        return;
    }

    m_activities = ids;
    Q_EMIT activitiesChanged();
}

void View::applyActivitiesToWindows()
{
    if (m_visibility && m_positioner && m_layout) {
        QStringList runningActivities = activities();

        m_positioner->setWindowOnActivities(m_positioner->trackedWindowId(), runningActivities);

        //! config windows
        if (m_primaryConfigView) {
            m_primaryConfigView->setOnActivities(runningActivities);
        }

        if (m_appletConfigView) {
            const Latte::WindowSystem::WindowId appletconfigviewid = m_corona->wm()->winIdFor(App::preferredWaylandAppId(), m_appletConfigView->title());

            m_positioner->setWindowOnActivities(appletconfigviewid, runningActivities);
        }

        //! hidden windows
        if (m_visibility->supportsKWinEdges()) {
            m_visibility->applyActivitiesToHiddenWindows(runningActivities);
        }
    }
}

void View::showHiddenViewFromActivityStopping()
{
    if (m_layout && m_visibility && !inDelete() && !isVisible() && !m_visibility->isHidden()) {
        show();

        if (m_effects) {
            m_effects->updateEnabledBorders();
        }

        //qCDebug(latteView) << "View:: Enforce reshow from timer 1...";
        Q_EMIT forcedShown();
    } else if (m_layout && isVisible()) {
        m_inDelete = false;
        //qCDebug(latteView) << "View:: No needed reshow from timer 1...";
    }
}

Layout::GenericLayout *View::layout() const
{
    return m_layout;
}

void View::setLayout(Layout::GenericLayout *layout)
{
    if (m_layout == layout) {
        return;
    }

    // clear mode
    for (auto &c : connectionsLayout) {
        disconnect(c);
    }

    m_layout = layout;

    if (m_layout) {
        connectionsLayout << connect(containment(), &Plasma::Applet::destroyedChanged, m_layout, &Layout::GenericLayout::destroyedChanged);
        connectionsLayout << connect(containment(), &Plasma::Applet::locationChanged, m_corona, &Latte::Corona::viewLocationChanged);
        connectionsLayout << connect(containment(), &Plasma::Containment::appletAlternativesRequested, m_corona, &Latte::Corona::showAlternativesForApplet, Qt::QueuedConnection);

        if (m_corona->layoutsManager()->memoryUsage() == MemoryUsage::MultipleLayouts) {
            connectionsLayout << connect(containment(), &Plasma::Containment::appletCreated, m_layout, &Layout::GenericLayout::appletCreated);
        }

        connectionsLayout << connect(m_positioner, &Latte::ViewPart::Positioner::edgeChanged, m_layout, &Layout::GenericLayout::viewEdgeChanged);
        connectionsLayout << connect(m_layout, &Layout::GenericLayout::popUpMarginChanged, m_effects, &Latte::ViewPart::Effects::popUpMarginChanged);

        //! Sometimes the activity isn't completely ready, by adding a delay
        //! we try to catch up
        m_initLayoutTimer.setInterval(kInitLayoutIntervalMs);
        m_initLayoutTimer.setSingleShot(true);
        connectionsLayout << connect(&m_initLayoutTimer, &QTimer::timeout, this, [this]() {
            if (m_layout && m_visibility) {
                setActivities(m_layout->appliedActivities());
                qCDebug(latteView) << "DOCK VIEW FROM LAYOUT ::: " << m_layout->name() << " - activities: " << m_activities;
            }
        });

        m_initLayoutTimer.start();

        connectionsLayout << connect(m_layout, &Layout::GenericLayout::preferredViewForShortcutsChanged, this, &View::preferredViewForShortcutsChangedSlot);

        Latte::Corona *latteCorona = qobject_cast<Latte::Corona *>(this->corona());

        connectionsLayout << connect(latteCorona->activitiesConsumer(), &KActivities::Consumer::currentActivityChanged, this, [this]() {
            if (m_layout && m_visibility) {
                setActivities(m_layout->appliedActivities());
                //! update activities in case KWin did its magic and assigned windows to faulty activities
                applyActivitiesToWindows();
                showHiddenViewFromActivityStopping();
                qCDebug(latteView) << "DOCK VIEW FROM LAYOUT (currentActivityChanged) ::: " << m_layout->name() << " - activities: " << m_activities;
            }
        });

        if (latteCorona->layoutsManager()->memoryUsage() == MemoryUsage::MultipleLayouts) {
            connectionsLayout << connect(latteCorona->activitiesConsumer(), &KActivities::Consumer::activitiesChanged, this, [this]() {
                if (m_layout && m_visibility) {
                    setActivities(m_layout->appliedActivities());
                    qCDebug(latteView) << "DOCK VIEW FROM LAYOUT (runningActivitiesChanged) ::: " << m_layout->name()
                                       << " - activities: " << m_activities;
                }
            });

            connectionsLayout << connect(m_layout, &Layout::GenericLayout::activitiesChanged, this, [this]() {
                if (m_layout) {
                    setActivities(m_layout->appliedActivities());
                }
            });

            connectionsLayout << connect(latteCorona->layoutsManager()->synchronizer(), &Layouts::Synchronizer::layoutsChanged, this, [this]() {
                if (m_layout) {
                    setActivities(m_layout->appliedActivities());
                }
            });

            //! BEGIN OF KWIN HACK
            //! IMPORTANT ::: Fixing KWin Faulty Behavior that KWin hides ALL Views when an Activity stops
            //! with no reason!!

            m_visibleHackTimer1.setInterval(kVisibleHackTimer1Ms);
            m_visibleHackTimer2.setInterval(kVisibleHackTimer2Ms);
            m_visibleHackTimer1.setSingleShot(true);
            m_visibleHackTimer2.setSingleShot(true);

            connectionsLayout << connect(this, &QWindow::visibleChanged, this, [this]() {
                if (m_layout && !inDelete() && !isVisible() && !m_positioner->inLayoutUnloading()) {
                    m_visibleHackTimer1.start();
                    m_visibleHackTimer2.start();
                }
            });

            connectionsLayout << connect(&m_visibleHackTimer1, &QTimer::timeout, this, [this]() {
                applyActivitiesToWindows();
                showHiddenViewFromActivityStopping();
                Q_EMIT activitiesChanged();
            });

            connectionsLayout << connect(&m_visibleHackTimer2, &QTimer::timeout, this, [this]() {
                applyActivitiesToWindows();
                showHiddenViewFromActivityStopping();
                Q_EMIT activitiesChanged();
            });

            //! END OF KWIN HACK
        }

        Q_EMIT layoutChanged();
    } else {
        m_activities.clear();
    }
}

void View::hideWindowsForSlidingOut()
{
    if (m_primaryConfigView) {
        m_primaryConfigView->hideConfigWindow();
    }
}

//!check if the plasmoid with _name_ exists in the midedata
bool View::mimeContainsPlasmoid(QMimeData *mimeData, QString name)
{
    if (!mimeData) {
        return false;
    }

    if (mimeData->hasFormat(QStringLiteral("text/x-plasmoidservicename"))) {
        QString data = QString::fromUtf8(mimeData->data(QStringLiteral("text/x-plasmoidservicename")));
        const QStringList appletNames = data.split(QLatin1Char('\n'), Qt::SkipEmptyParts);

        for (const QString &appletName : appletNames) {
            if (appletName == name)
                return true;
        }
    }

    return false;
}

void View::updatePlasmoidDrag()
{
    // This slot is called via m_plasmoidDragTimer (0ms singleshot, deferred to next
    // event loop iteration).  Deferring avoids touching QQuickItems during Wayland
    // DragMove processing, which would break subsequent drag event delivery.
    if (!m_containsDrag || !m_plasmoidDragActive) {
        cleanupDndSpacer();
        return;
    }

    if (!m_interface || !m_interface->layoutManager()) {
        return;
    }

    QObject *lm = m_interface->layoutManager();
    QQuickItem *dndSpacer = lm->property("dndSpacerItem").value<QQuickItem *>();

    if (!dndSpacer) {
        return;
    }

    // Show dndSpacer at the insertion point — this pushes adjacent applet icons
    // apart and creates the wave/gap visual indicator.
    dndSpacer->setOpacity(1.0);

    // Map from window coords to rootItem coords for accurate positioning
    QQuickItem *rootItem = lm->property("rootItem").value<QQuickItem *>();
    QPointF mappedPos = (rootItem && contentItem())
                        ? rootItem->mapFromItem(contentItem(), m_lastPlasmoidDragPos)
                        : m_lastPlasmoidDragPos;

    QMetaObject::invokeMethod(lm, "insertAtCoordinates",
                              Q_ARG(QQuickItem *, dndSpacer),
                              Q_ARG(int, (int)mappedPos.x()),
                              Q_ARG(int, (int)mappedPos.y()));
}

void View::handlePlasmoidDrop(QDropEvent *de)
{
    // Drop is the terminal event in the drag sequence — synchronous QML access
    // is safe here because there are no subsequent DragMove events to lose.
    m_plasmoidDragTimer.stop();

    if (!m_interface) {
        return;
    }

    int eventx = (int)de->position().x();
    int eventy = (int)de->position().y();

    // Compute insertion index from dndSpacer position, then clean it up.
    // createApplet() returns the Applet* synchronously, and we call
    // LayoutManager::addAppletItem() directly with the insertion index.
    // The later Containment.onAppletAdded signal will be a no-op because
    // the applet container already exists.
    int dndIndex = -1;

    if (m_interface->layoutManager()) {
        QObject *lm = m_interface->layoutManager();
        QQuickItem *dndSpacer = lm->property("dndSpacerItem").value<QQuickItem *>();

        if (dndSpacer) {
            // Map from window coords to rootItem coords for accurate positioning
            QQuickItem *rootItem = lm->property("rootItem").value<QQuickItem *>();
            QPointF mappedPos = (rootItem && contentItem())
                                ? rootItem->mapFromItem(contentItem(), QPointF(eventx, eventy))
                                : QPointF(eventx, eventy);

            QMetaObject::invokeMethod(lm, "insertAtCoordinates",
                                      Q_ARG(QQuickItem *, dndSpacer),
                                      Q_ARG(int, (int)mappedPos.x()),
                                      Q_ARG(int, (int)mappedPos.y()));

            QMetaObject::invokeMethod(lm, "dndSpacerIndex", Q_RETURN_ARG(int, dndIndex));

            // Reset dndSpacer — we will insert via addAppletItem(index) directly
            dndSpacer->setOpacity(0.0);

            if (rootItem) {
                dndSpacer->setParentItem(rootItem);
            }
        }
    }

    // Set pending insertion index on the layout manager BEFORE createApplet().
    // The Containment.onAppletAdded signal fires during createApplet() with
    // default (0,0) coordinates.  addAppletItem checks for the pending index
    // and uses it instead, achieving position-aware insertion at the drop point.
    if (dndIndex >= 0 && m_interface->layoutManager()) {
        m_interface->layoutManager()->setProperty("_latte_pendingInsertionIndex", dndIndex);
    }

    // Create the applet.  The Containment.onAppletAdded handler in main.qml
    // calls addAppletItem() with default (0,0) but our pending index override
    // in addAppletItem(QObject*,int,int) redirects to addAppletItem(index).
    const QString data = QString::fromUtf8(de->mimeData()->data(QStringLiteral("text/x-plasmoidservicename")));
    const QStringList names = data.split(QLatin1Char('\n'), Qt::SkipEmptyParts);

    for (const QString &name : names) {
        if (auto *cont = containment()) {
            cont->createApplet(name);
        }
    }
}

void View::cleanupDndSpacer()
{
    if (!m_interface || !m_interface->layoutManager()) {
        return;
    }

    QObject *lm = m_interface->layoutManager();
    QQuickItem *dndSpacer = lm->property("dndSpacerItem").value<QQuickItem *>();

    if (dndSpacer) {
        dndSpacer->setOpacity(0.0);
        QQuickItem *rootItem = lm->property("rootItem").value<QQuickItem *>();

        if (rootItem) {
            dndSpacer->setParentItem(rootItem);
        }
    }
}

Latte::Data::View View::data() const
{
    Latte::Data::View vdata;
    vdata.id = QString::number(containment()->id());
    vdata.name = name();
    vdata.isActive = true;
    vdata.onPrimary = onPrimary();

    vdata.screen = containment()->screen();

    if (!Layouts::Storage::isValid(vdata.screen)) {
        vdata.screen = containment()->lastScreen();
    }

    vdata.screensGroup = screensGroup();

    //!screen edge margin can be more accurate in the config file
    vdata.screenEdgeMargin = m_screenEdgeMargin > 0 ? m_screenEdgeMargin : containment()->config().group(QStringLiteral("General")).readEntry(QStringLiteral("screenEdgeMargin"), (int) -1);

    vdata.edge = location();
    vdata.maxLength = m_maxLength * 100;
    vdata.alignment = m_alignment;
    vdata.subcontainments = Layouts::Storage::self()->subcontainments(layout(), containment());

    vdata.setState(Latte::Data::View::IsCreated);
    return vdata;
}

QQuickItem *View::colorizer() const
{
    return m_colorizer;
}

void View::setColorizer(QQuickItem *colorizer)
{
    if (m_colorizer == colorizer) {
        return;
    }

    m_colorizer = colorizer;
    Q_EMIT colorizerChanged();
}

QQuickItem *View::metrics() const
{
    return m_metrics;
}

void View::setMetrics(QQuickItem *metrics)
{
    if (m_metrics == metrics) {
        return;
    }

    m_metrics = metrics;
    Q_EMIT metricsChanged();
}

ViewPart::Effects *View::effects() const
{
    return m_effects;
}

ViewPart::Indicator *View::indicator() const
{
    return m_indicator;
}

ViewPart::ContainmentInterface *View::extendedInterface() const
{
    return m_interface;
}

ViewPart::Parabolic *View::parabolic() const
{
    return m_parabolic;
}

ViewPart::Positioner *View::positioner() const
{
    return m_positioner;
}

ViewPart::EventsSink *View::sink() const
{
    return m_sink;
}

ViewPart::VisibilityManager *View::visibility() const
{
    return m_visibility;
}

ViewPart::WindowsTracker *View::windowsTracker() const
{
    return m_windowsTracker;
}

Latte::Interfaces *View::interfacesGraphicObj() const
{
    return m_interfacesGraphicObj;
}

void View::setInterfacesGraphicObj(Latte::Interfaces *ifaces)
{
    if (m_interfacesGraphicObj == ifaces) {
        return;
    }

    m_interfacesGraphicObj = ifaces;

    if (containment()) {
        QQuickItem *containmentGraphicItem = qobject_cast<QQuickItem *>(containment()->property("_plasma_graphicObject").value<QObject *>());

        if (containmentGraphicItem) {
            containmentGraphicItem->setProperty("_latte_view_interfacesobject", QVariant::fromValue(m_interfacesGraphicObj));
        }
    }

    Q_EMIT interfacesGraphicObjChanged();
}

bool View::event(QEvent *e)
{
    // In edit mode, swallow all middle-button events so the plasmoid's
    // TaskMouseArea cannot process them.  Check every pointer / mouse event
    // type that may carry middle-button state.
    if (inEditMode()) {
        const auto t = e->type();

        if (t == QEvent::MouseButtonPress || t == QEvent::MouseButtonRelease
            || t == QEvent::MouseButtonDblClick || t == QEvent::MouseMove) {
            auto *me = static_cast<QMouseEvent *>(e);

            if (me->button() == Qt::MiddleButton
                || (t == QEvent::MouseMove && (me->buttons() & Qt::MiddleButton))) {
                e->accept();
                return true;
            }
        }

        if (t == QEvent::Pointer) {
            auto *spt = dynamic_cast<QSinglePointEvent *>(e);

            if (spt && (spt->button() == Qt::MiddleButton
                        || (spt->buttons() & Qt::MiddleButton))) {
                e->accept();
                return true;
            }
        }
    }

    QEvent *sunkevent = e;

    if (!m_inDelete) {
        Q_EMIT eventTriggered(e);

        bool sinkableevent{false};

        switch (e->type()) {
            case QEvent::Close:
                if (qApp->property("latte_session_ending").toBool()
                    || qEnvironmentVariableIntValue("LATTE_SESSION_ENDING") == 1) {
                    qInfo() << "[shutdown] Latte view close requested by compositor; quitting session shell.";
                    QMetaObject::invokeMethod(qGuiApp, &QCoreApplication::quit, Qt::QueuedConnection);
                }

                break;

            case QEvent::Enter:
                m_containsMouse = true;
                break;

            case QEvent::Leave:
                m_containsMouse = false;
                setContainsDrag(false);
                sinkableevent = true;
                break;

            case QEvent::DragEnter:
                setContainsDrag(true);
                sinkableevent = true;
                break;

            case QEvent::DragLeave:
                if (m_plasmoidDragActive) {
                    m_plasmoidDragActive = false;

                    // Restart timer to trigger dndSpacer cleanup in updatePlasmoidDrag
                    if (!m_plasmoidDragTimer.isActive()) {
                        m_plasmoidDragTimer.start();
                    }
                }

                setContainsDrag(false);
                break;

            case QEvent::DragMove:
                if (auto de = dynamic_cast<QDragMoveEvent *>(e)) {
                    if (de->mimeData()->hasFormat(QStringLiteral("text/x-plasmoidservicename"))) {
                        m_plasmoidDragActive = true;
                        m_lastPlasmoidDragPos = de->position();

                        if (!m_plasmoidDragTimer.isActive()) {
                            m_plasmoidDragTimer.start();
                        }
                    }
                }

                sinkableevent = true;
                break;

            case QEvent::Drop:
                if (auto de = dynamic_cast<QDropEvent *>(e)) {
                    if (de->mimeData()->hasFormat(QStringLiteral("text/x-plasmoidservicename"))) {
                        m_plasmoidDragActive = false;
                        handlePlasmoidDrop(de);
                    }
                }

                setContainsDrag(false);
                sinkableevent = true;
                break;

            case QEvent::MouseMove:
                sinkableevent = true;
                break;

            case QEvent::MouseButtonPress:
                if (auto me = dynamic_cast<QMouseEvent *>(e)) {
                    Q_EMIT mousePressed(me->pos(), me->button());
                    sinkableevent = true;
                    verticalUnityViewHasFocus();
                }

                break;

            case QEvent::MouseButtonRelease:
                if (auto me = dynamic_cast<QMouseEvent *>(e)) {
                    Q_EMIT mouseReleased(me->pos(), me->button());
                    sinkableevent = true;
                }

                break;

            case QEvent::PlatformSurface:
                if (auto pe = dynamic_cast<QPlatformSurfaceEvent *>(e)) {
                    switch (pe->surfaceEventType()) {
                        case QPlatformSurfaceEvent::SurfaceCreated:
                            setupWaylandIntegration();

                            if (m_shellSurface) {
                                //! immediateSyncGeometry helps avoiding binding loops from containment qml side
                                m_positioner->immediateSyncGeometry();
                                m_effects->updateShadows();
                            }

                            break;

                        case QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed:
                            if (m_shellSurface) {
                                delete m_shellSurface;
                                m_shellSurface = nullptr;
                                qCDebug(latteView) << "WAYLAND dock window surface was deleted...";
                                m_effects->clearShadows();
                            }

                            break;
                    }
                }

                break;

            case QEvent::Show:
                if (m_visibility) {
                    m_visibility->initViewFlags();
                }

                break;

            case QEvent::Wheel:
                if (auto we = dynamic_cast<QWheelEvent *>(e)) {
                    QPoint pos = we->position().toPoint();
                    Q_EMIT wheelScrolled(pos, we->angleDelta(), we->buttons());
                    sinkableevent = true;
                }

                break;

            default:
                break;
        }

        if (sinkableevent && m_sink->isActive()) {
            sunkevent = m_sink->onEvent(e);
        }
    }

    return ContainmentView::event(sunkevent);
}

void View::releaseConfigView()
{
    m_primaryConfigView = nullptr;
}

//! release grab and restore mouse state
void View::unblockMouse(int x, int y)
{
    setMouseGrabEnabled(false);

    m_releaseGrab_x = x;
    m_releaseGrab_y = y;
    m_releaseGrabTimer.start();
}

void View::releaseGrab()
{
    //! ungrab mouse
    if (mouseGrabberItem()) {
        mouseGrabberItem()->ungrabMouse();
    }

    // In Qt 6, mouseGrabberItem()->ungrabMouse() above handles release.
    // The setMouseGrabEnabled toggle was a Qt 5 workaround; removing it
    // avoids "cannot grab mouse: no event is currently being delivered".

    //! Send a fake QEvent::Leave to inform applets for mouse leaving the view
    const QPointF scenePos(-5, -5);
    const QPointF globalPos = mapToGlobal(scenePos.toPoint());
    const QPointF oldPos(m_releaseGrab_x, m_releaseGrab_y);
    QHoverEvent e(QEvent::Leave, scenePos, globalPos, oldPos);
    QCoreApplication::instance()->sendEvent(this, &e);
}

QAction *View::action(const QString &name)
{
    if (!containment()) {
        return nullptr;
    }

    return this->containment()->internalAction(name);
}

QVariantList View::containmentActions() const
{
    QVariantList actions;

    if (!containment()) {
        return actions;
    }

    const QString trigger = QStringLiteral("RightButton;NoModifier");
    Plasma::ContainmentActions *plugin = this->containment()->containmentActions().value(trigger);

    if (!plugin) {
        return actions;
    }

    if (plugin->containment() != this->containment()) {
        plugin->setContainment(this->containment());
        // now configure it
        KConfigGroup cfg(this->containment()->corona()->config(), QStringLiteral("ActionPlugins"));
        cfg = KConfigGroup(&cfg, QString::number(this->containment()->containmentType()));
        KConfigGroup pluginConfig = KConfigGroup(&cfg, trigger);
        plugin->restore(pluginConfig);
    }

    for (QAction *ac : plugin->contextualActions()) {
        actions << QVariant::fromValue<QAction *>(ac);
    }

    return actions;
}

bool View::isHighestPriorityView()
{
    if (m_layout) {
        return this == m_layout->highestPriorityView();
    }

    return false;
}

//! BEGIN: WORKAROUND order to force top panels always on top and above left/right panels
void View::topViewAlwaysOnTop()
{
    if (!m_visibility) {
        return;
    }

    if (location() == Plasma::Types::TopEdge
        && m_visibility->mode() != Latte::Types::WindowsCanCover
        && m_visibility->mode() != Latte::Types::WindowsAlwaysCover) {
        //! this is needed in order to preserve that the top dock will be above others.
        //! Unity layout paradigm is a good example for this. The top panel shadow
        //! should be always on top compared to left panel
        m_visibility->setViewOnFrontLayer();
    }
}

void View::verticalUnityViewHasFocus()
{
    if (formFactor() == Plasma::Types::Vertical
        && (y() != screenGeometry().y())
        && ((m_alignment == Latte::Types::Justify && m_maxLength == 1.0)
            || (m_alignment == Latte::Types::Top && m_offset == 0.0))) {
        Q_EMIT m_corona->verticalUnityViewHasFocus();
    }
}

//! END: WORKAROUND

//!BEGIN configuration functions
void View::saveConfig()
{
    if (!this->containment()) {
        return;
    }

    auto config = this->containment()->config();
    config.writeEntry(QStringLiteral("onPrimary"), onPrimary());
    config.writeEntry(QStringLiteral("isPreferredForShortcuts"), isPreferredForShortcuts());
    config.writeEntry(QStringLiteral("name"), m_name);
    config.writeEntry(QStringLiteral("viewType"), (int)m_type);
    config.sync();
}

void View::restoreConfig()
{
    if (!this->containment()) {
        return;
    }

    auto config = this->containment()->config();
    m_onPrimary = config.readEntry(QStringLiteral("onPrimary"), true);
    m_alignment = static_cast<Latte::Types::Alignment>(config.group(QStringLiteral("General")).readEntry(QStringLiteral("alignment"), (int)Latte::Types::Center));
    m_isPreferredForShortcuts = config.readEntry(QStringLiteral("isPreferredForShortcuts"), false);
    m_name = config.readEntry(QStringLiteral("name"), QString());

    //! Send changed signals at the end in order to be sure that saveConfig
    //! wont rewrite default/invalid values
    Q_EMIT alignmentChanged();
    Q_EMIT nameChanged();
    Q_EMIT onPrimaryChanged();
}

//!END configuration functions

}

//!END namespace
