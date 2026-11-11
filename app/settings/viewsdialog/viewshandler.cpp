/*
    SPDX-FileCopyrightText: 2021 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <latte_debug.h>
#include "viewshandler.h"

// local
#include "ui_viewsdialog.h"
#include "viewscontroller.h"
#include "viewsdialog.h"
#include "viewstableview.h"
#include "../exporttemplatedialog/exporttemplatedialog.h"
#include "../settingsdialog/layoutscontroller.h"
#include "../settingsdialog/layoutsmodel.h"
#include "../settingsdialog/delegates/layoutcmbitemdelegate.h"
#include "../../data/layoutstable.h"
#include "../../data/genericbasictable.h"
#include "../../data/viewstable.h"
#include "../../lattecorona.h"
#include "../../layout/abstractlayout.h"
#include "../../layout/centrallayout.h"
#include "../../layouts/manager.h"
#include "../../layouts/storage.h"
#include "../../layouts/synchronizer.h"
#include "../../templates/templatesmanager.h"
#include "../../tools/commontools.h"

// Qt
#include <QDir>
#include <QFileDialog>

// KDE
#include <KLocalizedString>
#include <KStandardGuiItem>
#include <KIO/OpenFileManagerWindowJob>
#include <KIO/OpenUrlJob>

namespace Latte {
namespace Settings {
namespace Handler {

ViewsHandler::ViewsHandler(Dialog::ViewsDialog *dialog)
    : Generic(dialog),
      m_dialog(dialog),
      m_ui(m_dialog->ui())
{
    m_viewsController = new Settings::Controller::Views(this);

    init();
}

ViewsHandler::~ViewsHandler()
{
}

void ViewsHandler::init()
{
    //! Layouts
    m_layoutsProxyModel = new QSortFilterProxyModel(this);
    m_layoutsProxyModel->setSourceModel(m_dialog->layoutsController()->baseModel());
    m_layoutsProxyModel->setSortRole(Model::Layouts::SORTINGROLE);
    m_layoutsProxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    m_layoutsProxyModel->sort(Model::Layouts::NAMECOLUMN, Qt::AscendingOrder);

    m_ui->layoutsCmb->setModel(m_layoutsProxyModel);
    m_ui->layoutsCmb->setModelColumn(Model::Layouts::NAMECOLUMN);
    m_ui->layoutsCmb->setItemDelegate(new Settings::Layout::Delegate::LayoutCmbItemDelegate(this));

    //! New Button
    m_newViewAction = new QAction(i18nc("new view", "&New"), this);
    m_newViewAction->setToolTip(i18n("New dock"));
    m_newViewAction->setIcon(QIcon::fromTheme(QStringLiteral("add")));
    m_newViewAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_N));
    connectActionWithButton(m_ui->newBtn, m_newViewAction);
    connect(m_newViewAction, &QAction::triggered, m_ui->newBtn, &QPushButton::showMenu);

    initViewTemplatesSubMenu();
    m_newViewAction->setMenu(m_viewTemplatesSubMenu);
    m_ui->newBtn->setMenu(m_viewTemplatesSubMenu);

    connect(corona()->templatesManager(), &Latte::Templates::Manager::viewTemplatesChanged, this, &ViewsHandler::initViewTemplatesSubMenu);

    //! Duplicate Button
    m_duplicateViewAction = new QAction(i18nc("duplicate dock", "&Duplicate"), this);
    m_duplicateViewAction->setToolTip(i18n("Duplicate selected dock"));
    m_duplicateViewAction->setIcon(QIcon::fromTheme(QStringLiteral("edit-copy")));
    m_duplicateViewAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_D));
    connectActionWithButton(m_ui->duplicateBtn, m_duplicateViewAction);
    connect(m_duplicateViewAction, &QAction::triggered, m_viewsController, &Controller::Views::duplicateSelectedViews);

    //! Remove Button
    m_removeViewAction = new QAction(i18nc("remove layout", "Remove"), m_ui->removeBtn);
    m_removeViewAction->setToolTip(i18n("Remove selected view"));
    m_removeViewAction->setIcon(QIcon::fromTheme(QStringLiteral("delete")));
    m_removeViewAction->setShortcut(QKeySequence(Qt::Key_Delete));
    connectActionWithButton(m_ui->removeBtn, m_removeViewAction);
    connect(m_removeViewAction, &QAction::triggered, this, &ViewsHandler::removeSelectedViews);
    m_ui->removeBtn->addAction(m_removeViewAction); //this is needed in order to be triggered properly

    //! Import
    m_importViewAction = new QAction(i18nc("import dock", "&Import..."));
    m_importViewAction->setToolTip(i18n("Import dock from local file"));
    m_importViewAction->setIcon(QIcon::fromTheme(QStringLiteral("document-import")));
    m_importViewAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_I));
    connectActionWithButton(m_ui->importBtn, m_importViewAction);
    connect(m_importViewAction, &QAction::triggered, this, &ViewsHandler::importView);

    //! Export
    m_exportViewAction = new QAction(i18nc("export layout", "&Export"), this);
    m_exportViewAction->setToolTip(i18n("Export selected dock at your system"));
    m_exportViewAction->setIcon(QIcon::fromTheme(QStringLiteral("document-export")));
    m_exportViewAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_E));
    connectActionWithButton(m_ui->exportBtn, m_exportViewAction);
    connect(m_exportViewAction, &QAction::triggered, m_ui->exportBtn, &QPushButton::showMenu);

    initViewExportSubMenu();
    m_exportViewAction->setMenu(m_viewExportSubMenu);
    m_ui->exportBtn->setMenu(m_viewExportSubMenu);

    //! signals
    connect(this, &ViewsHandler::currentLayoutChanged, this, &ViewsHandler::reload);

    reload();
    m_lastConfirmedLayoutIndex = m_ui->layoutsCmb->currentIndex();

    Q_EMIT currentLayoutChanged();

    //! connect layout combobox after the selected layout has been loaded
    connect(m_ui->layoutsCmb, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ViewsHandler::onCurrentLayoutIndexChanged);

    //!
    connect(m_viewsController, &Settings::Controller::Views::dataChanged, this, &ViewsHandler::dataChanged);

    connect(m_ui->viewsTable, &View::ViewsTableView::selectionsChanged, this, &ViewsHandler::onSelectionChanged);

    onSelectionChanged();
}

void ViewsHandler::initViewTemplatesSubMenu()
{
    if (!m_viewTemplatesSubMenu) {
        m_viewTemplatesSubMenu = new QMenu(m_ui->newBtn);
        m_viewTemplatesSubMenu->setMinimumWidth(m_ui->newBtn->width() * 2);
    } else {
        m_viewTemplatesSubMenu->clear();
    }

    /*Add View Templates for New Action*/
    Data::GenericBasicTable templates = corona()->templatesManager()->viewTemplates();

    bool customtemplateseparatoradded{false};

    for (int i = 0; i < templates.rowCount(); ++i) {
        if (!customtemplateseparatoradded && templates[i].id.startsWith(QDir::homePath())) {
            m_viewTemplatesSubMenu->addSeparator();
            customtemplateseparatoradded = true;
        }

        QAction *newview = m_viewTemplatesSubMenu->addAction(templates[i].name);
        newview->setIcon(QIcon::fromTheme(QStringLiteral("document-new")));

        Data::Generic templateData = templates[i];

        connect(newview, &QAction::triggered, this, [ &, templateData]() {
            newView(templateData);
        });
    }

    if (templates.rowCount() > 0) {
        //! This item opens a folder and creates no dock, so it is kept apart from the template entries above.
        m_viewTemplatesSubMenu->addSeparator();

        QAction *openTemplatesDirectory = m_viewTemplatesSubMenu->addAction(i18n("Templates..."));
        openTemplatesDirectory->setToolTip(i18n("Open templates directory"));
        openTemplatesDirectory->setIcon(QIcon::fromTheme(QStringLiteral("edit")));

        connect(openTemplatesDirectory, &QAction::triggered, this, [this]() {
            //! Custom *.view.latte templates live in this folder, which Templates::Manager watches. It is opened as a
            //! folder: the file that this item used to name is not in the folder, so the file manager showed a listing
            //! with nothing highlighted. The folder is created when missing, so the window opens on every system.
            const QString folder = Latte::configPath() + QLatin1String("/latte/templates");
            QDir().mkpath(folder);

            auto job = new KIO::OpenUrlJob(QUrl::fromLocalFile(folder), QStringLiteral("inode/directory"), this);
            job->start();
        });
    }
}

