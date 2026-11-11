/*
    R7j scratch harness (not part of the repository).

    Shows the production Settings::Layout::Delegate::Activities editor on a QTableView. The
    model mirrors Model::Layouts for one layout with no assigned activities and two running
    activities, 默认 (current) and Work. Every signal and event that a click on the Work item
    produces is logged with a time stamp, so that the sequences can be compared.

    --qtest   the clicks are sent with QTest (any QPA platform, e.g. offscreen).
    default   the global coordinates of the editor button and of the Work item are printed as
              "R7J-BUTTON x y" and "R7J-WORK x y". Commands on stdin: "state" logs the state
              and saves PNG files, "quit" exits. A pointer driver (wl/run_nested.py) clicks on a
              nested KWin Wayland session.

    The window is shown full screen in the default mode, so that window coordinates are the
    global coordinates of a single 1920x1080 nested output. R7J_OUT names the directory for the
    PNG files (default: the temporary directory). R7J_ASSIGNED=all starts the layout with the
    "All activities" item assigned.

    The pointer driver is wl/run_nested.py: it starts a nested KWin session and sends the clicks
    through wl/fakeinput.
*/

#include <QAbstractItemDelegate>
#include <QAbstractItemView>
#include <QAbstractTableModel>
#include <QAction>
#include <QApplication>
#include <QDir>
#include <QDialogButtonBox>
#include <QElapsedTimer>
#include <QEvent>
#include <QFocusEvent>
#include <QHeaderView>
#include <QMenu>
#include <QPointer>
#include <QPushButton>
#include <QSocketNotifier>
#include <QTableView>
#include <QTest>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <QWidgetAction>

#include <cstdio>

#include "activitydata.h"
#include "delegates/activitiesdelegate.h"
#include "layoutdata.h"
#include "layoutsmodel.h"
#include "persistentmenu.h"

using LayoutsModel = Latte::Settings::Model::Layouts;

namespace {

QElapsedTimer g_clock;
QString g_outDir;
int g_shotCounter = 0;

QPointer<QWidget> g_window;
QPointer<QTableView> g_view;
QPointer<QPushButton> g_button;
QPointer<QMenu> g_menu;
QPointer<QAction> g_work;

const QString kDefaultActivityId = QStringLiteral("11111111-0000-0000-0000-000000000001");
const QString kWorkActivityId = QStringLiteral("22222222-0000-0000-0000-000000000002");

void logLine(const QString &text)
{
    const QByteArray line = QStringLiteral("R7J %1 %2\n").arg(g_clock.elapsed(), 6).arg(text).toUtf8();
    fwrite(line.constData(), 1, static_cast<size_t>(line.size()), stdout);
    fflush(stdout);
}

QString describe(const QObject *object)
{
    if (!object) {
        return QStringLiteral("(none)");
    }

    QString text = QString::fromLatin1(object->metaObject()->className());

    if (const auto *button = qobject_cast<const QPushButton *>(object)) {
        text += QStringLiteral(" text='%1'").arg(button->text());
    }

    if (const auto *action = qobject_cast<const QAction *>(object)) {
        text += QStringLiteral(" text='%1' checked=%2").arg(action->text()).arg(action->isChecked());
    }

    if (const auto *widget = qobject_cast<const QWidget *>(object)) {
        text += QStringLiteral(" visible=%1").arg(widget->isVisible());
    }

    return text;
}

QString eventName(QEvent::Type type)
{
    switch (type) {
    case QEvent::Show:
        return QStringLiteral("Show");
    case QEvent::Hide:
        return QStringLiteral("Hide");
    case QEvent::Close:
        return QStringLiteral("Close");
    case QEvent::WindowActivate:
        return QStringLiteral("WindowActivate");
    case QEvent::WindowDeactivate:
        return QStringLiteral("WindowDeactivate");
    case QEvent::FocusIn:
        return QStringLiteral("FocusIn");
    case QEvent::FocusOut:
        return QStringLiteral("FocusOut");
    case QEvent::MouseButtonPress:
        return QStringLiteral("MouseButtonPress");
    case QEvent::MouseButtonRelease:
        return QStringLiteral("MouseButtonRelease");
    default:
        return QString::number(static_cast<int>(type));
    }
}

//! Logs the window events of the object it is installed on.
class EventSpy : public QObject
{
public:
    EventSpy(const QString &label, QObject *parent)
        : QObject(parent),
          m_label(label)
    {
    }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        switch (event->type()) {
        case QEvent::Show:
        case QEvent::Hide:
        case QEvent::Close:
        case QEvent::WindowActivate:
        case QEvent::WindowDeactivate:
        case QEvent::FocusIn:
        case QEvent::FocusOut:
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
            logLine(QStringLiteral("EVENT %1 %2 on %3").arg(m_label, eventName(event->type()), describe(watched)));
            break;
        default:
            break;
        }

