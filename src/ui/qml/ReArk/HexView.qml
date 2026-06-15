import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import com.reark.app

Rectangle {
    id: root

    property var hexModel: null
    readonly property bool darkTheme: Material.theme === Material.Dark
    readonly property color backgroundColor: "#171819"
    readonly property color alternateRowColor: "#1b1d20"
    readonly property color headerColor: "#202226"
    readonly property color dividerColor: "#34383d"
    readonly property color addressColor: "#3f8fd2"
    readonly property color byteColor: "#e7e7e7"
    readonly property color mutedByteColor: "#747b84"
    readonly property color asciiColor: "#b8bec5"
    readonly property color cellHoverColor: "#282b30"
    readonly property int rowHeight: 22
    readonly property int addressWidth: 92
    readonly property int byteWidth: 34
    readonly property int asciiWidth: 150
    readonly property int tableWidth: addressWidth + byteWidth * 16 + asciiWidth + 44

    color: backgroundColor

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            color: headerColor

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 14
                anchors.rightMargin: 14
                spacing: 16

                Label {
                    Layout.fillWidth: true
                    text: root.hexModel && root.hexModel.path
                          ? root.hexModel.path
                          : qsTr("Hex view")
                    color: Material.foreground
                    font.pixelSize: 13
                    font.bold: true
                    elide: Text.ElideMiddle
                }

                Label {
                    text: root.hexModel && root.hexModel.kind ? root.hexModel.kind : ""
                    color: addressColor
                    font.family: "Consolas"
                    font.pixelSize: 12
                }

                Label {
                    text: root.hexModel && root.hexModel.size ? qsTr("%1 bytes").arg(root.hexModel.size) : ""
                    color: asciiColor
                    font.family: "Consolas"
                    font.pixelSize: 12
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: dividerColor
        }

        Flickable {
            id: horizontalFlick
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            contentWidth: Math.max(root.tableWidth, width)
            contentHeight: height
            flickableDirection: Flickable.HorizontalFlick

            ColumnLayout {
                width: Math.max(root.tableWidth, horizontalFlick.width)
                height: horizontalFlick.height
                spacing: 0

                Rectangle {
                    id: headerRow
                    Layout.fillWidth: true
                    Layout.preferredHeight: 30
                    color: root.backgroundColor

                    Row {
                        anchors.fill: parent
                        anchors.leftMargin: 14
                        anchors.rightMargin: 14
                        spacing: 0

                        Label {
                            width: root.addressWidth
                            height: parent.height
                            text: qsTr("Address")
                            color: root.asciiColor
                            font.family: "Consolas"
                            font.pixelSize: 12
                            verticalAlignment: Text.AlignVCenter
                        }

                        Repeater {
                            model: 16

                            Label {
                                width: root.byteWidth
                                height: headerRow.height
                                text: index.toString(16).toUpperCase().padStart(2, "0")
                                color: root.asciiColor
                                font.family: "Consolas"
                                font.pixelSize: 12
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }

                        Label {
                            width: root.asciiWidth
                            height: parent.height
                            text: qsTr("ASCII")
                            color: root.asciiColor
                            font.family: "Consolas"
                            font.pixelSize: 12
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                }

                Flickable {
                    id: rowsFlick
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds
                    contentWidth: Math.max(hexViewer.documentWidth, width)
                    contentHeight: Math.max(hexViewer.documentHeight, height)

                    HexViewerItem {
                        id: hexViewer
                        x: rowsFlick.contentX
                        y: rowsFlick.contentY
                        width: rowsFlick.width
                        height: rowsFlick.height
                        model: root.hexModel
                        scrollY: rowsFlick.contentY
                    }

                    ScrollBar.vertical: ScrollBar {
                        policy: ScrollBar.AsNeeded
                    }
                }
            }

            ScrollBar.horizontal: ScrollBar {
                policy: ScrollBar.AsNeeded
            }
        }
    }
}
