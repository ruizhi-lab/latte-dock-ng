/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "actionlistwidgetitem.h"
#include "generictools.h"
#include "genericviewtools.h"
#include "layoutscombobox.h"
#include "../app/settings/settingsdialog/delegates/layoutnamedelegate.h"
#include "../app/settings/settingsdialog/layoutsmodel.h"
#include "normalcheckboxdelegate.h"
#include "screendata.h"
#include "screensmodel.h"
#include "../app/settings/settingsdialog/delegates/checkboxdelegate.h"
#include "../app/settings/screensdialog/delegates/checkboxdelegate.h"
#include "schemescombobox.h"

#include <QAbstractTableModel>
#include <QImage>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMouseEvent>
#include <QStandardItemModel>
#include <QStyleOptionViewItem>
#include <QTest>

class ToolsUnitTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void actionListWidgetItemStoresIdAndSortOrder();
    void customComboBoxesStoreDecorationState();
    void layoutCheckBoxDelegateTogglesUserRole();
    void layoutNameDelegateEditsUserRole();
    void normalCheckBoxDelegateTogglesCheckState();
    void screensCheckBoxDelegateOnlyTogglesRemovableScreens();
    void styleStatePredicatesReflectOptionState();
    void colorGroupFollowsEnabledActiveAndSelectedState();
    void horizontalAlignmentPrefersCenterThenRightThenLeft();
    void remainedRectHelpersReserveExpectedSpace();
    void subtractedKeepsOriginalOrder();
    void screenMaxLengthUsesOddAspectLength();
    void screenDrawingReturnsAvailableInnerRect();
    void viewDrawingPaintsHorizontalAndVerticalEdges();
};

namespace {

bool hasPaintedPixel(const QImage &image)
{
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (qAlpha(image.pixel(x, y)) > 0) {
                return true;
            }
        }
    }

    return false;
}

class ScreenDelegateModel : public QAbstractTableModel
{
public:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : 1;
    }

    int columnCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : 1;
    }

    QVariant data(const QModelIndex &index, int role) const override
    {
        if (!index.isValid()) {
            return {};
        }

        if (role == Qt::CheckStateRole) {
            return m_checked ? Qt::Checked : Qt::Unchecked;
        } else if (role == Latte::Settings::Model::Screens::SCREENDATAROLE) {
            return QVariant::fromValue(m_screen);
        }

        return {};
    }

    bool setData(const QModelIndex &index, const QVariant &value, int role) override
    {
        if (!index.isValid() || role != Qt::CheckStateRole) {
            return false;
        }

        m_checked = value.toBool();
        return true;
    }

    Latte::Data::Screen m_screen;
    bool m_checked{false};
};

}

void ToolsUnitTest::actionListWidgetItemStoresIdAndSortOrder()
{
    using Latte::Settings::ActionsDialog::ActionListWidgetItem;

    ActionListWidgetItem first(QIcon(), QStringLiteral("First"), 20, QStringLiteral("first"));
    ActionListWidgetItem second(QIcon(), QStringLiteral("Second"), 10, QStringLiteral("second"));

    QCOMPARE(first.data(ActionListWidgetItem::IDROLE).toString(), QStringLiteral("first"));
    QCOMPARE(first.data(ActionListWidgetItem::ORDERROLE).toInt(), 20);
    QVERIFY(second < first);
    QVERIFY(!(first < second));
}

void ToolsUnitTest::customComboBoxesStoreDecorationState()
{
    Latte::Settings::LayoutsComboBox layoutsCombo;
    Latte::Data::LayoutIcon icon;
    icon.name = QStringLiteral("layout-icon");
    icon.isBackgroundFile = true;

    layoutsCombo.setLayoutIcon(icon);
    QCOMPARE(layoutsCombo.layoutIcon().name, QStringLiteral("layout-icon"));
    QCOMPARE(layoutsCombo.layoutIcon().isBackgroundFile, true);

    Latte::Settings::SchemesComboBox schemesCombo;
    schemesCombo.setBackgroundColor(Qt::black);
    schemesCombo.setTextColor(Qt::white);
    QCOMPARE(schemesCombo.backgroundColor(), QColor(Qt::black));
    QCOMPARE(schemesCombo.textColor(), QColor(Qt::white));
}

