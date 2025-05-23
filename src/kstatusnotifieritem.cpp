/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2009 Marco Martin <notmart@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kstatusnotifieritem.h"
#include "config-kstatusnotifieritem.h"
#include "debug_p.h"
#include "kstatusnotifieritemprivate_p.h"

#include <QApplication>
#include <QImage>
#include <QMenu>
#include <QMessageBox>
#include <QMovie>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QStandardPaths>
#ifdef Q_OS_MACOS
#include <QFontDatabase>
#endif

#if HAVE_DBUS
#include "kstatusnotifieritemdbus_p.h"

#include <QDBusConnection>

#if HAVE_DBUSMENUQT
#include "libdbusmenu-qt/dbusmenuexporter.h"
#endif // HAVE_DBUSMENUQT
#endif

#include <QTimer>
#include <kwindowsystem.h>

#if HAVE_X11
#include <KWindowInfo>
#include <KX11Extras>
#endif

#ifdef Q_OS_MACOS
namespace MacUtils
{
void setBadgeLabelText(const QString &s);
}
#endif
#include <cstdlib>

static const char s_statusNotifierWatcherServiceName[] = "org.kde.StatusNotifierWatcher";
static const int s_legacyTrayIconSize = 24;

KStatusNotifierItem::KStatusNotifierItem(QObject *parent)
    : QObject(parent)
    , d(new KStatusNotifierItemPrivate(this))
{
    d->init(QString());
}

KStatusNotifierItem::KStatusNotifierItem(const QString &id, QObject *parent)
    : QObject(parent)
    , d(new KStatusNotifierItemPrivate(this))
{
    d->init(id);
}

KStatusNotifierItem::~KStatusNotifierItem()
{
#if HAVE_DBUS
    delete d->statusNotifierWatcher;
    delete d->notificationsClient;
#endif
    delete d->systemTrayIcon;
    if (!qApp->closingDown()) {
        delete d->menu;
    }
    if (d->associatedWindow) {
        KWindowSystem::self()->disconnect(d->associatedWindow);
    }
}

QString KStatusNotifierItem::id() const
{
    // qCDebug(LOG_KSTATUSNOTIFIERITEM) << "id requested" << d->id;
    return d->id;
}

void KStatusNotifierItem::setCategory(const ItemCategory category)
{
    d->category = category;
}

KStatusNotifierItem::ItemStatus KStatusNotifierItem::status() const
{
    return d->status;
}

KStatusNotifierItem::ItemCategory KStatusNotifierItem::category() const
{
    return d->category;
}

void KStatusNotifierItem::setTitle(const QString &title)
{
    d->title = title;
}

void KStatusNotifierItem::setStatus(const ItemStatus status)
{
    if (d->status == status) {
        return;
    }

    d->status = status;

#if HAVE_DBUS
    Q_EMIT d->statusNotifierItemDBus->NewStatus(
        QString::fromLatin1(metaObject()->enumerator(metaObject()->indexOfEnumerator("ItemStatus")).valueToKey(d->status)));
#endif
    if (d->systemTrayIcon) {
        d->syncLegacySystemTrayIcon();
    }
}

// normal icon

void KStatusNotifierItem::setIconByName(const QString &name)
{
    if (d->iconName == name) {
        return;
    }

    d->iconName = name;

#if HAVE_DBUS
    d->serializedIcon = KDbusImageVector();
    Q_EMIT d->statusNotifierItemDBus->NewIcon();
#endif

    if (d->systemTrayIcon) {
        d->systemTrayIcon->setIcon(QIcon::fromTheme(name));
    }
}

QString KStatusNotifierItem::iconName() const
{
    return d->iconName;
}

void KStatusNotifierItem::setIconByPixmap(const QIcon &icon)
{
    if (d->iconName.isEmpty() && d->icon.cacheKey() == icon.cacheKey()) {
        return;
    }

    d->iconName.clear();

#if HAVE_DBUS
    d->serializedIcon = d->iconToVector(icon);
    Q_EMIT d->statusNotifierItemDBus->NewIcon();
#endif

    d->icon = icon;
    if (d->systemTrayIcon) {
        d->systemTrayIcon->setIcon(icon);
    }
}

QIcon KStatusNotifierItem::iconPixmap() const
{
    return d->icon;
}

void KStatusNotifierItem::setOverlayIconByName(const QString &name)
{
    if (d->overlayIconName == name) {
        return;
    }

    d->overlayIconName = name;
#if HAVE_DBUS
    Q_EMIT d->statusNotifierItemDBus->NewOverlayIcon();
#endif
    if (d->systemTrayIcon) {
        QPixmap iconPixmap = QIcon::fromTheme(d->iconName).pixmap(s_legacyTrayIconSize, s_legacyTrayIconSize);
        if (!name.isEmpty()) {
            QPixmap overlayPixmap = QIcon::fromTheme(d->overlayIconName).pixmap(s_legacyTrayIconSize / 2, s_legacyTrayIconSize / 2);
            QPainter p(&iconPixmap);
            p.drawPixmap(iconPixmap.width() - overlayPixmap.width(), iconPixmap.height() - overlayPixmap.height(), overlayPixmap);
            p.end();
        }
        d->systemTrayIcon->setIcon(iconPixmap);
    }
}