        return QObject::eventFilter(watched, event);
    }

private:
    QString m_label;
};

//! The layout model of one layout row. The roles are the ones Model::Layouts::data() returns
//! for the activity column; the assigned activities are kept in Qt::UserRole, as in the model.
class FixtureModel : public QAbstractTableModel
{
public:
    explicit FixtureModel(QObject *parent)
        : QAbstractTableModel(parent)
    {
        Latte::Data::Activity allActivities;
        allActivities.id = Latte::Data::Layout::ALLACTIVITIESID;
        allActivities.name = QStringLiteral("[ All Activities ]");
        allActivities.icon = QStringLiteral("activities");
        allActivities.state = Latte::Data::Activity::Stopped;
        m_table << allActivities;

        Latte::Data::Activity freeActivities;
        freeActivities.id = Latte::Data::Layout::FREEACTIVITIESID;
        freeActivities.name = QStringLiteral("[ Free Activities ]");
        freeActivities.icon = QStringLiteral("activities");
        freeActivities.state = Latte::Data::Activity::Stopped;
        m_table << freeActivities;

        Latte::Data::Activity currentActivity;
        currentActivity.id = Latte::Data::Layout::CURRENTACTIVITYID;
        currentActivity.name = QStringLiteral("[ Current Activity ]");
        currentActivity.icon = QStringLiteral("dialog-yes");
        currentActivity.state = Latte::Data::Activity::Stopped;
        m_table << currentActivity;

        Latte::Data::Activity defaultActivity;
        defaultActivity.id = kDefaultActivityId;
        defaultActivity.name = QString::fromUtf8("\xE9\xBB\x98\xE8\xAE\xA4"); // 默认
        defaultActivity.icon = QStringLiteral("activities");
        defaultActivity.state = Latte::Data::Activity::Running;
        defaultActivity.isCurrent = true;
        m_table << defaultActivity;

        Latte::Data::Activity workActivity;
        workActivity.id = kWorkActivityId;
        workActivity.name = QStringLiteral("Work");
        workActivity.icon = QStringLiteral("activities");
        workActivity.state = Latte::Data::Activity::Running;
        m_table << workActivity;

        // R7J_ASSIGNED=all starts the layout with the "All activities" item assigned (the master item).
        if (qEnvironmentVariable("R7J_ASSIGNED") == QLatin1String("all")) {
            m_assigned << Latte::Data::Layout::ALLACTIVITIESID;
        }

        m_sorted << Latte::Data::Layout::ALLACTIVITIESID << Latte::Data::Layout::FREEACTIVITIESID
                 << Latte::Data::Layout::CURRENTACTIVITYID << kDefaultActivityId << kWorkActivityId;
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : 1;
    }

    int columnCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : LayoutsModel::ACTIVITYCOLUMN + 1;
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        if (!index.isValid() || index.column() != LayoutsModel::ACTIVITYCOLUMN) {
            return {};
        }

        if (role == LayoutsModel::ISACTIVEROLE) {
            return false;
        } else if (role == LayoutsModel::ALLACTIVITIESSORTEDROLE) {
            return m_sorted;
        } else if (role == LayoutsModel::ALLACTIVITIESDATAROLE) {
            QVariant activitiesData;
            activitiesData.setValue(m_table);
            return activitiesData;
        } else if (role == LayoutsModel::ORIGINALASSIGNEDACTIVITIESROLE) {
            return QStringList();
        } else if (role == Qt::UserRole) {
            return m_assigned;
        }

