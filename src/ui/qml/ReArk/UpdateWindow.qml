import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

ApplicationWindow {
    id: updateWindow

    width: 520
    height: 420
    minimumWidth: 460
    minimumHeight: 360
    visible: false
    title: qsTr("Software Update")
    modality: Qt.ApplicationModal
    flags: Qt.WindowCloseButtonHint | Qt.CustomizeWindowHint | Qt.Dialog | Qt.WindowTitleHint

    property string currentTheme: "dark"
    property string version: ""
    property string changelog: ""
    property string releaseUrl: ""
    property string releaseDate: ""
    property var closeCallback: null
    readonly property bool darkTheme: currentTheme === "system"
                                      ? Qt.styleHints.colorScheme === Qt.Dark
                                      : currentTheme === "dark"
    readonly property color backgroundColor: darkTheme ? "#1e1e1e" : "#ffffff"
    readonly property color panelColor: darkTheme ? "#202226" : "#f5f7f8"
    readonly property color dividerColor: darkTheme ? "#34383d" : "#d5dcdf"
    readonly property color secondaryTextColor: darkTheme ? "#a6a6a6" : "#5f6872"

    color: backgroundColor
    Material.theme: darkTheme ? Material.Dark : Material.Light
    Material.accent: "#3f8fd2"
    onClosing: {
        if (closeCallback) {
            closeCallback()
        }
        destroy()
    }

    Rectangle {
        anchors.fill: parent
        color: updateWindow.backgroundColor

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 26
            spacing: 16

            Text {
                Layout.fillWidth: true
                text: qsTr("Update Available")
                color: Material.foreground
                font.pointSize: 20
                font.bold: true
            }

            Text {
                Layout.fillWidth: true
                text: qsTr("ReArk %1 is available.").arg(updateWindow.version)
                color: updateWindow.secondaryTextColor
                font.pointSize: 11
                wrapMode: Text.WordWrap
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                color: updateWindow.dividerColor
            }

            GridLayout {
                Layout.fillWidth: true
                columns: 2
                columnSpacing: 18
                rowSpacing: 8

                Text {
                    text: qsTr("Current Version")
                    color: updateWindow.secondaryTextColor
                    font.pointSize: 10
                }

                Text {
                    text: Qt.application.version
                    color: Material.foreground
                    font.pointSize: 10
                }

                Text {
                    text: qsTr("Latest Version")
                    color: updateWindow.secondaryTextColor
                    font.pointSize: 10
                }

                Text {
                    text: updateWindow.version
                    color: Material.foreground
                    font.pointSize: 10
                    font.bold: true
                }

                Text {
                    text: qsTr("Release Date")
                    color: updateWindow.secondaryTextColor
                    font.pointSize: 10
                    visible: updateWindow.releaseDate.length > 0
                }

                Text {
                    text: updateWindow.formatReleaseDate(updateWindow.releaseDate)
                    color: Material.foreground
                    font.pointSize: 10
                    visible: updateWindow.releaseDate.length > 0
                }
            }

            Text {
                Layout.fillWidth: true
                text: qsTr("What's New")
                color: Material.foreground
                font.pointSize: 12
                font.bold: true
            }

            ScrollView {
                id: changelogScrollView

                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                ScrollBar.vertical.policy: ScrollBar.AsNeeded

                TextArea {
                    width: changelogScrollView.availableWidth
                    text: updateWindow.changelog.length > 0
                          ? updateWindow.changelog
                          : qsTr("No changelog information is available for this release.")
                    readOnly: true
                    selectByMouse: true
                    wrapMode: TextEdit.Wrap
                    color: Material.foreground
                    font.pointSize: 10
                    topPadding: 10
                    bottomPadding: 10
                    leftPadding: 12
                    rightPadding: 12
                    background: Rectangle {
                        color: updateWindow.panelColor
                        radius: 4
                        border.width: 1
                        border.color: updateWindow.dividerColor
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Item {
                    Layout.fillWidth: true
                }

                Button {
                    text: qsTr("Open Release Page")
                    enabled: updateWindow.releaseUrl.length > 0
                    onClicked: {
                        updateController.openReleasePage(updateWindow.releaseUrl)
                        updateWindow.close()
                    }
                }

                Button {
                    text: qsTr("Later")
                    onClicked: updateWindow.close()
                }
            }
        }
    }

    function formatReleaseDate(value) {
        const date = new Date(value)
        if (isNaN(date.getTime())) {
            return value
        }
        return Qt.formatDateTime(date, "yyyy-MM-dd")
    }
}
