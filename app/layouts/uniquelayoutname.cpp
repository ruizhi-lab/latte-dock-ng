/*
    SPDX-FileCopyrightText: 2026 Latte Dock NG contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "uniquelayoutname.h"

#include "../tools/commontools.h"

#include <QRandomGenerator>

namespace Latte::Layouts::Private {

QString
uniqueLayoutName(QString name, const LayoutNameExists &layoutExists)
{
    if (layoutExists(name)) {
        name = Latte::stripUniqueNameSuffix(name);
    }

    int i = 2;

    QString namePart = name;

    while (layoutExists(name) && i < 10000) {
        name = namePart + QLatin1String(" - ") + QString::number(i);
        i++;
    }

    if (layoutExists(name)) {
        //! All numbered suffixes are taken — fall back to a random
        //! suffix instead of returning a name that still exists.
        name = namePart + QLatin1String(" - ") + QString::number(QRandomGenerator::global()->generate(), 16);
    }

    return name;
}

}
