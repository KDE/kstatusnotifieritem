#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2025 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

import sys

import KStatusNotifierItem
from PySide6.QtCore import QObject, QPoint, Qt
from PySide6.QtGui import QColor, QIcon, QPixmap
from PySide6.QtWidgets import QApplication, QMenu


class StatusNotifierItemExample(QObject):

    def __init__(self, parent=None) -> None:
        super().__init__(parent)

        # Create a new SNI whose id is "org.kde.statusnotifierexample"
        self.status_notifier_item = KStatusNotifierItem.KStatusNotifierItem("org.kde.statusnotifierexample", self)
        self.status_notifier_item.setCategory(KStatusNotifierItem.KStatusNotifierItem.ItemCategory.Communications)
        icon = self.create_icon(Qt.red)
        self.status_notifier_item.setIconByPixmap(icon)

        self.status_notifier_item.setStatus(KStatusNotifierItem.KStatusNotifierItem.ItemStatus.Active)
        self.status_notifier_item.setToolTipTitle("KStatusNotifierItem Python Binding Example")
        self.status_notifier_item.setTitle("This is a title")
        self.status_notifier_item.setToolTipSubTitle("Some explanation from the beach.")

        # Add a context menu
        menu = QMenu()
        icon = self.create_icon(QColor(0, 255, 0))  # green
        action = menu.addAction(icon, "NeedsAttention")
        action.triggered.connect(self.action_triggered)
        icon = self.create_icon(QColor(255, 85, 255))  # purple
        action = menu.addAction(icon, "Active")
        action.triggered.connect(self.action_triggered)

        sub_menu = QMenu("Sub Menu")
        sub_menu.setIcon(self.create_icon(QColor(85, 0, 255)))
        icon = self.create_icon(QColor(255, 255, 0))  # yellow
        sub_action = sub_menu.addAction(icon, "Passive")
        sub_action.triggered.connect(self.action_triggered)
        menu.addMenu(sub_menu)

        # The menu is owned by C++ after passing it to setContextMenu
        self.status_notifier_item.setContextMenu(menu)

        # Connect to signals
        self.status_notifier_item.activateRequested.connect(self.activate_requested)
        self.status_notifier_item.secondaryActivateRequested.connect(self.secondary_activate_requested)
        self.status_notifier_item.scrollRequested.connect(self.scroll_requested)

    @staticmethod
    def create_icon(color) -> QIcon:
        pixmap = QPixmap(16, 16)
        pixmap.fill(color)
        return QIcon(pixmap)

    def action_triggered(self) -> None:
        text = self.sender().text()
        print(text)

        if text == "NeedsAttention":
            self.status_notifier_item.setStatus(KStatusNotifierItem.KStatusNotifierItem.ItemStatus.NeedsAttention)
            self.status_notifier_item.setIconByPixmap(self.create_icon(Qt.blue))
        elif text == "Active":
            self.status_notifier_item.setStatus(KStatusNotifierItem.KStatusNotifierItem.ItemStatus.Active)
            self.status_notifier_item.setIconByPixmap(self.create_icon(Qt.red))
        elif text == "Passive":
            self.status_notifier_item.setStatus(KStatusNotifierItem.KStatusNotifierItem.ItemStatus.Passive)

    def activate_requested(self, active: bool, pos: QPoint) -> None:
        print("Activated", active, pos)

    def secondary_activate_requested(self, pos: QPoint) -> None:
        print("SecondaryActivated", pos)

    def scroll_requested(self, delta: int, orientation: Qt.Orientation) -> None:
        direction = "Horizontally" if orientation == Qt.Horizontal else "Vertically"
        print(f"Scrolled by {delta} {direction}")


if __name__ == "__main__":
    app = QApplication(sys.argv)
    sni = StatusNotifierItemExample()
    sys.exit(app.exec())
