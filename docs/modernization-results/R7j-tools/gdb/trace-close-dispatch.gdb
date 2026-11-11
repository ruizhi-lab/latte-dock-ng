set pagination off
set print frame-arguments none
set breakpoint pending on
break QApplicationPrivate::notify_helper(QObject*, QEvent*) if *(int*)($rsi+8) == 19
commands
silent
printf "=== notify_helper Close receiver=%p\n", $rdi
bt 6
continue
end
break QWidget::event(QEvent*) if *(int*)($rsi+8) == 19
commands
silent
printf "=== QWidget::event Close this=%p\n", $rdi
bt 4
continue
end
break Latte::Settings::Layout::Delegate::PersistentMenu::closeEvent
commands
silent
printf "=== PersistentMenu::closeEvent\n"
bt 4
continue
end
run
quit
