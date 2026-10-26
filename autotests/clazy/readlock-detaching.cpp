// Negative fixture for scripts/test-clazy.py: must trigger the clazy readlock-detaching check.
#include <QMap>
#include <QReadLocker>
#include <QReadWriteLock>
#include <QString>
class Probe {
public:
    void run(const QString &key)
    {
        QReadLocker locker(&m_lock);
        auto it = m_map.find(key);
        Q_UNUSED(it)
    }
private:
    QReadWriteLock m_lock;
    QMap<QString, QString> m_map;
};
