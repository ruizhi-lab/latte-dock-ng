/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "knscompat.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTextStream>

static constexpr int kCompatVersion = 6;

//! Patched DrawerHandle.qml — Qt 6.10 removed DragHandler.xAxis.onActiveValueChanged.
//! Use DragHandler.onActiveTranslationChanged instead.
static constexpr auto kPatchedDrawerHandle = R"qml(/*
 *  SPDX-FileCopyrightText: 2023 Marco Martin <mart@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Templates as T
import org.kde.kirigami as Kirigami
import org.kde.kirigami.templates as KT

Item {
    id: root

    property bool displayToolTip: true
    property T.Drawer drawer

    readonly property T.Overlay overlay: drawer.T.Overlay.overlay

    parent: overlay?.parent ?? null
    z: overlay ? overlay.z  + (drawer?.modal && (drawer as KT.OverlayDrawer)?.drawerOpen ? 1 : - 1) : 0

    QQC2.ToolButton {
        id: button
        anchors.centerIn: parent
        flat: false

        icon.name: root.drawer.position > 0 ? root.drawer.handleOpenIcon.name : root.drawer.handleClosedIcon.name
        icon.source: root.drawer.position > 0 ? root.drawer.handleOpenIcon.source ?? "" : root.drawer.handleClosedIcon.source ?? ""
        icon.width: root.drawer.handleOpenIcon.width
        icon.height: root.drawer.handleOpenIcon.height
        Accessible.name: QQC2.ToolTip.text

        onClicked: {
            root.displayToolTip = false;
            Qt.callLater(() => {
                const oDrawer = root.drawer as KT.OverlayDrawer
                oDrawer.drawerOpen = !oDrawer.drawerOpen;
            })
        }
        Keys.onEscapePressed: {
            if (root.drawer.closePolicy & T.Popup.CloseOnEscape) {
                root.drawer.drawerOpen = false;
            }
        }

        QQC2.ToolTip.visible: root.displayToolTip && hovered
        QQC2.ToolTip.text: {
            const oDrawer = root.drawer as KT.OverlayDrawer
            return oDrawer.drawerOpen ? oDrawer.handleOpenToolTip : oDrawer.handleClosedToolTip
        }
        QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay

        DragHandler {
            id: dragHandler
            target: null
            acceptedDevices: PointerDevice.TouchScreen | PointerDevice.Stylus
            property real prevTranslationX: 0
            onActiveTranslationChanged: {
                if (!root.drawer.contentItem) return;
                let delta = dragHandler.activeTranslation.x - dragHandler.prevTranslationX;
                dragHandler.prevTranslationX = dragHandler.activeTranslation.x;
                let cw = root.drawer.contentItem.width;
                if (cw <= 0) return;
                let positionDelta = delta / cw;
                if (root.drawer.edge === Qt.RightEdge) {
                    positionDelta *= -1;
                }
                root.drawer.position += positionDelta;
            }
            onGrabChanged: (transition, point) => {
                switch (transition) {
                case PointerDevice.GrabExclusive:
                case PointerDevice.GrabPassive:
                    root.drawer.peeking = true;
                    break;
                case PointerDevice.UngrabExclusive:
                case PointerDevice.UngrabPassive:
                case PointerDevice.CancelGrabExclusive:
                case PointerDevice.CancelGrabPassive:
                    root.drawer.peeking = false;
                    break;
                default:
                    break;
                }
            }
        }
    }

    property Item handleAnchor: {
        if (typeof applicationWindow === "undefined") return null;
        const window = applicationWindow();
        const globalToolBar = window.pageStack?.globalToolBar;
        if (!globalToolBar) return null;
        return (drawer.edge === Qt.LeftEdge && !LayoutMirroring.enabled) || (drawer.edge === Qt.RightEdge && LayoutMirroring.enabled)
            ? globalToolBar.leftHandleAnchor : globalToolBar.rightHandleAnchor;
    }

    enabled: (drawer as KT.OverlayDrawer).handleVisible

    x: {
        if (!drawer || !drawer.background) return 0;
        switch (drawer.edge) {
        case Qt.LeftEdge:  return drawer.background.width * drawer.position + Kirigami.Units.smallSpacing;
        case Qt.RightEdge: return parent.width - (drawer.background.width * drawer.position) - width - Kirigami.Units.smallSpacing;
        default:           return 0;
        }
    }

    Binding {
        when: root.handleAnchor && root.anchors.bottom
        target: root
        property: "y"
        value: root.handleAnchor ? root.handleAnchor.Kirigami.ScenePosition.y : 0
        restoreMode: Binding.RestoreBinding
    }

    anchors {
        bottom: handleAnchor ? undefined : parent.bottom
        bottomMargin: {
            if (typeof applicationWindow === "undefined") return undefined;
            const window = applicationWindow();
            let margin = Kirigami.Units.smallSpacing;
            if (window.footer) margin = window.footer.height + Kirigami.Units.smallSpacing;
            if (drawer.parent && drawer.height < drawer.parent.height)
                margin = drawer.parent.height - drawer.height - drawer.y + Kirigami.Units.smallSpacing;
            if (!window || !window.pageStack || !window.pageStack.contentItem || !window.pageStack.contentItem.itemAt) return margin;
            let item;
            if (window.pageStack.layers.depth > 1) item = window.pageStack.layers.currentItem;
            else item = window.pageStack.contentItem.itemAt(window.pageStack.contentItem.contentX + x, 0);
            if (!item) item = window.pageStack.lastItem;
            let pageFooter = item && item.page ? item.page.footer : (item ? item.footer : undefined);
            if (pageFooter && drawer.parent) margin = drawer.height < drawer.parent.height ? margin : margin + pageFooter.height;
            return margin;
        }
        Behavior on bottomMargin {
            NumberAnimation { duration: Kirigami.Units.shortDuration; easing.type: Easing.InOutQuad }
        }
    }

    visible: drawer.enabled && drawer.modal && (drawer.edge === Qt.LeftEdge || drawer.edge === Qt.RightEdge) && opacity > 0
    width: handleAnchor?.visible ? handleAnchor.width : Kirigami.Units.iconSizes.smallMedium + Kirigami.Units.smallSpacing * 2
    height: handleAnchor?.visible ? handleAnchor.height : width
    opacity: handleAnchor && applicationWindow()?.pageStack.depth > 0
            ? drawer.position * (root.drawer as KT.OverlayDrawer).handleVisible : 1

    transform: Translate {
        x: (root.drawer as KT.OverlayDrawer).handleVisible ? 0
           : (root.drawer.edge === Qt.LeftEdge ? -Math.max(root.width, button.width)
                                               : Math.max(root.width, button.width))
        Behavior on x {
            NumberAnimation {
                duration: Kirigami.Units.longDuration
                easing.type: !(root.drawer as KT.OverlayDrawer).handleVisible ? Easing.OutQuad : Easing.InQuad
            }
        }
    }
}
)qml";

