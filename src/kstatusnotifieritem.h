/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2009 Marco Martin <notmart@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KSTATUSNOTIFIERITEM_H
#define KSTATUSNOTIFIERITEM_H

#include <QObject>
#include <QPoint>
#include <QString>
#include <QWindow>

#include <kstatusnotifieritem_export.h>

#include <memory>

class QAction;
class QMenu;

class KStatusNotifierItemPrivate;

/**
 * @class KStatusNotifierItem kstatusnotifieritem.h KStatusNotifierItem
 *
 * \brief Use this to create tray icons.
 *
 * This class allows applications to provide an icon used to notify the user
 * of different statuses: whether an application is running, an action is
 * being done by the application, online state, message count, etc.
 *
 * The application communicates with the system tray implementation (or
 * equivalent software) via D-Bus to display this icon, and the system tray
 * implementation then chooses how to render the icon and, since version 4.5,
 * also its menu.
 *
 * The type of icon to be shown is announced by the application with
 * @ref ItemCategory and its current state is set with @ref ItemStatus.
 *
 * Whenever possible, prefer passing an icon by name rather than by
 * pixmap, because:
 *
 * - It is much lighter on D-Bus (no need to pass all image pixels).
 *
 * - It allows for the system tray to load an icon of the appropriate
 *   size or to replace your icon with a system tray-specific icon that matches
 *   the desktop theme.
 *
 * - Some implementations of the system tray do not support passing icons by
 *   pixmap and will show a blank icon instead.
 *
 * Usage:
 * @code
 * KStatusNotifierItem* tray = new KStatusNotifierItem();
 * tray->setIconByName("yourappicon");
 * tray->setStatus(KStatusNotifierItem::Active);
 * QAction* settingsAction = new QAction(QIcon::fromTheme("settings-configure-symbolic"), "YourApp settings...", nullptr);
 * tray->contextMenu()->addAction(settingsAction);
 * @endcode
 *
 * @author Marco Martin <notmart@gmail.com>
 * @since 4.4
 */
class KSTATUSNOTIFIERITEM_EXPORT KStatusNotifierItem : public QObject
{
    Q_OBJECT

    /**
     * @see category()
     * @see setCategory()
     * @see ItemCategory
     */
    Q_PROPERTY(ItemCategory category READ category WRITE setCategory)
    /**
     * @see title()
     * @see setTitle()
     */
    Q_PROPERTY(QString title READ title WRITE setTitle)
    /**
     * @see status()
     * @see setStatus()
     */
    Q_PROPERTY(ItemStatus status READ status WRITE setStatus)
    /**
     * @see iconName()
     * @see setIconByName()
     */
    Q_PROPERTY(QString iconName READ iconName WRITE setIconByName)
    /**
     * @see overlayIconName()
     * @see setOverlayIconByName()
     */
    Q_PROPERTY(QString overlayIconName READ overlayIconName WRITE setOverlayIconByName)
    /**
     * @see attentionIconName()
     * @see setAttentionIconByName()
     */
    Q_PROPERTY(QString attentionIconName READ attentionIconName WRITE setAttentionIconByName)
    /**
     * @see toolTipIconName()
     * @see setToolTipIconByName()
     */
    Q_PROPERTY(QString toolTipIconName READ toolTipIconName WRITE setToolTipIconByName)
    /**
     * @see toolTipTitle()
     * @see setToolTipTitle()
     */
    Q_PROPERTY(QString toolTipTitle READ toolTipTitle WRITE setToolTipTitle)
    /**
     * @see toolTipSubTitle()
     * @see setToolTipSubTitle()
     */
    Q_PROPERTY(QString toolTipSubTitle READ toolTipSubTitle WRITE setToolTipSubTitle)

