/*
    SPDX-FileCopyrightText: 2020 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef LATTEENVIRONMENT_H
#define LATTEENVIRONMENT_H

// Qt
#include <QObject>
#include <QString>
#include <QTimer>
#include <QVariant>
#include <QQuickItem>
#include <QtQml>


namespace Latte {

class Environment final: public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Environment)
    QML_SINGLETON

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
    Q_INVOKABLE void refreshAppletIcons(QQuickItem *root) const;

Q_SIGNALS:
    void longDurationChanged();
    void shortDurationChanged();
    void iconThemeVersionChanged();

private Q_SLOTS:
    void emitIconThemeVersionChanged();

private:
    void markIconThemeChanged();
    QString currentIconTheme() const;

    QTimer m_iconThemeChangedTimer;
    uint m_iconThemeVersion{0};

};

}

#endif
