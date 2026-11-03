/*
    SPDX-FileCopyrightText: 2026 Latte Dock NG contributors

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef LATTELAYOUTFILEMOVE_H
#define LATTELAYOUTFILEMOVE_H

// Qt
#include <QList>
#include <QString>
#include <QStringList>

namespace Latte {
namespace Layouts {
namespace LayoutFileMove {

//! One layout file that Apply renames: the path the layout has before Apply and the path
//! the layout must have after it.
struct Request {
    QString fromPath;
    QString toPath;
};

struct Result {
    //! moved[i] is true when requests[i] ended at its toPath. When it is false the file is
    //! still at its fromPath, and the layout must keep that path.
    QList<bool> moved;
    //! Files that could not be returned to either path (an I/O failure while a move was being
    //! undone). They are left where they are, and a file still inside the temporary directory
    //! is removed with that directory unless the caller keeps it.
    QStringList unrecovered;
};

//! Moves the files of all requests so that a swap (A to B and B to A) or a name chain (A to B
//! and B to C) can be applied in one Apply.
//!
//! Trigger: Apply gives layouts new names, and a new name can be the old name of another
//! layout that is renamed in the same Apply. Authoritative state: the files on disk. The caller
//! takes the id of each layout from Result::moved, so a layout whose move failed keeps the path
//! its file is still at. Lifecycle: tempDir must be a directory on the same filesystem as the
//! requests, so that every move is an atomic rename; the caller creates it and removes it.
//! Failure mode prevented: a file moved onto a name another layout still needs, a layout whose
//! file is not at the id the model holds, and a file left in tempDir and deleted with it.
//!
//! Stage one parks each file in tempDir, which frees its old path. Stage two places the parked
//! files on their final paths. A placement that fails undoes the placements of its round, puts
//! the failed file back on its old path (free again after the undo, because only this round's
//! placements can have taken it) and starts the round again without that file. Every rename
//! refuses an existing target, so a file that Apply does not move is never overwritten.
Result moveFiles(const QList<Request> &requests, const QString &tempDir);

}
}
}

#endif