void ToolsUnitTest::layoutCheckBoxDelegateTogglesUserRole()
{
    Latte::Settings::Layout::Delegate::CheckBox delegate;
    QStandardItemModel model(1, Latte::Settings::Model::Layouts::ACTIVITYCOLUMN + 1);
    const QModelIndex index = model.index(0, Latte::Settings::Model::Layouts::MENUCOLUMN);
    model.setData(index, false, Qt::UserRole);
    model.setData(index, false, Latte::Settings::Model::Layouts::ORIGINALISSHOWNINMENUROLE);

    QStyleOptionViewItem option;
    option.rect = QRect(0, 0, 20, 20);

    QMouseEvent release(QEvent::MouseButtonRelease,
                        QPointF(10, 10),
                        QPointF(10, 10),
                        QPointF(10, 10),
                        Qt::LeftButton,
                        Qt::LeftButton,
                        Qt::NoModifier);
    QVERIFY(!delegate.editorEvent(&release, &model, option, index));
    QCOMPARE(model.data(index, Qt::UserRole).toBool(), false);

    QMouseEvent doubleClick(QEvent::MouseButtonDblClick,
                            QPointF(10, 10),
                            QPointF(10, 10),
                            QPointF(10, 10),
                            Qt::LeftButton,
                            Qt::LeftButton,
                            Qt::NoModifier);
    QVERIFY(delegate.editorEvent(&doubleClick, &model, option, index));
    QCOMPARE(model.data(index, Qt::UserRole).toBool(), true);

    QKeyEvent selectKey(QEvent::KeyPress, Qt::Key_Select, Qt::NoModifier);
    QVERIFY(delegate.editorEvent(&selectKey, &model, option, index));
    QCOMPARE(model.data(index, Qt::UserRole).toBool(), false);
}

void ToolsUnitTest::layoutNameDelegateEditsUserRole()
{
    Latte::Settings::Layout::Delegate::LayoutName delegate;
    QStandardItemModel model(1, 1);
    const QModelIndex index = model.index(0, 0);
    model.setData(index, QStringLiteral("Original"), Qt::UserRole);

    QStyleOptionViewItem option;
    QWidget *editor = delegate.createEditor(nullptr, option, index);
    QLineEdit *lineEdit = qobject_cast<QLineEdit *>(editor);
    QVERIFY(lineEdit);

    delegate.setEditorData(editor, index);
    QCOMPARE(lineEdit->text(), QStringLiteral("Original"));

    lineEdit->setText(QStringLiteral("Updated"));
    delegate.setModelData(editor, &model, index);
    QCOMPARE(model.data(index, Qt::UserRole).toString(), QStringLiteral("Updated"));

    delete editor;
}