    friend class KStatusNotifierItemDBus;
    friend class KStatusNotifierItemPrivate;

public:
    /**
     * The status the tray icon can have, depending on the
     * importance of the events that happens in the parent application.
     */
    enum ItemStatus {
        /**
         * Nothing is happening in the application, so showing this icon at
         * all times is not required and the system tray icon implementation
         * may choose to hide it. Use this so the icon is shown only when
         * necessary.
         *
         * This is the default value.
         */
        Passive = 1,
        /**
         * The application is doing something, or it is necessary for the
         * icon to always be reachable to the user. Use this to show the icon
         * at all times.
         */
        Active = 2,
        /**
         * The application is requesting the attention of the user. Use this
         * for instance to indicate a battery running out or a new IM message
         * was received.
         *
         * The animation used for requesting the attention of the user depends
         * on the system tray implementation.
         *
         * @see setAttentionIconByName()
         * @see attentionIconName()
         * @see setAttentionIconByPixmap()
         * @see attentionIconPixmap()
         * @see setAttentionMovieByName()
         * @see attentionMovieName()
         * @see showMessage()
         */
        NeedsAttention = 3,
    };
    Q_ENUM(ItemStatus)

    /**
     * The type of application announced to the system tray implementation.
     * This allows the application to signal the system tray implementation
     * to be drawn in a different way or at a different place.
     * @see category()
     * @see setCategory()
     */
    enum ItemCategory {
        /**
         * The tray icon used for a generic application.
         *
         * This is the default value.
         */
        ApplicationStatus = 1,
        /**
         * The tray icon used for a communications-oriented application, such
         * as an email client or messenger application. It will be used for
         * things such as notifying the user of a new message.
         *
         * @see overlayIconName()
         * @see overlayIconPixmap()
         * @see setOverlayIconByName()
         * @see setOverlayIconByPixmap()
         */
        Communications = 2,
        /**
         * The tray icon used for a system service.
         *
         * In case the system tray implementation distinguishes between
         * application tray icons and system tray icons, this will signal that
         * the application wants to be displayed in the system tray.
         *
         * Use this for system-related applications such as a notifications
         * manager or a clipboard.
         */
        SystemServices = 3,
        /**
         * The tray icon used for a hardware-oriented application.
         *
         * Use this for things such as a battery indicator or device manager.
         */
        Hardware = 4,
        Reserved = 129,
    };
    Q_ENUM(ItemCategory)

    /**
     * @brief Constructs a new tray icon.
     *
     * This will automatically generate an id based on the application name.
     *
     * @param parent The parent object for this object. If the object passed
     * in as a parent is also a QWidget, it will be used as the main
     * application window represented by this icon and it will be shown/hidden
     * when an activation is requested.
     *
     * @see KStatusNotifierItem(const QString& id, QObject* parent)
     * @see id()
     * @see associatedWindow()
     * @see setAssociatedWindow()
     **/
    explicit KStatusNotifierItem(QObject *parent = nullptr);

    /**
     * @brief Constructs a new tray icon with a unique identifier.
     *
     * If your application has more than one tray icon and the user
     * should be able to manipulate them separately (e.g. mark them for hiding
     * in a user interface), the @p id can be used to differentiate between them.
     *
     * The @p id should remain consistent even between application restarts.
     * A tray icon without @p id defaults to the application's name
     * for the @p id.
     *
     * This id may be used, for instance, by hosts displaying tray icons to
     * associate configuration information with this item in a way that can persist
     * between sessions or application restarts.
     *
     * @param id The unique id for this icon.
     * @param parent The parent object for this object. If the object passed in as
     * a parent is also a QWidget, it will  be used as the main application window
     * represented by this icon and will be shown/hidden when an activation is requested.
     *
     * @see KStatusNotifierItem()
     * @see id()
     * @see associatedWindow()
     **/
    explicit KStatusNotifierItem(const QString &id, QObject *parent = nullptr);

    ~KStatusNotifierItem() override;

