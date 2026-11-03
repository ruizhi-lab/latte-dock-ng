/*
    SPDX-FileCopyrightText: 2026 Latte Dock NG contributors

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "layoutfilemove.h"

// Qt
#include <QDir>
#include <QFile>
#include <QFileInfo>

// std
#include <utility>

namespace Latte {
namespace Layouts {
namespace LayoutFileMove {

namespace {

//! The existence check is made here and not left to QFile::rename(): an overwrite would destroy
//! the file of a layout that Apply does not move.
bool renameWithoutOverwrite(const QString &source, const QString &target)
{
    return !QFileInfo::exists(target) && QFile::rename(source, target);
}

}

Result moveFiles(const QList<Request> &requests, const QString &tempDir)
{
    const qsizetype count = requests.count();

    Result result;
    result.moved = QList<bool>(count, false);

    //! parkedPath[i] is where requests[i] waits between the two stages; pending[i] is true while
    //! that file still has to be placed on its toPath
    QList<QString> parkedPath(count);
    QList<bool> pending(count, false);

    //! Stage one: take every file out of its old path. A file that cannot be taken out is not
    //! pending, and it stays on its old path, which is the path its layout keeps.
    for (qsizetype i = 0; i < count; ++i) {
        const QString parked = QDir(tempDir).filePath(QString::number(i) + QLatin1Char('-') + QFileInfo(requests[i].fromPath).fileName());

        if (renameWithoutOverwrite(requests[i].fromPath, parked)) {
            parkedPath[i] = parked;
            pending[i] = true;
        }
    }

    //! Stage two: place the parked files, and stop each round at its first failure. The
    //! placements of a failed round are undone before the failed file goes back to its old path,
    //! so that path is free: a placement of a previous round was already undone when that round
    //! failed, and only the current round can have taken it.
    for (;;) {
        QList<qsizetype> placed;
        qsizetype failed = -1;

        for (qsizetype i = 0; i < count && failed < 0; ++i) {
            if (!pending[i]) {
                continue;
            }

            if (renameWithoutOverwrite(parkedPath[i], requests[i].toPath)) {
                placed << i;
            } else {
                failed = i;
            }
        }

        if (failed < 0) {
            for (const qsizetype i : std::as_const(placed)) {
                result.moved[i] = true;
            }

            return result;
        }

        for (qsizetype k = placed.count() - 1; k >= 0; --k) {
            const qsizetype i = placed[k];

            //! The file is back in tempDir and is placed again in a later round. If the undo fails,
            //! the file stays on its toPath and is reported, not retried.
            if (!renameWithoutOverwrite(requests[i].toPath, parkedPath[i])) {
                result.unrecovered << requests[i].toPath;
                pending[i] = false;
            }
        }

        //! The file that failed is not retried in this Apply; it returns to the path its layout keeps.
        if (!renameWithoutOverwrite(parkedPath[failed], requests[failed].fromPath)) {
            result.unrecovered << parkedPath[failed];
        }

        pending[failed] = false;
    }
}

}
}
}
