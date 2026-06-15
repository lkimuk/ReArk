import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

MenuSeparator {
    id: root

    readonly property bool darkTheme: Material.theme === Material.Dark

    height: 8
    padding: 0

    contentItem: Item {
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            height: 1
            color: root.darkTheme ? "#34383d" : "#d7dde2"
        }
    }
}
