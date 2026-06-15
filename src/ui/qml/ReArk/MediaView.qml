import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtMultimedia

Rectangle {
    id: root

    property string sourceUrl: ""
    property string fileName: ""
    readonly property bool darkTheme: Material.theme === Material.Dark
    readonly property color backgroundColor: darkTheme ? "#171819" : "#f5f7f8"
    readonly property color panelColor: darkTheme ? "#1b1d20" : "#ffffff"
    readonly property color dividerColor: darkTheme ? "#34383d" : "#cfd8de"
    readonly property color secondaryTextColor: darkTheme ? "#a6a6a6" : "#5f6872"
    readonly property color mediaAccentColor: darkTheme ? "#3f8fd2" : "#2f80c1"
    readonly property color mediaIconPanelColor: darkTheme ? "#202226" : "#e9f3f1"
    readonly property bool audioFile: isAudioFile(fileName.length > 0 ? fileName : sourceUrl)

    color: backgroundColor

    function isAudioFile(path) {
        var value = path.toLowerCase()
        return value.endsWith(".mp3")
            || value.endsWith(".wav")
            || value.endsWith(".ogg")
            || value.endsWith(".m4a")
            || value.endsWith(".aac")
            || value.endsWith(".flac")
            || value.endsWith(".amr")
            || value.endsWith(".mid")
            || value.endsWith(".midi")
    }

    function formatDuration(value) {
        var totalSeconds = Math.floor(Math.max(0, value) / 1000)
        var seconds = totalSeconds % 60
        var minutes = Math.floor(totalSeconds / 60) % 60
        var hours = Math.floor(totalSeconds / 3600)
        var mmss = minutes.toString().padStart(2, "0") + ":" + seconds.toString().padStart(2, "0")
        return hours > 0 ? hours + ":" + mmss : mmss
    }

    MediaPlayer {
        id: player
        source: root.visible ? root.sourceUrl : ""
        videoOutput: videoOutput
        audioOutput: AudioOutput {
            muted: muteButton.checked
            volume: volumeSlider.value
        }

        onErrorOccurred: errorLabel.text = player.errorString
    }

    onSourceUrlChanged: {
        errorLabel.text = ""
        if (visible && sourceUrl.length > 0) {
            player.play()
        }
    }

    onVisibleChanged: {
        errorLabel.text = ""
        if (visible && sourceUrl.length > 0) {
            player.play()
        } else {
            player.stop()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: root.backgroundColor

            VideoOutput {
                id: videoOutput
                anchors.fill: parent
                anchors.margins: 16
                visible: !root.audioFile
                fillMode: VideoOutput.PreserveAspectFit
            }

            Column {
                anchors.centerIn: parent
                visible: root.audioFile && errorLabel.text.length === 0 && root.sourceUrl.length > 0
                spacing: 16

                Rectangle {
                    width: 104
                    height: 104
                    radius: width / 2
                    anchors.horizontalCenter: parent.horizontalCenter
                    color: root.mediaIconPanelColor
                    border.width: 1
                    border.color: root.darkTheme ? "#34383d" : "#d0dfdc"

                    Canvas {
                        id: audioIconCanvas

                        anchors.centerIn: parent
                        width: 48
                        height: 48
                        antialiasing: true

                        Connections {
                            target: root
                            function onMediaAccentColorChanged() {
                                audioIconCanvas.requestPaint()
                            }
                        }

                        onPaint: {
                            var ctx = getContext("2d")
                            ctx.reset()
                            ctx.clearRect(0, 0, width, height)
                            ctx.strokeStyle = root.mediaAccentColor
                            ctx.fillStyle = root.mediaAccentColor
                            ctx.lineWidth = 4
                            ctx.lineCap = "round"
                            ctx.lineJoin = "round"

                            ctx.beginPath()
                            ctx.moveTo(30, 9)
                            ctx.lineTo(30, 31)
                            ctx.stroke()

                            ctx.beginPath()
                            ctx.moveTo(30, 9)
                            ctx.lineTo(18, 13)
                            ctx.lineTo(18, 36)
                            ctx.stroke()

                            ctx.beginPath()
                            ctx.ellipse(14, 37, 8, 6, -0.35, 0, Math.PI * 2)
                            ctx.fill()

                            ctx.beginPath()
                            ctx.ellipse(27, 32, 8, 6, -0.35, 0, Math.PI * 2)
                            ctx.fill()
                        }
                    }
                }

                Label {
                    width: Math.min(360, root.width - 80)
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: root.fileName.length > 0 ? root.fileName : qsTr("Audio Preview")
                    color: Material.foreground
                    font.pixelSize: 14
                    font.weight: Font.DemiBold
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideMiddle
                }

                Label {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: qsTr("Audio file")
                    color: root.secondaryTextColor
                    font.pixelSize: 12
                }
            }

            Label {
                id: errorLabel
                anchors.centerIn: parent
                width: Math.min(480, parent.width - 48)
                visible: text.length > 0
                color: root.secondaryTextColor
                font.pixelSize: 13
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
            }

            Label {
                anchors.centerIn: parent
                width: Math.min(480, parent.width - 48)
                visible: root.sourceUrl.length === 0
                text: qsTr("No media selected")
                color: root.secondaryTextColor
                font.pixelSize: 13
                horizontalAlignment: Text.AlignHCenter
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: root.dividerColor
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            color: root.panelColor

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 10

                MediaIconButton {
                    iconName: player.playbackState === MediaPlayer.PlayingState ? "pause" : "play"
                    enabled: root.sourceUrl.length > 0
                    ToolTip.text: player.playbackState === MediaPlayer.PlayingState ? qsTr("Pause") : qsTr("Play")
                    ToolTip.visible: hovered
                    onClicked: {
                        if (player.playbackState === MediaPlayer.PlayingState) {
                            player.pause()
                        } else {
                            player.play()
                        }
                    }
                }

                Slider {
                    Layout.fillWidth: true
                    enabled: player.seekable && player.duration > 0
                    from: 0
                    to: Math.max(1, player.duration)
                    value: player.position
                    onMoved: player.setPosition(value)
                }

                Label {
                    text: player.duration > 0
                          ? root.formatDuration(player.position) + " / " + root.formatDuration(player.duration)
                          : "00:00 / 00:00"
                    color: root.secondaryTextColor
                    font.family: "Consolas"
                    font.pixelSize: 12
                }

                MediaIconButton {
                    id: muteButton
                    checkable: true
                    iconName: checked ? "muted" : "sound"
                    ToolTip.text: checked ? qsTr("Muted") : qsTr("Sound")
                    ToolTip.visible: hovered
                }

                Slider {
                    id: volumeSlider
                    Layout.preferredWidth: 96
                    from: 0
                    to: 1
                    value: 0.8
                }
            }
        }
    }
}