//! Qmldir for org.kde.kirigami.templates — without prefer, linktarget, or plugin.
static constexpr auto kTemplatesQmldir = R"qmldir(module org.kde.kirigami.templates
typeinfo KirigamiTemplates.qmltypes
AbstractApplicationHeader 2.0 AbstractApplicationHeader.qml
AbstractCard 2.0 AbstractCard.qml
singleton AppHeaderSizeGroup 2.0 AppHeaderSizeGroup.qml
Chip 2.0 Chip.qml
Heading 2.0 Heading.qml
InlineMessage 2.0 InlineMessage.qml
LinkButton 2.0 LinkButton.qml
NavigationTabBar 2.0 NavigationTabBar.qml
NavigationTabButton 2.0 NavigationTabButton.qml
OverlayDrawer 2.0 OverlayDrawer.qml
OverlaySheet 2.0 OverlaySheet.qml
InlineViewHeader 2.0 InlineViewHeader.qml
internal BorderPropertiesGroup private/BorderPropertiesGroup.qml
internal DrawerHandle private/DrawerHandle.qml
)qmldir";

//! Qmldir for org.kde.newstuff — with plugin but without prefer.
static constexpr auto kNewstuffQmldir = R"qmldir(module org.kde.newstuff
linktarget KF6NewStuffQmlPlugin
optional plugin newstuffqmlplugin
classname org_kde_newstuff_qmlPlugin
typeinfo newstuffqmlplugin.qmltypes
depends org.kde.kirigami
depends QtQuick
depends QtQuick.Controls
depends QtQuick.Layouts
depends org.kde.kcmutils
depends org.kde.coreaddons
Action 1.81 Action.qml
Button 1.1 Button.qml
Dialog 1.1 Dialog.qml
DialogContent 1.1 DialogContent.qml
DownloadItemsSheet 1.1 DownloadItemsSheet.qml
EntryDetails 1.1 EntryDetails.qml
Page 1.1 Page.qml
QuestionAsker 1.1 QuestionAsker.qml
UploadPage 1.85 UploadPage.qml
)qmldir";

