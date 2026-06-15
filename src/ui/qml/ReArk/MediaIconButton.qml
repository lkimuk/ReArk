import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

ToolButton {
    id: root

    property string iconName: "play"
    readonly property bool darkTheme: Material.theme === Material.Dark
    readonly property color iconColor: enabled
                                      ? Material.foreground
                                      : (darkTheme ? "#68727d" : "#9aa4ad")
    readonly property color hoverColor: darkTheme ? "#282b30" : "#e4eaed"
    readonly property color checkedColor: darkTheme ? "#2a3038" : "#d6e8e7"

    implicitWidth: 34
    implicitHeight: 34
    display: AbstractButton.IconOnly

    background: Rectangle {
        radius: 4
        color: root.checked
               ? root.checkedColor
               : root.hovered ? root.hoverColor : "transparent"
    }

    contentItem: Canvas {
        id: iconCanvas

        anchors.fill: parent
        antialiasing: true

        Connections {
            target: root
            function onIconNameChanged() { iconCanvas.requestPaint() }
            function onIconColorChanged() { iconCanvas.requestPaint() }
            function onEnabledChanged() { iconCanvas.requestPaint() }
            function onCheckedChanged() { iconCanvas.requestPaint() }
        }

        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()
            ctx.clearRect(0, 0, width, height)
            ctx.fillStyle = root.iconColor
            ctx.strokeStyle = root.iconColor
            ctx.lineWidth = 1.8
            ctx.lineCap = "round"
            ctx.lineJoin = "round"

            var cx = width / 2
            var cy = height / 2

            if (root.iconName === "pause") {
                ctx.fillRect(cx - 7, cy - 8, 4, 16)
                ctx.fillRect(cx + 3, cy - 8, 4, 16)
                return
            }

            if (root.iconName === "sound" || root.iconName === "muted") {
                ctx.beginPath()
                ctx.moveTo(cx - 10, cy - 5)
                ctx.lineTo(cx - 5, cy - 5)
                ctx.lineTo(cx + 2, cy - 11)
                ctx.lineTo(cx + 2, cy + 11)
                ctx.lineTo(cx - 5, cy + 5)
                ctx.lineTo(cx - 10, cy + 5)
                ctx.closePath()
                ctx.fill()

                if (root.iconName === "sound") {
                    ctx.beginPath()
                    ctx.arc(cx + 5, cy, 6, -0.8, 0.8)
                    ctx.stroke()
                    ctx.beginPath()
                    ctx.arc(cx + 7, cy, 10, -0.7, 0.7)
                    ctx.stroke()
                } else {
                    ctx.beginPath()
                    ctx.moveTo(cx + 7, cy - 6)
                    ctx.lineTo(cx + 17, cy + 6)
                    ctx.moveTo(cx + 17, cy - 6)
                    ctx.lineTo(cx + 7, cy + 6)
                    ctx.stroke()
                }
                return
            }

            ctx.beginPath()
            ctx.moveTo(cx - 5, cy - 9)
            ctx.lineTo(cx - 5, cy + 9)
            ctx.lineTo(cx + 10, cy)
            ctx.closePath()
            ctx.fill()
        }
    }
}
