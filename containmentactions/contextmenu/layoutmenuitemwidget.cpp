/*
    SPDX-FileCopyrightText: 2021 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "layoutmenuitemwidget.h"

// local
#include "generictools.h"

// Qt
#include <QApplication>
#include <QDebug>
#include <QHBoxLayout>
#include <QPainter>
#include <QRadioButton>
#include <QStyleOptionMenuItem>
#include <QTextDocument>

namespace Latte {
namespace {
const int ICONMARGIN = 1;
const int MARGIN = 2;

// Current layouts (CURRENTLAYOUTSINDEX, the list the Layouts menu checks) are drawn in bold.
// Latte::drawFormattedText() renders the name through a QTextDocument that never receives the
// item font, so the weight has to be markup. sizeHint() measures the same markup; otherwise the
// wider bold name is clipped on the right.
QString displayedText(const QString &text, const bool &isCurrent)
{
    return isCurrent ? QStringLiteral("<b>%1</b>").arg(text) : text;
}
}

LayoutMenuItemWidget::LayoutMenuItemWidget(QAction* action, QWidget *parent)
    : QWidget(parent),
      m_action(action)
{
    QHBoxLayout *l = new QHBoxLayout;

    auto radiobtn = new QRadioButton(this);
    radiobtn->setCheckable(true);
    radiobtn->setChecked(action->isChecked());
    radiobtn->setVisible(action->isVisible() && action->isCheckable());

    l->addWidget(radiobtn);
    setLayout(l);

    setMouseTracking(true);
}

void LayoutMenuItemWidget::setIcon(const bool &isBackgroundFile, const QString &iconName)
{
    m_isBackgroundFile = isBackgroundFile;
    m_iconName = iconName;
}

void LayoutMenuItemWidget::setCurrent(const bool &current)
{
    if (m_isCurrent == current) {
        return;
    }

    m_isCurrent = current;
    updateGeometry();
    update();
}

bool LayoutMenuItemWidget::isCurrent() const
{
    return m_isCurrent;
}

QSize LayoutMenuItemWidget::sizeHint() const
{
    QStyleOptionMenuItem opt;
    opt.initFrom(this);
    opt.text = m_action->text();
    opt.menuItemType = QStyleOptionMenuItem::Normal;

    // The text is painted via QTextDocument (Latte::drawFormattedText), whose
    // HTML-rendered width is wider than fontMetrics().size(). Mirror the
    // paint code's measurement so the layout names aren't clipped on the
    // right.
    QTextDocument doc;
    doc.setHtml(QStringLiteral("<body>%1</body>").arg(displayedText(m_action->text(), m_isCurrent)));
    const int textWidth = static_cast<int>(doc.size().width()) + MARGIN;

    const int rowHeight = fontMetrics().height() + 9;
    const int radioSize = rowHeight - 2 * MARGIN;
    const int iconSize = qMax(16, opt.maxIconWidth);
    const int iconLenMargin = MARGIN + ICONMARGIN; // matches drawLayoutIcon defaults
    const int iconTotal = iconSize + 2 * iconLenMargin;

    QSize contentSize(radioSize + iconTotal + textWidth + 2 * MARGIN, rowHeight);
    return style()->sizeFromContents(QStyle::CT_MenuItem, &opt, contentSize, this);
}

QSize LayoutMenuItemWidget::minimumSizeHint() const
{
    return sizeHint();
}

void LayoutMenuItemWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.save();
    QStyleOptionMenuItem opt;
    opt.initFrom(this);
    opt.text = displayedText(m_action->text(), m_isCurrent);
    opt.menuItemType = QStyleOptionMenuItem::Normal;
    opt.menuHasCheckableItems = false;

    if (rect().contains(mapFromGlobal(QCursor::pos()))) {
        opt.state |= QStyle::State_Selected;
    }

    //! background
    Latte::drawBackground(&painter, style(), opt);

    //! radio button
    int radiosize = opt.rect.height() - 2 * MARGIN;
    QRect remained;

    if (qApp->layoutDirection() == Qt::LeftToRight) {
        remained = QRect(opt.rect.x() + radiosize, opt.rect.y(), opt.rect.width() - radiosize, opt.rect.height());
    } else {
        remained = QRect(opt.rect.x(), opt.rect.y(), opt.rect.width() - radiosize, opt.rect.height());
    }

    opt.rect  = remained;

    //! icon
    int thickpadding = (opt.rect.height() - qMax(16, opt.maxIconWidth)) / 2; //old value 4
    remained = Latte::remainedFromLayoutIcon(opt, Qt::AlignLeft, 1, thickpadding);
    Latte::drawLayoutIcon(&painter, opt, m_isBackgroundFile, m_iconName, Qt::AlignLeft, 1, thickpadding);
    opt.rect  = remained;

    //! text
    opt.text.remove(QStringLiteral("&"));
    //style()->drawControl(QStyle::CE_MenuItem, &opt, &painter, this);
    Latte::drawFormattedText(&painter, opt);

    painter.restore();
}

} // namespace Latte

