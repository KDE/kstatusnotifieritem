/* This file is part of the dbusmenu-qt library
   SPDX-FileCopyrightText: 2009 Canonical
   Author: Aurelien Gateau <aurelien.gateau@canonical.com>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/
#ifndef DBUSMENUEXPORTER_H
#define DBUSMENUEXPORTER_H

// Qt
#include <QDBusConnection>
#include <QObject>

class QAction;
class QMenu;

class DBusMenuExporterPrivate;

/*!
 * \class DBusMenuExporter
 * \inmodule KStatusNotifierItem
 * \brief Allows to serialize a menu over DBus.
 *
 * \internal
 */
class DBusMenuExporter : public QObject
{
    Q_OBJECT
public:
    /*!dbus object path
     * \brief Creates a DBusMenuExporter exporting menu at the \a dbusObjectPath,
     * using the given \a dbusConnection.
     *
     * The instance adds itself to the menu children.
     */
    DBusMenuExporter(const QString &dbusObjectPath, QMenu *menu, const QDBusConnection &dbusConnection = QDBusConnection::sessionBus());

    ~DBusMenuExporter() override;

    /*!
     * \brief Asks the matching DBusMenuImporter to activate \a action.
     *
     * For menus it means popup them, for items it means triggering the
     * associated \a action.
     *
     * \a action The action to be activated.
     */
    void activateAction(QAction *action);

    /*!
     * \brief Sets the \a status of the menu.
     *
     * Can be one of "normal" or "notice". This can be
     * used to notify the other side the menu should be made more visible.
     * For example, appmenu uses it to tell Unity panel to show/hide the menubar
     * when the Alt modifier is pressed/released.
     *
     * \sa status()
     */
    void setStatus(const QString &status);

    /*!
     * \brief Returns the status of the menu.
     *
     * \sa setStatus()
     */
    QString status() const;

protected:
    /*!
     * \brief The icon name used to present an \a action icon over DBus.
     *
     * Default implementation returns action->icon().name() when built on Qt
     * >= 4.7 and a null string otherwise.
     */
    virtual QString iconNameForAction(QAction *action);

private Q_SLOTS:
    void doUpdateActions();
    void doEmitLayoutUpdated();
    void slotActionDestroyed(QObject *);

private:
    Q_DISABLE_COPY(DBusMenuExporter)
    DBusMenuExporterPrivate *const d;

    friend class DBusMenuExporterPrivate;
    friend class DBusMenuExporterDBus;
    friend class DBusMenu;
};

#endif /* DBUSMENUEXPORTER_H */