QString KStatusNotifierItem::overlayIconName() const
{
    return d->overlayIconName;
}

void KStatusNotifierItem::setOverlayIconByPixmap(const QIcon &icon)
{
    if (d->overlayIconName.isEmpty() && d->overlayIcon.cacheKey() == icon.cacheKey()) {
        return;
    }

    d->overlayIconName.clear();

#if HAVE_DBUS
    d->serializedOverlayIcon = d->iconToVector(icon);
    Q_EMIT d->statusNotifierItemDBus->NewOverlayIcon();
#endif

    d->overlayIcon = icon;
    if (d->systemTrayIcon) {
        QPixmap iconPixmap = d->icon.pixmap(s_legacyTrayIconSize, s_legacyTrayIconSize);
        QPixmap overlayPixmap = d->overlayIcon.pixmap(s_legacyTrayIconSize / 2, s_legacyTrayIconSize / 2);

        QPainter p(&iconPixmap);
        p.drawPixmap(iconPixmap.width() - overlayPixmap.width(), iconPixmap.height() - overlayPixmap.height(), overlayPixmap);
        p.end();
        d->systemTrayIcon->setIcon(iconPixmap);
    }
}

QIcon KStatusNotifierItem::overlayIconPixmap() const
{
    return d->overlayIcon;
}

// Icons and movie for requesting attention state

void KStatusNotifierItem::setAttentionIconByName(const QString &name)
{
    if (d->attentionIconName == name) {
        return;
    }

    d->attentionIconName = name;

#if HAVE_DBUS
    d->serializedAttentionIcon = KDbusImageVector();
    Q_EMIT d->statusNotifierItemDBus->NewAttentionIcon();
#endif
}

QString KStatusNotifierItem::attentionIconName() const
{
    return d->attentionIconName;
}

void KStatusNotifierItem::setAttentionIconByPixmap(const QIcon &icon)
{
    if (d->attentionIconName.isEmpty() && d->attentionIcon.cacheKey() == icon.cacheKey()) {
        return;
    }

    d->attentionIconName.clear();
    d->attentionIcon = icon;

#if HAVE_DBUS
    d->serializedAttentionIcon = d->iconToVector(icon);
    Q_EMIT d->statusNotifierItemDBus->NewAttentionIcon();
#endif
}

QIcon KStatusNotifierItem::attentionIconPixmap() const
{
    return d->attentionIcon;
}

void KStatusNotifierItem::setAttentionMovieByName(const QString &name)
{
    if (d->movieName == name) {
        return;
    }

    d->movieName = name;

    delete d->movie;
    d->movie = nullptr;

#if HAVE_DBUS
    Q_EMIT d->statusNotifierItemDBus->NewAttentionIcon();
#endif

    if (d->systemTrayIcon) {
        d->movie = new QMovie(d->movieName);
        d->systemTrayIcon->setMovie(d->movie);
    }
}

QString KStatusNotifierItem::attentionMovieName() const
{
    return d->movieName;
}

// ToolTip

#ifdef Q_OS_MACOS
static void setTrayToolTip(KStatusNotifierLegacyIcon *systemTrayIcon, const QString &title, const QString &subTitle)
{
    if (systemTrayIcon) {
        bool tEmpty = title.isEmpty(), stEmpty = subTitle.isEmpty();
        if (tEmpty) {
            if (!stEmpty) {
                systemTrayIcon->setToolTip(subTitle);
            } else {
                systemTrayIcon->setToolTip(title);
            }
        } else {
            if (stEmpty) {
                systemTrayIcon->setToolTip(title);
            } else {
                systemTrayIcon->setToolTip(title + QStringLiteral("\n") + subTitle);
            }
        }
    }
}
#else
static void setTrayToolTip(KStatusNotifierLegacyIcon *systemTrayIcon, const QString &title, const QString &)
{
    if (systemTrayIcon) {
        systemTrayIcon->setToolTip(title);
    }
}
#endif

void KStatusNotifierItem::setToolTip(const QString &iconName, const QString &title, const QString &subTitle)
{
    if (d->toolTipIconName == iconName && d->toolTipTitle == title && d->toolTipSubTitle == subTitle) {
        return;
    }

    d->toolTipIconName = iconName;

    d->toolTipTitle = title;
    setTrayToolTip(d->systemTrayIcon, title, subTitle);
    d->toolTipSubTitle = subTitle;

#if HAVE_DBUS
    d->serializedToolTipIcon = KDbusImageVector();
    Q_EMIT d->statusNotifierItemDBus->NewToolTip();
#endif
}

void KStatusNotifierItem::setToolTip(const QIcon &icon, const QString &title, const QString &subTitle)
{
    if (d->toolTipIconName.isEmpty() && d->toolTipIcon.cacheKey() == icon.cacheKey() //
        && d->toolTipTitle == title //
        && d->toolTipSubTitle == subTitle) {
        return;
    }

    d->toolTipIconName.clear();
    d->toolTipIcon = icon;

    d->toolTipTitle = title;
    setTrayToolTip(d->systemTrayIcon, title, subTitle);

    d->toolTipSubTitle = subTitle;
#if HAVE_DBUS
    d->serializedToolTipIcon = d->iconToVector(icon);
    Q_EMIT d->statusNotifierItemDBus->NewToolTip();
#endif
}