    /**
     * @brief The tray icon's id.
     *
     * If KStatusNotifierItem() was used, the id will be the application name.
     *
     * If KStatusNotifierItem(QString& id, QObject*) was used, the id will be
     * the id specified in the constructor.
     *
     * This should be guaranteed to be consistent between application starts and
     * untranslated, as host applications displaying items may use it for
     * storing configuration related to this item.
     *
     * @return The id specified in the constructor.
     * @see KStatusNotifierItem()
     * @see KStatusNotifierItem(QString&, QObject*)
     */
    QString id() const;

    /**
     * @brief Sets the category for this tray icon.
     *
     * @param category The new category to be set.
     *
     * @see category()
     * @see ItemCategory
     */
    void setCategory(const ItemCategory category);

    /**
     * @return The application's ItemCategory.
     *
     * @see setCategory()
     * @see ItemCategory
     */
    ItemCategory category() const;

    /**
     * @brief Sets a title for this tray icon.
     *
     * @see title()
     */
    void setTitle(const QString &title);

    /**
     * @return The title of this tray icon.
     *
     * @see setTitle()
     */
    QString title() const;

    /**
     * @brief Sets a new status for this tray icon.
     *
     * @see status()
     * @see ItemStatus
     */
    void setStatus(const ItemStatus status);

    /**
     * @return The current application's ItemStatus.
     *
     * @see setStatus()
     * @see ItemStatus
     */
    ItemStatus status() const;

    // Main icon related functions
    /**
     * @brief Sets a new main icon.
     *
     * @param name The platform icon name to be set. This should be the same
     *             name as returned via QIcon::fromTheme().
     *
     * @see iconName()
     */
    void setIconByName(const QString &name);

    /**
     * @return The name of the main icon to be displayed.
     *
     * @see setIconByName()
     */
    QString iconName() const;

    /**
     * @brief Sets a new main icon using a pixmap.
     *
     * Prefer using setIconByName() when possible.
     *
     * @param icon The pixmap to be set.
     *
     * @see iconPixmap()
     * @see setIconByName()
     */
    void setIconByPixmap(const QIcon &icon);

    /**
     * @return A pixmap of the icon.
     *
     * @see setIconByPixmap()
     */
    QIcon iconPixmap() const;

    /**
     * @brief Sets an icon to be used as overlay on top of the main one.
     *
     * The overlay icon is typically used for notification indicators, such as
     * a dot or number in a messenger application.
     *
     * If @p name is an empty QString() and overlayIconName() is also empty,
     * the icon will be removed.
     *
     * @param name The icon name to be set.
     *
     * @see overlayIconName()
     * @see ItemCategory
     */
    void setOverlayIconByName(const QString &name);

    /**
     * @return The name of the overlay icon.
     *
     * The overlay icon is typically used for notification indicators, such as
     * a dot or number in a messenger application.
     *
     * @see setOverlayIconByName()
     * @see ItemCategory
     */
    QString overlayIconName() const;

    /**
     * @brief Sets an icon to be used as overlay on top of the main one using
     * a pixmap.
     *
     * The overlay icon is typically used for notification indicators, such as
     * a dot or number in a messenger application.
     *
     * Prefer using setOverlayIconByName() whenever possible.
     *
     * If @p icon is an empty QIcon() and overlayIconPixmap() is also empty,
     * the overlay icon will be removed.
     *
     * @param icon The overlay icon.
     *
     * @see overlayIconPixmap()
     * @see setOverlayIconByName()
     */
    void setOverlayIconByPixmap(const QIcon &icon);

    /**
     * @return A pixmap of the icon.
     *
     * @see setOverlayIconByPixmap()
     */
    QIcon overlayIconPixmap() const;

    // Requesting attention icon

    /**
     * @brief Sets a new icon to be used when the application
     * requests attention.
     *
     * @param name The platform icon name to be set. This should be the same
     *             name as returned via QIcon::fromTheme().
     *
     * @see attentionIconName()
     * @see ItemStatus
     */
    void setAttentionIconByName(const QString &name);

    /**
     * @return The name of the icon to be displayed when the application
     * is requesting the user's attention.
     *
     * @see setAttentionIconByName()
     * @see ItemStatus
     */
    QString attentionIconName() const;