        return {};
    }

    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override
    {
        if (!index.isValid() || index.column() != LayoutsModel::ACTIVITYCOLUMN || role != Qt::UserRole) {
            return false;
        }

        m_assigned = value.toStringList();
        logLine(QStringLiteral("MODEL setData(UserRole) = [%1]").arg(m_assigned.join(QLatin1String(", "))));
        Q_EMIT dataChanged(index, index);
        return true;
    }

    Qt::ItemFlags flags(const QModelIndex &index) const override
    {
        return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
    }

    QStringList assigned() const
    {
        return m_assigned;
    }

private:
    Latte::Data::ActivitiesTable m_table;
    QStringList m_sorted;
    QStringList m_assigned;
};

void hookAction(QAction *action)
{
    QObject::connect(action, &QAction::toggled, action, [action](bool checked) {
        logLine(QStringLiteral("SIGNAL action '%1' toggled(%2)").arg(action->text()).arg(checked));
    });
    QObject::connect(action, &QAction::triggered, action, [action](bool checked) {
        logLine(QStringLiteral("SIGNAL action '%1' triggered(%2)").arg(action->text()).arg(checked));
    });
}

QAction *findAction(QMenu *menu, const QString &text)
{
    for (QAction *action : menu->actions()) {
        if (action->text() == text) {
            return action;
        }
    }
    return nullptr;
}

//! Hooks the menu, the delegate and the editor of the first activity cell. Called once per editor.
void hookEditor(QTableView *view, QPushButton *button)
{
    QMenu *menu = button->menu();

    auto *menuSpy = new EventSpy(QStringLiteral("menu"), menu);
    menu->installEventFilter(menuSpy);
    auto *buttonSpy = new EventSpy(QStringLiteral("editor"), button);
    button->installEventFilter(buttonSpy);

    QObject::connect(menu, &QMenu::aboutToShow, menu, []() {
        logLine(QStringLiteral("SIGNAL menu aboutToShow"));
    });
    QObject::connect(menu, &QMenu::aboutToHide, menu, []() {
        logLine(QStringLiteral("SIGNAL menu aboutToHide"));
    });

    for (QAction *action : menu->actions()) {
        hookAction(action);
    }

    for (QPushButton *dialogButton : menu->findChildren<QPushButton *>()) {
        QObject::connect(dialogButton, &QPushButton::clicked, dialogButton, [dialogButton]() {
            logLine(QStringLiteral("SIGNAL dialog button '%1' clicked").arg(dialogButton->text()));
        });
    }

    QAbstractItemDelegate *delegate = view->itemDelegateForColumn(LayoutsModel::ACTIVITYCOLUMN);
    QObject::connect(delegate, &QAbstractItemDelegate::commitData, delegate, [](QWidget *editor) {
        const QVariant okPressed = editor->property("OKPRESSED");
        logLine(QStringLiteral("SIGNAL delegate commitData OKPRESSED=%1").arg(okPressed.isValid() ? QString::number(okPressed.toBool()) : QStringLiteral("unset")));
    });
    QObject::connect(delegate, &QAbstractItemDelegate::closeEditor, delegate, [](QWidget *, QAbstractItemDelegate::EndEditHint hint) {
        logLine(QStringLiteral("SIGNAL delegate closeEditor hint=%1").arg(static_cast<int>(hint)));
    });
    QObject::connect(button, &QObject::destroyed, button, []() {
        logLine(QStringLiteral("SIGNAL editor destroyed"));
    });

    g_menu = menu;
    g_work = findAction(menu, QStringLiteral("Work"));
}

void logState(const QString &label)
{
    const bool workChecked = g_work ? g_work->isChecked() : false;
    logLine(QStringLiteral("STATE %1: Work checked=%2 menuVisible=%3 editorAlive=%4 buttonText='%5' activePopup=%6 focus=%7")
                .arg(label)
                .arg(workChecked)
                .arg(g_menu ? g_menu->isVisible() : false)
                .arg(!g_button.isNull())
                .arg(g_button ? g_button->text() : QString())
                .arg(describe(QApplication::activePopupWidget()))
                .arg(describe(QApplication::focusWidget())));

    if (g_menu) {
        QString states;
        for (const QAction *action : g_menu->actions()) {
            if (action->isCheckable()) {
                states += QStringLiteral("[%1=%2]").arg(action->text(), QString::number(action->isChecked()));
            }
        }
        logLine(QStringLiteral("STATE %1: checkable items %2").arg(label, states));
    }

    if (g_window) {
        const QString shot = g_outDir + QStringLiteral("/window-%1-%2.png").arg(++g_shotCounter).arg(label);
        g_window->grab().save(shot);
        logLine(QStringLiteral("SHOT %1").arg(shot));
    }

    if (g_menu && g_menu->isVisible()) {
        const QString shot = g_outDir + QStringLiteral("/menu-%1-%2.png").arg(++g_shotCounter).arg(label);
        g_menu->grab().save(shot);
        logLine(QStringLiteral("SHOT %1").arg(shot));
    }
}