static constexpr auto kSystemQmlBase = "/usr/lib64/qt6/qml";

static bool writeIfChanged(const QString &path, const QString &content)
{
    QFile f(path);
    if (f.open(QFile::ReadOnly)) {
        if (f.readAll() == content.toUtf8()) {
            return false; // unchanged
        }
        f.close();
    }

    QFileInfo fi(path);
    QDir().mkpath(fi.absolutePath());

    if (f.open(QFile::WriteOnly | QFile::Truncate)) {
        f.write(content.toUtf8());
        return true;
    }
    qWarning() << "KnsCompat: cannot write" << path;
    return false;
}

//! Patched HandleButton.qml — same fix for controls/private/globaltoolbar/HandleButton.qml.
static constexpr auto kPatchedHandleButton = R"qml(/*
 *  SPDX-FileCopyrightText: 2023 Marco Martin <mart@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Templates as T
import org.kde.kirigami as Kirigami
import org.kde.kirigami.templates as KT

Item {
    id: root
    property T.Drawer drawer

    QQC2.ToolButton {
        anchors.centerIn: parent
        flat: false
        icon.name: root.drawer.position > 0 ? root.drawer.handleOpenIcon.name : root.drawer.handleClosedIcon.name
        icon.source: root.drawer.position > 0 ? root.drawer.handleOpenIcon.source ?? "" : root.drawer.handleClosedIcon.source ?? ""
        icon.width: root.drawer.handleOpenIcon.width
        icon.height: root.drawer.handleOpenIcon.height

        onClicked: {
            const oDrawer = root.drawer as KT.OverlayDrawer
            oDrawer.drawerOpen = !oDrawer.drawerOpen;
        }
        Keys.onEscapePressed: {
            if (root.drawer.closePolicy & T.Popup.CloseOnEscape)
                root.drawer.drawerOpen = false;
        }

        DragHandler {
            id: dragHandler
            target: null
            acceptedDevices: PointerDevice.TouchScreen
            property real prevTranslationX: 0
            onActiveTranslationChanged: {
                if (!root.drawer.contentItem) return;
                let delta = dragHandler.activeTranslation.x - dragHandler.prevTranslationX;
                dragHandler.prevTranslationX = dragHandler.activeTranslation.x;
                let cw = root.drawer.contentItem.width;
                if (cw <= 0) return;
                let positionDelta = delta / cw;
                if (root.drawer.edge === Qt.RightEdge)
                    positionDelta *= -1;
                root.drawer.position += positionDelta;
            }
            onGrabChanged: (transition, point) => {
                switch (transition) {
                case PointerDevice.GrabExclusive:
                case PointerDevice.GrabPassive:
                    root.drawer.peeking = true; break;
                case PointerDevice.UngrabExclusive:
                case PointerDevice.UngrabPassive:
                case PointerDevice.CancelGrabExclusive:
                case PointerDevice.CancelGrabPassive:
                    root.drawer.peeking = false; break;
                default: break;
                }
            }
        }
    }

    x: {
        switch (drawer.edge) {
        case Qt.LeftEdge:  return drawer.background.width * drawer.position + Kirigami.Units.smallSpacing;
        case Qt.RightEdge: return parent.width - (drawer.background.width * drawer.position) - width - Kirigami.Units.smallSpacing;
        default:           return 0;
        }
    }
}
)qml";