    /**
     * Sets the pixmap of the requesting attention icon.
     *
     * Prefer using setAttentionIconByName() when possible.
     *
     * @param icon The icon to use for requesting attention.
     *
     * @see attentionIconPixmap()
     * @see setAttentionIconByName()
     * @see ItemStatus
     */
    void setAttentionIconByPixmap(const QIcon &icon);

    /**
     * @return A pixmap of the requesting attention icon.
     *
     * @see setAttentionIconByPixmap()
     * @see ItemStatus
     */
    QIcon attentionIconPixmap() const;

    /**
     * @brief Sets a movie as the requesting attention icon.
     *
     * This overrides anything set by setAttentionIconByName() and
     * setAttentionIconByPixmap().
     *
     * @see setAttentionIconByName()
     * @see setAttentionIconByPixmap()
     * @see ItemStatus
     */
    void setAttentionMovieByName(const QString &name);

    /**
     * @return The name of the movie to be displayed when the application is
     * requesting the user's attention.
     *
     * @see setAttentionMovieByName()
     * @see ItemStatus
     */
    QString attentionMovieName() const;

    // ToolTip handling
    /**
     * @brief Sets a new tooltip for the tray icon using a platform icon name.
     *
     * A tooltip is composed of an icon, a title, and text, all optional.
     *
     * @param iconName The platform icon name to be set. This should be the same
     *                 name as returned via QIcon::fromTheme().
     * @param title The tooltip title.
     * @param subTitle The tooltip text.
     *
     * @see setToolTip(const QIcon &icon, const QString &title, const QString &subTitle)
     */
    void setToolTip(const QString &iconName, const QString &title, const QString &subTitle);

    /**
     * @brief Sets a new tooltip for the tray icon using a pixmap.
     *
     * A tooltip is composed of an icon, a title, and text, all optional.
     *
     * @param iconName The platform icon name to be set. This should be the same
     *                 name as returned via QIcon::fromTheme().
     * @param title The tooltip title.
     * @param subTitle The tooltip text.
     *
     * @see setToolTip(const QString &iconName, const QString &title, const QString &subTitle)
     */
    void setToolTip(const QIcon &icon, const QString &title, const QString &subTitle);

    /**
     * @brief Sets a new tooltip icon.
     *
     * @param name The platform icon name to be set. This should be the same
     *             name as returned via QIcon::fromTheme().
     *
     * @see toolTipIconName()
     * @see setToolTipIconByPixmap()
     */
    void setToolTipIconByName(const QString &name);

    /**
     * @return The name of the tooltip icon.
     *
     * @see setToolTipIconByName()
     */
    QString toolTipIconName() const;

    /**
     * @brief Sets a new tooltip icon using a pixmap.
     *
     * Prefer using setToolTipIconByName() when possible.
     *
     * @param icon The icon to be set.
     *
     * @see toolTipIconPixmap()
     * @see setToolTipIconByName()
     */
    void setToolTipIconByPixmap(const QIcon &icon);

    /**
     * @return A pixmap of the tooltip icon.
     *
     * @see setToolTipIconByPixmap()
     */
    QIcon toolTipIconPixmap() const;

    /**
     * @brief Sets a new tooltip title.
     *
     * @param title The tooltip title to be set.
     *
     * @see toolTipTitle()
     */
    void setToolTipTitle(const QString &title);

    /**
     * @return The tooltip title.
     *
     * @see setToolTipTitle()
     */
    QString toolTipTitle() const;

    /**
     * @brief Sets a new tooltip subtitle.
     *
     * The subtitle is the main text of the tooltip.
     *
     * @param subTitle The tooltip text.
     *
     * @see toolTipSubTitle()
     */
    void setToolTipSubTitle(const QString &subTitle);

    /**
     * @return The tooltip subtitle.
     *
     * @see setToolTipSubTitle()
     */
    QString toolTipSubTitle() const;

