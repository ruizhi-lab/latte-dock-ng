/*
    SPDX-FileCopyrightText: 2020 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "subwindow.h"

// local
#include "../../apptypes.h"
#include "../view.h"
#include "../visibilitymanager.h"

// Qt
#include <QDebug>
#include <QSurfaceFormat>
#include <QQuickView>
#include <QTimer>

// KDE
#include <KWayland/Client/plasmashell.h>
#include <KWayland/Client/surface.h>

namespace Latte {
namespace ViewPart {

SubWindow::SubWindow(Latte::View *view, QString debugType) :
    m_latteView(view)
{
    m_corona = qobject_cast<Latte::Corona *>(view->corona());

    m_debugMode = (qApp->arguments().contains("-d") && qApp->arguments().contains("--kwinedges"));
    m_debugType = debugType;

    m_showColor = QColor(Qt::transparent);
    m_hideColor = QColor(Qt::transparent);

    setTitle(validTitle());
    setColor(m_showColor);
    setDefaultAlphaBuffer(true);

    setFlags(Qt::FramelessWindowHint
             | Qt::WindowStaysOnTopHint
             | Qt::NoDropShadowWindowHint
             | Qt::WindowDoesNotAcceptFocus);

    m_fixGeometryTimer.setSingleShot(true);
    m_fixGeometryTimer.setInterval(500);
    connect(&m_fixGeometryTimer, &QTimer::timeout, this, &SubWindow::fixGeometry);

    connect(this, &QQuickView::xChanged, this, &SubWindow::startGeometryTimer);
    connect(this, &QQuickView::yChanged, this, &SubWindow::startGeometryTimer);
    connect(this, &QQuickView::widthChanged, this, &SubWindow::startGeometryTimer);
    connect(this, &QQuickView::heightChanged, this, &SubWindow::startGeometryTimer);

    connect(this, &SubWindow::calculatedGeometryChanged, this, &SubWindow::fixGeometry);

    connect(m_latteView, &Latte::View::absoluteGeometryChanged, this, &SubWindow::updateGeometry);
    connect(m_latteView, &Latte::View::screenGeometryChanged, this, &SubWindow::updateGeometry);
    connect(m_latteView, &Latte::View::locationChanged, this, &SubWindow::updateGeometry);
    connect(m_latteView, &QQuickView::screenChanged, this, [this]() {
        setScreen(m_latteView->screen());
        updateGeometry();
    });

    setupWaylandIntegration();

    connect(this, &QWindow::windowTitleChanged, this, &SubWindow::updateWaylandId);
    connect(m_corona->wm(), &WindowSystem::AbstractWindowInterface::latteWindowAdded, this, &SubWindow::updateWaylandId);

    setScreen(m_latteView->screen());
    show();
    hideWithMask();
}

SubWindow::~SubWindow()
{
    m_inDelete = true;

    if (!m_trackedWindowId.isNull()) {
        m_corona->wm()->unregisterIgnoredWindow(m_trackedWindowId);
    }

    m_latteView = nullptr;

    // clear mode
    m_visibleHackTimer1.stop();
    m_visibleHackTimer2.stop();
    for (auto &c : connectionsHack) {
        disconnect(c);
    }

    if (m_shellSurface) {
        delete m_shellSurface;
    }
}

int SubWindow::location()
{
    return (int)m_latteView->location();
}

int SubWindow::thickness() const
{
    return m_thickness;
}

QString SubWindow::validTitlePrefix() const
{
    return QString("#subwindow#");
}

QString SubWindow::validTitle() const
{
    return QString(validTitlePrefix() + QString::number(m_latteView->containment()->id()));
}

Latte::View *SubWindow::parentView()
{
    return m_latteView;
}

Latte::WindowSystem::WindowId SubWindow::trackedWindowId()
{
    if (m_trackedWindowId.isEmpty()) {
        updateWaylandId();
    }

    return m_trackedWindowId;
}

KWayland::Client::PlasmaShellSurface *SubWindow::surface()
{
    return m_shellSurface;
}

void SubWindow::fixGeometry()
{
    if (!m_calculatedGeometry.isEmpty()
            && (m_calculatedGeometry.x() != x() || m_calculatedGeometry.y() != y()
                || m_calculatedGeometry.width() != width() || m_calculatedGeometry.height() != height())) {
        setMinimumSize(m_calculatedGeometry.size());
        setMaximumSize(m_calculatedGeometry.size());
        resize(m_calculatedGeometry.size());
        setPosition(m_calculatedGeometry.x(), m_calculatedGeometry.y());

        if (m_shellSurface) {
            m_shellSurface->setPosition(m_calculatedGeometry.topLeft());
        }
    }
}

void SubWindow::updateWaylandId()
{
    Latte::WindowSystem::WindowId newId = m_corona->wm()->winIdFor(App::preferredWaylandAppId(), validTitle());

    if (m_trackedWindowId != newId) {
        if (!m_trackedWindowId.isNull()) {
            m_corona->wm()->unregisterIgnoredWindow(m_trackedWindowId);
        }

        m_trackedWindowId = newId;
        m_corona->wm()->registerIgnoredWindow(m_trackedWindowId);
    }
}

void SubWindow::startGeometryTimer()
{
    m_fixGeometryTimer.start();
}

void SubWindow::setupWaylandIntegration()
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

        qDebug() << "wayland screen edge ghost window surface was created...";
        m_shellSurface = interface->createSurface(s, this);
        m_corona->wm()->setViewExtraFlags(m_shellSurface);

        m_shellSurface->setPanelTakesFocus(false);
    }
}

bool SubWindow::event(QEvent *e)
{
    if (e->type() == QEvent::Show) {
        m_corona->wm()->setViewExtraFlags(this);
    }

    return QQuickView::event(e);
}


void SubWindow::hideWithMask()
{
    if (m_debugMode) {
        qDebug() << m_debugType + " :: MASK HIDE...";
    }

    setMask(VisibilityManager::ISHIDDENMASK);

    //! repaint in order to update mask immediately
    setColor(m_hideColor);
}

void SubWindow::showWithMask()
{
    if (m_debugMode) {
        qDebug() << m_debugType + " :: MASK SHOW...";
    }

    setMask(QRegion());

    //! repaint in order to update mask immediately
    setColor(m_showColor);
}

}
}
