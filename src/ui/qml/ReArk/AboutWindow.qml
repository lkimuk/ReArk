import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

ApplicationWindow {
    id: aboutWindow
    width: 500
    height: 380
    minimumWidth: 450
    minimumHeight: 340
    visible: false
    title: qsTr("About ReArk")
    modality: Qt.ApplicationModal
    flags: Qt.WindowCloseButtonHint | Qt.CustomizeWindowHint | Qt.Dialog | Qt.WindowTitleHint

    property string currentTheme: "dark"
    readonly property bool darkTheme: currentTheme === "system"
                                      ? Qt.styleHints.colorScheme === Qt.Dark
                                      : currentTheme === "dark"
    readonly property color backgroundColor: darkTheme ? "#15171d" : "#ffffff"
    readonly property color dividerColor: darkTheme ? "#3a404a" : "#d5dcdf"
    readonly property color secondaryTextColor: darkTheme ? "#aab2bd" : "#5f6872"

    color: backgroundColor
    Material.theme: darkTheme ? Material.Dark : Material.Light
    Material.accent: Material.Teal

    Rectangle {
        anchors.fill: parent
        color: aboutWindow.backgroundColor

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 30
            spacing: 20

            RowLayout {
                Layout.fillWidth: true
                spacing: 16

                Image {
                    Layout.preferredWidth: 64
                    Layout.preferredHeight: 64
                    source: "qrc:/images/app_icon.png"
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                    mipmap: true
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    Text {
                        Layout.fillWidth: true
                        text: qsTr("ReArk")
                        font.pointSize: 24
                        font.bold: true
                        color: Material.foreground
                    }

                    Text {
                        Layout.fillWidth: true
                        text: qsTr("HarmonyOS Ark Decompiler GUI")
                        font.pointSize: 12
                        color: aboutWindow.secondaryTextColor
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: aboutWindow.dividerColor
            }

            GridLayout {
                Layout.fillWidth: true
                columns: 2
                columnSpacing: 20
                rowSpacing: 10

                Text {
                    text: qsTr("Version")
                    font.pointSize: 10
                    color: aboutWindow.secondaryTextColor
                }

                Text {
                    text: Qt.application.version
                    font.pointSize: 10
                    font.bold: true
                    color: Material.foreground
                }

                Text {
                    text: qsTr("Build Date")
                    font.pointSize: 10
                    color: aboutWindow.secondaryTextColor
                }

                Text {
                    text: Qt.formatDateTime(new Date(), "yyyy-MM-dd")
                    font.pointSize: 10
                    color: Material.foreground
                }
            }

            Text {
                Layout.fillWidth: true
                text: qsTr("A lightweight desktop shell for browsing HarmonyOS package decompilation output. Hyle integration will provide the file tree and source content.")
                font.pointSize: 10
                wrapMode: Text.WordWrap
                lineHeight: 1.35
                color: Material.foreground
            }

            Item {
                Layout.fillHeight: true
            }

            Text {
                Layout.fillWidth: true
                text: qsTr("Copyright 2026 ReArk. Licensed under Apache-2.0.")
                font.pointSize: 9
                color: aboutWindow.secondaryTextColor
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }
}