    /**
     * @brief Sets a new context menu for the tray icon.
     *
     * The menu will be shown with a contextMenu(int,int)
     * call by the system tray over D-Bus.
     *
     * Use this if you want to use a custom QMenu as a context menu.
     * Otherwise, use QMenu functions available from contextMenu().
     *
     * The KStatusNotifierItem instance takes ownership of the menu,
     * and will delete it upon its destruction.
     *
     * Usage:
     * @code
     * KStatusNotifierItem* tray = new KStatusNotifierItem();
     * QMenu* customContextMenu = new QMenu();
     * QAction* showAction = new QAction();
     * customContextMenu->addAction(showAction);
     * tray->setContextMenu(newContextMenu);
     * @endcode
     *
     * @see contextMenu()
     * @see QMenu
     */
    void setContextMenu(QMenu *menu);

    /**
     * @brief Allows to access the context menu associated with the tray icon.
     *
     * This is the main way to add, remove, and access actions from the main
     * context menu. Use the inherited QMenu / QWidget methods for this.
     *
     * If you need to set a custom context menu, use setContextMenu() instead.
     *
     * Usage:
     * @code
     * KStatusNotifierItem* tray = new KStatusNotifierItem();
     * QAction* showAction = new QAction();
     * QAction* settingsAction = new QAction();
     * tray->contextMenu()->addAction(showAction);
     * tray->contextMenu()->addAction(settingsAction);
     * for (QAction* action : tray->contextMenu()->actions() ) {
     *     tray->contextMenu()->removeAction(action);
     * }
     * @endcode
     *
     * @see setContextMenu()
     * @see QMenu
     * @see QWidget
     */
    QMenu *contextMenu() const;

    /**
     * @brief Sets the main window associated with this tray icon.
     *
     * This is necessary to be able to use hideAssociatedWindow().
     *
     * This is useful in case the window that needs to be activated is not
     * the top-level window, or if you need to override the default behavior
     * with a custom implementation to toggle window state.
     *
     * @param window The window to be associated with the tray icon actions.
     *
     * Usage:
     * @code
     * tray->setAssociatedWindow(mainWindow->windowHandle());
     * @endcode
     *
     * @since 6.0
     *
     * @see associatedWindow()
     * @see hideAssociatedWindow()
     */
    void setAssociatedWindow(QWindow *window);

    /**
     * @return The main window associated with this tray icon.
     *
     * The default is the top-level window.
     *
     * @since 6.0
     *
     * @see setAssociatedWindow()
     */
    QWindow *associatedWindow() const;

    /**
     * All the actions present in the menu
     */
    QList<QAction *> actionCollection() const;

    /**
     * Adds an action to the actionCollection()
     *
     * @param name the name of the action
     * @param action the action we want to add
     */
    void addAction(const QString &name, QAction *action);

    /**
     * Removes an action from the collection
     *
     * @param name the name of the action
     */
    void removeAction(const QString &name);

    /**
     * Retrieves an action from the action collection
     * by the action name
     *
     * @param name the name of the action to retrieve
     * @since 5.12
     */
    QAction *action(const QString &name) const;

    /**
     * @brief Whether to show standard actions in the menu.
     *
     * The available standard actions can be: Minimize, Restore and Quit.
     *
     * An associated window needs to be set with setAssociatedWindow() so
     * that the Minimize and Restore actions show up.
     *
     * @param enabled Whether to enable standard items in the context menu.
     *                The default is @c true.
     *
     * @see standardActionsEnabled()
     * @see setAssociatedWindow()
     */
    void setStandardActionsEnabled(bool enabled);

    /**
     * @return Whether standard actions are shown in the menu.
     *
     * The default in case setStandardActionsEnabled() was not called is @c true.
     *
     * The available standard actions can be: Minimize, Restore and Quit.
     *
     * @see setStandardActionsEnabled()
     */
    bool standardActionsEnabled() const;

