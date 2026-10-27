/*
    SPDX-FileCopyrightText: 2020 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef LATTECORETOOLS_H
#define LATTECORETOOLS_H

// Qt
#include <QObject>
#include <QColor>
#include <QtQml/qqmlregistration.h>


namespace Latte {

class Tools final: public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Tools)
    QML_SINGLETON

public:
    explicit Tools(QObject *parent = nullptr);

public Q_SLOTS:
    Q_INVOKABLE float colorBrightness(QColor color);
    Q_INVOKABLE float colorLumina(QColor color);

private:
    float colorBrightness(QRgb rgb);
    float colorBrightness(float r, float g, float b);

    float colorLumina(QRgb rgb);
    float colorLumina(float r, float g, float b);
};

}

#endif
