/*
    SPDX-FileCopyrightText: 2016 Smith AR <audoban@openmailbox.org>
    SPDX-FileCopyrightText: 2016 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef PRIMARYCONFIGVIEW_H
#define PRIMARYCONFIGVIEW_H

// local
#include <QStringList>
#include <coretypes.h>
#include "subconfigview.h"
#include "../../wm/windowinfowrap.h"

//Qt
#include <QObject>
#include <QPointer>
#include <QQuickView>
#include <QTimer>
#include <QWindow>

// Plasma
#include <Plasma/package.h>
#include <Plasma/Plasma>
#include <KSvg/FrameSvg>

namespace Plasma {
class Applet;
class Containment;
class FrameSvg;
class Types;
}

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
class CanvasConfigView;

namespace Config {
class IndicatorUiManager;
}
}
}

namespace Latte {
namespace ViewPart {

class PrimaryConfigView : public SubConfigView
{
    Q_OBJECT
    Q_PROPERTY(bool isReady READ isReady NOTIFY isReadyChanged)

    Q_PROPERTY(int x READ x NOTIFY xChanged)
    Q_PROPERTY(int y READ y NOTIFY yChanged)

    Q_PROPERTY(QRect availableScreenGeometry READ availableScreenGeometry NOTIFY availableScreenGeometryChanged)

    Q_PROPERTY(Latte::ViewPart::Config::IndicatorUiManager *indicatorUiManager READ indicatorUiManager NOTIFY indicatorUiManagerChanged)

public:
    PrimaryConfigView(Latte::View *view);
    ~PrimaryConfigView() override;

    bool hasFocus() const;

    bool isReady() const;

    bool sticker() const;

    QRect availableScreenGeometry() const;
    QRect geometryWhenVisible() const;

    Config::IndicatorUiManager *indicatorUiManager();

    void setParentView(Latte::View *view, const bool &immediate = false) override;
    void setOnActivities(QStringList activities);

    void showConfigWindow();

    void requestActivate() override;

public Q_SLOTS:
    Q_INVOKABLE void syncGeometry() override;
    Q_INVOKABLE void hideConfigWindow();
    Q_INVOKABLE void setSticker(bool blockFocusLost);
    Q_INVOKABLE void openHelp(const QString &section);
    Q_INVOKABLE void updateEffects();

Q_SIGNALS:
    void availableScreenGeometryChanged();
    void indicatorUiManagerChanged();
    void isReadyChanged();
    void raiseDocksTemporaryChanged();
    void showSignal();
    void xChanged();
    void yChanged();

protected:
    void showEvent(QShowEvent *ev) override;
    void hideEvent(QHideEvent *ev) override;
    void focusOutEvent(QFocusEvent *ev) override;

    void init() override;
    void initParentView(Latte::View *view) override;
    void updateEnabledBorders() override;

private Q_SLOTS:
    void immutabilityChanged(Plasma::Types::ImmutabilityType type);
    void updateAvailableScreenGeometry(View *origin = nullptr);

    void showCanvasWindow();
    void hideCanvasWindow();

private:
    void setIsReady(bool ready);
    void instantUpdateAvailableScreenGeometry();

    bool inAdvancedMode() const;

private:
    bool m_blockFocusLost{false};
    bool m_blockFocusLostOnStartup{true};
    bool m_inReverse{false};    //! it is used by the borders
    bool m_isReady{false};

    Latte::Types::Visibility m_originalMode{Latte::Types::DodgeActive};

    QTimer m_availableScreemGeometryTimer;

    QRect m_availableScreenGeometry;
    QRect m_geometryWhenVisible;

    QPointer<CanvasConfigView> m_canvasConfigView;

    Config::IndicatorUiManager *m_indicatorUiManager{nullptr};

    //only for the mask on disabled compositing, not to actually paint
    KSvg::FrameSvg *m_background{nullptr};
};

}
}

#endif //PRIMARYCONFIGVIEW_H
