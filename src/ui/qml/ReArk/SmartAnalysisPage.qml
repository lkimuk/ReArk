import QtQuick
import QtQuick.Controls
import QtQuick.Effects
import QtQuick.Controls.Material
import QtQuick.Layouts

Rectangle {
    id: root

    readonly property bool darkTheme: Material.theme === Material.Dark
    readonly property color pageColor: "#e8eef7"
    readonly property color panelColor: "#fbfcff"
    readonly property color primaryTextColor: "#0f172a"
    readonly property color secondaryTextColor: "#748094"
    readonly property color borderColor: "#cfd8e6"
    readonly property color iconColor: "#1f3354"
    readonly property color accentColor: "#5d83f4"
    readonly property color accentHoverColor: "#4e74e4"

    color: pageColor

    Button {
        id: newChatButton

        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: 16
        anchors.rightMargin: 24
        width: Math.max(118, newChatContent.implicitWidth + 30)
        height: 36
        padding: 0
        hoverEnabled: true

        background: Rectangle {
            radius: height / 2
            color: newChatButton.hovered ? "#ffffff" : "#f7f9fd"
            border.width: 1
            border.color: root.borderColor
        }

        contentItem: Row {
            id: newChatContent

            anchors.centerIn: parent
            spacing: 8

            Text {
                text: "\uE8F4"
                color: root.iconColor
                font.family: "Segoe MDL2 Assets"
                font.pixelSize: 12
                anchors.verticalCenter: parent.verticalCenter
                renderType: Text.NativeRendering
            }

            Text {
                text: qsTr("New Chat")
                color: root.primaryTextColor
                font.pixelSize: 13
                font.weight: Font.DemiBold
                anchors.verticalCenter: parent.verticalCenter
                renderType: Text.NativeRendering
            }
        }
    }

    ColumnLayout {
        width: Math.min(930, Math.max(660, parent.width * 0.56))
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        anchors.verticalCenterOffset: -14
        spacing: 16

        Label {
            Layout.fillWidth: true
            text: qsTr("What do you want to protect?")
            color: root.primaryTextColor
            font.pixelSize: 32
            font.weight: Font.Bold
            horizontalAlignment: Text.AlignHCenter
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 118
            radius: 8
            color: root.panelColor
            border.width: 1
            border.color: root.borderColor
            layer.enabled: true
            layer.effect: MultiEffect {
                shadowEnabled: true
                shadowBlur: 0.5
                shadowOpacity: 0.15
                shadowVerticalOffset: 4
            }

            TextEdit {
                id: promptInput

                anchors.left: parent.left
                anchors.right: sendButton.left
                anchors.top: parent.top
                anchors.bottom: toolRow.top
                anchors.leftMargin: 18
                anchors.rightMargin: 16
                anchors.topMargin: 15
                anchors.bottomMargin: 8
                wrapMode: TextEdit.Wrap
                color: root.primaryTextColor
                selectedTextColor: "#ffffff"
                selectionColor: root.accentColor
                font.pixelSize: 13
            }

            Label {
                anchors.left: promptInput.left
                anchors.top: promptInput.top
                text: qsTr("Ask anything about app protection")
                color: root.secondaryTextColor
                font.pixelSize: 13
                visible: promptInput.text.length === 0
            }

            Row {
                id: toolRow

                anchors.left: parent.left
                anchors.bottom: parent.bottom
                anchors.leftMargin: 24
                anchors.bottomMargin: 21
                spacing: 18

                Image {
                    width: 16
                    height: 16
                    anchors.verticalCenter: parent.verticalCenter
                    source: "qrc:/icons/paperclip-navy.svg"
                    sourceSize.width: 32
                    sourceSize.height: 32
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                }

                Image {
                    width: 16
                    height: 16
                    anchors.verticalCenter: parent.verticalCenter
                    source: "qrc:/icons/diamond-navy.svg"
                    sourceSize.width: 32
                    sourceSize.height: 32
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                }
            }

            Button {
                id: sendButton

                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.rightMargin: 16
                anchors.bottomMargin: 14
                width: 40
                height: width
                padding: 0
                hoverEnabled: true

                background: Rectangle {
                    radius: width / 2
                    color: sendButton.hovered ? root.accentHoverColor : root.accentColor
                }

                contentItem: Text {
                    text: "\uE74A"
                    color: "#ffffff"
                    font.family: "Segoe MDL2 Assets"
                    font.pixelSize: 16
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    renderType: Text.NativeRendering
                }
            }
        }
    }
}