void KStatusNotifierItem::setToolTipIconByName(const QString &name)
{
    if (d->toolTipIconName == name) {
        return;
    }

    d->toolTipIconName = name;
#if HAVE_DBUS
    d->serializedToolTipIcon = KDbusImageVector();
    Q_EMIT d->statusNotifierItemDBus->NewToolTip();
#endif
}

QString KStatusNotifierItem::toolTipIconName() const
{
    return d->toolTipIconName;
}

void KStatusNotifierItem::setToolTipIconByPixmap(const QIcon &icon)
{
    if (d->toolTipIconName.isEmpty() && d->toolTipIcon.cacheKey() == icon.cacheKey()) {
        return;
    }

    d->toolTipIconName.clear();
    d->toolTipIcon = icon;

#if HAVE_DBUS
    d->serializedToolTipIcon = d->iconToVector(icon);
    Q_EMIT d->statusNotifierItemDBus->NewToolTip();
#endif
}

QIcon KStatusNotifierItem::toolTipIconPixmap() const
{
    return d->toolTipIcon;
}

void KStatusNotifierItem::setToolTipTitle(const QString &title)
{
    if (d->toolTipTitle == title) {
        return;
    }

    d->toolTipTitle = title;

#if HAVE_DBUS
    Q_EMIT d->statusNotifierItemDBus->NewToolTip();
#endif
    setTrayToolTip(d->systemTrayIcon, title, d->toolTipSubTitle);
}

QString KStatusNotifierItem::toolTipTitle() const
{
    return d->toolTipTitle;
}

void KStatusNotifierItem::setToolTipSubTitle(const QString &subTitle)
{
    if (d->toolTipSubTitle == subTitle) {
        return;
    }

    d->toolTipSubTitle = subTitle;
#if HAVE_DBUS
    Q_EMIT d->statusNotifierItemDBus->NewToolTip();
#else
    setTrayToolTip(d->systemTrayIcon, d->toolTipTitle, subTitle);
#endif
}

QString KStatusNotifierItem::toolTipSubTitle() const
{
    return d->toolTipSubTitle;
}

void KStatusNotifierItem::setContextMenu(QMenu *menu)
{
    if (d->menu && d->menu != menu) {
        d->menu->removeEventFilter(this);
        delete d->menu;
    }

    if (!menu) {
        d->menu = nullptr;
        return;
    }

    if (d->systemTrayIcon) {
        d->systemTrayIcon->setContextMenu(menu);
    } else if (d->menu != menu) {
        if (getenv("KSNI_NO_DBUSMENU")) {
            // This is a hack to make it possible to disable DBusMenu in an
            // application. The string "/NO_DBUSMENU" must be the same as in
            // DBusSystemTrayWidget::findDBusMenuInterface() in the Plasma
            // systemtray applet.
            d->menuObjectPath = QStringLiteral("/NO_DBUSMENU");
            menu->installEventFilter(this);
        } else {
            d->menuObjectPath = QStringLiteral("/MenuBar");
#if HAVE_DBUSMENUQT
            new DBusMenuExporter(d->menuObjectPath, menu, d->statusNotifierItemDBus->dbusConnection());
            Q_EMIT d->statusNotifierItemDBus->NewMenu();
#endif
        }

        connect(menu, SIGNAL(aboutToShow()), this, SLOT(contextMenuAboutToShow()));
    }

    d->menu = menu;
    Qt::WindowFlags oldFlags = d->menu->windowFlags();
    d->menu->setParent(nullptr);
    d->menu->setWindowFlags(oldFlags);
}

QMenu *KStatusNotifierItem::contextMenu() const
{
    return d->menu;
}

void KStatusNotifierItem::setAssociatedWindow(QWindow *associatedWindow)
{
    if (associatedWindow) {
        d->associatedWindow = associatedWindow;
        d->associatedWindow->installEventFilter(this);
        d->associatedWindowPos = QPoint(-1, -1);
    } else {
        if (d->associatedWindow) {
            d->associatedWindow->removeEventFilter(this);
            d->associatedWindow = nullptr;
        }
    }

    if (d->systemTrayIcon) {
        delete d->systemTrayIcon;
        d->systemTrayIcon = nullptr;
        d->setLegacySystemTrayEnabled(true);
    }

    if (d->associatedWindow) {
        QAction *action = d->actionCollection.value(QStringLiteral("minimizeRestore"));

        if (!action) {
            action = new QAction(this);
            d->actionCollection.insert(QStringLiteral("minimizeRestore"), action);
            action->setText(tr("&Minimize", "@action:inmenu"));
            action->setIcon(QIcon::fromTheme(QStringLiteral("window-minimize")));
            connect(action, SIGNAL(triggered(bool)), this, SLOT(minimizeRestore()));
        }

#if HAVE_X11
        if (KWindowSystem::isPlatformX11()) {
            KWindowInfo info(d->associatedWindow->winId(), NET::WMDesktop);
            d->onAllDesktops = info.onAllDesktops();
        }
#endif
    } else {
        if (d->menu && d->hasQuit) {
            QAction *action = d->actionCollection.value(QStringLiteral("minimizeRestore"));
            if (action) {
                d->menu->removeAction(action);
            }
        }

        d->onAllDesktops = false;
    }
}

