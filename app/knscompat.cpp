/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <latte_debug.h>
#include <latte_debug.h>
#include "knscompat.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLibraryInfo>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTextStream>

static constexpr int kCompatVersion = 9;

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

    readonly property bool drawerReady: drawer && drawer.background

    parent: overlay?.parent ?? null
    z: overlay ? overlay.z  + (drawer?.modal && (drawer as KT.OverlayDrawer)?.drawerOpen ? 1 : - 1) : 0

    QQC2.ToolButton {
        id: button
        anchors.centerIn: parent
        flat: false

        icon.name: root.drawer?.position > 0 ? root.drawer?.handleOpenIcon.name ?? root.drawer?.handleClosedIcon.name ?? ""
                                    : root.drawer?.handleClosedIcon.name ?? root.drawer?.handleOpenIcon.name ?? ""
        icon.source: root.drawer?.position > 0 ? root.drawer?.handleOpenIcon.source ?? root.drawer?.handleClosedIcon.source ?? ""
                                    : root.drawer?.handleClosedIcon.source ?? root.drawer?.handleOpenIcon.source ?? ""
        icon.width: root.drawer?.handleOpenIcon.width ?? Kirigami.Units.iconSizes.smallMedium
        icon.height: root.drawer?.handleOpenIcon.height ?? Kirigami.Units.iconSizes.smallMedium
        Accessible.name: QQC2.ToolTip.text

        onClicked: {
            root.displayToolTip = false;
            Qt.callLater(() => {
                const oDrawer = root.drawer as KT.OverlayDrawer
                oDrawer.drawerOpen = !oDrawer.drawerOpen;
            })
        }
        Keys.onEscapePressed: {
            if (root.drawer?.closePolicy & T.Popup.CloseOnEscape) {
                root.drawer.drawerOpen = false;
            }
        }

        QQC2.ToolTip.visible: root.displayToolTip && hovered
        QQC2.ToolTip.text: {
            const oDrawer = root.drawer as KT.OverlayDrawer
            return oDrawer?.drawerOpen ? oDrawer?.handleOpenToolTip ?? "" : oDrawer?.handleClosedToolTip ?? ""
        }
        QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay

        DragHandler {
            id: dragHandler
            target: null
            acceptedDevices: PointerDevice.TouchScreen | PointerDevice.Stylus
            property real prevTranslationX: 0
            onActiveTranslationChanged: {
                if (!root.drawer?.contentItem) return;
                let delta = dragHandler.activeTranslation.x - dragHandler.prevTranslationX;
                dragHandler.prevTranslationX = dragHandler.activeTranslation.x;
                let cw = root.drawer?.contentItem?.width ?? 0;
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

    enabled: (drawer as KT.OverlayDrawer)?.handleVisible ?? false

    x: {
        if (!root.drawerReady) return 0;
        switch (drawer.edge) {
        case Qt.LeftEdge:  return (drawer.background?.width ?? 0) * drawer.position + Kirigami.Units.smallSpacing;
        case Qt.RightEdge: return (parent?.width ?? 0) - ((drawer.background?.width ?? 0) * drawer.position) - width - Kirigami.Units.smallSpacing;
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
        bottom: handleAnchor || !parent ? undefined : parent.bottom
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
Badge 2.0 Badge.qml
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

static constexpr auto kDisableCompatEnv = "LATTE_DISABLE_KNS_COMPAT";
static constexpr auto kSystemRootsEnv = "LATTE_KNS_COMPAT_SYSTEM_QML_ROOTS";
static constexpr auto kUserRootEnv = "LATTE_KNS_COMPAT_USER_QML_ROOT";

static QStringList splitPathList(const QString &paths)
{
    QStringList result;

    for (const QString &path : paths.split(QDir::listSeparator(), Qt::SkipEmptyParts)) {
        const QString cleaned = QDir::cleanPath(path);
        if (!cleaned.isEmpty() && QDir::isAbsolutePath(cleaned) && !result.contains(cleaned)) {
            result << cleaned;
        }
    }

    return result;
}

static QStringList systemQmlBaseCandidates()
{
    if (qEnvironmentVariableIsSet(kSystemRootsEnv)) {
        return splitPathList(QString::fromLocal8Bit(qgetenv(kSystemRootsEnv)));
    }

    QStringList candidates;
    const auto addCandidate = [&candidates](const QString &path) {
        const QString cleaned = QDir::cleanPath(path);
        if (!cleaned.isEmpty() && !candidates.contains(cleaned)) {
            candidates << cleaned;
        }
    };

    addCandidate(QLibraryInfo::path(QLibraryInfo::QmlImportsPath));
    addCandidate(QStringLiteral("/usr/lib64/qt6/qml"));
    addCandidate(QStringLiteral("/usr/lib/qt6/qml"));
    addCandidate(QStringLiteral("/usr/lib/x86_64-linux-gnu/qt6/qml"));

    return candidates;
}

//! Kirigami >= 6.12 merged org.kde.kirigami.controls into the main
//! module (the files moved under org/kde/kirigami/), so the standalone
//! controls module only exists on older Kirigami versions.
static bool systemHasLegacyControlsModule(const QString &qmlBase)
{
    return QFileInfo(qmlBase + QStringLiteral("/org/kde/kirigami/controls/qmldir")).isFile();
}

static bool systemQmlBaseIsComplete(const QString &qmlBase)
{
    if (qmlBase.isEmpty() || !QFileInfo(qmlBase).isDir()) {
        return false;
    }

    const QString templatesDir = qmlBase + QStringLiteral("/org/kde/kirigami/templates");
    const QString newstuffDir = qmlBase + QStringLiteral("/org/kde/newstuff");

    //! Accept both the legacy standalone controls module and the merged
    //! layout used by Kirigami >= 6.12 (private/globaltoolbar lives inside
    //! the main org.kde.kirigami module there).
    const QString mainModule = qmlBase + QStringLiteral("/org/kde/kirigami");
    const bool controlsPresent = systemHasLegacyControlsModule(qmlBase)
                                 || (QFileInfo(mainModule + QStringLiteral("/qmldir")).isFile()
                                     && QFileInfo(mainModule + QStringLiteral("/private/globaltoolbar")).isDir());

    return QFileInfo(templatesDir).isDir()
           && QFileInfo(templatesDir + QStringLiteral("/private")).isDir()
           && QFileInfo(newstuffDir).isDir()
           && controlsPresent;
}

static QString resolvedSystemQmlBase()
{
    for (const QString &candidate : systemQmlBaseCandidates()) {
        if (systemQmlBaseIsComplete(candidate)) {
            return candidate;
        }
    }

    return QString();
}

static QString userLocalQmlBase(const QString &systemQmlBase)
{
    Q_UNUSED(systemQmlBase);

    if (qEnvironmentVariableIsSet(kUserRootEnv)) {
        const QString configuredRoot = QDir::cleanPath(QString::fromLocal8Bit(qgetenv(kUserRootEnv)));
        if (!configuredRoot.isEmpty() && QDir::isAbsolutePath(configuredRoot)) {
            return configuredRoot;
        }
    }

    // Use a latte-dock-private directory to avoid polluting the Qt
    // default user-local QML import path (~/.local/lib*/qt6/qml),
    // which would be searched by ALL Qt applications and can crash
    // incompatible consumers (e.g. systemsettings).
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
           + QStringLiteral("/latte-dock-ng/qml-kns-compat");
}

QString knsCompatUserQmlRoot()
{
    const QString systemQmlBase = resolvedSystemQmlBase();

    if (systemQmlBase.isEmpty()) {
        return QString();
    }

    return userLocalQmlBase(systemQmlBase);
}

static bool writeIfChanged(const QString &path, const QString &content)
{
    if (QFileInfo(path).isSymLink()) {
        QFile::remove(path);
    }

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
        icon.name: root.drawer?.position > 0 ? root.drawer?.handleOpenIcon.name ?? "" : root.drawer?.handleClosedIcon.name ?? ""
        icon.source: root.drawer?.position > 0 ? root.drawer?.handleOpenIcon.source ?? "" : root.drawer?.handleClosedIcon.source ?? ""
        icon.width: root.drawer?.handleOpenIcon.width ?? Kirigami.Units.iconSizes.smallMedium
        icon.height: root.drawer?.handleOpenIcon.height ?? Kirigami.Units.iconSizes.smallMedium

        onClicked: {
            const oDrawer = root.drawer as KT.OverlayDrawer
            oDrawer.drawerOpen = !oDrawer.drawerOpen;
        }
        Keys.onEscapePressed: {
            if (root.drawer?.closePolicy & T.Popup.CloseOnEscape)
                root.drawer.drawerOpen = false;
        }

        DragHandler {
            id: dragHandler
            target: null
            acceptedDevices: PointerDevice.TouchScreen
            property real prevTranslationX: 0
            onActiveTranslationChanged: {
                if (!root.drawer?.contentItem) return;
                let delta = dragHandler.activeTranslation.x - dragHandler.prevTranslationX;
                dragHandler.prevTranslationX = dragHandler.activeTranslation.x;
                let cw = root.drawer?.contentItem?.width ?? 0;
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
        if (!drawer) return 0;
        switch (drawer.edge) {
        case Qt.LeftEdge:  return (drawer.background?.width ?? 0) * drawer.position + Kirigami.Units.smallSpacing;
        case Qt.RightEdge: return (parent?.width ?? 0) - ((drawer.background?.width ?? 0) * drawer.position) - width - Kirigami.Units.smallSpacing;
        default:           return 0;
        }
    }
}
)qml";

static bool symlinkChecked(const QString &target, const QString &link);

//! Build the compat templates qmldir: mirror the system type list so types
//! added by newer Kirigami releases (e.g. Badge, used by controls/Badge.qml
//! via KT.Badge) keep resolving inside latte. The "prefer" line is stripped
//! so the user-local override files win over the qrc resources, and the
//! plugin lines are dropped because the override dir has no plugin. Falls
//! back to a static list when the system qmldir is unavailable.
static QString templatesQmldirForSystem(const QString &systemQmldirPath)
{
    QFile systemQmldir(systemQmldirPath);

    if (systemQmldir.open(QFile::ReadOnly)) {
        static const QRegularExpression preferPattern(QStringLiteral("^prefer .*\n?"), QRegularExpression::MultilineOption);
        static const QRegularExpression linktargetPattern(QStringLiteral("^linktarget .*\n?"), QRegularExpression::MultilineOption);
        static const QRegularExpression optionalPluginPattern(QStringLiteral("^optional plugin .*\n?"), QRegularExpression::MultilineOption);
        static const QRegularExpression classnamePattern(QStringLiteral("^classname .*\n?"), QRegularExpression::MultilineOption);

        QString text = QString::fromUtf8(systemQmldir.readAll());
        text.remove(preferPattern);
        text.remove(linktargetPattern);
        text.remove(optionalPluginPattern);
        text.remove(classnamePattern);
        return text;
    }

    return QLatin1String(kTemplatesQmldir);
}

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
    if (!QFileInfo::exists(target)) {
        return false;
    }

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
    if (qEnvironmentVariableIntValue(kDisableCompatEnv) == 1) {
        qCDebug(latteApp) << "KnsCompat: disabled by" << kDisableCompatEnv;
        return;
    }

    // Clean up overrides from the old user-local Qt QML path (pre-1.2.x)
    // that could crash other applications like systemsettings.
    const QStringList oldBases = {
                                     QDir::homePath() + QStringLiteral("/.local/lib64/qt6/qml"),
                                     QDir::homePath() + QStringLiteral("/.local/lib/qt6/qml"),
                                 };
    for (const auto &oldBase : oldBases) {
        const QStringList oldDirs = {
                                        oldBase + QStringLiteral("/org/kde/kirigami/templates"),
                                        oldBase + QStringLiteral("/org/kde/kirigami/controls"),
                                        oldBase + QStringLiteral("/org/kde/newstuff"),
                                    };
        for (const auto &d : oldDirs) {
            if (QFile::exists(d + QStringLiteral("/qmldir"))) {
                qCDebug(latteApp) << "KnsCompat: removing old overrides from" << d;
                QDir(d).removeRecursively();
            }
        }
    }

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

    const QString systemQmlBase = resolvedSystemQmlBase();
    if (systemQmlBase.isEmpty()) {
        qWarning() << "KnsCompat: system QML root not found, compatibility overrides were not created";
        return;
    }

    const QString qmlBase = userLocalQmlBase(systemQmlBase);
    const QString templatesDir = qmlBase + QStringLiteral("/org/kde/kirigami/templates");
    const QString newstuffDir = qmlBase + QStringLiteral("/org/kde/newstuff");
    const QString controlsDir = qmlBase + QStringLiteral("/org/kde/kirigami/controls");

    //! The controls override only exists on systems with the legacy
    //! standalone controls module; on newer Kirigami the files are part of
    //! the main module and there is nothing to override.
    const bool legacyControls = systemHasLegacyControlsModule(systemQmlBase);

    if (!needsUpdate && QFile::exists(templatesDir + QStringLiteral("/qmldir"))
        && QFile::exists(newstuffDir + QStringLiteral("/qmldir"))
        && (!legacyControls || QFile::exists(controlsDir + QStringLiteral("/qmldir")))) {
        return;
    }

    qCDebug(latteApp) << "KnsCompat: setting up KNS dialog compatibility overrides (v" << kCompatVersion << ")";

    // --- Kirigami templates module (pure filesystem, no plugin) ---
    const QString sysTemplates = systemQmlBase + QStringLiteral("/org/kde/kirigami/templates");

    writeIfChanged(templatesDir + QStringLiteral("/qmldir"), templatesQmldirForSystem(sysTemplates + QStringLiteral("/qmldir")));

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
    const QString sysNewstuff = systemQmlBase + QStringLiteral("/org/kde/newstuff");

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
    //! Only applicable to the legacy standalone controls module; on
    //! Kirigami >= 6.12 the files are part of the main module and the
    //! HandleButton fix does not apply.
    const QString sysControls = systemQmlBase + QStringLiteral("/org/kde/kirigami/controls");
    if (legacyControls) {

    // Use the system qmldir but strip the prefer line
    {
        QFile sysQmldir(sysControls + QStringLiteral("/qmldir"));
        if (sysQmldir.open(QFile::ReadOnly)) {
            static const QRegularExpression preferPattern(QStringLiteral("^prefer .*\n?"), QRegularExpression::MultilineOption);

            QString content = QString::fromUtf8(sysQmldir.readAll());
            content.remove(preferPattern);
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
    }

    // Write stamp
    QDir().mkpath(QFileInfo(stampPath).absolutePath());
    QFile f(stampPath);
    if (f.open(QFile::WriteOnly | QFile::Truncate)) {
        f.write(QByteArray::number(kCompatVersion));
    }

    qCDebug(latteApp) << "KnsCompat: done";
}
