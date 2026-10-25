/*
    SPDX-FileCopyrightText: 2021 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef GENERICERRORDATA_H
#define GENERICERRORDATA_H

//! local
#include "genericdata.h"
#include "appletdata.h"
#include "errorinformationdata.h"

//! Qt
#include <QList>
#include <QMetaType>
#include <QString>

namespace Latte {
namespace Data {

class Error : public Data::Generic
{
public:
    //!errors and warnings use a step of four between them
    static inline const QString APPLETSWITHSAMEID = QStringLiteral("E103");
    static inline const QString ORPHANEDPARENTAPPLETOFSUBCONTAINMENT = QStringLiteral("E107");
    static inline const QString ORPHANEDSUBCONTAINMENT = QStringLiteral("W201");
    static inline const QString APPLETANDCONTAINMENTWITHSAMEID = QStringLiteral("W205");

    Error();

    bool isValid() const;

    //! Operators
    bool operator==(const Error &rhs) const;
    bool operator!=(const Error &rhs) const;

    GenericTable<Data::ErrorInformation> information;

};

typedef Error Warning;
typedef QList<Error> ErrorsList;
typedef QList<Warning> WarningsList;

}
}

Q_DECLARE_METATYPE(Latte::Data::Error)
Q_DECLARE_METATYPE(Latte::Data::ErrorsList)

#endif
