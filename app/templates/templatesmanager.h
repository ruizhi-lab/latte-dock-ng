/*
    SPDX-FileCopyrightText: 2020 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef TEMPLATESMANAGER_H
#define TEMPLATESMANAGER_H

// local
#include "viewtemplatenames.h"
#include "../lattecorona.h"
#include "../data/appletdata.h"
#include "../data/layoutdata.h"
#include "../data/layoutstable.h"
#include "../data/genericbasictable.h"

// Qt
#include <QObject>
#include <QStringList>

// KDE
#include <KLocalizedString>

namespace Latte {
class Corona;
class View;
}

namespace Latte {
namespace Templates {

const char DEFAULTLAYOUTTEMPLATENAME[] = "Default";
const char EMPTYLAYOUTTEMPLATENAME[] = "Empty";

//! Lists the view templates of one folder with the names the UI shows: a system folder's templates are
//! translated, user templates keep their base name. Manager and the start-up check of --add-dock both use it,
//! so the set of names they accept is the same.
QList<ViewTemplateEntry> viewTemplateEntries(const QString &folder, bool systemFolder);

class Manager : public QObject
{
    Q_OBJECT

public:
    Manager(Latte::Corona *corona = nullptr);
    ~Manager() override;

    Latte::Corona *corona();
    void init();

    bool hasCustomLayoutTemplate(const QString &templateName) const;
    bool hasLayoutTemplate(const QString &templateName) const;
    //! true for the name the UI shows and for the untranslated base name (see viewtemplatenames.h)
    bool hasViewTemplate(const QString &templateName) const;

    bool exportTemplate(const QString &originFile, const QString &destinationFile, const Data::AppletsTable &approvedApplets);
    bool exportTemplate(const Latte::View *view, const QString &destinationFile, const Data::AppletsTable &approvedApplets);

    Data::Layout layoutTemplateForName(const QString &layoutName);

    Data::LayoutsTable layoutTemplates();
    Data::GenericBasicTable viewTemplates();

    //! creates a new layout with layoutName based on specific layout template and returns the new layout path
    QString newLayout(QString layoutName, QString layoutTemplate = i18n(DEFAULTLAYOUTTEMPLATENAME));

    QString proposedTemplateAbsolutePath(QString templateFilename);

    //! file path of the view template selected by a shown or base name, empty when none matches
    QString viewTemplateFilePath(const QString templateName) const;
    //! names that viewTemplateFilePath() accepts, for diagnostics
    QStringList knownViewTemplateNames() const;

    static QString templateName(const QString &filePath);

    void importSystemLayouts();
    void installCustomLayoutTemplate(const QString &templateFilePath);

Q_SIGNALS:
    void newLayoutAdded(const QString &path);
    void layoutTemplatesChanged();
    void viewTemplatesChanged();

private Q_SLOTS:
    void onCustomTemplatesCountChanged(const QString &file);

private:
    void initLayoutTemplates();
    void initViewTemplates();

    void initLayoutTemplates(const QString &path);
    void initViewTemplates(const QString &path);

    void exposeTranslatedTemplateNames();

    QString uniqueLayoutTemplateName(QString name) const;
    QString uniqueViewTemplateName(QString name) const;

private:
    Latte::Corona *m_corona;

    Data::LayoutsTable m_layoutTemplates;
    Data::GenericBasicTable m_viewTemplates;

};

}
}

#endif //TEMPLATESMANAGER_H