QWindow *KStatusNotifierItem::associatedWindow() const
{
    return d->associatedWindow;
}

#if KSTATUSNOTIFIERITEM_BUILD_DEPRECATED_SINCE(6, 6)
QList<QAction *> KStatusNotifierItem::actionCollection() const
{
    return d->actionCollection.values();
}
#endif

#if KSTATUSNOTIFIERITEM_BUILD_DEPRECATED_SINCE(6, 6)
void KStatusNotifierItem::addAction(const QString &name, QAction *action)
{
    d->actionCollection.insert(name, action);
}
#endif

#if KSTATUSNOTIFIERITEM_BUILD_DEPRECATED_SINCE(6, 6)
void KStatusNotifierItem::removeAction(const QString &name)
{
    d->actionCollection.remove(name);
}
#endif

#if KSTATUSNOTIFIERITEM_BUILD_DEPRECATED_SINCE(6, 6)
QAction *KStatusNotifierItem::action(const QString &name) const
{
    return d->actionCollection.value(name);
}
#endif

void KStatusNotifierItem::setStandardActionsEnabled(bool enabled)
{
    if (d->standardActionsEnabled == enabled) {
        return;
    }

    d->standardActionsEnabled = enabled;

    if (d->menu && !enabled && d->hasQuit) {
        QAction *action = d->actionCollection.value(QStringLiteral("minimizeRestore"));
        if (action) {
            d->menu->removeAction(action);
        }

        action = d->actionCollection.value(QStringLiteral("quit"));
        if (action) {
            d->menu->removeAction(action);
        }

        d->hasQuit = false;
    }
}

bool KStatusNotifierItem::standardActionsEnabled() const
{
    return d->standardActionsEnabled;
}

void KStatusNotifierItem::showMessage(const QString &title, const QString &message, const QString &icon, int timeout)
{
#if HAVE_DBUS
    if (!d->notificationsClient) {
        d->notificationsClient = new org::freedesktop::Notifications(QStringLiteral("org.freedesktop.Notifications"),
                                                                     QStringLiteral("/org/freedesktop/Notifications"),
                                                                     QDBusConnection::sessionBus());
    }

    uint id = 0;
    QVariantMap hints;

    QString desktopFileName = QGuiApplication::desktopFileName();
    if (!desktopFileName.isEmpty()) {
        // handle apps which set the desktopFileName property with filename suffix,
        // due to unclear API dox (https://bugreports.qt.io/browse/QTBUG-75521)
        if (desktopFileName.endsWith(QLatin1String(".desktop"))) {
            desktopFileName.chop(8);
        }
        hints.insert(QStringLiteral("desktop-entry"), desktopFileName);
    }

    d->notificationsClient->Notify(d->title, id, icon, title, message, QStringList(), hints, timeout);
#else
    if (d->systemTrayIcon) {
        // Growl is not needed anymore for QSystemTrayIcon::showMessage() since OS X 10.8
        d->systemTrayIcon->showMessage(title, message, QSystemTrayIcon::Information, timeout);
    }
#endif
}

QString KStatusNotifierItem::title() const
{
    return d->title;
}

void KStatusNotifierItem::activate(const QPoint &pos)
{
    // if the user activated the icon the NeedsAttention state is no longer necessary
    // FIXME: always true?
    if (d->status == NeedsAttention) {
        d->status = Active;
#ifdef Q_OS_MACOS
        MacUtils::setBadgeLabelText(QString());
#endif
#if HAVE_DBUS
        Q_EMIT d->statusNotifierItemDBus->NewStatus(
            QString::fromLatin1(metaObject()->enumerator(metaObject()->indexOfEnumerator("ItemStatus")).valueToKey(d->status)));
#endif
    }

    if (d->menu && d->menu->isVisible()) {
        d->menu->hide();
    }

    if (!d->associatedWindow) {
        Q_EMIT activateRequested(true, pos);
        return;
    }

    d->checkVisibility(pos);
}

void KStatusNotifierItem::hideAssociatedWindow()
{
    if (!d->associatedWindow) {
        return;
    }
    d->minimizeRestore(false);
}

QString KStatusNotifierItem::providedToken() const
{
#if HAVE_DBUS
    return d->statusNotifierItemDBus->m_xdgActivationToken;
#else
    return {};
#endif
}

void KStatusNotifierItem::setIsMenu(bool isMenu)
{
    d->isMenu = isMenu;
}

bool KStatusNotifierItem::isMenu() const
{
    return d->isMenu;
}

