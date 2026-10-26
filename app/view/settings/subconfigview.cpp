/*
    SPDX-FileCopyrightText: 2020 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <latte_debug.h>
#include "subconfigview.h"

//local
#include <config-latte.h>
#include "../../apptypes.h"
#include "../view.h"

// Qt
#include <QQmlEngine>
#include <QQuickItem>
#include "../../lattecorona.h"
#include "../../layouts/manager.h"
#include "../../plasma/extended/theme.h"
#include "../../settings/universalsettings.h"
#include "../../wm/abstractwindowinterface.h"
#include "../../knscompat.h"

// KDE
#include <KWayland/Client/plasmashell.h>
#include <KWayland/Client/surface.h>

namespace {
QObject *findGraphicContextObject(QObject *containmentObject)
{
    if (!containmentObject) {
        return nullptr;
    }

    QObject *candidate = containmentObject->property("_plasma_graphicObject").value<QObject *>();

    if (candidate) {
        return candidate;
    }

    candidate = containmentObject->property("graphicObject").value<QObject *>();

    if (candidate) {
        return candidate;
    }

    return nullptr;
}
}

namespace Latte {
namespace ViewPart {

constexpr int kScreenSyncIntervalMs = 100;
constexpr int kMaxPlasmoidInputSyncAttempts = 200;
constexpr int kPlasmoidInputSyncRetryMs = 50;

SubConfigView::SubConfigView(Latte::View *view, const QString &title, const bool &isNormalWindow)
    : QQuickView(nullptr),
      m_isNormalWindow(isNormalWindow)
{
    m_corona = qobject_cast<Latte::Corona *>(view->containment()->corona());

    setupWaylandIntegration();

    connect(this, &QWindow::windowTitleChanged, this, &SubConfigView::updateWaylandId);
    connect(m_corona->wm(), &WindowSystem::AbstractWindowInterface::latteWindowAdded, this, &SubConfigView::updateWaylandId);

    m_validTitle = title;
    setTitle(m_validTitle);

    setScreen(view->screen());
    setIcon(qGuiApp->windowIcon());

    if (!m_isNormalWindow) {
        setFlags(wFlags());
        m_corona->wm()->setViewExtraFlags(this, true);
    }

    m_screenSyncTimer.setSingleShot(true);
    m_screenSyncTimer.setInterval(kScreenSyncIntervalMs);

    connections << connect(&m_screenSyncTimer, &QTimer::timeout, this, [this]() {
        if (!m_latteView) {
            return;
        }

        setScreen(m_latteView->screen());

        syncGeometry();
    });

    m_showTimer.setSingleShot(true);
    m_showTimer.setInterval(0);

    connections << connect(&m_showTimer, &QTimer::timeout, this, [this]() {
        syncSlideEffect();
        show();
    });
}

SubConfigView::~SubConfigView()
{
    qCDebug(latteView) << validTitle() << " deleting...";

    // Unload QML content before the base QQuickView destructor runs.
    // The PlasmaShell.WidgetExplorer component holds a KIO-backed model
    // whose workers need a live KIO connection infrastructure to shut
    // down cleanly. Clearing the source destroys the QML object tree
    // while the engine and connections are still intact.
    setSource(QUrl());
    engine()->clearComponentCache();

    m_corona->dialogShadows()->removeWindow(this);

    if (!m_waylandWindowId.isNull()) {
        m_corona->wm()->unregisterIgnoredWindow(m_waylandWindowId);
    }

    for (const auto &var : connections) {
        QObject::disconnect(var);
    }

    for (const auto &var : viewconnections) {
        QObject::disconnect(var);
    }
}

void SubConfigView::init()
{
    qCDebug(latteView) << validTitle() << " : initialization started...";

    addLatteQmlImportPaths(engine());

    setDefaultAlphaBuffer(true);
    setColor(Qt::transparent);

    setRootInput(QStringLiteral("viewConfig"), this);

    if (m_corona) {
        setRootInput(QStringLiteral("universalSettings"), m_corona->universalSettings());
        setRootInput(QStringLiteral("layoutsManager"), m_corona->layoutsManager());
        setRootInput(QStringLiteral("themeExtended"), m_corona->themeExtended());
    }
}

void SubConfigView::setRootInput(const QString &name, QObject *value)
{
    m_rootInputs.insert(name, value);

    QQuickItem *root = rootObject();

    if (!root) {
        //! loadRootSource() hands the collected inputs to the root it creates
        return;
    }

    //! QObject::setProperty() answers false for a name the root does not declare
    //! and only adds a dynamic property that no binding reads. Report the
    //! missing declaration instead of leaving the window on a stale dock.
    if (!root->setProperty(name.toLatin1().constData(), QVariant::fromValue(value))) {
        qCWarning(latteView) << validTitle() << "root does not declare the input" << name;
    }
}

void SubConfigView::loadRootSource(const QUrl &source)
{
    QVariantMap inputs;

    for (auto it = m_rootInputs.cbegin(); it != m_rootInputs.cend(); ++it) {
        inputs.insert(it.key(), QVariant::fromValue(it.value().data()));
    }

    //! The root is created with its inputs already set, before its bindings
    //! run. Set after setSource(), the first evaluation of a guard such as the
    //! settings Loader's `active` would see undefined objects, and the dynamic
    //! indicator pages created during component completion a transient null.
    setInitialProperties(inputs);
    setSource(source);
}

Qt::WindowFlags SubConfigView::wFlags() const
{
    return (flags() | Qt::FramelessWindowHint) & ~Qt::WindowDoesNotAcceptFocus;
}

QString SubConfigView::validTitle() const
{
    return m_validTitle;
}

Latte::WindowSystem::WindowId SubConfigView::trackedWindowId()
{
    if (m_waylandWindowId.isEmpty()) {
        updateWaylandId();
    }

    return m_waylandWindowId;
}

Latte::Corona *SubConfigView::corona() const
{
    return m_corona;
}

Latte::View *SubConfigView::parentView() const
{
    return m_latteView;
}

void SubConfigView::setParentView(Latte::View *view, const bool &)
{
    if (m_latteView == view) {
        return;
    }

    initParentView(view);
}

void SubConfigView::initParentView(Latte::View *view)
{
    for (const auto &var : viewconnections) {
        QObject::disconnect(var);
    }

    m_latteView = view;

    viewconnections << connect(m_latteView->positioner(), &ViewPart::Positioner::canvasGeometryChanged, this, &SubConfigView::syncGeometry);

    //! One window serves every dock it is moved to, so both inputs that point at
    //! the dock are published again on every switch. latteView goes first and
    //! plasmoid second, as it did when they were context properties.
    //! PrimaryConfigView keeps its tree unloaded (isReady) while this runs.
    setRootInput(QStringLiteral("latteView"), m_latteView);
    m_plasmoidInputSyncAttempts = 0;
    m_hasGraphicPlasmoidInput = false;
    m_loggedPlasmoidInput = false;
    syncPlasmoidInput();
}

void SubConfigView::syncPlasmoidInput()
{
    if (!m_latteView || !m_latteView->containment()) {
        return;
    }

    QObject *containmentObject = m_latteView->containment();
    QObject *containmentGraphicObject = findGraphicContextObject(containmentObject);
    QObject *plasmoidObject = nullptr;
    const bool hasGraphicObject = containmentGraphicObject != nullptr;

    if (hasGraphicObject) {
        // Keep legacy semantics: configuration QML expects the containment
        // graphic object, not an arbitrary child object.
        plasmoidObject = containmentGraphicObject;
    }

    if (!plasmoidObject) {
        plasmoidObject = containmentObject;
    }

    setRootInput(QStringLiteral("plasmoid"), plasmoidObject);

    if (!m_loggedPlasmoidInput && plasmoidObject) {
        const QMetaObject *metaObj = plasmoidObject->metaObject();
        qCDebug(latteView) << "latte::config plasmoid input class:" << metaObj->className()
                           << "has configuration:" << (metaObj->indexOfProperty("configuration") >= 0)
                           << "has location:" << (metaObj->indexOfProperty("location") >= 0)
                           << "has formFactor:" << (metaObj->indexOfProperty("formFactor") >= 0);
        m_loggedPlasmoidInput = true;
    }

    if (hasGraphicObject) {
        if (!m_hasGraphicPlasmoidInput) {
            qCDebug(latteView) << "latte::config switched plasmoid input to graphic object";
            m_hasGraphicPlasmoidInput = true;
        }

        return;
    }

    // In some startup paths the graphic object arrives noticeably later than the
    // config window construction. Keep probing for a while and upgrade the input
    // to the real graphic object as soon as it is available. The Loader guards
    // of the primary and canvas roots wait for plasmoid.configuration, which
    // the containment published meanwhile may not provide.
    if (m_plasmoidInputSyncAttempts == 0) {
        const QMetaObject *metaObj = containmentObject->metaObject();
        qCDebug(latteView) << "latte::config containment input fallback class:" << metaObj->className()
                           << "has _plasma_graphicObject property:" << (metaObj->indexOfProperty("_plasma_graphicObject") >= 0)
                           << "has graphicObject property:" << (metaObj->indexOfProperty("graphicObject") >= 0);
    }

    if (m_plasmoidInputSyncAttempts < kMaxPlasmoidInputSyncAttempts) {
        ++m_plasmoidInputSyncAttempts;
        QTimer::singleShot(kPlasmoidInputSyncRetryMs, this, &SubConfigView::syncPlasmoidInput);
    }
}

void SubConfigView::requestActivate()
{
    if (m_shellSurface) {
        updateWaylandId();
        m_corona->wm()->requestActivate(m_waylandWindowId);
    } else {
        QQuickView::requestActivate();
    }
}

void SubConfigView::showAfter(int msecs)
{
    if (isVisible()) {
        return;
    }

    m_showTimer.setInterval(msecs);
    m_showTimer.start();

}

void SubConfigView::syncSlideEffect()
{
    if (!m_latteView || !m_latteView->containment()) {
        return;
    }

    auto slideLocation = WindowSystem::AbstractWindowInterface::Slide::None;

    switch (m_latteView->containment()->location()) {
        case Plasma::Types::TopEdge:
            slideLocation = WindowSystem::AbstractWindowInterface::Slide::Top;
            break;

        case Plasma::Types::RightEdge:
            slideLocation = WindowSystem::AbstractWindowInterface::Slide::Right;
            break;

        case Plasma::Types::BottomEdge:
            slideLocation = WindowSystem::AbstractWindowInterface::Slide::Bottom;
            break;

        case Plasma::Types::LeftEdge:
            slideLocation = WindowSystem::AbstractWindowInterface::Slide::Left;
            break;

        default:
            qCDebug(latteView) << staticMetaObject.className() << "wrong location";
            break;
    }

    m_corona->wm()->slideWindow(*this, slideLocation);
}

KWayland::Client::PlasmaShellSurface *SubConfigView::surface()
{
    return m_shellSurface;
}

void SubConfigView::setupWaylandIntegration()
{
    if (m_shellSurface || !m_latteView || !m_latteView->containment()) {
        // already setup
        return;
    }

    if (m_corona) {
        using namespace KWayland::Client;
        PlasmaShell *interface = m_corona->waylandCoronaInterface();

        if (!interface) {
            return;
        }

        Surface *s = Surface::fromWindow(this);

        if (!s) {
            return;
        }

        qCDebug(latteView) << "wayland " << title() <<  " surface was created...";

        m_shellSurface = interface->createSurface(s, this);

        if (m_isNormalWindow) {
            m_corona->wm()->setViewExtraFlags(m_shellSurface, false);
        } else {
            m_corona->wm()->setViewExtraFlags(m_shellSurface, true);
        }

        updateWaylandId();
        syncGeometry();
    }
}

void SubConfigView::showEvent(QShowEvent *ev)
{
    QQuickView::showEvent(ev);

    if (m_shellSurface) {
        //! readd shadows after hiding because the window shadows are not shown again after first showing
        m_corona->dialogShadows()->addWindow(this, m_enabledBorders);
    }
}

bool SubConfigView::event(QEvent *e)
{
    if (e->type() == QEvent::PlatformSurface) {
        if (auto pe = dynamic_cast<QPlatformSurfaceEvent *>(e)) {
            switch (pe->surfaceEventType()) {
                case QPlatformSurfaceEvent::SurfaceCreated:

                    if (m_shellSurface) {
                        break;
                    }

                    setupWaylandIntegration();
                    break;

                case QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed:
                    if (m_shellSurface) {
                        delete m_shellSurface;
                        m_shellSurface = nullptr;
                        qCDebug(latteView) << "WAYLAND " << title() <<  " window surface was deleted...";
                    }

                    break;
            }
        }
    }

    return QQuickView::event(e);
}

void SubConfigView::updateWaylandId()
{
    Latte::WindowSystem::WindowId newId = m_corona->wm()->winIdFor(App::preferredWaylandAppId(), validTitle());

    if (m_waylandWindowId != newId) {
        if (!m_waylandWindowId.isNull()) {
            m_corona->wm()->unregisterIgnoredWindow(m_waylandWindowId);
        }

        m_waylandWindowId = newId;
        m_corona->wm()->registerIgnoredWindow(m_waylandWindowId);
    }
}

KSvg::FrameSvg::EnabledBorders SubConfigView::enabledBorders() const
{
    return m_enabledBorders;
}

}
}
