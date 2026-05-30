/*
    SPDX-FileCopyrightText: 2020 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "detailshandler.h"

// local
#include "ui_detailsdialog.h"
#include "detailsdialog.h"
#include "schemesmodel.h"
#include "delegates/schemecmbitemdelegate.h"
#include "../settingsdialog/layoutscontroller.h"
#include "../settingsdialog/layoutsmodel.h"
#include "../settingsdialog/delegates/layoutcmbitemdelegate.h"

// Qt
#include <QFileDialog>
#include <QIcon>

// KDE
#include <KIconDialog>

namespace Latte {
namespace Settings {
namespace Handler {

DetailsHandler::DetailsHandler(Dialog::DetailsDialog *dialog)
    : Generic(dialog),
      m_dialog(dialog),
      m_ui(m_dialog->ui()),
      m_schemesModel(new Model::Schemes(this))
{
    init();
}

DetailsHandler::~DetailsHandler()
{
}

void DetailsHandler::init()
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

    //! Schemes
    m_ui->customSchemeCmb->setModel(m_schemesModel);
    m_ui->customSchemeCmb->setItemDelegate(new Settings::Details::Delegate::SchemeCmbItemDelegate(this));

    connect(m_ui->iconBtn, &QPushButton::pressed, this, &DetailsHandler::selectIcon);
    connect(m_ui->iconClearBtn, &QPushButton::pressed, this, &DetailsHandler::clearIcon);

    //! Options
    connect(m_ui->popUpMarginSpinBox, qOverload<int>(&QSpinBox::valueChanged), this, [&](int i) {
        setPopUpMargin(i);
    });

    connect(m_ui->inMenuChk, &QCheckBox::toggled, this, [&]() {
        setIsShownInMenu(m_ui->inMenuChk->isChecked());
    });

    connect(this, &DetailsHandler::currentLayoutChanged, this, &DetailsHandler::reload);

    reload();
    m_lastConfirmedLayoutIndex = m_ui->layoutsCmb->currentIndex();

    //! connect layout combobox after the selected layout has been loaded
    connect(m_ui->layoutsCmb, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &DetailsHandler::onCurrentLayoutIndexChanged);

    //! connect custom scheme combobox after the selected layout has been loaded
    connect(m_ui->customSchemeCmb, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &DetailsHandler::onCurrentSchemeIndexChanged);

    //! data were changed
    connect(this, &DetailsHandler::dataChanged, this, [&]() {
        loadLayout(c_data);
    });
}

void DetailsHandler::reload()
{
    o_data = m_dialog->layoutsController()->selectedLayoutCurrentData();
    c_data = o_data;

    Latte::Data::LayoutIcon icon = m_dialog->layoutsController()->selectedLayoutIcon();

    m_ui->layoutsCmb->setCurrentText(o_data.name);
    m_ui->layoutsCmb->setLayoutIcon(icon);

    loadLayout(c_data);
}

void DetailsHandler::loadLayout(const Latte::Data::Layout &data)
{
    if (data.icon.isEmpty()) {
        m_ui->iconBtn->setIcon(QIcon::fromTheme("add"));
        m_ui->iconClearBtn->setVisible(false);
    } else {
        m_ui->iconBtn->setIcon(QIcon::fromTheme(data.icon));
        m_ui->iconClearBtn->setVisible(true);
    }

    int schind = m_schemesModel->row(data.schemeFile);
    m_ui->customSchemeCmb->setCurrentIndex(schind);
    updateCustomSchemeCmb(schind);

    m_ui->popUpMarginSpinBox->setValue(data.popUpMargin);

    m_ui->inMenuChk->setChecked(data.isShownInMenu);

    updateWindowTitle();
}

Latte::Data::Layout DetailsHandler::currentData() const
{
    return c_data;
}

bool DetailsHandler::hasChangedData() const
{
    return o_data != c_data;
}

bool DetailsHandler::inDefaultValues() const
{
    return true;
}


void DetailsHandler::reset()
{
    c_data = o_data;
    Q_EMIT currentLayoutChanged();
}

void DetailsHandler::resetDefaults()
{
}

void DetailsHandler::save()
{
    m_dialog->layoutsController()->setLayoutProperties(currentData());
}

void DetailsHandler::clearIcon()
{
    setIcon("");
}

void DetailsHandler::onCurrentLayoutIndexChanged(int row)
{
    bool switchtonewlayout{false};

    if (m_lastConfirmedLayoutIndex != row) {
        if (hasChangedData()) {
            KMessageBox::ButtonCode result = saveChangesConfirmation();

            if (result == KMessageBox::PrimaryAction) {
                switchtonewlayout = true;
                m_lastConfirmedLayoutIndex = row;
                save();
            } else if (result == KMessageBox::SecondaryAction) {
                switchtonewlayout = true;
                m_lastConfirmedLayoutIndex = row;
            } else if (result == KMessageBox::Cancel) {
                //do nothing
            }
        } else {
            switchtonewlayout = true;
            m_lastConfirmedLayoutIndex = row;
        }
    }

    if (switchtonewlayout) {
        QString layoutId = m_layoutsProxyModel->data(m_layoutsProxyModel->index(row, Model::Layouts::IDCOLUMN), Qt::UserRole).toString();
        m_dialog->layoutsController()->selectRow(layoutId);
        reload();
        Q_EMIT currentLayoutChanged();
    } else {
        m_ui->layoutsCmb->setCurrentText(c_data.name);
    }
}

void DetailsHandler::updateCustomSchemeCmb(const int &row)
{
    int scmind = row;
    m_ui->customSchemeCmb->setCurrentText(m_ui->customSchemeCmb->itemData(scmind, Qt::DisplayRole).toString());
    m_ui->customSchemeCmb->setTextColor(m_ui->customSchemeCmb->itemData(scmind, Model::Schemes::TEXTCOLORROLE).value<QColor>());
    m_ui->customSchemeCmb->setBackgroundColor(m_ui->customSchemeCmb->itemData(scmind, Model::Schemes::BACKGROUNDCOLORROLE).value<QColor>());
}

void DetailsHandler::onCurrentSchemeIndexChanged(int row)
{
    updateCustomSchemeCmb(row);
    QString selectedScheme = m_ui->customSchemeCmb->itemData(row, Model::Schemes::IDROLE).toString();
    setCustomSchemeFile(selectedScheme);
}

void DetailsHandler::setCustomSchemeFile(const QString &file)
{
    if (c_data.schemeFile == file) {
        return;
    }

    c_data.schemeFile = file;
    Q_EMIT dataChanged();
}

void DetailsHandler::setIcon(const QString &icon)
{
    if (c_data.icon == icon) {
        return;
    }

    c_data.icon = icon;
    Q_EMIT dataChanged();
}

void DetailsHandler::setIsShownInMenu(bool inMenu)
{
    if (c_data.isShownInMenu == inMenu) {
        return;
    }

    c_data.isShownInMenu = inMenu;
    Q_EMIT dataChanged();
}

void DetailsHandler::setPopUpMargin(const int &margin)
{
    if (c_data.popUpMargin == margin) {
        return;
    }

    c_data.popUpMargin = margin;
    Q_EMIT dataChanged();
}

void DetailsHandler::selectIcon()
{
    QString icon = KIconDialog::getIcon();

    if (!icon.isEmpty()) {
        setIcon(icon);
    }
}

void DetailsHandler::updateWindowTitle()
{
    m_dialog->setWindowTitle(i18nc("<layout name> Details","%1 Details", m_ui->layoutsCmb->currentText()));
}

KMessageBox::ButtonCode DetailsHandler::saveChangesConfirmation()
{
    if (hasChangedData()) {
        QString layoutName = c_data.name;
        QString saveChangesText = i18n("The settings of <b>%1</b> layout have changed.<br/>Do you want to apply the changes or discard them?", layoutName);

        return m_dialog->saveChangesConfirmation(saveChangesText);
    }

    return KMessageBox::Cancel;
}

}
}
}