bool KStatusNotifierItemPrivate::checkVisibility(QPoint pos, bool perform)
{
    // mapped = visible (but possibly obscured)
    const bool mapped = associatedWindow->isVisible() && !(associatedWindow->windowState() & Qt::WindowMinimized);

    //    - not mapped -> show, raise, focus
    //    - mapped
    //        - obscured -> raise, focus
    //        - not obscured -> hide
    // info1.mappingState() != NET::Visible -> window on another desktop?
    if (!mapped) {
        if (perform) {
            minimizeRestore(true);
            Q_EMIT q->activateRequested(true, pos);
        }

        return true;
#if HAVE_X11
    } else if (QGuiApplication::platformName() == QLatin1String("xcb")) {
        if (KWindowSystem::isPlatformX11()) {
            const KWindowInfo info1(associatedWindow->winId(), NET::XAWMState | NET::WMState | NET::WMDesktop);
            QListIterator<WId> it(KX11Extras::stackingOrder());
            it.toBack();
            while (it.hasPrevious()) {
                WId id = it.previous();
                if (id == associatedWindow->winId()) {
                    break;
                }

                KWindowInfo info2(id, NET::WMDesktop | NET::WMGeometry | NET::XAWMState | NET::WMState | NET::WMWindowType);

                if (info2.mappingState() != NET::Visible) {
                    continue; // not visible on current desktop -> ignore
                }

                if (!info2.geometry().intersects(associatedWindow->geometry())) {
                    continue; // not obscuring the window -> ignore
                }

                if (!info1.hasState(NET::KeepAbove) && info2.hasState(NET::KeepAbove)) {
                    continue; // obscured by window kept above -> ignore
                }

                /* clang-format off */
            static constexpr auto flags = (NET::NormalMask
                                           | NET::DesktopMask
                                           | NET::DockMask
                                           | NET::ToolbarMask
                                           | NET::MenuMask
                                           | NET::DialogMask
                                           | NET::OverrideMask
                                           | NET::TopMenuMask
                                           | NET::UtilityMask
                                           | NET::SplashMask);
                /* clang-format on */
                NET::WindowType type = info2.windowType(flags);

                if (type == NET::Dock || type == NET::TopMenu) {
                    continue; // obscured by dock or topmenu -> ignore
                }

                if (perform) {
                    KX11Extras::forceActiveWindow(associatedWindow->winId());
                    Q_EMIT q->activateRequested(true, pos);
                }

                return true;
            }

            // not on current desktop?
            if (!info1.isOnCurrentDesktop()) {
                if (perform) {
                    KWindowSystem::activateWindow(associatedWindow);
                    Q_EMIT q->activateRequested(true, pos);
                }

                return true;
            }

            if (perform) {
                minimizeRestore(false); // hide
                Q_EMIT q->activateRequested(false, pos);
            }
        }
        return false;
#endif
    } else {
        if (perform) {
            if (!associatedWindow->isActive()) {
                KWindowSystem::activateWindow(associatedWindow);
                Q_EMIT q->activateRequested(true, pos);
            } else {
                minimizeRestore(false); // hide
                Q_EMIT q->activateRequested(false, pos);
            }
        }
        return false;
    }

    return true;
}

bool KStatusNotifierItem::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == d->associatedWindow) {
        if (event->type() == QEvent::Show) {
            d->associatedWindow->setPosition(d->associatedWindowPos);
        } else if (event->type() == QEvent::Hide) {
            d->associatedWindowPos = d->associatedWindow->position();
        }
    }

    if (d->systemTrayIcon == nullptr) {
        // FIXME: ugly ugly workaround to weird QMenu's focus problems
        if (watched == d->menu
            && (event->type() == QEvent::WindowDeactivate
                || (event->type() == QEvent::MouseButtonRelease && static_cast<QMouseEvent *>(event)->button() == Qt::LeftButton))) {
            // put at the back of even queue to let the action activate anyways
            QTimer::singleShot(0, this, [this]() {
                d->hideMenu();
            });
        }
    }
    return false;
}

// KStatusNotifierItemPrivate

const int KStatusNotifierItemPrivate::s_protocolVersion = 0;

KStatusNotifierItemPrivate::KStatusNotifierItemPrivate(KStatusNotifierItem *item)
    : q(item)
    , category(KStatusNotifierItem::ApplicationStatus)
    , status(KStatusNotifierItem::Passive)
    , movie(nullptr)
    , systemTrayIcon(nullptr)
    , menu(nullptr)
    , associatedWindow(nullptr)
    , titleAction(nullptr)
    , hasQuit(false)
    , onAllDesktops(false)
    , standardActionsEnabled(true)
{
}

