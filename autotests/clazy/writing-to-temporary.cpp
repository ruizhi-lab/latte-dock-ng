// Negative fixture for scripts/test-clazy.py: must trigger the clazy writing-to-temporary check.
#include <QSizePolicy>
#include <QWidget>
void probe(QWidget *widget)
{
    widget->sizePolicy().setHorizontalStretch(1);
}
