/*
    SPDX-FileCopyrightText: 2020 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef DETAILSDIALOGHANDLER_H
#define DETAILSDIALOGHANDLER_H

// local
#include "../generic/generichandler.h"
#include "../../data/layoutdata.h"

// Qt
#include <QSortFilterProxyModel>
#include <QButtonGroup>

// KDE
#include <KMessageBox>

namespace Ui {
class DetailsDialog;
}

namespace Latte{
namespace Settings{
namespace Dialog{
class DetailsDialog;
}
}
}

namespace Latte{
namespace Settings{
namespace Model {
class Schemes;
}
}
}


namespace Latte {
namespace Settings {
namespace Handler {

class DetailsHandler : public Generic
{
    Q_OBJECT
public:
    DetailsHandler(Dialog::DetailsDialog *dialog);
    ~DetailsHandler();

    bool hasChangedData() const override;
    bool inDefaultValues() const override;

    Latte::Data::Layout currentData() const;

public Q_SLOTS:
    void reset() override;
    void resetDefaults() override;
    void save() override;

Q_SIGNALS:
    void currentLayoutChanged();

private Q_SLOTS:
    void onCurrentLayoutIndexChanged(int row);
    void onCurrentSchemeIndexChanged(int row);

    void clearIcon();
    void selectIcon();

private:
    void init();
    void reload();

    void setIsShownInMenu(bool inMenu);

    void setCustomSchemeFile(const QString &file);
    void setIcon(const QString &icon);
    void setPopUpMargin(const int &margin);

    void loadLayout(const Latte::Data::Layout &data);

    KMessageBox::ButtonCode saveChangesConfirmation();

    void updateCustomSchemeCmb(const int &row);
    void updateWindowTitle();

private:
    Dialog::DetailsDialog *m_dialog{nullptr};
    Ui::DetailsDialog *m_ui{nullptr};

    QSortFilterProxyModel *m_layoutsProxyModel{nullptr};

    int m_lastConfirmedLayoutIndex{-1};

    Model::Schemes *m_schemesModel{nullptr};

    Latte::Data::Layout o_data;
    Latte::Data::Layout c_data;
};

}
}
}

#endif
