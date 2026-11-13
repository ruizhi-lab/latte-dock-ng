/*
    SPDX-FileCopyrightText: 2016 Smith AR <audoban@openmailbox.org>
    SPDX-FileCopyrightText: 2016 Michail Vourlakos <mvourlakos@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "primaryconfigview.h"
#include "dockpresetdefaults.h"
#include "../containmentinterface.h"
#include "../indicator/indicator.h"
#include "../visibilitymanager.h"
#include "../../pluginids.h"
#include <latte_debug.h>
#include <Plasma/Applet>
#include <Plasma/Containment>
#include <QFileInfo>
#include <KConfig>
#include <utility>

// local
#include <config-latte.h>
#include "canvasconfigview.h"
#include "indicatoruimanager.h"
#include "../effects.h"
#include "../panelshadows_p.h"
#include "../view.h"
#include "../viewgeometryhelpers.h"
#include "../../lattecorona.h"
#include "../../layouts/manager.h"
#include "../../layout/genericlayout.h"
#include "../../settings/universalsettings.h"
#include "../../settings/handbook.h"
#include "../../wm/abstractwindowinterface.h"

// Qt
#include <QQuickItem>
#include <QQmlEngine>
#include <QScreen>

// KDE
#include <KWayland/Client/plasmashell.h>
#include <KWayland/Client/surface.h>
#include <KWindowEffects>

// Plasma

#define CANVASWINDOWINTERVAL 50
#define PRIMARYWINDOWINTERVAL 250
#define SLIDEOUTINTERVAL 400

namespace Latte {
namespace ViewPart {

PrimaryConfigView::PrimaryConfigView(Latte::View *view)
    : SubConfigView(view, QStringLiteral("#primaryconfigview#")),
      m_indicatorUiManager(new Config::IndicatorUiManager(this))
{
    connect(this, &QQuickWindow::xChanged, this, &PrimaryConfigView::xChanged);
    connect(this, &QQuickWindow::yChanged, this, &PrimaryConfigView::yChanged);

    connect(this, &QQuickView::widthChanged, this, &PrimaryConfigView::updateEffects);
    connect(this, &QQuickView::heightChanged, this, &PrimaryConfigView::updateEffects);

    connect(this, &PrimaryConfigView::availableScreenGeometryChanged, this, &PrimaryConfigView::syncGeometry);

    connect(this, &QQuickView::statusChanged, this, [this](QQuickView::Status status) {
        if (status == QQuickView::Ready) {
            updateEffects();
        }
    });

    if (m_corona) {
        connections << connect(m_corona, &Latte::Corona::raiseViewsTemporaryChanged, this, &PrimaryConfigView::raiseDocksTemporaryChanged);
        connections << connect(m_corona, &Latte::Corona::availableScreenRectChangedFrom, this, &PrimaryConfigView::updateAvailableScreenGeometry);
        connections << connect(m_corona->wm(), &WindowSystem::AbstractWindowInterface::plasmaPanelGeometriesChanged, this, [this]() {
            updateAvailableScreenGeometry();
        });

        connections << connect(m_corona->layoutsManager(), &Latte::Layouts::Manager::currentLayoutIsSwitching, this, [this]() {
            if (isVisible()) {
                hideConfigWindow();
            }
        });

        connect(m_corona->universalSettings(), &Latte::UniversalSettings::inAdvancedModeForEditSettingsChanged,
                this, &PrimaryConfigView::syncGeometry);
    }

    m_availableScreemGeometryTimer.setSingleShot(true);
    m_availableScreemGeometryTimer.setInterval(250);

    connections << connect(&m_availableScreemGeometryTimer, &QTimer::timeout, this, [this]() {
        instantUpdateAvailableScreenGeometry();
    });

    setParentView(view);
    init();
}

void
PrimaryConfigView::openHelp(const QString &section)
{ Settings::openHandbook(section); }

PrimaryConfigView::~PrimaryConfigView()
{
    if (m_canvasConfigView) {
        delete m_canvasConfigView;
    }

}

bool
PrimaryConfigView::restoreDockPreset()
{
    auto *view = parentView();
    auto *containment = view ? view->containment() : nullptr;
    if (!isReady() || !containment || !view->indicator() || !view->visibility()) {
        return false;
    }
    const QString templatePath = corona()->kPackage().filePath("templates") + QStringLiteral("/Default Dock.view.latte");
    if (!QFileInfo::exists(templatePath)) {
        qCWarning(latteView) << "Cannot restore Dock preset: missing default Dock template" << templatePath;
        return false;
    }
    KConfig preset(templatePath, KConfig::SimpleConfig);
    const auto dockPreset = preset.group(QStringLiteral("Containments")).group(QStringLiteral("1"));
    const auto taskPreset = dockPreset.group(QStringLiteral("Applets")).group(QStringLiteral("2")).group(QStringLiteral("Configuration")).group(QStringLiteral("General"));
    // The editor's graphic plasmoid is the authority for its live map. A
    // separately created Applet map can persist values without updating UI bindings.
    auto *plasmoid = rootObject() ? rootObject()->property("plasmoid").value<QObject *>() : nullptr;
    auto *dockMap = plasmoid ? qobject_cast<KConfigPropertyMap *>(plasmoid->property("configuration").value<QObject *>()) : nullptr;
    auto *dockSchema = containment->configScheme();
    if (!dockMap || !dockSchema) {
        return false;
    }

    // Resolve and guard every live task map before changing any settings. The
    // Tasks page may use a context-owned map rather than Applet::configuration().
    struct TaskConfig
    {
        QPointer<Plasma::Applet> applet;
        QPointer<KConfigPropertyMap> map;
        QStringList launchers;
    };
    QList<TaskConfig> tasks;
    int taskIndex = 0;
    for (auto *applet : containment->applets()) {
        if (applet->pluginMetaData().pluginId() != QLatin1String(PluginId::kPlasmoid)) {
            continue;
        }
        auto *map = qobject_cast<KConfigPropertyMap *>(view->extendedInterface()->configurationForAppletVisualIndex(taskIndex++));
        if (!map || !applet->configScheme()) {
            qCWarning(latteView) << "Cannot restore Dock preset: task configuration is unavailable";
            return false;
        }
        const auto group = static_cast<Types::LaunchersGroup>(map->value(QStringLiteral("launchersGroup")).toInt());
        QStringList launchers = map->value(QStringLiteral("launchers59")).toStringList();
        if (group == Types::LayoutLaunchers && view->layout()) {
            launchers = view->layout()->launchers();
        } else if (group == Types::GlobalLaunchers) {
            launchers = corona()->universalSettings()->launchers();
        }
        tasks.append({ applet, map, launchers });
    }
    if (!view->indicator()->restoreDefaults()) {
        qCWarning(latteView) << "Cannot restore Dock preset: default indicator configuration is unavailable";
        return false;
    }
    Config::DockPreset::restore(dockSchema, dockMap, Config::DockPreset::dockKeys(), dockPreset.group(QStringLiteral("General")));
    for (const auto &task : std::as_const(tasks)) {
        if (!task.applet || !task.map) {
            qCWarning(latteView) << "Dock preset restore interrupted: task configuration was destroyed";
            return false;
        }
        // Reset launcher sharing to the preset without losing the effective
        // shared list or modifying another Dock's layout/global launcher source.
        auto keys = Config::DockPreset::taskKeys();
        if (!task.map->isImmutable(QStringLiteral("launchers59")) && !task.map->isImmutable(QStringLiteral("launchersGroup"))) {
            task.map->insert(QStringLiteral("launchers59"), task.launchers);
        } else {
            keys.removeAll(QStringLiteral("launchersGroup"));
        }
        Config::DockPreset::restore(task.applet->configScheme(), task.map, keys, taskPreset);
    }
    view->setAlignment(static_cast<Types::Alignment>(dockMap->value(QStringLiteral("alignment")).toInt()));
    view->setIsPreferredForShortcuts(dockPreset.readEntry(QStringLiteral("isPreferredForShortcuts"), false));
    auto *visibility = view->visibility();
    visibility->setTimerShow(dockPreset.readEntry(QStringLiteral("timerShow"), 0));
    visibility->setTimerHide(dockPreset.readEntry(QStringLiteral("timerHide"), 700));
    visibility->setRaiseOnDesktop(dockPreset.readEntry(QStringLiteral("raiseOnDesktopChange"), false));
    visibility->setRaiseOnActivity(dockPreset.readEntry(QStringLiteral("raiseOnActivityChange"), false));
    visibility->setEnableKWinEdges(dockPreset.readEntry(QStringLiteral("enableKWinEdges"), true));
    visibility->setMode(static_cast<Types::Visibility>(dockPreset.readEntry(QStringLiteral("visibility"), static_cast<int>(Types::DodgeActive))));
    containment->config().sync();
    syncGeometry();
    return true;
}

void PrimaryConfigView::init()
{
    SubConfigView::init();

    QByteArray tempFilePath = "lattedockconfigurationui";

    // Configuration QML creates dynamic indicator pages during component
    // completion. loadRootSource() makes every input available before the root
    // is created so those pages never observe a transient undefined value.
    auto source = QUrl::fromLocalFile(m_latteView->containment()->corona()->kPackage().filePath(tempFilePath));
    loadRootSource(source);
    syncGeometry();

    // The QML root may resolve its implicit size asynchronously in Qt 6, so
    // re-run syncGeometry whenever it changes. Otherwise the window stays at
    // its initial 0x0 and Wayland kills the connection.
    if (rootObject()) {
        connect(rootObject(), &QQuickItem::widthChanged, this, &PrimaryConfigView::syncGeometry);
        connect(rootObject(), &QQuickItem::heightChanged, this, &PrimaryConfigView::syncGeometry);
    }
}

Config::IndicatorUiManager *PrimaryConfigView::indicatorUiManager()
{
    return m_indicatorUiManager;
}

void PrimaryConfigView::setOnActivities(QStringList activities)
{
    m_corona->wm()->setWindowOnActivities(trackedWindowId(), activities);

    if (m_canvasConfigView) {
        m_corona->wm()->setWindowOnActivities(m_canvasConfigView->trackedWindowId(), activities);
    }
}

void PrimaryConfigView::requestActivate()
{
    if (m_latteView && m_latteView->visibility()) {
        if (m_shellSurface) {
            m_corona->wm()->requestActivate(m_latteView->positioner()->trackedWindowId());
        }
    }

    SubConfigView::requestActivate();
}

void PrimaryConfigView::showConfigWindow()
{
    if (isVisible()) {
        return;
    }

    if (m_latteView && m_latteView->containment()) {
        m_latteView->containment()->setUserConfiguring(true);
    }

    showAfter(PRIMARYWINDOWINTERVAL);
    showCanvasWindow();
}

void PrimaryConfigView::hideConfigWindow()
{
    if (m_shellSurface) {
        //! Avoid races where input events arrive after the surface starts teardown.
        close();
    } else {
        hide();
    }

    hideCanvasWindow();
}

void PrimaryConfigView::showCanvasWindow()
{
    if (!m_canvasConfigView) {
        m_canvasConfigView = new CanvasConfigView(m_latteView, this);
    }

    if (m_canvasConfigView && !m_canvasConfigView->isVisible()) {
        m_canvasConfigView->showAfter(CANVASWINDOWINTERVAL);
    }
}

void PrimaryConfigView::hideCanvasWindow()
{
    if (m_canvasConfigView) {
        m_canvasConfigView->hideConfigWindow();
    }
}

void PrimaryConfigView::setParentView(Latte::View *view, const bool &immediate)
{
    if (m_latteView == view) {
        return;
    }

    if (m_latteView && !immediate) {
        hideConfigWindow();

        //!slide-out delay
        QTimer::singleShot(SLIDEOUTINTERVAL, this, [this, view]() {
            initParentView(view);
            showConfigWindow();
        });
    } else {
        initParentView(view);
        showConfigWindow();
    }
}

void PrimaryConfigView::initParentView(Latte::View *view)
{
    setIsReady(false);

    //! This window is shared by all docks, so SubConfigView publishes the new
    //! dock as the latteView and plasmoid inputs on every parent switch:
    //! otherwise the dialog keeps editing the first dock it was opened for
    //! (Add... > Duplicate Dock copies that dock) and turns null once that
    //! dock is deleted.
    SubConfigView::initParentView(view);

    //! A deleted dock takes the settings tree down first. QObject::destroyed
    //! is emitted before the QML engine releases its guards on the dock, so
    //! clearing isReady here unloads LatteDockConfiguration.qml (its Loader
    //! waits for isReady) before any binding re-evaluates against the dying
    //! dock and logs null-property TypeErrors. The window stays orphaned until
    //! another dock adopts it, and initParentView() sets isReady again.
    viewconnections << connect(m_latteView, &QObject::destroyed, this, [this]() {
        setIsReady(false);
    });

    viewconnections << connect(m_latteView, &Latte::View::layoutChanged, this, [this]() {
        if (m_latteView->layout()) {
            updateAvailableScreenGeometry();
        }
    });

    viewconnections << connect(m_latteView, &Latte::View::editThicknessChanged, this, [this]() {
        updateAvailableScreenGeometry();
    });

    viewconnections << connect(m_latteView, &Latte::View::maxNormalThicknessChanged, this, [this]() {
        updateAvailableScreenGeometry();
    });

    viewconnections << connect(m_latteView, &Latte::View::locationChanged, this, [this]() {
        updateAvailableScreenGeometry();
    });

    viewconnections << connect(m_latteView->positioner(), &Latte::ViewPart::Positioner::currentScreenChanged, this, [this]() {
        updateAvailableScreenGeometry();
    });
    viewconnections << connect(m_latteView->positioner(), &Latte::ViewPart::Positioner::canvasGeometryChanged,
                               this, &PrimaryConfigView::instantUpdateAvailableScreenGeometry);

    viewconnections << connect(m_corona->universalSettings(), &Latte::UniversalSettings::inAdvancedModeForEditSettingsChanged, m_latteView, &Latte::View::inSettingsAdvancedModeChanged);
    viewconnections << connect(m_latteView->containment(), &Plasma::Containment::immutabilityChanged, this, &PrimaryConfigView::immutabilityChanged);

    m_originalMode = m_latteView->visibility()->mode();

    updateEnabledBorders();
    updateAvailableScreenGeometry();
    syncGeometry();

    setIsReady(true);

    if (m_canvasConfigView) {
        m_canvasConfigView->setParentView(view);
    }

    //! inform view about the current settings level
    Q_EMIT m_latteView->inSettingsAdvancedModeChanged();
}

void PrimaryConfigView::instantUpdateAvailableScreenGeometry()
{
    if (!m_latteView || !m_latteView->positioner() || !m_latteView->layout() || !m_latteView->containment()) {
        return;
    }

    int currentScrId = m_latteView->positioner()->currentScreenId();

    QList<Latte::Types::Visibility> ignoreModes{Latte::Types::SidebarOnDemand, Latte::Types::SidebarAutoHide};

    if (m_latteView->visibility() && m_latteView->visibility()->isSidebar()) {
        ignoreModes.removeAll(Latte::Types::SidebarOnDemand);
        ignoreModes.removeAll(Latte::Types::SidebarAutoHide);
    }

    QString activityid = m_latteView->layout()->lastUsedActivity();

    const QRect available = m_corona->availableScreenRectWithCriteria(currentScrId, activityid, ignoreModes, {}, false, true);
    const QRect geometry = configViewAvailableGeometry(m_latteView->screenGeometry(), available,
                           m_corona->wm()->livePlasmaPanelGeometries(), m_latteView->positioner()->canvasGeometry(), m_latteView->location());
    if (m_availableScreenGeometry != geometry) {
        m_availableScreenGeometry = geometry;
        Q_EMIT availableScreenGeometryChanged();
    }
}

void PrimaryConfigView::updateAvailableScreenGeometry(View *origin)
{
    if (!m_latteView || !m_latteView->layout() || m_latteView == origin) {
        return;
    }

    if (!m_availableScreemGeometryTimer.isActive()) {
        m_availableScreemGeometryTimer.start();
    }
}

QRect PrimaryConfigView::availableScreenGeometry() const
{
    return m_availableScreenGeometry;
}

QRect PrimaryConfigView::geometryWhenVisible() const
{
    return m_geometryWhenVisible;
}

void PrimaryConfigView::syncGeometry()
{
    if (!m_latteView || !m_latteView->layout() || !m_latteView->containment() || !rootObject()) {
        return;
    }

    int resolvedWidth = static_cast<int>(rootObject()->width());
    int resolvedHeight = static_cast<int>(rootObject()->height());

    if (resolvedWidth <= 0 || resolvedHeight <= 0) {
        resolvedWidth = static_cast<int>(rootObject()->implicitWidth());
        resolvedHeight = static_cast<int>(rootObject()->implicitHeight());
    }

    const QSize size(resolvedWidth, resolvedHeight);

    // In Qt 6 the QML root may not yet have its implicit size when init() calls
    // syncGeometry(). Showing a 0x0 surface on Wayland aborts the Wayland
    // connection ("invalid window geometry size") and crashes Latte. Bail out
    // and let a later widthChanged/heightChanged trigger schedule a real
    // geometry sync once the QML has laid itself out.
    if (size.width() <= 0 || size.height() <= 0) {
        return;
    }

    const auto location = m_latteView->containment()->location();
    const auto scrGeometry = m_latteView->screenGeometry();
    const auto availGeometry = m_availableScreenGeometry;
    if (!availGeometry.isValid()) {
        return;
    }

    QPoint position{0, 0};

    int xPos{0};
    int yPos{0};

    switch (m_latteView->formFactor()) {
        case Plasma::Types::Horizontal: {
            if (inAdvancedMode()) {
                if (qApp->isLeftToRight()) {
                    xPos = availGeometry.x() + availGeometry.width() - size.width();
                } else {
                    xPos = availGeometry.x();
                }
            } else {
                xPos = scrGeometry.center().x() - size.width() / 2;
            }

            if (location == Plasma::Types::TopEdge) {
                yPos = availGeometry.top();
            } else if (location == Plasma::Types::BottomEdge) {
                yPos = availGeometry.bottom() + 1 - size.height();
            }
        }
        break;

        case Plasma::Types::Vertical: {
            if (location == Plasma::Types::LeftEdge) {
                xPos = availGeometry.left();
                yPos =  availGeometry.y() + (availGeometry.height() - size.height()) / 2;
            } else if (location == Plasma::Types::RightEdge) {
                xPos = availGeometry.right() + 1 - size.width();
                yPos =  availGeometry.y() + (availGeometry.height() - size.height()) / 2;
            }
        }
        break;

        default:
            qWarning() << "no sync geometry, wrong formFactor";
            break;
    }

    // Both basic centering and advanced edge alignment must stay in the same
    // reserved rectangle that caps QML's viewport, including on offset outputs.
    xPos = qBound(availGeometry.left(), xPos, qMax(availGeometry.left(), availGeometry.right() + 1 - size.width()));
    yPos = qBound(availGeometry.top(), yPos, qMax(availGeometry.top(), availGeometry.bottom() + 1 - size.height()));
    position = {xPos, yPos};

    updateEnabledBorders();

    auto geometry = QRect(position.x(), position.y(), size.width(), size.height());

    QRect winGeometry(x(), y(), width(), height());

    if (m_geometryWhenVisible == geometry && winGeometry == geometry) {
        return;
    }

    m_geometryWhenVisible = geometry;

    setPosition(position);

    if (m_shellSurface) {
        m_shellSurface->setPosition(position);
    }

    setMaximumSize(size);
    setMinimumSize(size);
    resize(size);

    Q_EMIT m_latteView->configWindowGeometryChanged();
}

void PrimaryConfigView::showEvent(QShowEvent *ev)
{
    updateAvailableScreenGeometry();

    if (m_shellSurface) {
        //! under wayland it needs to be set again after its hiding
        m_shellSurface->setPosition(m_geometryWhenVisible.topLeft());
    }

    SubConfigView::showEvent(ev);

    if (!m_latteView) {
        return;
    }

    setFlags(wFlags());
    m_corona->wm()->setViewExtraFlags(this, false, Latte::Types::NormalWindow);

    syncGeometry();

    m_screenSyncTimer.start();
    QTimer::singleShot(400, this, &PrimaryConfigView::syncGeometry);

    showCanvasWindow();

    Q_EMIT showSignal();

    if (m_latteView && m_latteView->layout()) {
        m_latteView->layout()->setLastConfigViewFor(m_latteView);
    }
}

void PrimaryConfigView::hideEvent(QHideEvent *)
{
    if (!m_latteView) {
        return;
    }

    if (m_latteView->containment()) {
        m_latteView->containment()->setUserConfiguring(false);
    }
}

bool PrimaryConfigView::hasFocus() const
{
    bool primaryHasHocus{isActive()};
    bool canvasHasFocus{m_canvasConfigView && m_canvasConfigView->isActive()};
    bool viewHasFocus{m_latteView && (m_latteView->containsMouse() || m_latteView->alternativesIsShown())};

    return (m_blockFocusLost || viewHasFocus || primaryHasHocus || canvasHasFocus);
}

void PrimaryConfigView::focusOutEvent(QFocusEvent *ev)
{
    Q_UNUSED(ev);

    if (!m_latteView) {
        return;
    }

    const auto *focusWindow = qGuiApp->focusWindow();

    if (focusWindow && (focusWindow->flags().testFlag(Qt::Popup)
                        || focusWindow->flags().testFlag(Qt::ToolTip))) {
        return;
    }

    if (!hasFocus()) {
        hideConfigWindow();
    }
}

void PrimaryConfigView::immutabilityChanged(Plasma::Types::ImmutabilityType type)
{
    if (type != Plasma::Types::Mutable && isVisible()) {
        hideConfigWindow();
    }
}

bool PrimaryConfigView::isReady() const
{
    return m_isReady;
}

void PrimaryConfigView::setIsReady(bool ready)
{
    if (m_isReady == ready) {
        return;
    }

    m_isReady = ready;
    Q_EMIT isReadyChanged();
}


bool PrimaryConfigView::sticker() const
{
    return m_blockFocusLost;
}

void PrimaryConfigView::setSticker(bool blockFocusLost)
{
    if (m_blockFocusLost == blockFocusLost)
        return;

    m_blockFocusLost = blockFocusLost;
}

bool PrimaryConfigView::inAdvancedMode() const
{
    return m_corona->universalSettings()->inAdvancedModeForEditSettings();
}

//!BEGIN borders
void PrimaryConfigView::updateEnabledBorders()
{
    if (!this->screen()) {
        return;
    }

    KSvg::FrameSvg::EnabledBorders borders = KSvg::FrameSvg::AllBorders;

    switch (m_latteView->location()) {
        case Plasma::Types::TopEdge:
            borders &= m_inReverse ? ~KSvg::FrameSvg::BottomBorder : ~KSvg::FrameSvg::TopBorder;
            break;

        case Plasma::Types::LeftEdge:
            borders &= ~KSvg::FrameSvg::LeftBorder;
            break;

        case Plasma::Types::RightEdge:
            borders &= ~KSvg::FrameSvg::RightBorder;
            break;

        case Plasma::Types::BottomEdge:
            borders &= m_inReverse ? ~KSvg::FrameSvg::TopBorder : ~KSvg::FrameSvg::BottomBorder;
            break;

        default:
            break;
    }

    if (m_enabledBorders != borders) {
        m_enabledBorders = borders;

        m_corona->dialogShadows()->addWindow(this, m_enabledBorders);

        Q_EMIT enabledBordersChanged();
    }
}

//!END borders

void PrimaryConfigView::updateEffects()
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

}
}
