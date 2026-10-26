/*
    SPDX-FileCopyrightText: 2020 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef INDICATORUIMANAGER_H
#define INDICATORUIMANAGER_H

//Qt
#include <QList>
#include <QObject>
#include <QQuickItem>
#include <QPointer>

namespace KDeclarative {
class QmlObjectSharedEngine;
}

namespace Latte {
class View;
}

namespace Latte {
namespace ViewPart {
class PrimaryConfigView;
}
}

namespace Latte {
namespace ViewPart {
namespace Config {

struct IndicatorUiData
{
    QString type;
    QString pluginPath;
    QString name;
    QPointer<Latte::View> view;
    QPointer<KDeclarative::QmlObjectSharedEngine> ui;
};

class IndicatorUiManager : public QObject
{
    Q_OBJECT

public:
    IndicatorUiManager(ViewPart::PrimaryConfigView *parent);
    ~IndicatorUiManager() override;

public Q_SLOTS:
    Q_INVOKABLE void addIndicator();
    Q_INVOKABLE void downloadIndicator();
    Q_INVOKABLE void removeIndicator(QString pluginId);
    Q_INVOKABLE void setParentItem(QQuickItem *parentItem);
    //! Creates the config UI item of \a type on first use and re-parents it
    //! into the parent item on every call, even when it is already shown;
    //! indicators without X-Latte-ConfigUi get no item. Showing the item, or
    //! an empty page, in the options StackView belongs to the QML caller
    //! (IndicatorConfigUiManager.qml showCurrentIndicator()), which must
    //! update the shown page after every call.
    Q_INVOKABLE void ui(const QString &type, Latte::View *view);

    Q_INVOKABLE int index(const QString &type);
private:
    bool contains(const QString &type);

private:
    //! Config dialog item assigned from QML; it can be destroyed with the
    //! dialog while this manager lives on, so guard it.
    QPointer<QQuickItem> m_parentItem;
    PrimaryConfigView *m_primary{nullptr};

    QList<IndicatorUiData> m_uidata;

};

}
}
}

#endif