static bool symlinkChecked(const QString &target, const QString &link);

//! Recursively symlink a private QML directory, skipping specified paths.
static void symlinkPrivDir(const QDir &srcDir, const QString &dstPath, const QStringList &skipPaths)
{
    QDir().mkpath(dstPath);
    for (const auto &entry : srcDir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot)) {
        const QString src = srcDir.absoluteFilePath(entry);
        const QString dst = dstPath + QLatin1Char('/') + entry;
        const QString relPath = QDir(dstPath).relativeFilePath(dst);
        bool matched = skipPaths.contains(relPath) || skipPaths.contains(entry);
        if (!matched) {
            QString checkPath = QDir::cleanPath(dst);
            for (const auto &sp : skipPaths) {
                if (checkPath.endsWith(sp)) {
                    matched = true;
                    break;
                }
            }
        }
        if (matched) continue;
        if (QFileInfo(src).isDir()) {
            symlinkPrivDir(QDir(src), dst, skipPaths);
        } else {
            symlinkChecked(src, dst);
        }
    }
}

static bool symlinkChecked(const QString &target, const QString &link)
{
    QFileInfo li(link);
    if (li.isSymLink() && li.symLinkTarget() == target) {
        return false; // already correct
    }
    QDir().mkpath(QFileInfo(link).absolutePath());
    QFile::remove(link);
    return QFile::link(target, link);
}

