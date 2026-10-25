/*
    SPDX-FileCopyrightText: 2020 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef LAYOUTICONDATA_H
#define LAYOUTICONDATA_H

//! local
#include "genericdata.h"

//! Qt
#include <QMetaType>
#include <QString>

namespace Latte {
namespace Data {

class LayoutIcon : public Generic
{
public:
    LayoutIcon();

    //! Layout data
    bool isBackgroundFile{true};

    bool isEmpty() const;

    //! Operators
    bool operator==(const LayoutIcon &rhs) const;
    bool operator!=(const LayoutIcon &rhs) const;
};

}
}

Q_DECLARE_METATYPE(Latte::Data::LayoutIcon)

#endif
