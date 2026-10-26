/*
    SPDX-FileCopyrightText: 2021 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "widgetexplorerview.h"

// local
#include "../panelshadows_p.h"
#include "../view.h"
#include "../../lattecorona.h"
#include "../../wm/abstractwindowinterface.h"

// Qt
#include <QDir>
#include <QProcess>
#include <QProcessEnvironment>
#include <QQuickItem>
#include <QScreen>
#include <QStandardPaths>

// KDE
#include <KWindowEffects>
#include <KWayland/Client/plasmashell.h>

// Plasma

namespace Latte {
namespace ViewPart {

constexpr int kWidgetExplorerVerticalMargin = 100;
constexpr int kSyncGeometryDelayMs = 400;

namespace {

QString defaultWaylandDisplay()
{
    const QString runtimeDir = QString::fromLocal8Bit(qgetenv("XDG_RUNTIME_DIR"));

    if (runtimeDir.isEmpty()) {
        return QString();
    }

    const QDir dir(runtimeDir);
    const QStringList candidates = dir.entryList({QStringLiteral("wayland-*")},
                                   QDir::System | QDir::Files | QDir::NoDotAndDotDot,
                                   QDir::Name);

    for (const QString &candidate : candidates) {
        if (!candidate.endsWith(QStringLiteral(".lock"))) {
            return candidate;
        }
    }

    return QString();
}

}

WidgetExplorerView::WidgetExplorerView(Latte::View *view)
    : SubConfigView(view, QStringLiteral("#widgetexplorerview#"), true)
{
    setResizeMode(QQuickView::SizeRootObjectToView);
    //!set flags early in order for wayland to initialize properly
    setFlags(wFlags());

    connect(this, &QQuickView::widthChanged, this, &WidgetExplorerView::updateEffects);
    connect(this, &QQuickView::heightChanged, this, &WidgetExplorerView::updateEffects);

    connect(this, &QQuickView::statusChanged, this, [this](QQuickView::Status status) {
        if (status == QQuickView::Ready) {
            updateEffects();
        }
    });

    setParentView(view);
    init();
}

void WidgetExplorerView::init()
{
    SubConfigView::init();

    QByteArray tempFilePath = "widgetexplorerui";

    updateEnabledBorders();

    auto source = QUrl::fromLocalFile(m_latteView->containment()->corona()->kPackage().filePath(tempFilePath));
    loadRootSource(source);
    syncGeometry();
}

bool WidgetExplorerView::hideOnWindowDeactivate() const
{
    return m_hideOnWindowDeactivate;
}

void WidgetExplorerView::setHideOnWindowDeactivate(bool hide)
{
    if (m_hideOnWindowDeactivate == hide) {
        return;
    }

    m_hideOnWindowDeactivate = hide;
    Q_EMIT hideOnWindowDeactivateChanged();
}

bool WidgetExplorerView::openGetNewWidgetsDialog()
{
    const QString executable = QStandardPaths::findExecutable(QStringLiteral("knewstuff-dialog6"));

    if (executable.isEmpty()) {
        qWarning() << "WidgetExplorerView: knewstuff-dialog6 executable was not found";
        return false;
    }

    QProcess process;
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();

    if (environment.value(QStringLiteral("WAYLAND_DISPLAY")).isEmpty()) {
        const QString waylandDisplay = defaultWaylandDisplay();

        if (!waylandDisplay.isEmpty()) {
            environment.insert(QStringLiteral("WAYLAND_DISPLAY"), waylandDisplay);
        }
    }

    if (environment.value(QStringLiteral("QT_QPA_PLATFORM")).isEmpty()
        && !environment.value(QStringLiteral("WAYLAND_DISPLAY")).isEmpty()) {
        environment.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("wayland"));
    }

    process.setProgram(executable);
    process.setArguments({QStringLiteral("plasmoids.knsrc")});
    process.setProcessEnvironment(environment);

    return process.startDetached();
}

Qt::WindowFlags WidgetExplorerView::wFlags() const
{
    return (flags() | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
}

QRect WidgetExplorerView::geometryWhenVisible() const
{
    return m_geometryWhenVisible;
}

void WidgetExplorerView::initParentView(Latte::View *view)
{
    SubConfigView::initParentView(view);

    //! WidgetExplorer.qml lists the applets of this containment. It is this
    //! window's own input next to the six of SubConfigView, and follows the
    //! dock the same way.
    setRootInput(QStringLiteral("containmentFromView"), m_latteView->containment());

    updateEnabledBorders();
    syncGeometry();
}

QRect WidgetExplorerView::availableScreenGeometry() const
{
    int currentScrId = m_latteView->positioner()->currentScreenId();

    QList<Latte::Types::Visibility> ignoreModes{Latte::Types::SidebarOnDemand, Latte::Types::SidebarAutoHide};

    if (m_latteView->visibility() && m_latteView->visibility()->isSidebar()) {
        ignoreModes.removeAll(Latte::Types::SidebarOnDemand);
        ignoreModes.removeAll(Latte::Types::SidebarAutoHide);
    }

    QString activityid = m_latteView->layout()->lastUsedActivity();

    return m_corona->availableScreenRectWithCriteria(currentScrId, activityid, ignoreModes, {}, false, true);
}

void WidgetExplorerView::syncGeometry()
{
    if (!m_latteView || !m_latteView->layout() || !m_latteView->containment() || !rootObject()) {
        return;
    }

    const QSize size(rootObject()->width(), rootObject()->height());
    auto availGeometry = availableScreenGeometry();

    int margin = availGeometry.height() == m_latteView->screenGeometry().height() ? kWidgetExplorerVerticalMargin : 0;
    auto geometry = QRect(availGeometry.x(), availGeometry.y(), size.width(), availGeometry.height() - margin);

    updateEnabledBorders();

    if (m_geometryWhenVisible == geometry) {
        return;
    }

    m_geometryWhenVisible = geometry;

    setPosition(geometry.topLeft());

    if (m_shellSurface) {
        m_shellSurface->setPosition(geometry.topLeft());
    }

    setMaximumSize(geometry.size());
    setMinimumSize(geometry.size());
    resize(geometry.size());
}

void WidgetExplorerView::showEvent(QShowEvent *ev)
{
    SubConfigView::showEvent(ev);

    if (m_shellSurface) {
        //! under wayland it needs to be set again after its hiding
        m_shellSurface->setPosition(m_geometryWhenVisible.topLeft());

        // Use AppletPopup role so the compositor applies the panel/Plasma
        // color scheme instead of the application color scheme. This way
        // the dialog matches the panel appearance even when the app color
        // scheme differs (e.g. dark panel + light applications).
        if (m_shellSurface->role() != KWayland::Client::PlasmaShellSurface::Role::AppletPopup) {
            m_shellSurface->setRole(KWayland::Client::PlasmaShellSurface::Role::AppletPopup);
        }
    }

    if (!m_latteView) {
        return;
    }

    syncGeometry();

    requestActivate();

    m_screenSyncTimer.start();
    QTimer::singleShot(kSyncGeometryDelayMs, this, &WidgetExplorerView::syncGeometry);

    Q_EMIT showSignal();
}

void WidgetExplorerView::focusOutEvent(QFocusEvent *ev)
{
    Q_UNUSED(ev);

    if (!m_latteView) {
        return;
    }

    hideConfigWindow();
}

void WidgetExplorerView::updateEffects()
{
    // Apply effects only after the shell surface is ready.
    if (!m_shellSurface) {
        return;
    }

    if (!m_background) {
        m_background = new KSvg::FrameSvg(this);
    }

    if (m_background->imagePath() != QStringLiteral("dialogs/background")) {
        m_background->setImagePath(QStringLiteral("dialogs/background"));
    }

    m_background->setEnabledBorders(m_enabledBorders);
    m_background->resizeFrame(size());

    QRegion mask = m_background->mask();

    QRegion fixedMask = mask.isNull() ? QRegion(QRect(0, 0, width(), height())) : mask;

    if (!fixedMask.isEmpty()) {
        setMask(fixedMask);
    } else {
        setMask(QRegion());
    }

    KWindowEffects::enableBlurBehind(this, true, fixedMask);
}

void WidgetExplorerView::hideConfigWindow()
{
    if (!m_hideOnWindowDeactivate) {
        return;
    }

    deleteLater();
}

void WidgetExplorerView::syncSlideEffect()
{
    if (!m_latteView || !m_latteView->containment()) {
        return;
    }

    auto slideLocation = WindowSystem::AbstractWindowInterface::Slide::Left;

    m_corona->wm()->slideWindow(*this, slideLocation);
}

//!BEGIN borders
void WidgetExplorerView::updateEnabledBorders()
{
    if (!this->screen()) {
        return;
    }

    KSvg::FrameSvg::EnabledBorders borders = KSvg::FrameSvg::AllBorders;

    if (!m_geometryWhenVisible.isEmpty()) {
        if (m_geometryWhenVisible.x() == m_latteView->screenGeometry().x()) {
            borders &= ~KSvg::FrameSvg::LeftBorder;
        }

        if (m_geometryWhenVisible.y() == m_latteView->screenGeometry().y()) {
            borders &= ~KSvg::FrameSvg::TopBorder;
        }

        if (m_geometryWhenVisible.height() == m_latteView->screenGeometry().height()) {
            borders &= ~KSvg::FrameSvg::BottomBorder;
        }
    }

    if (m_enabledBorders != borders) {
        if (isVisible()) {
            m_enabledBorders = borders;
        }

        m_corona->dialogShadows()->addWindow(this, m_enabledBorders);

        Q_EMIT enabledBordersChanged();
    }
}

//!END borders

}
}