void KStatusNotifierItemPrivate::init(const QString &extraId)
{
    QWidget *parentWidget = qobject_cast<QWidget *>(q->parent());

    q->setAssociatedWindow(parentWidget ? parentWidget->window()->windowHandle() : nullptr);
#if HAVE_DBUS
    qDBusRegisterMetaType<KDbusImageStruct>();
    qDBusRegisterMetaType<KDbusImageVector>();
    qDBusRegisterMetaType<KDbusToolTipStruct>();

    statusNotifierItemDBus = new KStatusNotifierItemDBus(q);

    QDBusServiceWatcher *watcher = new QDBusServiceWatcher(QString::fromLatin1(s_statusNotifierWatcherServiceName),
                                                           QDBusConnection::sessionBus(),
                                                           QDBusServiceWatcher::WatchForOwnerChange,
                                                           q);
    QObject::connect(watcher, SIGNAL(serviceOwnerChanged(QString, QString, QString)), q, SLOT(serviceChange(QString, QString, QString)));
#endif

    // create a default menu, just like in KSystemtrayIcon
    QMenu *m = new QMenu(parentWidget);

    title = QGuiApplication::applicationDisplayName();
    if (title.isEmpty()) {
        title = QCoreApplication::applicationName();
    }
#ifdef Q_OS_MACOS
    // OS X doesn't have texted separators so we emulate QAction::addSection():
    // we first add an action with the desired text (title) and icon
    titleAction = m->addAction(qApp->windowIcon(), title);
    // this action should be disabled
    titleAction->setEnabled(false);
    // Give the titleAction a visible menu icon:
    // Systray icon and menu ("menu extra") are often used by applications that provide no other interface.
    // It is thus reasonable to show the application icon in the menu; Finder, Dock and App Switcher
    // all show it in addition to the application name (and Apple's input "menu extra" also shows icons).
    titleAction->setIconVisibleInMenu(true);
    m->addAction(titleAction);
    // now add a regular separator
    m->addSeparator();
#else
    titleAction = m->addSection(qApp->windowIcon(), title);
    m->setTitle(title);
#endif
    q->setContextMenu(m);

    QAction *action = new QAction(q);
    action->setText(KStatusNotifierItem::tr("Quit", "@action:inmenu"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("application-exit")));
    // cannot yet convert to function-pointer-based connect:
    // some apps like kalarm or korgac have a hack to rewire the connection
    // of the "quit" action to a own slot, and rely on the name-based slot to disconnect
    // quitRequested/abortQuit was added for this use case
    QObject::connect(action, SIGNAL(triggered()), q, SLOT(maybeQuit()));
    actionCollection.insert(QStringLiteral("quit"), action);

    id = title;
    if (!extraId.isEmpty()) {
        id.append(QLatin1Char('_')).append(extraId);
    }

    // Init iconThemePath to the app folder for now
    iconThemePath = QStandardPaths::locate(QStandardPaths::AppDataLocation, QStringLiteral("icons"), QStandardPaths::LocateDirectory);

    registerToDaemon();
}

void KStatusNotifierItemPrivate::registerToDaemon()
{
    bool useLegacy = false;
#if HAVE_DBUS
    qCDebug(LOG_KSTATUSNOTIFIERITEM) << "Registering a client interface to the KStatusNotifierWatcher";
    if (!statusNotifierWatcher) {
        statusNotifierWatcher = new org::kde::StatusNotifierWatcher(QString::fromLatin1(s_statusNotifierWatcherServiceName),
                                                                    QStringLiteral("/StatusNotifierWatcher"),
                                                                    QDBusConnection::sessionBus());
    }

    if (statusNotifierWatcher->isValid()) {
        // get protocol version in async way
        QDBusMessage msg = QDBusMessage::createMethodCall(QString::fromLatin1(s_statusNotifierWatcherServiceName),
                                                          QStringLiteral("/StatusNotifierWatcher"),
                                                          QStringLiteral("org.freedesktop.DBus.Properties"),
                                                          QStringLiteral("Get"));
        msg.setArguments(QVariantList{QStringLiteral("org.kde.StatusNotifierWatcher"), QStringLiteral("ProtocolVersion")});
        QDBusPendingCall async = QDBusConnection::sessionBus().asyncCall(msg);
        QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(async, q);
        QObject::connect(watcher, &QDBusPendingCallWatcher::finished, q, [this, watcher] {
            watcher->deleteLater();
            QDBusPendingReply<QVariant> reply = *watcher;
            if (reply.isError()) {
                qCDebug(LOG_KSTATUSNOTIFIERITEM) << "Failed to read protocol version of KStatusNotifierWatcher";
                setLegacySystemTrayEnabled(true);
            } else {
                bool ok = false;
                const int protocolVersion = reply.value().toInt(&ok);
                if (ok && protocolVersion == s_protocolVersion) {
                    statusNotifierWatcher->RegisterStatusNotifierItem(statusNotifierItemDBus->service());
                    setLegacySystemTrayEnabled(false);
                } else {
                    qCDebug(LOG_KSTATUSNOTIFIERITEM) << "KStatusNotifierWatcher has incorrect protocol version";
                    setLegacySystemTrayEnabled(true);
                }
            }
        });
    } else {
        qCDebug(LOG_KSTATUSNOTIFIERITEM) << "KStatusNotifierWatcher not reachable";
        useLegacy = true;
    }
#else
    useLegacy = true;
#endif
    setLegacySystemTrayEnabled(useLegacy);
}

void KStatusNotifierItemPrivate::serviceChange(const QString &name, const QString &oldOwner, const QString &newOwner)
{
    Q_UNUSED(name)
    if (newOwner.isEmpty()) {
        // unregistered
        qCDebug(LOG_KSTATUSNOTIFIERITEM) << "Connection to the KStatusNotifierWatcher lost";
        setLegacyMode(true);
#if HAVE_DBUS
        delete statusNotifierWatcher;
        statusNotifierWatcher = nullptr;
#endif
    } else if (oldOwner.isEmpty()) {
        // registered
        setLegacyMode(false);
    }
}

void KStatusNotifierItemPrivate::setLegacyMode(bool legacy)
{
    if (legacy) {
        // unregistered
        setLegacySystemTrayEnabled(true);
    } else {
        // registered
        registerToDaemon();
    }
}

void KStatusNotifierItemPrivate::legacyWheelEvent(int delta)
{
#if HAVE_DBUS
    statusNotifierItemDBus->Scroll(delta, QStringLiteral("vertical"));
#endif
}

