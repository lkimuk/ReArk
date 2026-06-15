import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Rectangle {
    Layout.fillWidth: true
    Layout.preferredHeight: 92
    radius: 6
    color: Material.theme === Material.Dark ? "#202226" : "#eef3f4"
    border.color: Material.theme === Material.Dark ? "#34383d" : "#d5dcdf"

    property string title: ""
    property string value: ""

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 8

        Label {
            text: title
            color: Material.theme === Material.Dark ? "#a6a6a6" : "#5f6872"
            font.pixelSize: 12
            Layout.fillWidth: true
            elide: Text.ElideRight
        }

        Label {
            text: value
            color: Material.foreground
            font.pixelSize: 15
            font.weight: Font.DemiBold
            Layout.fillWidth: true
            elide: Text.ElideRight
        }
    }
}
