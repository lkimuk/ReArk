import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Rectangle {
    id: root

    property string sourceData: ""
    readonly property bool darkTheme: Material.theme === Material.Dark
    readonly property color backgroundColor: darkTheme ? "#171819" : "#f5f7f8"
    readonly property color checkerA: darkTheme ? "#202226" : "#e4eaed"
    readonly property color checkerB: darkTheme ? "#1b1d20" : "#f8fafb"
    readonly property color borderColor: darkTheme ? "#34383d" : "#cfd8de"

    color: backgroundColor

    Flickable {
        id: flickable
        anchors.fill: parent
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        contentWidth: Math.max(width, imageFrame.width + 48)
        contentHeight: Math.max(height, imageFrame.height + 48)

        Rectangle {
            id: imageFrame
            x: Math.max(24, (flickable.width - width) / 2)
            y: Math.max(24, (flickable.height - height) / 2)
            width: Math.max(240, preview.implicitWidth)
            height: Math.max(160, preview.implicitHeight)
            color: "transparent"
            border.width: 1
            border.color: root.borderColor

            Grid {
                id: checkerGrid
                anchors.fill: parent
                columns: Math.ceil(parent.width / 16)
                rows: Math.ceil(parent.height / 16)
                clip: true

                Repeater {
                    model: checkerGrid.columns * checkerGrid.rows

                    Rectangle {
                        required property int index

                        width: 16
                        height: 16
                        color: ((Math.floor(index / checkerGrid.columns) + index) % 2) === 0
                               ? root.checkerA
                               : root.checkerB
                    }
                }
            }

            Image {
                id: preview
                anchors.centerIn: parent
                source: root.sourceData
                asynchronous: true
                fillMode: Image.PreserveAspectFit
                width: Math.min(sourceSize.width, Math.max(1, flickable.width - 96))
                height: Math.min(sourceSize.height, Math.max(1, flickable.height - 96))
            }
        }

        Label {
            anchors.centerIn: parent
            visible: preview.status === Image.Error
            text: qsTr("Image preview is unavailable")
            color: Material.foreground
            font.pixelSize: 13
        }

        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
        }

        ScrollBar.horizontal: ScrollBar {
            policy: ScrollBar.AsNeeded
        }
    }
}