void KStatusNotifierItemPrivate::legacyActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::MiddleClick) {
        Q_EMIT q->secondaryActivateRequested(systemTrayIcon->geometry().topLeft());
    } else if (reason == QSystemTrayIcon::Trigger) {
        q->activate(systemTrayIcon->geometry().topLeft());
    }
}

void KStatusNotifierItemPrivate::setLegacySystemTrayEnabled(bool enabled)
{
    if (enabled == (systemTrayIcon != nullptr)) {
        // already in the correct state
        return;
    }

    if (enabled) {
        bool isKde = !qEnvironmentVariableIsEmpty("KDE_FULL_SESSION")
            || qgetenv("XDG_CURRENT_DESKTOP") == "KDE"
            || qgetenv("QT_QPA_PLATFORMTHEME").toLower() == "kde";
        if (!systemTrayIcon && !isKde) {
            if (!QSystemTrayIcon::isSystemTrayAvailable()) {
                return;
            }
            systemTrayIcon = new KStatusNotifierLegacyIcon(q);
            syncLegacySystemTrayIcon();
            systemTrayIcon->setToolTip(toolTipTitle);
            systemTrayIcon->show();
            QObject::connect(systemTrayIcon, SIGNAL(wheel(int)), q, SLOT(legacyWheelEvent(int)));
            QObject::connect(systemTrayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), q, SLOT(legacyActivated(QSystemTrayIcon::ActivationReason)));
        } else if (isKde) {
            // prevent infinite recursion if the KDE platform plugin is loaded
            // but SNI is not available; see bug 350785
            qCWarning(LOG_KSTATUSNOTIFIERITEM) << "KDE platform plugin is loaded but SNI unavailable";
            return;
        }

        if (menu) {
            menu->setWindowFlags(Qt::Popup);
        }
    } else {
        delete systemTrayIcon;
        systemTrayIcon = nullptr;

        if (menu) {
            menu->setWindowFlags(Qt::Window);
        }
    }

    if (menu) {
        QMenu *m = menu;
        menu = nullptr;
        q->setContextMenu(m);
    }
}

void KStatusNotifierItemPrivate::syncLegacySystemTrayIcon()
{
    if (status == KStatusNotifierItem::NeedsAttention) {
#ifdef Q_OS_MACOS
        MacUtils::setBadgeLabelText(QString(QChar(0x26a0)) /*QStringLiteral("!")*/);
        if (attentionIconName.isNull() && attentionIcon.isNull()) {
            // code adapted from kmail's KMSystemTray::updateCount()
            int overlaySize = 22;
            QIcon attnIcon = qApp->windowIcon();
            if (!attnIcon.availableSizes().isEmpty()) {
                overlaySize = attnIcon.availableSizes().at(0).width();
            }
            QFont labelFont = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
            labelFont.setBold(true);
            QFontMetrics qfm(labelFont);
            float attnHeight = overlaySize * 0.667;
            if (qfm.height() > attnHeight) {
                float labelSize = attnHeight;
                labelFont.setPointSizeF(labelSize);
            }
            // Paint the label in a pixmap
            QPixmap overlayPixmap(overlaySize, overlaySize);
            overlayPixmap.fill(Qt::transparent);

            QPainter p(&overlayPixmap);
            p.setFont(labelFont);
            p.setBrush(Qt::NoBrush);
            // this sort of badge/label is red on OS X
            p.setPen(QColor(224, 0, 0));
            p.setOpacity(1.0);
            // use U+2022, the Unicode bullet
            p.drawText(overlayPixmap.rect(), Qt::AlignRight | Qt::AlignTop, QString(QChar(0x2022)));
            p.end();

            QPixmap iconPixmap = attnIcon.pixmap(overlaySize, overlaySize);
            QPainter pp(&iconPixmap);
            pp.drawPixmap(0, 0, overlayPixmap);
            pp.end();
            systemTrayIcon->setIcon(iconPixmap);
        } else
#endif
        {
            if (!movieName.isNull()) {
                if (!movie) {
                    movie = new QMovie(movieName);
                }
                systemTrayIcon->setMovie(movie);
            } else if (!attentionIconName.isNull()) {
                systemTrayIcon->setIcon(QIcon::fromTheme(attentionIconName));
            } else {
                systemTrayIcon->setIcon(attentionIcon);
            }
        }
    } else {
#ifdef Q_OS_MACOS
        if (!iconName.isNull()) {
            QIcon theIcon = QIcon::fromTheme(iconName);
            systemTrayIcon->setIconWithMask(theIcon, status == KStatusNotifierItem::Passive);
        } else {
            systemTrayIcon->setIconWithMask(icon, status == KStatusNotifierItem::Passive);
        }
        MacUtils::setBadgeLabelText(QString());
#else
        if (!iconName.isNull()) {
            systemTrayIcon->setIcon(QIcon::fromTheme(iconName));
        } else {
            systemTrayIcon->setIcon(icon);
        }
#endif
    }

    systemTrayIcon->setToolTip(toolTipTitle);
}