void ensureKnsCompat()
{
    const QString stampPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
                              + QStringLiteral("/latte-dock-ng/kns-compat.stamp");
    int currentVersion = 0;
    {
        QFile f(stampPath);
        if (f.open(QFile::ReadOnly)) {
            currentVersion = f.readAll().trimmed().toInt();
        }
    }

    // Check if we need to re-create the overrides
    bool needsUpdate = (currentVersion < kCompatVersion);

    const QString qmlBase = QDir::homePath() + QStringLiteral("/.local/lib64/qt6/qml");
    const QString templatesDir = qmlBase + QStringLiteral("/org/kde/kirigami/templates");
    const QString newstuffDir = qmlBase + QStringLiteral("/org/kde/newstuff");

    if (!needsUpdate && QFile::exists(templatesDir + QStringLiteral("/qmldir"))
        && QFile::exists(newstuffDir + QStringLiteral("/qmldir"))) {
        return;
    }

    qDebug() << "KnsCompat: setting up KNS dialog compatibility overrides (v" << kCompatVersion << ")";

    // --- Kirigami templates module (pure filesystem, no plugin) ---
    const QString sysTemplates = QLatin1String(kSystemQmlBase) + QStringLiteral("/org/kde/kirigami/templates");

    writeIfChanged(templatesDir + QStringLiteral("/qmldir"), QLatin1String(kTemplatesQmldir));

    // Write patched DrawerHandle.qml FIRST (before symlinking the rest)
    writeIfChanged(templatesDir + QStringLiteral("/private/DrawerHandle.qml"), QLatin1String(kPatchedDrawerHandle));

    // Symlink all other QML files and typeinfo from system
    QDir sysDir(sysTemplates);
    for (const auto &entry : sysDir.entryList({QStringLiteral("*.qml"), QStringLiteral("*.qmltypes")}, QDir::Files)) {
        symlinkChecked(sysDir.absoluteFilePath(entry), templatesDir + QLatin1Char('/') + entry);
    }
    // Symlink private files (except DrawerHandle which is our patched version)
    QDir sysPriv(sysTemplates + QStringLiteral("/private"));
    const auto privFilter = QStringList{QStringLiteral("*.qml")};
    for (const auto &entry : sysPriv.entryList(privFilter, QDir::Files)) {
        if (entry == QStringLiteral("DrawerHandle.qml")) {
            continue; // our patched version, not a symlink
        }
        symlinkChecked(sysPriv.absoluteFilePath(entry), templatesDir + QStringLiteral("/private/") + entry);
    }

    // --- Newstuff module (with plugin, no prefer) ---
    const QString sysNewstuff = QLatin1String(kSystemQmlBase) + QStringLiteral("/org/kde/newstuff");

    writeIfChanged(newstuffDir + QStringLiteral("/qmldir"), QLatin1String(kNewstuffQmldir));

    QDir nsDir(sysNewstuff);
    for (const auto &entry : nsDir.entryList({QStringLiteral("*.qml"), QStringLiteral("*.qmltypes")}, QDir::Files)) {
        symlinkChecked(nsDir.absoluteFilePath(entry), newstuffDir + QLatin1Char('/') + entry);
    }
    // Symlink plugin and private files
    symlinkChecked(sysNewstuff + QStringLiteral("/libnewstuffqmlplugin.so"),
                   newstuffDir + QStringLiteral("/libnewstuffqmlplugin.so"));
    symlinkChecked(sysNewstuff + QStringLiteral("/kde-qmlmodule.version"),
                   newstuffDir + QStringLiteral("/kde-qmlmodule.version"));

    // Private subdirectories
    QDir nsPriv(sysNewstuff + QStringLiteral("/private"));
    for (const auto &entry : nsPriv.entryList(QDir::Filters(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot))) {
        const QString target = nsPriv.absoluteFilePath(entry);
        const QString link = newstuffDir + QStringLiteral("/private/") + entry;
        if (QFileInfo(target).isDir()) {
            QDir().mkpath(link);
            QDir targetDir(target);
            const auto entries = targetDir.entryList(QDir::Filters(QDir::Files | QDir::NoDotAndDotDot));
            for (const auto &sub : entries) {
                symlinkChecked(target + QLatin1Char('/') + sub, link + QLatin1Char('/') + sub);
            }
        } else {
            symlinkChecked(target, link);
        }
    }

    // --- Kirigami controls module (with plugin, no prefer; patched HandleButton) ---
    const QString sysControls = QLatin1String(kSystemQmlBase) + QStringLiteral("/org/kde/kirigami/controls");
    const QString controlsDir = qmlBase + QStringLiteral("/org/kde/kirigami/controls");

    // Use the system qmldir but strip the prefer line
    {
        QFile sysQmldir(sysControls + QStringLiteral("/qmldir"));
        if (sysQmldir.open(QFile::ReadOnly)) {
            QString content = QString::fromUtf8(sysQmldir.readAll());
            content.remove(QRegularExpression(QStringLiteral("^prefer .*\n?"), QRegularExpression::MultilineOption));
            writeIfChanged(controlsDir + QStringLiteral("/qmldir"), content);
        }
    }

    // Write patched HandleButton.qml
    writeIfChanged(controlsDir + QStringLiteral("/private/globaltoolbar/HandleButton.qml"),
                   QLatin1String(kPatchedHandleButton));

    // Symlink all system files (but don't overwrite our patched files)
    QDir scDir(sysControls);
    for (const auto &entry : scDir.entryList({QStringLiteral("*.qml"), QStringLiteral("*.qmltypes")}, QDir::Files)) {
        symlinkChecked(scDir.absoluteFilePath(entry), controlsDir + QLatin1Char('/') + entry);
    }
    // Plugin and version files
    symlinkChecked(sysControls + QStringLiteral("/libKirigamiControlsplugin.so"),
                   controlsDir + QStringLiteral("/libKirigamiControlsplugin.so"));
    symlinkChecked(sysControls + QStringLiteral("/kde-qmlmodule.version"),
                   controlsDir + QStringLiteral("/kde-qmlmodule.version"));
    // Private subdirectories (recursively, skipping our patched HandleButton)
    const QDir scPriv(sysControls + QStringLiteral("/private"));
    symlinkPrivDir(scPriv, controlsDir + QStringLiteral("/private"),
                   {QStringLiteral("globaltoolbar/HandleButton.qml")});

    // Write stamp
    QFile f(stampPath);
    if (f.open(QFile::WriteOnly | QFile::Truncate)) {
        f.write(QByteArray::number(kCompatVersion));
    }

    qDebug() << "KnsCompat: done";
}