void ViewsHandler::initViewExportSubMenu()
{
    if (!m_viewExportSubMenu) {
        m_viewExportSubMenu = new QMenu(m_ui->exportBtn);
        m_viewExportSubMenu->setMinimumWidth(m_ui->exportBtn->width() * 2);
    } else {
        m_viewExportSubMenu->clear();
    }

    QAction *exportforbackup = m_viewExportSubMenu->addAction(i18nc("export for backup", "&Export For Backup..."));
    exportforbackup->setIcon(QIcon::fromTheme(QStringLiteral("document-export")));
    exportforbackup->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_E));
    connect(exportforbackup, &QAction::triggered, this, &ViewsHandler::exportViewForBackup);

    QAction *exportastemplate = m_viewExportSubMenu->addAction(i18nc("export as template", "Export As &Template..."));
    exportastemplate->setIcon(QIcon::fromTheme(QStringLiteral("document-export")));
    exportastemplate->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_T));
    connect(exportastemplate, &QAction::triggered, this, &ViewsHandler::exportViewAsTemplate);
}

void ViewsHandler::reload()
{
    o_data = m_dialog->layoutsController()->selectedLayoutCurrentData();
    o_data.views = m_dialog->layoutsController()->selectedLayoutViews();

    Latte::Data::LayoutIcon icon = m_dialog->layoutsController()->selectedLayoutIcon();

    m_ui->layoutsCmb->setCurrentText(o_data.name);
    m_ui->layoutsCmb->setLayoutIcon(icon);

    loadLayout(o_data);
}

