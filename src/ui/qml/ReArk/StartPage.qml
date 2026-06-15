import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Rectangle {
    id: root

    readonly property bool darkTheme: Material.theme === Material.Dark
    readonly property color pageColor: darkTheme ? "#1e1e1e" : "#f5f7f8"
    readonly property color panelColor: darkTheme ? "#1b1d20" : "#ffffff"
    readonly property color elevatedColor: darkTheme ? "#202226" : "#ffffff"
    readonly property color primaryTextColor: darkTheme ? "#e7e7e7" : "#17202a"
    readonly property color secondaryTextColor: darkTheme ? "#b8bec5" : "#46515d"
    readonly property color subtleTextColor: darkTheme ? "#9299a1" : "#6f7a86"
    readonly property color mutedTextColor: darkTheme ? "#747b84" : "#89939e"
    readonly property color dividerColor: darkTheme ? "#34383d" : "#dde5eb"
    readonly property color borderColor: darkTheme ? "#34383d" : "#d7e0e6"
    readonly property color hoverColor: darkTheme ? "#282b30" : "#eef5f6"
    readonly property color accentColor: darkTheme ? "#3f8fd2" : "#2f80c1"
    readonly property color accentHoverColor: darkTheme ? "#52a0df" : "#2b72ad"
    readonly property color accentPressedColor: darkTheme ? "#3379b6" : "#256699"
    readonly property color dropOverlayColor: darkTheme ? "#24272c" : "#e5f3f2"
    readonly property int recentSectionMaxWidth: 760
    property bool busy: false
    property string status: ""

    signal openRequested()
    signal fileDropped(url fileUrl)
    signal recentFileRequested(string filePath)

    function recentFileKind(filePath, exists) {
        if (!exists) {
            return "N/A"
        }

        const lowerPath = filePath.toLowerCase()
        if (lowerPath.endsWith(".abc")) {
            return "ABC"
        }
        if (lowerPath.endsWith(".app")) {
            return "APP"
        }
        if (lowerPath.endsWith(".hap")) {
            return "HAP"
        }
        return "FILE"
    }

    color: pageColor

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: root.darkTheme ? "#202123" : "#f8fbfc" }
            GradientStop { position: 0.48; color: root.pageColor }
            GradientStop { position: 1.0; color: root.darkTheme ? "#171819" : "#eef3f5" }
        }
    }

    DropArea {
        id: dropArea

        anchors.fill: parent

        onDropped: function(drop) {
            if (drop.hasUrls && drop.urls.length > 0) {
                root.fileDropped(drop.urls[0])
                drop.accept()
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        visible: dropArea.containsDrag
        color: dropOverlayColor
        opacity: 0.94
        z: 10

        Label {
            anchors.centerIn: parent
            text: qsTr("Release to open")
            color: root.primaryTextColor
            font.pixelSize: 18
            font.weight: Font.DemiBold
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 38
        anchors.rightMargin: 38
        anchors.topMargin: 56
        anchors.bottomMargin: 34
        spacing: 0

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(292, Math.max(250, root.height * 0.43))

            ColumnLayout {
                width: Math.min(parent.width, 560)
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                anchors.topMargin: 0
                spacing: 18

                RowLayout {
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 16

                    Image {
                        Layout.preferredWidth: 84
                        Layout.preferredHeight: 84
                        source: "qrc:/images/app_icon.png"
                        sourceSize.width: 168
                        sourceSize.height: 168
                        fillMode: Image.PreserveAspectFit
                        smooth: true
                        mipmap: true
                    }

                    Label {
                        text: qsTr("ReArk")
                        color: root.primaryTextColor
                        font.pixelSize: 54
                        font.weight: Font.DemiBold
                    }
                }

                Label {
                    Layout.fillWidth: true
                    text: qsTr("HarmonyOS NEXT app reverse engineering and intelligent analysis")
                    color: root.secondaryTextColor
                    font.pixelSize: 16
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideRight
                }

                Button {
                    id: openButton

                    Layout.alignment: Qt.AlignHCenter
                    Layout.topMargin: 12
                    Layout.preferredWidth: 242
                    Layout.preferredHeight: 40
                    padding: 0
                    leftInset: 0
                    rightInset: 0
                    topInset: 0
                    bottomInset: 0
                    enabled: !root.busy
                    hoverEnabled: true
                    onClicked: root.openRequested()

                    background: Rectangle {
                        radius: 6
                        border.width: 1
                        border.color: openButton.down ? root.accentPressedColor : root.accentHoverColor
                        color: openButton.down
                               ? root.accentPressedColor
                               : (openButton.hovered ? root.accentHoverColor : root.accentColor)
                    }

                    contentItem: Item {
                        Row {
                            anchors.centerIn: parent
                            height: 22
                            spacing: 12

                            Image {
                                width: 20
                                height: 20
                                anchors.verticalCenter: parent.verticalCenter
                                source: "qrc:/icons/folder-open-white.svg"
                                sourceSize.width: 40
                                sourceSize.height: 40
                                fillMode: Image.PreserveAspectFit
                                smooth: true
                                mipmap: true
                            }

                            Text {
                                height: parent.height
                                text: qsTr("Open File")
                                color: "#ffffff"
                                font.pixelSize: 16
                                font.weight: Font.Medium
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: 304
                    Layout.preferredHeight: 50
                    Layout.topMargin: -1
                    radius: 8
                    color: "transparent"

                    Canvas {
                        anchors.fill: parent
                        opacity: 0.85

                        onPaint: {
                            const ctx = getContext("2d")
                            const radius = 8
                            const left = 0.5
                            const top = 0.5
                            const right = width - 0.5
                            const bottom = height - 0.5
                            ctx.clearRect(0, 0, width, height)
                            ctx.strokeStyle = root.borderColor
                            ctx.lineWidth = 1
                            ctx.setLineDash([4, 4])
                            ctx.beginPath()
                            ctx.moveTo(left + radius, top)
                            ctx.lineTo(right - radius, top)
                            ctx.quadraticCurveTo(right, top, right, top + radius)
                            ctx.lineTo(right, bottom - radius)
                            ctx.quadraticCurveTo(right, bottom, right - radius, bottom)
                            ctx.lineTo(left + radius, bottom)
                            ctx.quadraticCurveTo(left, bottom, left, bottom - radius)
                            ctx.lineTo(left, top + radius)
                            ctx.quadraticCurveTo(left, top, left + radius, top)
                            ctx.closePath()
                            ctx.stroke()
                        }

                        Connections {
                            target: root
                            function onBorderColorChanged() { parent.requestPaint() }
                        }
                    }

                    RowLayout {
                        anchors.centerIn: parent
                        spacing: 10

                        Item {
                            Layout.preferredWidth: 18
                            Layout.preferredHeight: 18
                            Layout.alignment: Qt.AlignVCenter

                            Image {
                                anchors.fill: parent
                                source: "qrc:/icons/upload-muted.svg"
                                sourceSize.width: 36
                                sourceSize.height: 36
                                fillMode: Image.PreserveAspectFit
                                smooth: true
                                mipmap: true
                            }
                        }

                        Label {
                            text: qsTr("or drop .hap / .abc files here")
                            color: root.subtleTextColor
                            font.pixelSize: 13
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.maximumWidth: root.recentSectionMaxWidth
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredHeight: 1
            color: root.dividerColor
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.maximumWidth: root.recentSectionMaxWidth
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 22
            spacing: 13

            RowLayout {
                Layout.fillWidth: true

                Label {
                    Layout.fillWidth: true
                    text: qsTr("Recent Files")
                    color: root.primaryTextColor
                    font.pixelSize: 15
                    font.weight: Font.Medium
                }

                ToolButton {
                    id: clearRecentButton

                    visible: recentFilesModel.count > 0
                    enabled: visible
                    implicitHeight: 28
                    leftPadding: 4
                    rightPadding: 0
                    hoverEnabled: true
                    onClicked: recentFilesModel.clear()

                    background: Rectangle {
                        radius: 4
                        color: clearRecentButton.hovered ? root.hoverColor : "transparent"
                    }

                    contentItem: RowLayout {
                        spacing: 5

                        Item {
                            Layout.preferredWidth: 16
                            Layout.preferredHeight: 16
                            Layout.alignment: Qt.AlignVCenter

                            Image {
                                anchors.fill: parent
                                source: "qrc:/icons/trash-muted.svg"
                                sourceSize.width: 32
                                sourceSize.height: 32
                                fillMode: Image.PreserveAspectFit
                                smooth: true
                                mipmap: true
                            }
                        }

                        Label {
                            text: qsTr("Clear History")
                            color: root.subtleTextColor
                            font.pixelSize: 12
                        }
                    }
                }
            }

            ListView {
                id: recentList

                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredHeight: Math.min(contentHeight, 260)
                Layout.maximumHeight: Math.max(contentHeight, 80)
                clip: true
                interactive: contentHeight > height
                boundsBehavior: Flickable.StopAtBounds
                model: recentFilesModel
                spacing: 6
                visible: recentFilesModel.count > 0

                delegate: ItemDelegate {
                    id: recentDelegate

                    required property string name
                    required property string path
                    required property bool exists
                    required property string iconUrl
                    readonly property string fileKind: root.recentFileKind(path, exists)

                    width: recentList.width
                    height: 52
                    padding: 0
                    hoverEnabled: true
                    ToolTip.text: recentDelegate.path
                    ToolTip.visible: hovered && recentDelegate.path.length > 0
                    ToolTip.delay: 450
                    onClicked: root.recentFileRequested(path)

                    background: Rectangle {
                        radius: 4
                        color: recentDelegate.hovered ? root.hoverColor : "transparent"
                        border.width: 1
                        border.color: recentDelegate.hovered
                                      ? (root.darkTheme ? "#424952" : "#cbdde1")
                                      : (root.darkTheme ? "#2c3035" : "#e7edf1")
                    }

                    contentItem: RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12
                        spacing: 11

                        PackageFileIcon {
                            Layout.preferredWidth: 32
                            Layout.preferredHeight: 32
                            Layout.alignment: Qt.AlignVCenter
                            iconUrl: recentDelegate.iconUrl
                            fileKind: recentDelegate.fileKind
                            exists: recentDelegate.exists
                            darkTheme: root.darkTheme
                            elevatedColor: root.elevatedColor
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4

                            Label {
                                Layout.fillWidth: true
                                text: recentDelegate.exists
                                      ? recentDelegate.name
                                      : qsTr("%1 (missing)").arg(recentDelegate.name)
                                color: recentDelegate.exists ? root.primaryTextColor : root.mutedTextColor
                                font.pixelSize: 14
                                font.weight: Font.Medium
                                elide: Text.ElideRight
                            }

                            Label {
                                Layout.fillWidth: true
                                text: recentDelegate.path
                                color: root.subtleTextColor
                                font.pixelSize: 11
                                elide: Text.ElideMiddle
                            }
                        }

                        Label {
                            text: qsTr("Ready")
                            visible: recentDelegate.exists
                            color: root.mutedTextColor
                            font.pixelSize: 11
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 80
                visible: recentFilesModel.count <= 0
                radius: 6
                color: root.elevatedColor
                border.width: 1
                border.color: root.borderColor

                Label {
                    anchors.centerIn: parent
                    text: qsTr("No recent files")
                    color: root.mutedTextColor
                    font.pixelSize: 13
                }
            }
        }
    }
}
