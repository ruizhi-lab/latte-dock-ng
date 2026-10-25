/*
    SPDX-FileCopyrightText: 2020 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef GENERICDATA_H
#define GENERICDATA_H

// local
#include "generictable.h"

// Qt
#include <QString>

namespace Latte {
namespace Data {

//! Data records are plain values: copy and move are compiler-generated (Rule
//! of Zero), so every member, including one added later, is copied and moved.
//! The former hand-written versions copied on move and had to list each field
//! again; a missed field was silently dropped. .clang-tidy rejects a partial
//! set of special members under app/data.
class Generic
{
public:
    Generic() = default;
    Generic(const QString &newid, const QString &newname);

    //! Layout data
    QString id;
    QString name;

    //! Operators
    bool operator==(const Generic &rhs) const;
    bool operator!=(const Generic &rhs) const;
};

}
}



#endif