void ToolsUnitTest::normalCheckBoxDelegateTogglesCheckState()
{
    Latte::Settings::Applets::Delegate::NormalCheckBox delegate;
    QStandardItemModel model(1, 1);
    const QModelIndex index = model.index(0, 0);
    model.setData(index, Qt::Unchecked, Qt::CheckStateRole);

    QStyleOptionViewItem option;
    option.rect = QRect(0, 0, 20, 20);

    QKeyEvent ignoredKey(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
    QVERIFY(!delegate.editorEvent(&ignoredKey, &model, option, index));
    QCOMPARE(model.data(index, Qt::CheckStateRole).value<Qt::CheckState>(), Qt::Unchecked);

    QKeyEvent spaceKey(QEvent::KeyPress, Qt::Key_Space, Qt::NoModifier);
    QVERIFY(delegate.editorEvent(&spaceKey, &model, option, index));
    QCOMPARE(model.data(index, Qt::CheckStateRole).value<Qt::CheckState>(), Qt::Checked);

    QMouseEvent outsideRelease(QEvent::MouseButtonRelease,
                               QPointF(40, 40),
                               QPointF(40, 40),
                               QPointF(40, 40),
                               Qt::LeftButton,
                               Qt::LeftButton,
                               Qt::NoModifier);
    QVERIFY(!delegate.editorEvent(&outsideRelease, &model, option, index));
    QCOMPARE(model.data(index, Qt::CheckStateRole).value<Qt::CheckState>(), Qt::Checked);

    QMouseEvent insideRelease(QEvent::MouseButtonRelease,
                              QPointF(10, 10),
                              QPointF(10, 10),
                              QPointF(10, 10),
                              Qt::LeftButton,
                              Qt::LeftButton,
                              Qt::NoModifier);
    QVERIFY(delegate.editorEvent(&insideRelease, &model, option, index));
    QCOMPARE(model.data(index, Qt::CheckStateRole).value<Qt::CheckState>(), Qt::Unchecked);
}

void ToolsUnitTest::screensCheckBoxDelegateOnlyTogglesRemovableScreens()
{
    Latte::Settings::Screens::Delegate::CheckBox delegate;
    ScreenDelegateModel model;
    const QModelIndex index = model.index(0, 0);

    model.m_screen.id = QStringLiteral("10");
    model.m_screen.name = QStringLiteral("DP-1");
    model.m_screen.isRemovable = false;

    QStyleOptionViewItem option;
    option.rect = QRect(0, 0, 40, 20);

    QKeyEvent spaceKey(QEvent::KeyPress, Qt::Key_Space, Qt::NoModifier);
    QVERIFY(!delegate.editorEvent(&spaceKey, &model, option, index));
    QCOMPARE(model.data(index, Qt::CheckStateRole).value<Qt::CheckState>(), Qt::Unchecked);

    model.m_screen.isRemovable = true;
    QVERIFY(delegate.editorEvent(&spaceKey, &model, option, index));
    QCOMPARE(model.data(index, Qt::CheckStateRole).value<Qt::CheckState>(), Qt::Checked);

    QMouseEvent outsideRelease(QEvent::MouseButtonRelease,
                               QPointF(80, 80),
                               QPointF(80, 80),
                               QPointF(80, 80),
                               Qt::LeftButton,
                               Qt::LeftButton,
                               Qt::NoModifier);
    QVERIFY(!delegate.editorEvent(&outsideRelease, &model, option, index));
    QCOMPARE(model.data(index, Qt::CheckStateRole).value<Qt::CheckState>(), Qt::Checked);
}

void ToolsUnitTest::styleStatePredicatesReflectOptionState()
{
    QStyleOption option;
    option.state = QStyle::State_Enabled | QStyle::State_Active | QStyle::State_Selected
            | QStyle::State_HasFocus | QStyle::State_MouseOver;

    QVERIFY(Latte::isEnabled(option));
    QVERIFY(Latte::isActive(option));
    QVERIFY(Latte::isSelected(option));
    QVERIFY(Latte::isFocused(option));
    QVERIFY(Latte::isHovered(option));

    option.state = QStyle::State_None;
    QVERIFY(!Latte::isEnabled(option));
    QVERIFY(!Latte::isActive(option));
    QVERIFY(!Latte::isSelected(option));
    QVERIFY(!Latte::isFocused(option));
    QVERIFY(!Latte::isHovered(option));

    QStyleOptionViewItem viewOption;
    viewOption.displayAlignment = Qt::AlignHCenter | Qt::AlignVCenter;
    QVERIFY(Latte::isTextCentered(viewOption));
    viewOption.displayAlignment = Qt::AlignLeft | Qt::AlignVCenter;
    QVERIFY(!Latte::isTextCentered(viewOption));
}

void ToolsUnitTest::colorGroupFollowsEnabledActiveAndSelectedState()
{
    QStyleOption option;

    option.state = QStyle::State_None;
    QCOMPARE(Latte::colorGroup(option), QPalette::Disabled);

    option.state = QStyle::State_Enabled | QStyle::State_Active;
    QCOMPARE(Latte::colorGroup(option), QPalette::Active);

    option.state = QStyle::State_Enabled | QStyle::State_HasFocus;
    QCOMPARE(Latte::colorGroup(option), QPalette::Active);

    option.state = QStyle::State_Enabled | QStyle::State_Selected;
    QCOMPARE(Latte::colorGroup(option), QPalette::Inactive);

    option.state = QStyle::State_Enabled;
    QCOMPARE(Latte::colorGroup(option), QPalette::Normal);
}

void ToolsUnitTest::horizontalAlignmentPrefersCenterThenRightThenLeft()
{
    QCOMPARE(Latte::horizontalAlignment(Qt::AlignHCenter | Qt::AlignRight), Qt::AlignHCenter);
    QCOMPARE(Latte::horizontalAlignment(Qt::AlignRight | Qt::AlignVCenter), Qt::AlignRight);
    QCOMPARE(Latte::horizontalAlignment(Qt::AlignLeft | Qt::AlignVCenter), Qt::AlignLeft);
}

void ToolsUnitTest::remainedRectHelpersReserveExpectedSpace()
{
    QStyleOption option;
    option.rect = QRect(0, 0, 100, 20);

    QCOMPARE(Latte::remainedFromIcon(option, Qt::AlignLeft, 2, 1), QRect(22, 0, 78, 20));
    QCOMPARE(Latte::remainedFromIcon(option, Qt::AlignRight, 2, 1), QRect(0, 0, 78, 20));
    QCOMPARE(Latte::remainedFromLayoutIcon(option, Qt::AlignHCenter, 2, 1), option.rect);
    QCOMPARE(Latte::remainedFromColorSchemeIcon(option, Qt::AlignLeft, 2, 1), QRect(22, 0, 78, 20));

    QStyleOptionButton buttonOption;
    buttonOption.rect = option.rect;
    const QRect checkBoxRemained = Latte::remainedFromCheckBox(buttonOption, Qt::AlignLeft);
    QVERIFY(checkBoxRemained.x() > option.rect.x());
    QVERIFY(checkBoxRemained.width() < option.rect.width());

    QCOMPARE(Latte::remainedFromFormattedText(option, QStringLiteral("Latte"), Qt::AlignHCenter), option.rect);
    const QRect textRemained = Latte::remainedFromFormattedText(option, QStringLiteral("Latte"), Qt::AlignLeft);
    QVERIFY(textRemained.x() > option.rect.x());
    QVERIFY(textRemained.width() < option.rect.width());
}

void ToolsUnitTest::subtractedKeepsOriginalOrder()
{
    const QStringList original{QStringLiteral("one"), QStringLiteral("two"), QStringLiteral("three"), QStringLiteral("two")};
    const QStringList current{QStringLiteral("two")};

    QCOMPARE(Latte::subtracted(original, current), QStringList({QStringLiteral("one"), QStringLiteral("three")}));
}

void ToolsUnitTest::screenMaxLengthUsesOddAspectLength()
{
    QStyleOption option;
    option.rect = QRect(0, 0, 200, 20);

    QCOMPARE(Latte::screenMaxLength(option), 33);
    QCOMPARE(Latte::screenMaxLength(option, 10), 17);
}

void ToolsUnitTest::screenDrawingReturnsAvailableInnerRect()
{
    QImage image(QSize(160, 80), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);

    QStyleOption option;
    option.rect = QRect(0, 0, 160, 40);
    option.state = QStyle::State_Enabled;

    const QRect available = Latte::drawScreen(&painter, option, false, QRect(0, 0, 1920, 1080), 32);

    QVERIFY(available.isValid());
    QVERIFY(option.rect.contains(available.topLeft()));
    QVERIFY(option.rect.contains(available.bottomRight()));
    QVERIFY(available.width() > 0);
    QVERIFY(available.height() > 0);
}

void ToolsUnitTest::viewDrawingPaintsHorizontalAndVerticalEdges()
{
    QStyleOption option;
    option.state = QStyle::State_Enabled;
    option.palette = QPalette();

    Latte::Data::View horizontalView;
    horizontalView.edge = Plasma::Types::BottomEdge;
    horizontalView.alignment = Latte::Types::Center;

    QImage horizontalImage(QSize(120, 80), QImage::Format_ARGB32_Premultiplied);
    horizontalImage.fill(Qt::transparent);
    QPainter horizontalPainter(&horizontalImage);
    Latte::drawView(&horizontalPainter, option, horizontalView, QRect(10, 10, 100, 50));
    horizontalPainter.end();
    QVERIFY(hasPaintedPixel(horizontalImage));

    Latte::Data::View verticalView;
    verticalView.edge = Plasma::Types::LeftEdge;
    verticalView.alignment = Latte::Types::Center;

    QImage verticalImage(QSize(120, 80), QImage::Format_ARGB32_Premultiplied);
    verticalImage.fill(Qt::transparent);
    QPainter verticalPainter(&verticalImage);
    Latte::drawView(&verticalPainter, option, verticalView, QRect(10, 10, 100, 50));
    verticalPainter.end();
    QVERIFY(hasPaintedPixel(verticalImage));
}

QTEST_MAIN(ToolsUnitTest)

#include "toolsunittest.moc"
