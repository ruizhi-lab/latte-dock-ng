/*
    SPDX-FileCopyrightText: 2021 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <latte_debug.h>
#include "actionsdialog.h"

// local
#include "ui_actionsdialog.h"
#include "actionshandler.h"
#include "../settingsdialog/tabpreferenceshandler.h"

// Qt
#include <QDebug>

namespace Latte {
namespace Settings {
namespace Dialog {

ActionsDialog::ActionsDialog(QDialog *parent, Handler::TabPreferences *handler)
    : GenericDialog(parent),
      m_ui(new Ui::ActionsDialog), /*this is necessary, in order to create the ui*/
      m_preferencesHandler(handler)
{
    setAttribute(Qt::WA_DeleteOnClose, true);
    init();
}

Ui::ActionsDialog *ActionsDialog::ui() const
{
    return m_ui;
}

Handler::TabPreferences *ActionsDialog::preferencesHandler() const
{
    return m_preferencesHandler;
}

void ActionsDialog::init()
{
    m_ui->setupUi(this);
    m_actionsHandler = new Handler::ActionsHandler(this);
}

void ActionsDialog::accept()
{
    qCDebug(latteSettings) << Q_FUNC_INFO;
}

void ActionsDialog::onCancel()
{
    qCDebug(latteSettings) << Q_FUNC_INFO;
    close();
}

void ActionsDialog::onReset()
{
    qCDebug(latteSettings) << Q_FUNC_INFO;
    close();
}

}
}
}
