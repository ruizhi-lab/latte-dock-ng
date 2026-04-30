/*
    SPDX-FileCopyrightText: 2016 Smith AR <audoban@openmailbox.org>
    SPDX-FileCopyrightText: 2016 Michail Vourlakos <mvourlakos@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "waylandinterface.h"

// local
#include "../apptypes.h"
#include <coretypes.h>
#include "../view/positioner.h"
#include "../view/view.h"
#include "../view/settings/subconfigview.h"
#include "../view/helpers/screenedgeghostwindow.h"
#include "../lattecorona.h"

// Qt
#include <QDebug>
#include <QTimer>
#include <QApplication>
#include <QQuickView>
// KDE
#include <KWindowSystem>
#include <KWayland/Client/plasmawindowmanagement.h>
#include <KWayland/Client/plasmashell.h>
#include <KWayland/Client/surface.h>
#include <KWayland/Client/plasmavirtualdesktop.h>


using namespace KWayland::Client;

namespace Latte {

namespace {

inline bool appIdMatches(const QString &windowAppId, const QString &requestedAppId)
{
    if (App::matchesSelfAppId(requestedAppId)) {
        return App::matchesSelfAppId(windowAppId);
    }

    return windowAppId == requestedAppId;
}

}

class Private::GhostWindow : public QQuickView
{
    Q_OBJECT

public:
    WindowSystem::WindowId m_winId;

    GhostWindow(WindowSystem::WaylandInterface *waylandInterface)
        : m_waylandInterface(waylandInterface) {
        setFlags(Qt::FramelessWindowHint
                 | Qt::WindowStaysOnTopHint
                 | Qt::NoDropShadowWindowHint
                 | Qt::WindowDoesNotAcceptFocus
                 | Qt::WindowTransparentForInput);

        setColor(QColor(Qt::transparent));

        connect(m_waylandInterface, &WindowSystem::AbstractWindowInterface::latteWindowAdded, this, &GhostWindow::identifyWinId);

        setupWaylandIntegration();
        show();
    }

    ~GhostWindow() {
        m_waylandInterface->unregisterIgnoredWindow(m_winId);
        delete m_shellSurface;
    }

    void setGeometry(const QRect &rect) {
        if (geometry() == rect) {
            return;
        }

        m_validGeometry = rect;

        setMinimumSize(rect.size());
        setMaximumSize(rect.size());
        resize(rect.size());

        m_shellSurface->setPosition(rect.topLeft());
    }

    void setupWaylandIntegration() {
        using namespace KWayland::Client;

        if (m_shellSurface)
            return;

        Surface *s{Surface::fromWindow(this)};

        if (!s)
            return;

        m_shellSurface = m_waylandInterface->waylandCoronaInterface()->createSurface(s, this);
        qDebug() << "wayland ghost window surface was created...";

        m_shellSurface->setSkipTaskbar(true);
        m_shellSurface->setPanelTakesFocus(false);
        m_shellSurface->setRole(PlasmaShellSurface::Role::Panel);
        m_shellSurface->setPanelBehavior(PlasmaShellSurface::PanelBehavior::AlwaysVisible);
    }

    KWayland::Client::PlasmaShellSurface *m_shellSurface{nullptr};
    WindowSystem::WaylandInterface *m_waylandInterface{nullptr};

    //! geometry() function under wayland does not return nice results
    QRect m_validGeometry;

public Q_SLOTS:
    void identifyWinId() {
        if (m_winId.isNull()) {
            m_winId = m_waylandInterface->winIdFor(App::preferredWaylandAppId(), m_validGeometry);
            m_waylandInterface->registerIgnoredWindow(m_winId);
        }
    }
};

namespace WindowSystem {

WaylandInterface::WaylandInterface(QObject *parent)
    : AbstractWindowInterface(parent)
{
    m_corona = qobject_cast<Latte::Corona *>(parent);
}

WaylandInterface::~WaylandInterface()
{
}

void WaylandInterface::init()
{
}

void WaylandInterface::initWindowManagement(KWayland::Client::PlasmaWindowManagement *windowManagement)
{
    if (m_windowManagement == windowManagement) {
        return;
    }

    if (m_windowManagement) {
        disconnect(m_windowManagement, nullptr, this, nullptr);
    }

    m_windowManagement = windowManagement;

    if (!m_windowManagement) {
        return;
    }

    connect(m_windowManagement, &QObject::destroyed, this, [this]() {
        m_windowManagement = nullptr;
    });

    connect(m_windowManagement, &PlasmaWindowManagement::windowCreated, this, &WaylandInterface::windowCreatedProxy);
    connect(m_windowManagement, &PlasmaWindowManagement::activeWindowChanged, this, [this]() noexcept {
        if (!m_windowManagement || !m_windowManagement->isValid()) {
            Q_EMIT activeWindowChanged(QByteArray());
            return;
        }

        auto w = m_windowManagement->activeWindow();
        if (!w || (w && (!m_ignoredWindows.contains(w->uuid()))) ) {
            Q_EMIT activeWindowChanged(w ? w->uuid() : QByteArray());
        }

    }, Qt::QueuedConnection);
}

void WaylandInterface::initVirtualDesktopManagement(KWayland::Client::PlasmaVirtualDesktopManagement *virtualDesktopManagement)
{
    if (m_virtualDesktopManagement == virtualDesktopManagement) {
        return;
    }

    if (m_virtualDesktopManagement) {
        disconnect(m_virtualDesktopManagement, nullptr, this, nullptr);
    }

    m_virtualDesktopManagement = virtualDesktopManagement;

    if (!m_virtualDesktopManagement) {
        return;
    }

    connect(m_virtualDesktopManagement, &QObject::destroyed, this, [this]() {
        m_virtualDesktopManagement = nullptr;
    });

    connect(m_virtualDesktopManagement, &KWayland::Client::PlasmaVirtualDesktopManagement::desktopCreated, this,
            [this](const QString &id, quint32 position) {
        addDesktop(id, position);
    });

    connect(m_virtualDesktopManagement, &KWayland::Client::PlasmaVirtualDesktopManagement::desktopRemoved, this,
            [this](const QString &id) {
        m_desktops.removeAll(id);

        if (m_currentDesktop == id) {
            setCurrentDesktop(QString());
        }
    });
}

void WaylandInterface::addDesktop(const QString &id, quint32 position)
{
    if (m_desktops.contains(id)) {
        return;
    }

    m_desktops.append(id);

    const KWayland::Client::PlasmaVirtualDesktop *desktop = m_virtualDesktopManagement->getVirtualDesktop(id);

    QObject::connect(desktop, &KWayland::Client::PlasmaVirtualDesktop::activated, this,
                     [desktop, this]() {
        setCurrentDesktop(desktop->id());
    }
    );

    if (desktop->isActive()) {
        setCurrentDesktop(id);
    }
}

void WaylandInterface::setCurrentDesktop(QString desktop)
{
    if (m_currentDesktop == desktop) {
        return;
    }

    m_currentDesktop = desktop;
    Q_EMIT currentDesktopChanged();
}

KWayland::Client::PlasmaShell *WaylandInterface::waylandCoronaInterface() const
{
    return m_corona->waylandCoronaInterface();
}

//! Register Latte Ignored Windows in order to NOT be tracked
void WaylandInterface::registerIgnoredWindow(WindowId wid)
{
    if (!wid.isNull() && !m_ignoredWindows.contains(wid)) {
        m_ignoredWindows.append(wid);

        KWayland::Client::PlasmaWindow *w = windowFor(wid);

        if (w) {
            untrackWindow(w);
        }

        Q_EMIT windowChanged(wid);
    }
}

void WaylandInterface::unregisterIgnoredWindow(WindowId wid)
{
    if (m_ignoredWindows.contains(wid)) {
        m_ignoredWindows.removeAll(wid);
        Q_EMIT windowRemoved(wid);
    }
}

void WaylandInterface::setViewExtraFlags(QObject *view, bool isPanelWindow, Latte::Types::Visibility mode)
{
    KWayland::Client::PlasmaShellSurface *surface = qobject_cast<KWayland::Client::PlasmaShellSurface *>(view);
    Latte::View *latteView = qobject_cast<Latte::View *>(view);
    Latte::ViewPart::SubConfigView *configView = qobject_cast<Latte::ViewPart::SubConfigView *>(view);

    WindowId winId;

    if (latteView) {
        surface = latteView->surface();
        winId = latteView->positioner()->trackedWindowId();
    } else if (configView) {
        surface = configView->surface();
        winId = configView->trackedWindowId();
    }

    if (!surface) {
        return;
    }

    surface->setSkipTaskbar(true);
    surface->setSkipSwitcher(true);

    bool atBottom{!isPanelWindow && (mode == Latte::Types::WindowsCanCover || mode == Latte::Types::WindowsAlwaysCover)};

    if (isPanelWindow) {
        surface->setRole(PlasmaShellSurface::Role::Panel);

        // Keep compositor-side panel behavior aligned with Latte visibility mode.
        switch (mode) {
        case Latte::Types::AutoHide:
        case Latte::Types::DodgeActive:
        case Latte::Types::DodgeMaximized:
        case Latte::Types::DodgeAllWindows:
        case Latte::Types::SidebarOnDemand:
        case Latte::Types::SidebarAutoHide:
            surface->setPanelBehavior(PlasmaShellSurface::PanelBehavior::AutoHide);
            break;
        case Latte::Types::WindowsCanCover:
            surface->setPanelBehavior(PlasmaShellSurface::PanelBehavior::WindowsCanCover);
            break;
        case Latte::Types::WindowsGoBelow:
            surface->setPanelBehavior(PlasmaShellSurface::PanelBehavior::WindowsGoBelow);
            break;
        case Latte::Types::AlwaysVisible:
        case Latte::Types::WindowsAlwaysCover:
        case Latte::Types::NormalWindow:
        case Latte::Types::None:
        default:
            surface->setPanelBehavior(PlasmaShellSurface::PanelBehavior::AlwaysVisible);
            break;
        }
    } else {
        surface->setRole(PlasmaShellSurface::Role::Normal);
    }

    if (latteView || configView) {
        auto w = windowFor(winId);
        if (w && !w->isOnAllDesktops()) {
            requestToggleIsOnAllDesktops(winId);
        }

        //! Layer to be applied
        if (mode == Latte::Types::WindowsCanCover || mode == Latte::Types::WindowsAlwaysCover) {
            setKeepBelow(winId, true);
        } else if (mode == Latte::Types::NormalWindow) {
            setKeepBelow(winId, false);
            setKeepAbove(winId, false);
        } else {
            setKeepAbove(winId, true);
        }
    }

    if (atBottom){
        //! trying to workaround WM behavior in order
        //!  1. View at the end MUST NOT HAVE FOCUSABILITY (issue example: clicking a single active task is not minimized)
        //!  2. View at the end MUST BE AT THE BOTTOM of windows stack

        QTimer::singleShot(50, [this, surface]() {
            surface->setRole(PlasmaShellSurface::Role::ToolTip);
        });
    }
}

void WaylandInterface::setViewStruts(QWindow &view, const QRect &rect, Plasma::Types::Location location)
{
    if (!m_ghostWindows.contains(&view)) {
        m_ghostWindows[&view] = new Private::GhostWindow(this);
    }

    auto w = m_ghostWindows[&view];

    switch (location) {
    case Plasma::Types::TopEdge:
    case Plasma::Types::BottomEdge:
        w->setGeometry({rect.x() + rect.width() / 2 - rect.height(), rect.y(), rect.height() + 1, rect.height()});
        break;

    case Plasma::Types::LeftEdge:
    case Plasma::Types::RightEdge:
        w->setGeometry({rect.x(), rect.y() + rect.height() / 2 - rect.width(), rect.width(), rect.width() + 1});
        break;

    default:
        break;
    }
}

void WaylandInterface::switchToNextVirtualDesktop()
{
    if (!m_virtualDesktopManagement || m_desktops.count() <= 1) {
        return;
    }

    int curPos = m_desktops.indexOf(m_currentDesktop);
    int nextPos = curPos + 1;

    if (curPos >= m_desktops.count()-1) {
        if (isVirtualDesktopNavigationWrappingAround()) {
            nextPos = 0;
        } else {
            return;
        }
    }

    KWayland::Client::PlasmaVirtualDesktop *desktopObj = m_virtualDesktopManagement->getVirtualDesktop(m_desktops[nextPos]);

    if (desktopObj) {
        desktopObj->requestActivate();
    }
}

void WaylandInterface::switchToPreviousVirtualDesktop()
{
    if (!m_virtualDesktopManagement || m_desktops.count() <= 1) {
        return;
    }

    int curPos = m_desktops.indexOf(m_currentDesktop);
    int nextPos = curPos - 1;

    if (curPos <= 0) {
        if (isVirtualDesktopNavigationWrappingAround()) {
            nextPos = m_desktops.count()-1;
        } else {
            return;
        }
    }

    KWayland::Client::PlasmaVirtualDesktop *desktopObj = m_virtualDesktopManagement->getVirtualDesktop(m_desktops[nextPos]);

    if (desktopObj) {
        desktopObj->requestActivate();
    }
}

void WaylandInterface::setWindowOnActivities(const WindowId &wid, const QStringList &nextactivities)
{
    auto winfo = requestInfo(wid);
    auto w = windowFor(wid);

    if (!w) {
        return;
    }

    QStringList curactivities = winfo.activities();

    if (!winfo.isOnAllActivities() && nextactivities.isEmpty()) {
        //! window must be set to all activities
        for(int i=0; i<curactivities.count(); ++i) {
            w->requestLeaveActivity(curactivities[i]);
        }
    } else if (curactivities != nextactivities) {
        QStringList requestenter;
        QStringList requestleave;

        for (int i=0; i<nextactivities.count(); ++i) {
            if (!curactivities.contains(nextactivities[i])) {
                requestenter << nextactivities[i];
            }
        }

        for (int i=0; i<curactivities.count(); ++i) {
            if (!nextactivities.contains(curactivities[i])) {
                requestleave << curactivities[i];
            }
        }

        //! leave afterwards from deprecated activities
        for (int i=0; i<requestleave.count(); ++i) {
            w->requestLeaveActivity(requestleave[i]);
        }

        //! first enter to new activities
        for (int i=0; i<requestenter.count(); ++i) {
            w->requestEnterActivity(requestenter[i]);
        }
    }
}

void WaylandInterface::removeViewStruts(QWindow &view)
{
    delete m_ghostWindows.take(&view);
}

WindowId WaylandInterface::activeWindow()
{
    if (!m_windowManagement || !m_windowManagement->isValid()) {
        return WindowId();
    }

    auto wid = m_windowManagement->activeWindow();

    return wid ? wid->uuid() : QByteArray();
}

void WaylandInterface::skipTaskBar(const QDialog &dialog)
{
    // On Wayland, transient dialogs are excluded from the taskbar by the compositor.
    Q_UNUSED(dialog)
}

void WaylandInterface::slideWindow(QWindow &view, AbstractWindowInterface::Slide location)
{
    auto slideLocation = KWindowEffects::NoEdge;

    switch (location) {
    case Slide::Top:
        slideLocation = KWindowEffects::TopEdge;
        break;

    case Slide::Bottom:
        slideLocation = KWindowEffects::BottomEdge;
        break;

    case Slide::Left:
        slideLocation = KWindowEffects::LeftEdge;
        break;

    case Slide::Right:
        slideLocation = KWindowEffects::RightEdge;
        break;

    default:
        break;
    }

    KWindowEffects::slideWindow(&view, slideLocation, -1);
}

void WaylandInterface::enableBlurBehind(QWindow &view)
{
    KWindowEffects::enableBlurBehind(&view);
}

void WaylandInterface::setActiveEdge(QWindow *view, bool active)
{
    ViewPart::ScreenEdgeGhostWindow *window = qobject_cast<ViewPart::ScreenEdgeGhostWindow *>(view);

    if (!window) {
        return;
    }

    auto parentView = window->parentView();

    if (!parentView || !parentView->visibility()) {
        return;
    }

    const auto mode = parentView->visibility()->mode();

    if (parentView->surface()
            && (mode == Types::DodgeActive
                || mode == Types::DodgeMaximized
                || mode == Types::DodgeAllWindows
                || mode == Types::AutoHide
                || mode == Types::SidebarAutoHide)) {
        if (active) {
            window->showWithMask();

            // This request is valid only for auto-hide panel surfaces.
            if (mode == Types::AutoHide || mode == Types::SidebarAutoHide) {
                parentView->surface()->requestHideAutoHidingPanel();
            }
        } else {
            window->hideWithMask();

            if (mode == Types::AutoHide || mode == Types::SidebarAutoHide) {
                parentView->surface()->requestShowAutoHidingPanel();
            }
        }
    }
}

void WaylandInterface::setFrameExtents(QWindow *view, const QMargins &extents)
{
    //! do nothing until there is a wayland way to provide this
}

void WaylandInterface::setInputMask(QWindow *window, const QRect &rect)
{
    //! do nothins, QWindow::mask() is sufficient enough in order to define Window input mask
}

WindowInfoWrap WaylandInterface::requestInfoActive()
{
    if (!m_windowManagement || !m_windowManagement->isValid()) {
        return {};
    }

    auto w = m_windowManagement->activeWindow();

    if (!w) return {};

    return requestInfo(w->uuid());
}

WindowInfoWrap WaylandInterface::requestInfo(WindowId wid)
{
    WindowInfoWrap winfoWrap;

    auto w = windowFor(wid);

    //!used to track Plasma DesktopView windows because during startup can not be identified properly
    bool plasmaBlockedWindow = w && (w->appId() == QLatin1String("org.kde.plasmashell")) && !isAcceptableWindow(w);

    if (w) {
        winfoWrap.setIsValid(isValidWindow(w) && !plasmaBlockedWindow);
        winfoWrap.setWid(wid);
        winfoWrap.setParentId(w->parentWindow() ? w->parentWindow()->uuid() : QByteArray());
        winfoWrap.setIsActive(w->isActive());
        winfoWrap.setIsMinimized(w->isMinimized());
        winfoWrap.setIsMaxVert(w->isMaximized());
        winfoWrap.setIsMaxHoriz(w->isMaximized());
        winfoWrap.setIsFullscreen(w->isFullscreen());
        winfoWrap.setIsShaded(w->isShaded());
        winfoWrap.setIsOnAllDesktops(w->isOnAllDesktops());
        winfoWrap.setIsOnAllActivities(w->plasmaActivities().isEmpty());
        winfoWrap.setIsKeepAbove(w->isKeepAbove());
        winfoWrap.setIsKeepBelow(w->isKeepBelow());
        winfoWrap.setGeometry(w->geometry());
        winfoWrap.setHasSkipSwitcher(w->skipSwitcher());
        winfoWrap.setHasSkipTaskbar(w->skipTaskbar());

        //! BEGIN:Window Abilities
        winfoWrap.setIsClosable(w->isCloseable());
        winfoWrap.setIsFullScreenable(w->isFullscreenable());
        winfoWrap.setIsMaximizable(w->isMaximizeable());
        winfoWrap.setIsMinimizable(w->isMinimizeable());
        winfoWrap.setIsMovable(w->isMovable());
        winfoWrap.setIsResizable(w->isResizable());
        winfoWrap.setIsShadeable(w->isShadeable());
        winfoWrap.setIsVirtualDesktopsChangeable(w->isVirtualDesktopChangeable());
        //! END:Window Abilities

        winfoWrap.setDisplay(w->title());
        winfoWrap.setDesktops(w->plasmaVirtualDesktops());
        winfoWrap.setActivities(w->plasmaActivities());

    } else {
        winfoWrap.setIsValid(false);
    }

    if (plasmaBlockedWindow) {
        windowRemoved(w->uuid());
    }

    return winfoWrap;
}

AppData WaylandInterface::appDataFor(WindowId wid)
{
    auto window = windowFor(wid);

    if (window) {
        const AppData &data = appDataFromUrl(windowUrlFromMetadata(window->appId(),
                                                                   window->pid(), rulesConfig));

        return data;
    }

    AppData empty;

    return empty;
}

KWayland::Client::PlasmaWindow *WaylandInterface::windowFor(WindowId wid)
{
    const auto windows = managedWindows();

    auto it = std::find_if(windows.constBegin(), windows.constEnd(), [&wid](PlasmaWindow *w) noexcept {
            return w->isValid() && w->uuid() == wid;
        });

    if (it == windows.constEnd()) {
        return nullptr;
    }

    return *it;
}

QList<KWayland::Client::PlasmaWindow *> WaylandInterface::managedWindows() const
{
    if (!m_windowManagement || !m_windowManagement->isValid()) {
        return {};
    }

    return m_windowManagement->windows();
}

QIcon WaylandInterface::iconFor(WindowId wid)
{
    auto window = windowFor(wid);

    if (window) {
        return window->icon();
    }


    return QIcon();
}

WindowId WaylandInterface::winIdFor(QString appId, QString title)
{
    const auto windows = managedWindows();

    auto it = std::find_if(windows.constBegin(), windows.constEnd(), [&appId, &title](PlasmaWindow *w) noexcept {
        return w->isValid() && appIdMatches(w->appId(), appId) && w->title().startsWith(title);
    });

    if (it == windows.constEnd()) {
        return WindowId();
    }

    return (*it)->uuid();
}

WindowId WaylandInterface::winIdFor(QString appId, QRect geometry)
{
    const auto windows = managedWindows();

    auto it = std::find_if(windows.constBegin(), windows.constEnd(), [&appId, &geometry](PlasmaWindow *w) noexcept {
        return w->isValid() && appIdMatches(w->appId(), appId) && w->geometry() == geometry;
    });

    if (it == windows.constEnd()) {
        return WindowId();
    }

    return (*it)->uuid();
}

bool WaylandInterface::windowCanBeDragged(WindowId wid)
{
    auto w = windowFor(wid);

    if (w && isValidWindow(w)) {
        WindowInfoWrap winfo = requestInfo(wid);
        return (winfo.isValid()
                && w->isMovable()
                && !winfo.isMinimized()
                && inCurrentDesktopActivity(winfo));
    }

    return false;
}

bool WaylandInterface::windowCanBeMaximized(WindowId wid)
{
    auto w = windowFor(wid);

    if (w && isValidWindow(w)) {
        WindowInfoWrap winfo = requestInfo(wid);
        return (winfo.isValid()
                && w->isMaximizeable()
                && !winfo.isMinimized()
                && inCurrentDesktopActivity(winfo));
    }

    return false;
}

void WaylandInterface::requestActivate(WindowId wid)
{
    auto w = windowFor(wid);

    if (w) {
        w->requestActivate();
    }
}

void WaylandInterface::requestClose(WindowId wid)
{
    auto w = windowFor(wid);

    if (w) {
        w->requestClose();
    }
}


void WaylandInterface::requestMoveWindow(WindowId wid, QPoint from)
{
    WindowInfoWrap wInfo = requestInfo(wid);

    if (windowCanBeDragged(wid) && inCurrentDesktopActivity(wInfo)) {
        auto w = windowFor(wid);

        if (w && isValidWindow(w)) {
            w->requestMove();
        }
    }
}

void WaylandInterface::requestToggleIsOnAllDesktops(WindowId wid)
{
    auto w = windowFor(wid);

    if (w && isValidWindow(w) && m_desktops.count() > 1) {
        if (w->isOnAllDesktops()) {
            w->requestEnterVirtualDesktop(m_currentDesktop);
        } else {
            const QStringList &now = w->plasmaVirtualDesktops();

            for (const auto &desktop : std::as_const(now)) {
                w->requestLeaveVirtualDesktop(desktop);
            }
        }
    }
}

void WaylandInterface::requestToggleKeepAbove(WindowId wid)
{
    auto w = windowFor(wid);

    if (w) {
        w->requestToggleKeepAbove();
    }
}

void WaylandInterface::setKeepAbove(WindowId wid, bool active)
{
    auto w = windowFor(wid);

    if (w) {
        if (active) {
            setKeepBelow(wid, false);
        }

        if ((w->isKeepAbove() && active) || (!w->isKeepAbove() && !active)) {
            return;
        }

        w->requestToggleKeepAbove();
    }
}

void WaylandInterface::setKeepBelow(WindowId wid, bool active)
{
    auto w = windowFor(wid);

    if (w) {
        if (active) {
            setKeepAbove(wid, false);
        }

        if ((w->isKeepBelow() && active) || (!w->isKeepBelow() && !active)) {
            return;
        }

        w->requestToggleKeepBelow();
    }
}

void WaylandInterface::requestToggleMinimized(WindowId wid)
{
    auto w = windowFor(wid);
    WindowInfoWrap wInfo = requestInfo(wid);

    if (w && isValidWindow(w) && inCurrentDesktopActivity(wInfo)) {
        if (!m_currentDesktop.isEmpty()) {
            w->requestEnterVirtualDesktop(m_currentDesktop);
        }
        w->requestToggleMinimized();
    }
}

void WaylandInterface::requestToggleMaximized(WindowId wid)
{
    auto w = windowFor(wid);
    WindowInfoWrap wInfo = requestInfo(wid);

    if (w && isValidWindow(w) && windowCanBeMaximized(wid) && inCurrentDesktopActivity(wInfo)) {
        if (!m_currentDesktop.isEmpty()) {
            w->requestEnterVirtualDesktop(m_currentDesktop);
        }
        w->requestToggleMaximized();
    }
}

bool WaylandInterface::isPlasmaPanel(const KWayland::Client::PlasmaWindow *w) const
{
    if (!w || (w->appId() != QLatin1String("org.kde.plasmashell"))) {
        return false;
    }

    return AbstractWindowInterface::isPlasmaPanel(w->geometry());
}

bool WaylandInterface::isFullScreenWindow(const KWayland::Client::PlasmaWindow *w) const
{
    if (!w) {
        return false;
    }

    return w->isFullscreen() || AbstractWindowInterface::isFullScreenWindow(w->geometry());
}

bool WaylandInterface::isSidepanel(const KWayland::Client::PlasmaWindow *w) const
{
    if (!w) {
        return false;
    }

    return AbstractWindowInterface::isSidepanel(w->geometry());
}

bool WaylandInterface::isValidWindow(const KWayland::Client::PlasmaWindow *w)
{
    if (!w || !w->isValid()) {
        return false;
    }

    if (windowsTracker()->isValidFor(w->uuid())) {
        return true;
    }

    return isAcceptableWindow(w);
}

bool WaylandInterface::isAcceptableWindow(const KWayland::Client::PlasmaWindow *w)
{
    if (!w || !w->isValid()) {
        return false;
    }

    //! ignored windows that are not tracked
    if (hasBlockedTracking(w->uuid())) {
        return false;
    }

    //! whitelisted/approved windows
    if (isWhitelistedWindow(w->uuid())) {
        return true;
    }

    //! Window Checks
    bool hasSkipTaskbar = w->skipTaskbar();
    bool isSkipped = hasSkipTaskbar;
    bool hasSkipSwitcher = w->skipSwitcher();
    isSkipped = hasSkipTaskbar && hasSkipSwitcher;

    if (isSkipped
            && ((w->appId() == QLatin1String("yakuake")
                 || (w->appId() == QLatin1String("krunner"))) )) {
        registerWhitelistedWindow(w->uuid());
    } else if (w->appId() == QLatin1String("org.kde.plasmashell")) {
        if (isSkipped && isSidepanel(w)) {
            registerWhitelistedWindow(w->uuid());
            return true;
        } else if (isPlasmaPanel(w) || isFullScreenWindow(w)) {
            registerPlasmaIgnoredWindow(w->uuid());
            return false;
        }
    } else if ((App::matchesSelfAppId(w->appId()))
               || (w->appId().startsWith(QLatin1String("ksmserver")))) {
        if (isFullScreenWindow(w)) {
            registerIgnoredWindow(w->uuid());
            return false;
        }
    }

    return !isSkipped;
}

void WaylandInterface::updateWindow()
{
    PlasmaWindow *pW = qobject_cast<PlasmaWindow*>(QObject::sender());

    if (isValidWindow(pW)) {
        considerWindowChanged(pW->uuid());
    }
}

void WaylandInterface::windowUnmapped()
{
    PlasmaWindow *pW = qobject_cast<PlasmaWindow*>(QObject::sender());

    if (pW) {
        untrackWindow(pW);
        Q_EMIT windowRemoved(pW->uuid());
    }
}

void WaylandInterface::trackWindow(KWayland::Client::PlasmaWindow *w)
{
    if (!w) {
        return;
    }

    connect(w, &PlasmaWindow::activeChanged, this, &WaylandInterface::updateWindow);
    connect(w, &PlasmaWindow::titleChanged, this, &WaylandInterface::updateWindow);
    connect(w, &PlasmaWindow::fullscreenChanged, this, &WaylandInterface::updateWindow);
    connect(w, &PlasmaWindow::geometryChanged, this, &WaylandInterface::updateWindow);
    connect(w, &PlasmaWindow::maximizedChanged, this, &WaylandInterface::updateWindow);
    connect(w, &PlasmaWindow::minimizedChanged, this, &WaylandInterface::updateWindow);
    connect(w, &PlasmaWindow::shadedChanged, this, &WaylandInterface::updateWindow);
    connect(w, &PlasmaWindow::skipTaskbarChanged, this, &WaylandInterface::updateWindow);
    connect(w, &PlasmaWindow::onAllDesktopsChanged, this, &WaylandInterface::updateWindow);
    connect(w, &PlasmaWindow::parentWindowChanged, this, &WaylandInterface::updateWindow);
    connect(w, &PlasmaWindow::plasmaVirtualDesktopEntered, this, &WaylandInterface::updateWindow);
    connect(w, &PlasmaWindow::plasmaVirtualDesktopLeft, this, &WaylandInterface::updateWindow);
    connect(w, &PlasmaWindow::plasmaActivityEntered, this, &WaylandInterface::updateWindow);
    connect(w, &PlasmaWindow::plasmaActivityLeft, this, &WaylandInterface::updateWindow);
    connect(w, &PlasmaWindow::unmapped, this, &WaylandInterface::windowUnmapped);
}

void WaylandInterface::untrackWindow(KWayland::Client::PlasmaWindow *w)
{
    if (!w) {
        return;
    }

    disconnect(w, &PlasmaWindow::activeChanged, this, &WaylandInterface::updateWindow);
    disconnect(w, &PlasmaWindow::titleChanged, this, &WaylandInterface::updateWindow);
    disconnect(w, &PlasmaWindow::fullscreenChanged, this, &WaylandInterface::updateWindow);
    disconnect(w, &PlasmaWindow::geometryChanged, this, &WaylandInterface::updateWindow);
    disconnect(w, &PlasmaWindow::maximizedChanged, this, &WaylandInterface::updateWindow);
    disconnect(w, &PlasmaWindow::minimizedChanged, this, &WaylandInterface::updateWindow);
    disconnect(w, &PlasmaWindow::shadedChanged, this, &WaylandInterface::updateWindow);
    disconnect(w, &PlasmaWindow::skipTaskbarChanged, this, &WaylandInterface::updateWindow);
    disconnect(w, &PlasmaWindow::onAllDesktopsChanged, this, &WaylandInterface::updateWindow);
    disconnect(w, &PlasmaWindow::parentWindowChanged, this, &WaylandInterface::updateWindow);
    disconnect(w, &PlasmaWindow::plasmaVirtualDesktopEntered, this, &WaylandInterface::updateWindow);
    disconnect(w, &PlasmaWindow::plasmaVirtualDesktopLeft, this, &WaylandInterface::updateWindow);
    disconnect(w, &PlasmaWindow::plasmaActivityEntered, this, &WaylandInterface::updateWindow);
    disconnect(w, &PlasmaWindow::plasmaActivityLeft, this, &WaylandInterface::updateWindow);
    disconnect(w, &PlasmaWindow::unmapped, this, &WaylandInterface::windowUnmapped);
}


void WaylandInterface::windowCreatedProxy(KWayland::Client::PlasmaWindow *w)
{
    if (!isAcceptableWindow(w))  {
        return;
    }

    trackWindow(w);
    Q_EMIT windowAdded(w->uuid());

    if (App::matchesSelfAppId(w->appId())) {
        Q_EMIT latteWindowAdded();
    }
}

}
}

#include "waylandinterface.moc"