Latte::Corona *ViewsHandler::corona() const
{
    return m_dialog->corona();
}

Ui::ViewsDialog *ViewsHandler::ui() const
{
    return m_ui;
}

Settings::Controller::Layouts *ViewsHandler::layoutsController() const
{
    return m_dialog->layoutsController();
}

void ViewsHandler::loadLayout(const Latte::Data::Layout &)
{
    updateWindowTitle();
}

Latte::Data::Layout ViewsHandler::currentData() const
{
    return o_data;
}

Latte::Data::Layout ViewsHandler::originalData() const
{
    return m_dialog->layoutsController()->selectedLayoutOriginalData();
}

bool ViewsHandler::hasChangedData() const
{
    return m_viewsController->hasChangedData();
}

bool ViewsHandler::inDefaultValues() const
{
    //nothing special
    return true;
}

bool ViewsHandler::isSelectedLayoutOriginal() const
{
    return m_dialog->layoutsController()->isSelectedLayoutOriginal();
}

void ViewsHandler::reset()
{
    m_viewsController->reset();
}

void ViewsHandler::resetDefaults()
{
    //do nothing
}

void ViewsHandler::save()
{
    int viewsforremoval = m_viewsController->viewsForRemovalCount();

    if (viewsforremoval <= 0 || removalConfirmation(viewsforremoval) == KMessageBox::PrimaryAction) {
        m_viewsController->save();
    }
}

QString ViewsHandler::storedView(const QString &viewId)
{
    Latte::Data::View viewdata = m_viewsController->currentData(viewId);

    if (!viewdata.isValid()) {
        return QString();
    }

    if (viewdata.isCreated()) {
        CentralLayout *central = m_dialog->layoutsController()->centralLayout(currentData().id);
        return central->storedView(viewdata.id.toInt());
    } else if (viewdata.hasViewTemplateOrigin() || viewdata.hasLayoutOrigin()) {
        return viewdata.originFile();
    }

    return QString();
}

void ViewsHandler::newView(const Data::Generic &templateData)
{
    Data::ViewsTable views = Latte::Layouts::Storage::self()->views(templateData.id);

    if (views.rowCount() <= 0) {
        //! The file holds no dock: a file of another kind, or an empty one. Refuse it visibly and name the file. This
        //! used to return without a word, so the New menu and Import looked like they did nothing.
        qCWarning(latteSettings) << "No dock is added: the dock file" << templateData.id << "has no dock.";
        showInlineMessage(i18nc("settings:dock file without dock", "<b>%1</b> has no dock, so no dock was added.", templateData.name), KMessageWidget::Warning);
        return;
    }

    Data::View viewfromtemplate = views[0];
    viewfromtemplate.setState(Data::View::OriginFromViewTemplate, templateData.id);
    viewfromtemplate.name = templateData.name;
    Data::View newview = m_viewsController->appendViewFromViewTemplate(viewfromtemplate);

    showInlineMessage(i18nc("settings:dock added successfully", "<b>%1</b> added successfully...", newview.name), KMessageWidget::Positive);
}

