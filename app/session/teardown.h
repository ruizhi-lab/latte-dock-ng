/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef LATTE_SESSION_TEARDOWN_H
#define LATTE_SESSION_TEARDOWN_H

// Qt
#include <QCoreApplication>
#include <QEvent>
#include <QObject>

namespace Latte::Session {

//! Deletes an application service now through the deferred-delete path, so
//! ~Corona() can impose its dependency order without mixing manual delete
//! with the DeferredDelete processing that main() runs after exec().
//!
//! The service may already be gone: Corona::onAboutToQuit() deleteLater()s
//! the view settings factory on a normal quit, and QCoreApplication::exec()
//! processes that deferred delete during its cleanup, before main() destroys
//! the Corona. Callers pass a QPointer so this sees null. Calling deleteLater()
//! on the freed object locked a garbage post-event mutex and hung the quit.
inline void deleteNow(QObject *service)
{
    if (!service) {
        return;
    }

    service->deleteLater();
    QCoreApplication::sendPostedEvents(service, QEvent::DeferredDelete);
}

}

#endif
