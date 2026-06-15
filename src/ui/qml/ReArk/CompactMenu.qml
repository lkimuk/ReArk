import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

Menu {
    id: root

    readonly property bool darkTheme: Material.theme === Material.Dark
    property int minimumItemWidth: 168

    function shortcutText(action) {
        if (!action || action.shortcut === undefined || action.shortcut === null) {
            return ""
        }

        const shortcut = action.shortcut
        if (shortcut.nativeText !== undefined && shortcut.nativeText.length > 0) {
            return shortcut.nativeText
        }
        if (shortcut.portableText !== undefined && shortcut.portableText.length > 0) {
            return shortcut.portableText
        }

        const text = String(shortcut)
        return /^\d+$/.test(text) ? "" : text
    }

    delegate: MenuItem {
        id: menuItem

        implicitWidth: root.minimumItemWidth
        implicitHeight: 28
        padding: 12
        leftPadding: 12
        rightPadding: 12
        verticalPadding: 4
        spacing: 12
        font.pixelSize: 13

        contentItem: Row {
            spacing: 24

            Label {
                width: Math.max(0, menuItem.availableWidth - (shortcutLabel.visible ? shortcutLabel.width + parent.spacing : 0))
                anchors.verticalCenter: parent.verticalCenter
                text: menuItem.text
                color: menuItem.enabled ? Material.foreground : Material.hintTextColor
                font: menuItem.font
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }

            Label {
                id: shortcutLabel

                anchors.verticalCenter: parent.verticalCenter
                text: root.shortcutText(menuItem.action)
                color: menuItem.enabled ? Material.hintTextColor : Material.hintTextColor
                font: menuItem.font
                visible: text.length > 0
                verticalAlignment: Text.AlignVCenter
            }
        }
    }

    background: Rectangle {
        implicitWidth: root.minimumItemWidth
        color: root.darkTheme ? "#202226" : "#ffffff"
        radius: 3
        border.width: 1
        border.color: root.darkTheme ? "#34383d" : "#d7dde2"
    }
}