void ViewsHandler::removeSelectedViews()
{
    qCDebug(latteSettings) << Q_FUNC_INFO;

    if (!m_removeViewAction->isEnabled() || !m_viewsController->hasSelectedView()) {
        return;
    }

    m_viewsController->removeSelectedViews();
}

void ViewsHandler::exportViewForBackup()
{
    if (!m_viewsController->hasSelectedView()) {
        return;
    }

    if (m_viewsController->selectedViewsCount() > 1) {
        showInlineMessage(i18n("<b>Export</b> functionality is supported only for one dock each time."),
                          KMessageWidget::Warning,
                          false);
        return;
    }

    Data::ViewsTable views =  m_viewsController->selectedViewsCurrentData();

    if (views.rowCount() != 1) {
        return;
    }

    QString temporiginfile = storedView(views[0].id);

    QFileDialog *exportFileDialog = new QFileDialog(m_dialog, i18n("Export Dock For Backup"), QDir::homePath(), QStringLiteral("view.latte"));

    exportFileDialog->setLabelText(QFileDialog::Accept, i18nc("export view", "Export"));
    exportFileDialog->setFileMode(QFileDialog::AnyFile);
    exportFileDialog->setAcceptMode(QFileDialog::AcceptSave);
    exportFileDialog->setDefaultSuffix(QStringLiteral("view.latte"));

    QStringList filters;
    QString filter1(i18nc("export view", "Latte Dock NG file v0.2") + QStringLiteral("(*.view.latte)"));

    filters << filter1;

    exportFileDialog->setNameFilters(filters);

    connect(exportFileDialog, &QFileDialog::finished, exportFileDialog, &QFileDialog::deleteLater);

    connect(exportFileDialog, &QFileDialog::fileSelected, this, [ &, temporiginfile](const QString & file) {
        auto showExportViewError = [this](const QString & destinationfile) {
            showInlineMessage(i18nc("settings:view export fail", "Export in file <b>%1</b> <b>failed</b>...", QFileInfo(destinationfile).fileName()),
                              KMessageWidget::Error,
                              true);
        };

        if (QFile::exists(file) && !QFile::remove(file)) {
            showExportViewError(file);
            return;
        }

        if (file.endsWith(QStringLiteral(".view.latte"))) {
            if (!QFile(temporiginfile).copy(file)) {
                showExportViewError(file);
                return;
            }

            QAction *openUrlAction = new QAction(i18n("Open Location..."), this);
            openUrlAction->setData(file);
            QList<QAction *> actions;
            actions << openUrlAction;

            connect(openUrlAction, &QAction::triggered, this, [ &, openUrlAction]() {
                QString file = openUrlAction->data().toString();

                if (!file.isEmpty()) {
                    KIO::highlightInFileManager(QList<QUrl>({QUrl::fromLocalFile(file)}));
                }
            });

            showInlineMessage(i18nc("settings:view export success", "Export in file <b>%1</b> succeeded...", QFileInfo(file).fileName()),
                              KMessageWidget::Positive,
                              false,
                              actions);
        }
    });

    exportFileDialog->open();
    exportFileDialog->selectFile(views[0].name);
}

void ViewsHandler::exportViewAsTemplate()
{
    if (!m_viewsController->hasSelectedView()) {
        return;
    }

    if (m_viewsController->selectedViewsCount() > 1) {
        showInlineMessage(i18n("<b>Export</b> functionality is supported only for one dock each time."),
                          KMessageWidget::Warning,
                          false);
        return;
    }

    Data::ViewsTable views =  m_viewsController->selectedViewsCurrentData();

    if (views.rowCount() != 1) {
        return;
    }

    Data::View exportview = views[0];
    exportview.id = storedView(views[0].id);
    exportview.setState(Data::View::OriginFromLayout, exportview.id, currentData().name, views[0].id);

    Dialog::ExportTemplateDialog *exportdlg = new Dialog::ExportTemplateDialog(m_dialog, exportview);
    exportdlg->exec();
}

