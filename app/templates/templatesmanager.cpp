/*
    SPDX-FileCopyrightText: 2020 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <latte_debug.h>
#include "templatesmanager.h"

// local
#include "../layout/abstractlayout.h"
#include "../layout/centrallayout.h"
#include "../layouts/importer.h"
#include "../layouts/manager.h"
#include "../layouts/storage.h"
#include "../tools/commontools.h"
#include "../view/view.h"

// Qt
#include <QDir>
#include <QRandomGenerator>

// KDE
#include <KDirWatch>
#include <KLocalizedString>

namespace Latte {
namespace Templates {

Manager::Manager(Latte::Corona *corona)
    : QObject(corona),
      m_corona(corona)
{
    KDirWatch::self()->addDir(Latte::configPath() + QLatin1String("/latte/templates"), KDirWatch::WatchFiles);
    connect(KDirWatch::self(), &KDirWatch::created, this, &Manager::onCustomTemplatesCountChanged);
    connect(KDirWatch::self(), &KDirWatch::deleted, this, &Manager::onCustomTemplatesCountChanged);
    connect(KDirWatch::self(), &KDirWatch::dirty, this, &Manager::onCustomTemplatesCountChanged);
}

Manager::~Manager()
{
}

void Manager::init()
{
    connect(this, &Manager::viewTemplatesChanged, m_corona->layoutsManager(), &Latte::Layouts::Manager::viewTemplatesChanged);

    initLayoutTemplates();
    initViewTemplates();
}

void Manager::initLayoutTemplates()
{
    m_layoutTemplates.clear();
    initLayoutTemplates(m_corona->kPackage().filePath("templates"));
    initLayoutTemplates(Latte::configPath() + QLatin1String("/latte/templates"));
    Q_EMIT layoutTemplatesChanged();
}

void Manager::initViewTemplates()
{
    m_viewTemplates.clear();
    initViewTemplates(m_corona->kPackage().filePath("templates"));
    initViewTemplates(Latte::configPath() + QLatin1String("/latte/templates"));
    Q_EMIT viewTemplatesChanged();
}

void Manager::initLayoutTemplates(const QString &path)
{
    QDir templatesDir(path);
    QStringList filter;
    filter.append(QStringLiteral("*.layout.latte"));
    QStringList templates = templatesDir.entryList(filter, QDir::Files | QDir::Hidden | QDir::NoSymLinks);

    for (int i = 0; i < templates.count(); ++i) {
        QString templatePath = templatesDir.path() + QLatin1String("/") + templates[i];

        if (!m_layoutTemplates.containsId(templatePath)) {
            CentralLayout layouttemplate(this, templatePath);

            Data::Layout tdata = layouttemplate.data();
            tdata.isTemplate = true;

            if (tdata.name == QLatin1String(DEFAULTLAYOUTTEMPLATENAME) || tdata.name == QLatin1String(EMPTYLAYOUTTEMPLATENAME)) {
                QByteArray templateNameChars = tdata.name.toUtf8();
                tdata.name = i18n(templateNameChars.constData());
            }

            m_layoutTemplates << tdata;
        }
    }
}

QList<ViewTemplateEntry> viewTemplateEntries(const QString &folder, bool systemFolder)
{
    QList<ViewTemplateEntry> entries;

    QDir templatesDir(folder);
    QStringList filter;
    filter.append(QStringLiteral("*.view.latte"));
    const QStringList files = templatesDir.entryList(filter, QDir::Files | QDir::Hidden | QDir::NoSymLinks);

    for (const QString &file : files) {
        ViewTemplateEntry entry;
        entry.id = templatesDir.path() + QLatin1String("/") + file;

        const QString baseName = viewTemplateBaseName(entry.id);

        if (systemFolder) {
            //! msgctxt "view template name": the msgid is the base name, see exposeTranslatedTemplateNames()
            QByteArray tnamechars = baseName.toUtf8();
            entry.name = i18nc("view template name", tnamechars.constData());
        } else {
            entry.name = baseName;
        }

        entries << entry;
    }

    return entries;
}

void Manager::initViewTemplates(const QString &path)
{
    //! Only the shell package's folder is shown translated.
    const bool istranslated = (m_corona->kPackage().filePath("templates") == path);

    const QList<ViewTemplateEntry> entries = viewTemplateEntries(path, istranslated);

    for (const ViewTemplateEntry &entry : entries) {
        if (!m_viewTemplates.containsId(entry.id)) {
            Data::Generic vdata;
            vdata.id = entry.id;
            vdata.name = entry.name;

            m_viewTemplates << vdata;
        }
    }
}

Data::Layout Manager::layoutTemplateForName(const QString &layoutName)
{
    if (m_layoutTemplates.containsName(layoutName)) {
        QString layoutid = m_layoutTemplates.idForName(layoutName);
        return m_layoutTemplates[layoutid];
    }

    return Data::Layout();
}

Data::LayoutsTable Manager::layoutTemplates()
{
    Data::LayoutsTable templates;

    QString id = m_layoutTemplates.idForName(i18n(DEFAULTLAYOUTTEMPLATENAME));
    templates << m_layoutTemplates[id];
    id = m_layoutTemplates.idForName(i18n(EMPTYLAYOUTTEMPLATENAME));
    templates << m_layoutTemplates[id];

    for (int i = 0; i < m_layoutTemplates.rowCount(); ++i) {
        if (m_layoutTemplates[i].name != i18n(DEFAULTLAYOUTTEMPLATENAME)
            && m_layoutTemplates[i].name != i18n(EMPTYLAYOUTTEMPLATENAME)
            && m_layoutTemplates[i].name != Layout::MULTIPLELAYOUTSHIDDENNAME()) {
            templates << m_layoutTemplates[i];
        }
    }

    return templates;
}

Data::GenericBasicTable Manager::viewTemplates()
{
    return m_viewTemplates;
}

QString Manager::newLayout(QString layoutName, QString layoutTemplate)
{
    if (!m_layoutTemplates.containsName(layoutTemplate)) {
        return QString();
    }

    if (layoutName.isEmpty()) {
        layoutName = Layouts::Importer::uniqueLayoutName(layoutTemplate);
    } else {
        layoutName = Layouts::Importer::uniqueLayoutName(layoutName);
    }

    QString newLayoutPath = Layouts::Importer::layoutUserFilePath(layoutName);

    Data::Layout dlayout = layoutTemplateForName(layoutTemplate);
    QFile(dlayout.id).copy(newLayoutPath);
    qCDebug(latteLayout) << "adding layout : " << layoutName << " based on layout template:" << layoutTemplate;

    Q_EMIT newLayoutAdded(newLayoutPath);

    return newLayoutPath;
}

bool Manager::exportTemplate(const QString &originFile, const QString &destinationFile, const Data::AppletsTable &approvedApplets)
{
    return Latte::Layouts::Storage::self()->exportTemplate(originFile, destinationFile, approvedApplets);
}

bool Manager::exportTemplate(const Latte::View *view, const QString &destinationFile, const Data::AppletsTable &approvedApplets)
{
    return Latte::Layouts::Storage::self()->exportTemplate(view->layout(), view->containment(), destinationFile, approvedApplets);
}

void Manager::onCustomTemplatesCountChanged(const QString &file)
{
    if (file.startsWith(Latte::configPath() + QLatin1String("/latte/templates"))) {
        if (file.endsWith(QStringLiteral(".layout.latte"))) {
            initLayoutTemplates();
        } else if (file.endsWith(QStringLiteral(".view.latte"))) {
            initViewTemplates();
        }
    }
}

void Manager::importSystemLayouts()
{
    for (int i = 0; i < m_layoutTemplates.rowCount(); ++i) {
        if (m_layoutTemplates[i].isSystemTemplate()) {
            QString userLayoutPath = Layouts::Importer::layoutUserFilePath(m_layoutTemplates[i].name);

            if (!QFile(userLayoutPath).exists()) {
                QFile(m_layoutTemplates[i].id).copy(userLayoutPath);
                qCDebug(latteLayout) << "adding layout : " << userLayoutPath << " based on layout template:" << m_layoutTemplates[i].name;
            }
        }
    }
}

QString Manager::proposedTemplateAbsolutePath(QString templateFilename)
{
    QString tempfilename = templateFilename;

    if (tempfilename.endsWith(QStringLiteral(".layout.latte"))) {
        QString clearedname = tempfilename.chopped(QStringLiteral(".layout.latte").size());
        tempfilename = uniqueLayoutTemplateName(clearedname) + QStringLiteral(".layout.latte");
    } else if (tempfilename.endsWith(QStringLiteral(".view.latte"))) {
        QString clearedname = tempfilename.chopped(QStringLiteral(".view.latte").size());
        tempfilename = uniqueViewTemplateName(clearedname) + QStringLiteral(".view.latte");
    }

    return Latte::configPath() + QLatin1String("/latte/templates/") + tempfilename;
}

bool Manager::hasCustomLayoutTemplate(const QString &templateName) const
{
    for (int i = 0; i < m_layoutTemplates.rowCount(); ++i) {
        if (m_layoutTemplates[i].name == templateName && !m_layoutTemplates[i].isSystemTemplate()) {
            return true;
        }
    }

    return false;
}

bool Manager::hasLayoutTemplate(const QString &templateName) const
{
    return m_layoutTemplates.containsName(templateName);
}

bool Manager::hasViewTemplate(const QString &templateName) const
{
    return !viewTemplateFilePath(templateName).isEmpty();
}

QString Manager::viewTemplateFilePath(const QString templateName) const
{
    QList<ViewTemplateEntry> entries;

    for (int i = 0; i < m_viewTemplates.rowCount(); ++i) {
        ViewTemplateEntry entry;
        entry.id = m_viewTemplates[i].id;
        entry.name = m_viewTemplates[i].name;
        entries << entry;
    }

    return selectViewTemplateId(entries, templateName);
}

QStringList Manager::knownViewTemplateNames() const
{
    QStringList names;

    for (int i = 0; i < m_viewTemplates.rowCount(); ++i) {
        const QString baseName = viewTemplateBaseName(m_viewTemplates[i].id);
        const QString shownName = m_viewTemplates[i].name;

        //! Both names select the template, so both are listed when they differ, e.g. "Default Dock (默认停靠栏)"
        names << (shownName == baseName ? baseName : QStringLiteral("%1 (%2)").arg(baseName, shownName));
    }

    return names;
}

void Manager::installCustomLayoutTemplate(const QString &templateFilePath)
{
    if (!templateFilePath.endsWith(QStringLiteral(".layout.latte"))) {
        return;
    }

    QString layoutName = QFileInfo(templateFilePath).baseName();

    QString destinationFilePath = Latte::configPath() + QLatin1String("/latte/templates/") + layoutName + QStringLiteral(".layout.latte");

    if (hasCustomLayoutTemplate(layoutName)) {
        QFile(destinationFilePath).remove();
    }

    QFile(templateFilePath).copy(destinationFilePath);
}

QString Manager::uniqueLayoutTemplateName(QString name) const
{
    if (hasLayoutTemplate(name)) {
        name = Latte::stripUniqueNameSuffix(name);
    }

    int i = 2;

    QString namePart = name;

    while (hasLayoutTemplate(name) && i < 10000) {
        name = namePart + QLatin1String(" - ") + QString::number(i);
        i++;
    }

    if (hasLayoutTemplate(name)) {
        //! All numbered suffixes are taken — fall back to a random
        //! suffix instead of returning a name that still exists.
        name = namePart + QLatin1String(" - ") + QString::number(QRandomGenerator::global()->generate(), 16);
    }

    return name;
}

QString Manager::uniqueViewTemplateName(QString name) const
{
    if (hasViewTemplate(name)) {
        name = Latte::stripUniqueNameSuffix(name);
    }

    int i = 2;

    QString namePart = name;

    while (hasViewTemplate(name) && i < 10000) {
        name = namePart + QLatin1String(" - ") + QString::number(i);
        i++;
    }

    if (hasViewTemplate(name)) {
        //! All numbered suffixes are taken — fall back to a random
        //! suffix instead of returning a name that still exists.
        name = namePart + QLatin1String(" - ") + QString::number(QRandomGenerator::global()->generate(), 16);
    }

    return name;
}

QString Manager::templateName(const QString &filePath)
{
    int lastSlash = filePath.lastIndexOf(QLatin1Char('/'));
    QString tempFilePath = filePath;
    QString templatename = tempFilePath.remove(0, lastSlash + 1);

    QString extension(QStringLiteral(".layout.latte"));
    int ext = templatename.lastIndexOf(extension);

    if (ext > 0) {
        templatename = templatename.remove(ext, extension.size());
    } else {
        extension = QStringLiteral(".view.latte");
        ext = templatename.lastIndexOf(extension);
        templatename = templatename.remove(ext, extension.size());
    }

    return templatename;
}

//! it is used in order to provide translations for system templates
void Manager::exposeTranslatedTemplateNames()
{
    //! layout templates default names
    i18nc("default layout template name", "Default");
    i18nc("empty layout template name", "Empty");

    //! dock templates default names
    i18nc("view template name", "Default Dock");
}

}
}
