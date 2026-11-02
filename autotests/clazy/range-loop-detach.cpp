// Negative fixture for scripts/test-clazy.py: must trigger the clazy range-loop-detach check.
#include <QList>
#include <QString>
class Holder
{
public:
    int total()
    {
        int sum = 0;
        for (const QString &item : m_items) {
            sum += item.size();
        }
        return sum;
    }

private:
    QList<QString> m_items;
};