void printCoordinates()
{
    if (g_button) {
        const QPoint centre = g_button->mapToGlobal(g_button->rect().center());
        logLine(QStringLiteral("R7J-BUTTON %1 %2").arg(centre.x()).arg(centre.y()));
    }
}

void printWorkCoordinates()
{
    if (g_menu && g_work) {
        const QPoint centre = g_menu->mapToGlobal(g_menu->actionGeometry(g_work).center());
        logLine(QStringLiteral("R7J-WORK %1 %2").arg(centre.x()).arg(centre.y()));
    }

    // The dialog buttons of the menu (确定 and 取消) are widgets of a QWidgetAction.
    if (g_menu) {
        if (auto *box = g_menu->findChild<QDialogButtonBox *>()) {
            const struct { QDialogButtonBox::StandardButton which; const char *tag; } buttons[] = {
                {QDialogButtonBox::Ok, "OK"}, {QDialogButtonBox::Cancel, "CANCEL"}};
            for (const auto &entry : buttons) {
                if (QPushButton *target = box->button(entry.which)) {
                    const QPoint centre = target->mapToGlobal(target->rect().center());
                    logLine(QStringLiteral("R7J-%1 %2 %3").arg(QLatin1String(entry.tag)).arg(centre.x()).arg(centre.y()));
                }
            }
        }
    }
}

