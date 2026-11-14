/*
    SPDX-FileCopyrightText: 2026 Latte Dock NG contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef LATTE_UNIQUELAYOUTNAME_H
#define LATTE_UNIQUELAYOUTNAME_H

#include <QString>

#include <functional>

namespace Latte::Layouts::Private {

using LayoutNameExists = std::function<bool(const QString &)>;

QString
uniqueLayoutName(QString name, const LayoutNameExists &layoutExists);

}

#endif
