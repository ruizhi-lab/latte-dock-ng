/*
    SPDX-FileCopyrightText: 2021 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "custommenuitemwidget.h"

// local
#include "../../generic/generictools.h"
#include "../../generic/genericviewtools.h"

// Qt
#include <QApplication>
#include <QDebug>
#include <QHBoxLayout>
#include <QPainter>
#include <QRadioButton>
#include <QStyleOptionMenuItem>
#include <QTextDocument>

namespace Latte {
namespace Settings {
namespace View {
namespace Widget {

constexpr int kRowHeightPadding = 9;

CustomMenuItemWidget::CustomMenuItemWidget(QAction* action, QWidget *parent)
    : QWidget(parent),
      m_action(action)
{
    QHBoxLayout *l = new QHBoxLayout;

    auto radiobtn = new QRadioButton(this);
    radiobtn->setCheckable(true);
    radiobtn->setChecked(action->isChecked());

    l->addWidget(radiobtn);
    setLayout(l);

    setMouseTracking(true);
}

void CustomMenuItemWidget::setScreen(const Latte::Data::Screen &screen)
{
    m_screen = screen;
}

void CustomMenuItemWidget::setView(const Latte::Data::View &view)
{
    m_view = view;
}

QSize CustomMenuItemWidget::sizeHint() const
{
    QStyleOptionMenuItem opt;
    opt.initFrom(this);
    opt.text = m_action->text();
    opt.menuItemType = QStyleOptionMenuItem::Normal;

    // The text is painted via QTextDocument (Latte::drawFormattedText), whose
    // HTML-rendered width is wider than fontMetrics().size(). Mirror the
    // paint code's measurement, including the <b>...</b> wrapping that
    // paintEvent applies for active screen entries, so the names aren't
    // clipped on the right.
    QString textForMeasure = opt.text;
    bool inScreensColumn = !m_view.isValid();

    if (m_screen.isActive && inScreensColumn) {
        textForMeasure = QStringLiteral("<b>%1</b>").arg(textForMeasure);
    }

    QTextDocument doc;
    doc.setHtml(QStringLiteral("<body>%1</body>").arg(textForMeasure));
    const int textWidth = static_cast<int>(doc.size().width()) + 2;

    const int rowHeight = fontMetrics().height() + kRowHeightPadding;
    const int radioWidth = rowHeight; // matches paintEvent's radiosize = opt.rect.height()

    int screenTotal = 0;

    if (!m_screen.id.isEmpty()) {
        const int scrMaxLength = Latte::screenMaxLength(opt, kMaxIconSize);
        screenTotal = scrMaxLength + kMargin * 2 + 1;
    }

    QSize contentSize(radioWidth + screenTotal + textWidth, rowHeight);
    return style()->sizeFromContents(QStyle::CT_MenuItem, &opt, contentSize, this);
}

QSize CustomMenuItemWidget::minimumSizeHint() const
{
    return sizeHint();
}

void CustomMenuItemWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.save();
    QStyleOptionMenuItem opt;
    opt.initFrom(this);
    opt.text = m_action->text();
    opt.menuItemType = QStyleOptionMenuItem::Normal;
    opt.menuHasCheckableItems = false;

    bool inScreensColumn = !m_view.isValid();

    if (rect().contains(mapFromGlobal(QCursor::pos()))) {
        opt.state |= QStyle::State_Selected;
    }

    Latte::drawBackground(&painter, style(), opt);

    //! radio button
    int radiosize = opt.rect.height();
    QRect remained;

    if (qApp->layoutDirection() == Qt::LeftToRight) {
        remained = QRect(opt.rect.x() + radiosize, opt.rect.y(), opt.rect.width() - radiosize, opt.rect.height());
    } else {
        remained = QRect(opt.rect.x(), opt.rect.y(), opt.rect.width() - radiosize, opt.rect.height());
    }

    opt.rect = remained;

    if (!m_screen.id.isEmpty()) {
        remained = Latte::remainedFromScreenDrawing(opt, m_screen.isScreensGroup(), kMaxIconSize);
        QRect availableScreenRect = Latte::drawScreen(&painter, opt, m_screen.isScreensGroup(), m_screen.geometry, kMaxIconSize);

        if (!m_view.id.isEmpty()) {
            Latte::drawView(&painter, opt, m_view, availableScreenRect);
        }
    }

    opt.rect = remained;

    //! text
    opt.text = opt.text.remove(QLatin1Char('&'));

    if (qApp->layoutDirection() == Qt::LeftToRight) {
        //! add spacing
        remained = QRect(opt.rect.x() + kMargin, opt.rect.y(), opt.rect.width() - kMargin, opt.rect.height());
    } else {
        //! add spacing
        remained = QRect(opt.rect.x(), opt.rect.y(), opt.rect.width() - kMargin, opt.rect.height());
    }

    opt.rect = remained;

    if (m_screen.isActive && inScreensColumn) {
        opt.text = QStringLiteral("<b>") + opt.text + QStringLiteral("</b>");
    }

    //style()->drawControl(QStyle::CE_MenuItem, &opt, &painter, this);
    Latte::drawFormattedText(&painter, opt);

    painter.restore();
}

}
}
}
}