void KStatusNotifierItemPrivate::contextMenuAboutToShow()
{
    if (!hasQuit && standardActionsEnabled) {
        // we need to add the actions to the menu afterwards so that these items
        // appear at the _END_ of the menu
        menu->addSeparator();
        if (associatedWindow) {
            QAction *action = actionCollection.value(QStringLiteral("minimizeRestore"));

            if (action) {
                menu->addAction(action);
            }
        }

        QAction *action = actionCollection.value(QStringLiteral("quit"));

        if (action) {
            menu->addAction(action);
        }

        hasQuit = true;
    }

    if (associatedWindow) {
        QAction *action = actionCollection.value(QStringLiteral("minimizeRestore"));
        if (checkVisibility(QPoint(0, 0), false)) {
            action->setText(KStatusNotifierItem::tr("&Restore", "@action:inmenu"));
            action->setIcon(QIcon::fromTheme(QStringLiteral("window-restore")));
        } else {
            action->setText(KStatusNotifierItem::tr("&Minimize", "@action:inmenu"));
            action->setIcon(QIcon::fromTheme(QStringLiteral("window-minimize")));
        }
    }
}

void KStatusNotifierItem::abortQuit()
{
    d->quitAborted = true;
}

void KStatusNotifierItemPrivate::maybeQuit()
{
    Q_EMIT q->quitRequested();

    if (quitAborted) {
        quitAborted = false;
        return;
    }

    QString caption = QGuiApplication::applicationDisplayName();
    if (caption.isEmpty()) {
        caption = QCoreApplication::applicationName();
    }

    const QString title = KStatusNotifierItem::tr("Confirm Quit From System Tray", "@title:window");
    const QString query = KStatusNotifierItem::tr("<qt>Are you sure you want to quit <b>%1</b>?</qt>").arg(caption);

    auto *dialog = new QMessageBox(QMessageBox::Question, title, query, QMessageBox::NoButton);
    dialog->setAttribute(Qt::WA_DeleteOnClose);

    auto *quitButton = dialog->addButton(KStatusNotifierItem::tr("Quit", "@action:button"), QMessageBox::AcceptRole);
    quitButton->setIcon(QIcon::fromTheme(QStringLiteral("application-exit")));
    dialog->addButton(QMessageBox::Cancel);
    QObject::connect(dialog, &QDialog::accepted, qApp, &QApplication::quit);
    dialog->show();
    dialog->windowHandle()->setTransientParent(associatedWindow);
}

void KStatusNotifierItemPrivate::minimizeRestore()
{
    q->activate(systemTrayIcon ? systemTrayIcon->geometry().topLeft() : QPoint(0, 0));
}

void KStatusNotifierItemPrivate::hideMenu()
{
    menu->hide();
}

void KStatusNotifierItemPrivate::minimizeRestore(bool show)
{
#if HAVE_X11
    if (KWindowSystem::isPlatformX11()) {
        KWindowInfo info(associatedWindow->winId(), NET::WMDesktop);

        if (show) {
            if (onAllDesktops) {
                KX11Extras::setOnAllDesktops(associatedWindow->winId(), true);
            } else {
                KX11Extras::setCurrentDesktop(info.desktop());
            }
        } else {
            onAllDesktops = info.onAllDesktops();
        }
    }
#endif

    if (show) {
        Qt::WindowState state = (Qt::WindowState)(associatedWindow->windowState() & ~Qt::WindowMinimized);
        associatedWindow->setWindowState(state);
        associatedWindow->show();
        associatedWindow->raise();
        KWindowSystem::activateWindow(associatedWindow);
    } else {
        associatedWindow->hide();
    }
}

#if HAVE_DBUS
KDbusImageStruct KStatusNotifierItemPrivate::imageToStruct(const QImage &image)
{
    KDbusImageStruct icon;
    icon.width = image.size().width();
    icon.height = image.size().height();
    if (image.format() == QImage::Format_ARGB32) {
        icon.data = QByteArray((char *)image.bits(), image.sizeInBytes());
    } else {
        QImage image32 = image.convertToFormat(QImage::Format_ARGB32);
        icon.data = QByteArray((char *)image32.bits(), image32.sizeInBytes());
    }

    // swap to network byte order if we are little endian
    if (QSysInfo::ByteOrder == QSysInfo::LittleEndian) {
        quint32 *uintBuf = (quint32 *)icon.data.data();
        for (uint i = 0; i < icon.data.size() / sizeof(quint32); ++i) {
            *uintBuf = qToBigEndian(*uintBuf);
            ++uintBuf;
        }
    }

    return icon;
}

KDbusImageVector KStatusNotifierItemPrivate::iconToVector(const QIcon &icon)
{
    KDbusImageVector iconVector;

    QPixmap iconPixmap;

    // if an icon exactly that size wasn't found don't add it to the vector
    auto lstSizes = icon.availableSizes();
    if (lstSizes.isEmpty() && !icon.isNull()) {
        // if the icon is a svg icon, then available Sizes will be empty, try some common sizes
        lstSizes = {{16, 16}, {22, 22}, {32, 32}};
    }
    for (QSize size : lstSizes) {
        iconPixmap = icon.pixmap(size);
        if (!iconPixmap.isNull()) {
            iconVector.append(imageToStruct(iconPixmap.toImage()));
        }
    }
    return iconVector;
}
#endif

#include "moc_kstatusnotifieritem.cpp"
#include "moc_kstatusnotifieritemprivate_p.cpp"
