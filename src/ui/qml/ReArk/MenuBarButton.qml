import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Rectangle {
    id: root

    required property string text
    required property Menu menu
    property bool embedded: false
    property bool menuNavigationActive: false
    readonly property bool darkTheme: Material.theme === Material.Dark

    signal menuRequested(var source, var menu, bool toggle)

    Layout.preferredWidth: label.implicitWidth + (embedded ? 20 : 22)
    Layout.fillHeight: true
    color: mouse.containsMouse || menu.visible
           ? (darkTheme ? (embedded ? "#282b30" : "#34383d") : "#e5e5e5")
           : "transparent"

    Label {
        id: label
        anchors.centerIn: parent
        text: root.text
        color: root.darkTheme ? "#e7e7e7" : "#202020"
        font.family: "Segoe UI"
        font.pixelSize: 13
        renderType: Text.NativeRendering
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        onEntered: {
            if (root.menuNavigationActive) {
                root.menuRequested(root, root.menu, false)
            }
        }
        onClicked: root.menuRequested(root, root.menu, true)
    }
}