void ViewsHandler::importView()
{
    qCDebug(latteSettings) << Q_FUNC_INFO;

    QFileDialog *importFileDialog = new QFileDialog(m_dialog, i18nc("import dock", "Import Dock"), QDir::homePath(), QStringLiteral("view.latte"));

    importFileDialog->setWindowIcon(QIcon::fromTheme(QStringLiteral("document-import")));
    importFileDialog->setLabelText(QFileDialog::Accept, i18n("Import"));
    importFileDialog->setFileMode(QFileDialog::AnyFile);
    importFileDialog->setAcceptMode(QFileDialog::AcceptOpen);
    importFileDialog->setDefaultSuffix(QStringLiteral("view.latte"));

    QStringList filters;
    filters << i18nc("import dock", "Latte Dock NG file v0.2") + QStringLiteral("(*.view.latte)");
    importFileDialog->setNameFilters(filters);

    connect(importFileDialog, &QFileDialog::finished, importFileDialog, &QFileDialog::deleteLater);

    connect(importFileDialog, &QFileDialog::fileSelected, this, [this](const QString & file) {
        Data::Generic templatedata;
        templatedata.id = file;
        templatedata.name = QFileInfo(file).fileName();
        templatedata.name = templatedata.name.remove(QStringLiteral(".view.latte"));
        newView(templatedata);
    });

    importFileDialog->open();
}

void ViewsHandler::onCurrentLayoutIndexChanged(int row)
{
    bool switchtonewlayout{false};

    if (m_lastConfirmedLayoutIndex != row) {
        if (hasChangedData()) { //new layout was chosen but there are changes
            KMessageBox::ButtonCode result = saveChangesConfirmation();

            if (result == KMessageBox::PrimaryAction) {
                int removalviews = m_viewsController->viewsForRemovalCount();
                KMessageBox::ButtonCode removalresponse = removalConfirmation(removalviews);

                if (removalresponse == KMessageBox::PrimaryAction) {
                    switchtonewlayout = true;
                    m_lastConfirmedLayoutIndex = row;
                    m_viewsController->save();
                } else {
                    //do nothing
                }
            } else if (result == KMessageBox::SecondaryAction) {
                switchtonewlayout = true;
                m_lastConfirmedLayoutIndex = row;
            } else if (result == KMessageBox::Cancel) {
                //do nothing
            }
        } else { //new layout was chosen and there are no changes
            switchtonewlayout = true;
            m_lastConfirmedLayoutIndex = row;
        }
    }

    if (switchtonewlayout) {
        m_dialog->deleteInlineMessages();
        QString layoutId = m_layoutsProxyModel->data(m_layoutsProxyModel->index(row, Model::Layouts::IDCOLUMN), Qt::UserRole).toString();
        m_dialog->layoutsController()->selectRow(layoutId);
        reload();
        Q_EMIT currentLayoutChanged();
    } else {
        //! reset combobox index
        m_ui->layoutsCmb->setCurrentText(o_data.name);
    }
}

void ViewsHandler::onSelectionChanged()
{
    bool hasselected = m_viewsController->hasSelectedView();

    setTwinProperty(m_duplicateViewAction, QLatin1String(TWINENABLED), hasselected);
    setTwinProperty(m_removeViewAction, QLatin1String(TWINENABLED), hasselected);
    setTwinProperty(m_exportViewAction, QLatin1String(TWINENABLED), hasselected);
    m_viewExportSubMenu->setEnabled(hasselected);
}

void ViewsHandler::updateWindowTitle()
{
    m_dialog->setWindowTitle(i18nc("<layout name> Docks",
                                   "%1 Docks",
                                   m_ui->layoutsCmb->currentText()));
}

KMessageBox::ButtonCode ViewsHandler::removalConfirmation(const int &viewsCount)
{
    if (viewsCount <= 0) {
        return KMessageBox::SecondaryAction;
    }

    if (hasChangedData() && viewsCount > 0) {
        return KMessageBox::questionTwoActions(m_dialog,
                                               i18np("You are going to <b>remove 1</b> dock completely from your layout.<br/>Would you like to continue?",
                                                       "You are going to <b>remove %1</b> docks completely from your layout.<br/>Would you like to continue?",
                                                       viewsCount),
                                               i18n("Approve Removal"),
                                               KGuiItem(i18n("Remove"), QStringLiteral("edit-delete")),
                                               KStandardGuiItem::cancel());
    }

    return KMessageBox::SecondaryAction;
}

KMessageBox::ButtonCode ViewsHandler::saveChangesConfirmation()
{
    if (hasChangedData()) {
        QString layoutName = o_data.name;
        QString saveChangesText = i18n("The settings of <b>%1</b> layout have changed.<br/>Do you want to apply the changes <b>now</b> or discard them?", layoutName);

        return m_dialog->saveChangesConfirmation(saveChangesText);
    }

    return KMessageBox::Cancel;
}

}
}
}
