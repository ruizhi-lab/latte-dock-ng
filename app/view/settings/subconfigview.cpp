/*
    SPDX-FileCopyrightText: 2020 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "subconfigview.h"

//local
#include <config-latte.h>
#include "../../apptypes.h"
#include "../view.h"
#include "../../lattecorona.h"
#include "../../layouts/manager.h"
#include "../../plasma/extended/theme.h"
#include "../../settings/universalsettings.h"
#include "../../shortcuts/globalshortcuts.h"
#include "../../shortcuts/shortcutstracker.h"
#include "../../wm/abstractwindowinterface.h"

// KDE
#include <KLocalizedContext>
#include <KDeclarative/KDeclarative>
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
    m_screenSyncTimer.setInterval(100);

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
    qDebug() << validTitle() << " deleting...";

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
    qDebug() << validTitle() << " : initialization started...";

    setDefaultAlphaBuffer(true);
    setColor(Qt::transparent);

    rootContext()->setContextProperty(QStringLiteral("viewConfig"), this);
    rootContext()->setContextProperty(QStringLiteral("shortcutsEngine"), m_corona->globalShortcuts()->shortcutsTracker());

    if (m_corona) {
        rootContext()->setContextProperty(QStringLiteral("universalSettings"), m_corona->universalSettings());
        rootContext()->setContextProperty(QStringLiteral("layoutsManager"), m_corona->layoutsManager());
        rootContext()->setContextProperty(QStringLiteral("themeExtended"), m_corona->themeExtended());
    }

    KDeclarative::KDeclarative kdeclarative;
    kdeclarative.setDeclarativeEngine(engine());
    kdeclarative.setTranslationDomain(QString::fromLatin1(App::TRANSLATIONDOMAIN));
    kdeclarative.setupContext();
    kdeclarative.setupEngine(engine());

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

void SubConfigView::setParentView(Latte::View *view, const bool &immediate)
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

    rootContext()->setContextProperty(QStringLiteral("latteView"), m_latteView);
    m_plasmoidContextSyncAttempts = 0;
    m_hasGraphicPlasmoidContext = false;
    m_loggedPlasmoidContext = false;
    syncPlasmoidContext();
}

void SubConfigView::syncPlasmoidContext()
{
    if (!m_latteView || !m_latteView->containment()) {
        return;
    }

    QObject *containmentObject = m_latteView->containment();
    QObject *containmentGraphicObject = findGraphicContextObject(containmentObject);
    QObject *plasmoidContextObject = nullptr;
    const bool hasGraphicObject = containmentGraphicObject != nullptr;

    if (hasGraphicObject) {
        // Keep legacy semantics: configuration QML expects the containment
        // graphic object, not an arbitrary child object.
        plasmoidContextObject = containmentGraphicObject;
    }

    if (!plasmoidContextObject) {
        plasmoidContextObject = containmentObject;
    }

    rootContext()->setContextProperty(QStringLiteral("plasmoid"), plasmoidContextObject);

    if (!m_loggedPlasmoidContext && plasmoidContextObject) {
        const QMetaObject *metaObj = plasmoidContextObject->metaObject();
        qDebug() << "latte::config plasmoid context class:" << metaObj->className()
                 << "has configuration:" << (metaObj->indexOfProperty("configuration") >= 0)
                 << "has location:" << (metaObj->indexOfProperty("location") >= 0)
                 << "has formFactor:" << (metaObj->indexOfProperty("formFactor") >= 0);
        m_loggedPlasmoidContext = true;
    }

    if (hasGraphicObject) {
        if (!m_hasGraphicPlasmoidContext) {
            qDebug() << "latte::config switched plasmoid context to graphic object";
            m_hasGraphicPlasmoidContext = true;
        }
        return;
    }

    // In some startup paths the graphic object arrives noticeably later than the
    // config window construction. Keep probing for a while and upgrade context
    // to the real graphic object as soon as it is available.
    if (m_plasmoidContextSyncAttempts == 0) {
        const QMetaObject *metaObj = containmentObject->metaObject();
        qDebug() << "latte::config containment context fallback class:" << metaObj->className()
                 << "has _plasma_graphicObject property:" << (metaObj->indexOfProperty("_plasma_graphicObject") >= 0)
                 << "has graphicObject property:" << (metaObj->indexOfProperty("graphicObject") >= 0);
    }

    if (m_plasmoidContextSyncAttempts < 200) {
        ++m_plasmoidContextSyncAttempts;
        QTimer::singleShot(50, this, &SubConfigView::syncPlasmoidContext);
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
        qDebug() << staticMetaObject.className() << "wrong location";
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

        qDebug() << "wayland " << title() <<  " surface was created...";

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
                    qDebug() << "WAYLAND " << title() <<  " window surface was deleted...";
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

Plasma::FrameSvg::EnabledBorders SubConfigView::enabledBorders() const
{
    return m_enabledBorders;
}

}
}
