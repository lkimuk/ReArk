import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Rectangle {
    height: 30
    color: Material.theme === Material.Dark ? "#1b1d20" : Material.background

    property string filePath: ""
    property string version: ""
    readonly property color dividerColor: Material.theme === Material.Dark ? "#34383d" : "#d5dcdf"
    readonly property color secondaryTextColor: Material.theme === Material.Dark ? "#a6a6a6" : "#5f6872"

    Rectangle {
        anchors.top: parent.top
        width: parent.width
        height: 1
        color: dividerColor
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        spacing: 10

        Label {
            Layout.fillWidth: true
            text: filePath.length > 0 ? filePath : qsTr("Ready")
            color: secondaryTextColor
            elide: Text.ElideMiddle
            font.pixelSize: 12
        }

        Label {
            text: qsTr("ReArk v%1").arg(version)
            color: secondaryTextColor
            font.pixelSize: 12
        }
    }
}