    /**
     * @brief Shows the user a notification.
     *
     * Prefer using KNotification when possible.
     *
     * @param title The notification title.
     * @param message The notification text.
     * @param icon The notification icon.
     * @param timeout The time to elapse before hiding the message.
     *                The default is 10 seconds (10000 ms).
     *
     * @see KNotification
     * @see ItemStatus
     */
    void showMessage(const QString &title, const QString &message, const QString &icon, int timeout = 10000);

    /**
     * @return The last provided token to be used with Wayland's xdg_activation_v1.
     */
    QString providedToken() const;

public Q_SLOTS:

    /**
     * @brief Shows the main window and tries to position it on top
     * of the other windows. If the window is already visible, hides it.
     *
     * This is used only internally.
     *
     * Use the activateRequested signal if you want to, for example, implement
     * toggling application visibility by left clicking.
     *
     * @param pos The mouse coordinates when the event was triggered.
     *
     * @see activateRequested
     * @see secondaryActivateRequested
     */
    virtual void activate(const QPoint &pos = QPoint());

    /**
     * @brief Hides the main window, if not already hidden.
     *
     * Stores some information about the window which would otherwise be lost due to unmapping
     * from the window system. Use when toggling the main window via activate(const QPoint &)
     * is not wanted, but instead the hidden state should be reached in any case.
     *
     * @since 6.0
     *
     * @see setAssociatedWindow()
     * @see associatedWindow()
     */
    void hideAssociatedWindow();

Q_SIGNALS:
    /**
     * @brief Inform the host application that the mouse wheel (or another
     * means of scrolling that the system tray implementation provides)
     * has been used.
     *
     * @param delta The amount of scrolling, either positive or negative.
     * @param orientation The scrolling direction, either horizontal or vertical.
     */
    void scrollRequested(int delta, Qt::Orientation orientation);

    /**
     * @brief Inform the system tray implementation that an activation
     * has been requested, typically with left mouse click.
     *
     * This is not guaranteed since it is dependent on the system tray
     * representation.
     *
     * @param active If @c true, the application asks for the activation
     *               of the main window, if @c false, it asks to hide it.
     * @param pos The position of the screen where the user clicked
     *            to trigger this signal. Optionally, QPoint() can be used to set
     *            an arbitrary position different than where the user
     *            has clicked.
     *
     * Usage:
     * @code
     * QObject::connect(
     *     tray, &KStatusNotifierItem::activateRequested,
     *     [&window]{window->isVisible() ? window->hide() : window->show();}
     * );
     * @endcode
     *
     * @see secondaryActivateRequested
     */
    void activateRequested(bool active, const QPoint &pos);

    /**
     * @brief Inform the system tray implementation that an alternate
     * activation has been requested, typically with middle mouse click.
     *
     * This is not guaranteed since it is dependent on the system tray
     * representation.
     *
     * @param pos The position of the screen where the user clicked to
     *            trigger this signal. Optionally, QPoint() can be used
     *            to set an arbitrary position different than where the
     *            user has clicked.
     *
     * Usage:
     * @code
     * QObject::connect(
     *     tray, &KStatusNotifierItem::secondaryActivateRequested,
     *     this, &MicrophoneIndicator::toggleMuted;}
     * );
     * @endcode
     *
     * @see activateRequested
     */
    void secondaryActivateRequested(const QPoint &pos);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    std::unique_ptr<KStatusNotifierItemPrivate> const d;

    Q_PRIVATE_SLOT(d, void serviceChange(const QString &name, const QString &oldOwner, const QString &newOwner))
    Q_PRIVATE_SLOT(d, void contextMenuAboutToShow())
    Q_PRIVATE_SLOT(d, void maybeQuit())
    Q_PRIVATE_SLOT(d, void minimizeRestore())
    Q_PRIVATE_SLOT(d, void legacyWheelEvent(int))
    Q_PRIVATE_SLOT(d, void legacyActivated(QSystemTrayIcon::ActivationReason))
};

#endif
