/*
    SPDX-FileCopyrightText: 2020 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef SUBCONFIGVIEW_H
#define SUBCONFIGVIEW_H

// local
#include <coretypes.h>
#include "../../wm/windowinfowrap.h"

//Qt
#include <QHash>
#include <QObject>
#include <QPointer>
#include <QQuickView>
#include <QTimer>
#include <QUrl>

// Plasma
#include <KSvg/FrameSvg>

namespace KWayland {
namespace Client {
class PlasmaShellSurface;
}
}

namespace Latte {
class Corona;
class View;
}


namespace Latte {
namespace ViewPart {

//! Base of the settings windows: PrimaryConfigView (LatteDockConfiguration.qml),
//! CanvasConfigView (CanvasConfiguration.qml) and WidgetExplorerView
//! (WidgetExplorer.qml).
//!
//! Root inputs replace the former context properties. Every window hands the
//! same six C++ objects to its QML root as nullable root properties:
//! - latteView: the dock the window edits;
//! - plasmoid: the containment's graphic object, or the containment itself
//!   until that object exists;
//! - viewConfig: the window itself;
//! - universalSettings, layoutsManager, themeExtended: application services.
//!
//! How they are delivered:
//! - setRootInput() is the only publisher. Until the root exists the values are
//!   collected; loadRootSource() applies them with setInitialProperties(), so
//!   the root's bindings never see an undefined input. Afterwards the root
//!   property is written directly. For an input the root does not declare Qt
//!   logs "Setting initial properties failed: ... does not have a property
//!   called ..." and the input stays unset, so a new input needs a declaration
//!   in all three roots.
//! - Files loaded by a root keep reading the inputs unqualified: a name is
//!   resolved through the context chain up to the root object of the window's
//!   QML file, so only the root declares them and the pages need no wiring.
//! - PrimaryConfigView is shared by all docks and re-points the canvas window
//!   with it. initParentView() publishes latteView and, through
//!   syncPlasmoidInput(), plasmoid again on every switch, so the window follows
//!   the dock it edits instead of staying on the first one.
//!
//! Context properties that remain on purpose, outside these windows' roots:
//! - "indicator" and "dialog" (IndicatorUiManager): third-party indicator
//!   configuration UIs read them. They live in a separate QML engine, which a
//!   root property of this window cannot reach.
//! - "alternativesHelper" (Corona::showAlternativesForApplet()): the applet
//!   alternatives dialog is not a settings window, and plasma-desktop's own
//!   dialog file, used below Plasma 6.5, reads it as a context property.
class SubConfigView : public QQuickView
{
    Q_OBJECT
    Q_PROPERTY(KSvg::FrameSvg::EnabledBorders enabledBorders READ enabledBorders NOTIFY enabledBordersChanged)

public:
    SubConfigView(Latte::View *view, const QString &title, const bool &isNormalWindow = true);
    ~SubConfigView() override;

    virtual void requestActivate();

    QString validTitle() const;

    KSvg::FrameSvg::EnabledBorders enabledBorders() const;

    Latte::Corona *corona() const;
    Latte::View *parentView() const;
    virtual void setParentView(Latte::View *view, const bool &immediate = false);
    virtual void showAfter(int msecs = 0);

    Latte::WindowSystem::WindowId trackedWindowId();
    KWayland::Client::PlasmaShellSurface *surface();

public Q_SLOTS:
    virtual void syncGeometry() = 0;

Q_SIGNALS:
    void enabledBordersChanged();

protected:
    virtual void syncSlideEffect();

    virtual void init();
    virtual void initParentView(Latte::View *view);
    virtual void updateEnabledBorders() = 0;

    void setRootInput(const QString &name, QObject *value);
    void loadRootSource(const QUrl &source);

    void showEvent(QShowEvent *ev) override;
    bool event(QEvent *e) override;

    virtual Qt::WindowFlags wFlags() const;

protected:
    bool m_isNormalWindow{true};
    QTimer m_screenSyncTimer;

    QPointer<Latte::View> m_latteView;

    QList<QMetaObject::Connection> connections;
    QList<QMetaObject::Connection> viewconnections;

    KSvg::FrameSvg::EnabledBorders m_enabledBorders{KSvg::FrameSvg::AllBorders};

    Latte::Corona *m_corona{nullptr};
    KWayland::Client::PlasmaShellSurface *m_shellSurface{nullptr};

private Q_SLOTS:
    void updateWaylandId();
    void syncPlasmoidInput();

private:
    void setupWaylandIntegration();

private:
    QString m_validTitle;

    QTimer m_showTimer;

    //! Last value of every root input; the guard drops an input whose object
    //! is deleted before the root exists
    QHash<QString, QPointer<QObject>> m_rootInputs;

    int m_plasmoidInputSyncAttempts{0};
    bool m_hasGraphicPlasmoidInput{false};
    bool m_loggedPlasmoidInput{false};

    Latte::WindowSystem::WindowId m_waylandWindowId;
};

}
}

#endif //SUBCONFIGVIEW_H