//! Creates the editor of the activity cell, as a click on the cell does.
QPushButton *openEditor()
{
    const QModelIndex cell = g_view->model()->index(0, LayoutsModel::ACTIVITYCOLUMN);
    g_view->edit(cell);
    QPushButton *button = g_view->viewport()->findChild<QPushButton *>();
    if (button) {
        g_button = button;
        hookEditor(g_view, button);
    }
    return button;
}

}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    g_clock.start();

    const bool qtestMode = argc > 1 && QString::fromLocal8Bit(argv[1]) == QLatin1String("--qtest");
    g_outDir = qEnvironmentVariable("R7J_OUT", QDir::tempPath());

    logLine(QStringLiteral("START mode=%1 platform=%2").arg(qtestMode ? QStringLiteral("qtest") : QStringLiteral("external"), QGuiApplication::platformName()));

    QObject::connect(qApp, &QApplication::focusChanged, qApp, [](QWidget *oldWidget, QWidget *newWidget) {
        logLine(QStringLiteral("FOCUS %1 -> %2").arg(describe(oldWidget), describe(newWidget)));
    });

    QWidget window;
    g_window = &window;
    window.setWindowTitle(QStringLiteral("R7j activity menu harness"));

    auto *layout = new QVBoxLayout(&window);
    auto *view = new QTableView(&window);
    g_view = view;
    auto *model = new FixtureModel(view);
    view->setModel(model);
    view->verticalHeader()->setVisible(false);
    view->setColumnHidden(LayoutsModel::ACTIVITYCOLUMN, false);
    view->setColumnWidth(LayoutsModel::ACTIVITYCOLUMN, 300);
    view->setItemDelegateForColumn(LayoutsModel::ACTIVITYCOLUMN, new Latte::Settings::Layout::Delegate::Activities(view));
    layout->addWidget(view);

    if (qtestMode) {
        window.resize(1000, 400);
        window.show();
        QTest::qWaitForWindowExposed(&window);
    } else {
        window.showFullScreen();
    }

    QTest::qWait(400);
    QPushButton *button = openEditor();
    logLine(QStringLiteral("EDITOR created=%1").arg(button != nullptr));

    if (qtestMode) {
        const QPoint buttonCentre = button->rect().center();
        QTest::mousePress(button, Qt::LeftButton, Qt::NoModifier, buttonCentre);
        QTest::qWait(250);
        logLine(QStringLiteral("QTEST after press on the editor: menuVisible=%1").arg(g_menu ? g_menu->isVisible() : false));
        QTest::mouseRelease(button, Qt::LeftButton, Qt::NoModifier, buttonCentre);
        QTest::qWait(250);
        logLine(QStringLiteral("QTEST after release on the editor: menuVisible=%1").arg(g_menu ? g_menu->isVisible() : false));
        logState(QStringLiteral("before-work"));

        if (g_menu && g_menu->isVisible() && g_work) {
            const QPoint workCentre = g_menu->actionGeometry(g_work).center();
            logLine(QStringLiteral("QTEST click Work at menu-local %1,%2").arg(workCentre.x()).arg(workCentre.y()));
            QTest::mouseMove(g_menu, workCentre);
            QTest::qWait(50);
            QTest::mousePress(g_menu, Qt::LeftButton, Qt::NoModifier, workCentre);
            QTest::qWait(50);
            QTest::mouseRelease(g_menu, Qt::LeftButton, Qt::NoModifier, workCentre);
        }

        QTest::qWait(600);
        logState(QStringLiteral("after-work"));

        // Optional second step, chosen by the command line: "ok", "cancel", "outside" or
        // "work-again" (a second click on Work, after the first one).
        const QString step = argc > 2 ? QString::fromLocal8Bit(argv[2]) : QString();
        if (step == QLatin1String("ok") || step == QLatin1String("cancel")) {
            const QDialogButtonBox::StandardButton which = step == QLatin1String("ok") ? QDialogButtonBox::Ok : QDialogButtonBox::Cancel;
            auto *box = g_menu ? g_menu->findChild<QDialogButtonBox *>() : nullptr;
            QPushButton *target = box ? box->button(which) : nullptr;
            if (target && g_menu->isVisible()) {
                logLine(QStringLiteral("QTEST click %1 button").arg(step));
                QTest::mouseClick(target, Qt::LeftButton, Qt::NoModifier, target->rect().center());
            }
        } else if (step == QLatin1String("outside")) {
            logLine(QStringLiteral("QTEST click outside the menu"));
            QTest::mouseClick(g_view->viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(5, 5));
        } else if (step == QLatin1String("work-again") && g_menu && g_menu->isVisible() && g_work) {
            const QPoint workCentre = g_menu->actionGeometry(g_work).center();
            logLine(QStringLiteral("QTEST click Work again"));
            QTest::mousePress(g_menu, Qt::LeftButton, Qt::NoModifier, workCentre);
            QTest::qWait(50);
            QTest::mouseRelease(g_menu, Qt::LeftButton, Qt::NoModifier, workCentre);
        }

        QTest::qWait(600);
        if (!step.isEmpty()) {
            logState(QStringLiteral("after-%1").arg(step));
        }
        app.quit();
        return 0;
    }

    // External mode: the pointer driver reads the coordinates and clicks.
    auto *commands = new QSocketNotifier(0, QSocketNotifier::Read, &app);
    QObject::connect(commands, &QSocketNotifier::activated, &app, [&app](QSocketDescriptor, QSocketNotifier::Type) {
        char buffer[256] = {0};
        if (!fgets(buffer, static_cast<int>(sizeof(buffer)), stdin)) {
            logLine(QStringLiteral("STDIN closed"));
            app.quit();
            return;
        }
        const QString command = QString::fromLatin1(buffer).trimmed();
        if (command == QLatin1String("state")) {
            logState(QStringLiteral("requested"));
        } else if (command == QLatin1String("quit")) {
            logState(QStringLiteral("quit"));
            app.quit();
        } else {
            logLine(QStringLiteral("STDIN unknown command '%1'").arg(command));
        }
    });

    QTimer::singleShot(500, &app, []() {
        printCoordinates();
        logLine(QStringLiteral("R7J-READY"));
    });

    // The menu is shown with the window's position applied: wait a moment before reading the
    // Work item's global position.
    QTimer *menuWatcher = new QTimer(&app);
    menuWatcher->setInterval(100);
    QObject::connect(menuWatcher, &QTimer::timeout, menuWatcher, []() {
        static bool announced = false;
        if (!announced && g_menu && g_menu->isVisible()) {
            announced = true;
            QTimer::singleShot(150, qApp, []() {
                printWorkCoordinates();
            });
        }
        if (announced && (!g_menu || !g_menu->isVisible())) {
            announced = false;
        }
    });
    menuWatcher->start();

    return app.exec();
}
