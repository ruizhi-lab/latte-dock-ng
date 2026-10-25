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
//! again, so a field missing from one of them was dropped without a compile
//! error. SourceContractTest::dataRecordsKeepRuleOfZero() rejects a
//! hand-written copy or move member in any app/data header; a record that
//! needs one must explain why and be exempted there by name.
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
