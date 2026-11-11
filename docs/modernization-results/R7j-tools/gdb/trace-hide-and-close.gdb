set pagination off
set print frame-arguments none
set breakpoint pending on
break Latte::Settings::Layout::Delegate::PersistentMenu::setVisible
commands
silent
printf "=== PersistentMenu::setVisible(%d)\n", visible
bt 14
continue
end
break QWidget::close()
commands
silent
printf "=== QWidget::close()\n"
bt 14
continue
end
break QWidgetPrivate::hide_helper()
commands
silent
printf "=== QWidgetPrivate::hide_helper()\n"
bt 14
continue
end
break QWidgetPrivate::close()
commands
silent
printf "=== QWidgetPrivate::close()\n"
bt 14
continue
end
break QWidget::closeEvent(QCloseEvent*)
commands
silent
printf "=== QWidget::closeEvent\n"
bt 14
continue
end
break QMenu::hideEvent(QHideEvent*)
commands
silent
printf "=== QMenu::hideEvent\n"
bt 14
continue
end
run
quit
