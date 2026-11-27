/*
    SPDX-FileCopyrightText: 2021 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <latte_debug.h>
#include "screensdialog.h"

// local
#include "ui_screensdialog.h"
#include "screenshandler.h"
#include "../settingsdialog/layoutscontroller.h"
#include "../../layouts/manager.h"
#include "../../layouts/synchronizer.h"

namespace Latte {
namespace Settings {
namespace Dialog {

ScreensDialog::ScreensDialog(SettingsDialog *parent, Controller::Layouts *controller)
    : GenericDialog(parent),
      m_ui(new Ui::ScreensDialog),
      m_layoutsController(controller)
{
    setAttribute(Qt::WA_DeleteOnClose, true);
    m_corona = parent->corona();

    init();
    initRemoveNowButton();
    //! we must create handlers after creating/adjusting the ui
    m_handler = new Handler::ScreensHandler(this);
}

Latte::Corona *ScreensDialog::corona() const
{
    return m_corona;
}

Ui::ScreensDialog *ScreensDialog::ui() const
{
    return m_ui;
}

Controller::Layouts *ScreensDialog::layoutsController() const
{
    return m_layoutsController;
}

void ScreensDialog::init()
{
    m_ui->setupUi(this);
    initButtons();
}

void ScreensDialog::initButtons()
{
    connect(m_ui->buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked,
            this, &ScreensDialog::onCancel);
}

void ScreensDialog::initRemoveNowButton()
{
    m_removeNowButton = new QPushButton(this);
    m_removeNowButton->setText(i18nc("remove selected screens", "Remove Now"));
    m_removeNowButton->setIcon(QIcon::fromTheme(QStringLiteral("edit-delete")));
    m_removeNowButton->setToolTip(i18n("Remove selected screen references"));

    m_ui->buttonBox->addButton(m_removeNowButton, QDialogButtonBox::AcceptRole);
}

QPushButton *ScreensDialog::removeNowButton() const
{
    return m_removeNowButton;
}

void ScreensDialog::accept()
{
    qCDebug(latteSettings) << Q_FUNC_INFO;
}

void ScreensDialog::onCancel()
{
    qCDebug(latteSettings) << Q_FUNC_INFO;
    close();
}

void ScreensDialog::onReset()
{
    qCDebug(latteSettings) << Q_FUNC_INFO;
}

}
}
}
