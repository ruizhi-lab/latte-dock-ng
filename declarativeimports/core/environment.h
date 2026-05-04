/*
    SPDX-FileCopyrightText: 2020 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef LATTEENVIRONMENT_H
#define LATTEENVIRONMENT_H

// Qt
#include <QObject>
#include <QQmlEngine>
#include <QJSEngine>
#include <QString>
#include <QVariant>


namespace Latte{

class Environment final: public QObject
{
    Q_OBJECT

    Q_PROPERTY(int separatorLength READ separatorLength CONSTANT)

    Q_PROPERTY(uint shortDuration READ shortDuration NOTIFY shortDurationChanged)
    Q_PROPERTY(uint longDuration READ longDuration NOTIFY longDurationChanged)
    Q_PROPERTY(uint iconThemeVersion READ iconThemeVersion NOTIFY iconThemeVersionChanged)

public:
    static const int SeparatorLength = 5;

    explicit Environment(QObject *parent = nullptr);

    int separatorLength() const;

    uint shortDuration() const;
    uint longDuration() const;
    uint iconThemeVersion() const;

public Q_SLOTS:
    Q_INVOKABLE uint makeVersion(uint major, uint minor, uint release) const;
    Q_INVOKABLE QVariant iconSourceForTheme(const QVariant &source) const;
    Q_INVOKABLE QString iconDescriptor(const QVariant &source) const;

Q_SIGNALS:
    void longDurationChanged();
    void shortDurationChanged();
    void iconThemeVersionChanged();

private:
    void markIconThemeChanged();
    QString currentIconTheme() const;
    uint m_iconThemeVersion{0};

};

static QObject *environment_qobject_singletontype_provider(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)

// NOTE: QML engine is the owner of this resource
    return new Environment;
}

}

#endif
