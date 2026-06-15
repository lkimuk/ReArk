import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Window

ApplicationWindow {
    id: aboutWindow
    width: 580
    height: 515
    minimumWidth: 520
    minimumHeight: 490
    visible: false
    title: qsTr("About ReArk")
    modality: Qt.ApplicationModal
    flags: Qt.WindowCloseButtonHint | Qt.CustomizeWindowHint | Qt.Dialog | Qt.WindowTitleHint

    property string currentTheme: "dark"
    property var closeCallback: null
    readonly property bool darkTheme: currentTheme === "system"
                                      ? Qt.styleHints.colorScheme === Qt.Dark
                                      : currentTheme === "dark"
    readonly property color backgroundColor: darkTheme ? "#1e1e1e" : "#ffffff"
    readonly property color dividerColor: darkTheme ? "#34383d" : "#d5dcdf"
    readonly property color secondaryTextColor: darkTheme ? "#a6a6a6" : "#5f6872"
    readonly property color buttonColor: darkTheme ? "#202226" : "#ffffff"
    readonly property color buttonHoverColor: darkTheme ? "#282b30" : "#edf3f5"
    readonly property color buttonBorderColor: darkTheme ? "#34383d" : "#d8e0e5"
    readonly property color buttonHoverBorderColor: darkTheme ? "#424952" : "#cbd8de"
    readonly property string githubUrl: "https://github.com/lkimuk/ReArk"
    readonly property string websiteUrl: "https://www.cppmore.com/"
    readonly property int copyrightStartYear: 2026
    readonly property int copyrightCurrentYear: new Date().getFullYear()
    readonly property string copyrightYearRange: copyrightCurrentYear <= copyrightStartYear
                                                ? String(copyrightStartYear)
                                                : copyrightStartYear + "-" + copyrightCurrentYear

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
        color: aboutWindow.backgroundColor

        ColumnLayout {
            anchors.fill: parent
            anchors.leftMargin: 30
            anchors.rightMargin: 30
            anchors.topMargin: 30
            anchors.bottomMargin: 20
            spacing: 14

            RowLayout {
                Layout.fillWidth: true
                spacing: 16

                Image {
                    Layout.preferredWidth: 58
                    Layout.preferredHeight: 58
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
                        font.pointSize: 25
                        font.bold: true
                        color: Material.foreground
                    }

                    Text {
                        Layout.fillWidth: true
                        text: qsTr("Reverse the Ark, Reveal the App")
                        font.pointSize: 12
                        color: aboutWindow.secondaryTextColor
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                color: aboutWindow.dividerColor
            }

            GridLayout {
                Layout.fillWidth: true
                columns: 2
                columnSpacing: 22
                rowSpacing: 9

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
                    text: qsTr("Date")
                    font.pointSize: 10
                    color: aboutWindow.secondaryTextColor
                }

                Text {
                    text: buildInfo.buildTimestamp
                    font.pointSize: 10
                    color: Material.foreground
                }

                Text {
                    text: qsTr("Author")
                    font.pointSize: 10
                    color: aboutWindow.secondaryTextColor
                }

                Text {
                    text: qsTr("Miles Li")
                    font.pointSize: 10
                    color: Material.foreground
                }

                Text {
                    text: qsTr("License")
                    font.pointSize: 10
                    color: aboutWindow.secondaryTextColor
                }

                Text {
                    text: qsTr("Apache-2.0")
                    font.pointSize: 10
                    color: Material.foreground
                }
            }

            Text {
                Layout.fillWidth: true
                Layout.topMargin: 16
                text: qsTr("A professional reverse engineering tool for HarmonyOS NEXT HAP/ABC, supporting disassembly, decompilation, agentic analysis, signature identification, package browsing, and more.")
                font.pointSize: 10
                wrapMode: Text.WordWrap
                lineHeight: 1.35
                color: Material.foreground
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.topMargin: 4
                spacing: 6

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Safety and Privacy")
                    font.pointSize: 10
                    font.bold: true
                    color: Material.foreground
                }

                Text {
                    Layout.fillWidth: true
                    text: qsTr("This software is intended only for legally authorized application analysis and security research. Do not use it for unauthorized reverse engineering, protection bypass, attacks, or other unlawful purposes; and do not share content containing secrets, certificates, user data, or trade secrets with AI.")
                    font.pointSize: 10
                    wrapMode: Text.WordWrap
                    lineHeight: 1.35
                    color: aboutWindow.secondaryTextColor
                }
            }

            Item {
                Layout.fillHeight: true
                Layout.maximumHeight: 8
            }

            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 2
                Layout.bottomMargin: 2
                spacing: 12

                Repeater {
                    model: [
                        {
                            "kind": "github",
                            "label": qsTr("GitHub"),
                            "url": aboutWindow.githubUrl
                        },
                        {
                            "kind": "website",
                            "label": qsTr("Official Website"),
                            "url": aboutWindow.websiteUrl
                        }
                    ]

                    delegate: Item {
                        id: contactButton

                        required property var modelData
                        readonly property string iconKind: modelData.kind
                        readonly property string targetUrl: modelData.url
                        readonly property string iconSource: {
                            if (contactButton.iconKind === "github") {
                                return aboutWindow.darkTheme ? "qrc:/icons/github-white.svg" : "qrc:/icons/github.svg"
                            }
                            return aboutWindow.darkTheme ? "qrc:/icons/website-white.svg" : "qrc:/icons/website.svg"
                        }
                        readonly property bool hovered: contactMouse.containsMouse

                        Layout.preferredWidth: 42
                        Layout.preferredHeight: 42
                        ToolTip.text: modelData.label
                        ToolTip.visible: contactButton.hovered
                        ToolTip.delay: 400

                        Rectangle {
                            anchors.fill: parent
                            radius: width / 2
                            color: contactButton.hovered
                                   ? aboutWindow.buttonHoverColor
                                   : aboutWindow.buttonColor
                            border.width: 1
                            border.color: contactButton.hovered
                                          ? aboutWindow.buttonHoverBorderColor
                                          : aboutWindow.buttonBorderColor
                        }

                        Row {
                            anchors.centerIn: parent

                            Image {
                                width: 20
                                height: width
                                source: contactButton.iconSource
                                sourceSize.width: width * 2
                                sourceSize.height: height * 2
                                fillMode: Image.PreserveAspectFit
                                smooth: true
                                mipmap: true
                            }
                        }

                        MouseArea {
                            id: contactMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: Qt.openUrlExternally(contactButton.targetUrl)
                        }
                    }
                }
            }

            Text {
                Layout.fillWidth: true
                Layout.topMargin: 0
                text: qsTr("Copyright © %1 Miles Li. All rights reserved.").arg(aboutWindow.copyrightYearRange)
                font.pointSize: 9
                color: aboutWindow.secondaryTextColor
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }
}
