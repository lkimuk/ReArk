import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

ToolButton {
    id: root

    property string buttonType: "minimize"
    property bool active: false
    readonly property bool darkTheme: Material.theme === Material.Dark
    readonly property bool aiButton: buttonType === "ai"
    readonly property string iconGlyph: {
        if (buttonType === "ai") {
            return ""
        }
        if (buttonType === "minimize") {
            return "\uE921"
        }
        if (buttonType === "maximize") {
            return "\uE922"
        }
        if (buttonType === "restore") {
            return "\uE923"
        }
        return "\uE8BB"
    }
    readonly property color iconColor: root.hovered && root.buttonType === "close"
                                      ? "#ffffff"
                                      : (root.darkTheme ? (root.aiButton ? "#52a0df" : "#d9dde3") : (root.aiButton ? "#2f80c1" : "#202020"))
    readonly property color hoverColor: root.buttonType === "close"
                                      ? "#c42b1c"
                                      : (root.darkTheme ? "#282b30" : "#e5e5e5")

    implicitWidth: 46
    implicitHeight: 32
    display: AbstractButton.IconOnly
    padding: 0

    background: Rectangle {
        color: root.active ? (root.darkTheme ? "#243546" : "#dceeed")
                           : root.hovered ? root.hoverColor : "transparent"
    }

    contentItem: Item {
        Text {
            visible: !root.aiButton
            anchors.centerIn: parent
            text: root.iconGlyph
            color: root.iconColor
            font.family: "Segoe MDL2 Assets"
            font.pixelSize: 10
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            renderType: Text.NativeRendering
        }

        Row {
            visible: root.aiButton
            anchors.centerIn: parent
            spacing: 2

            Canvas {
                id: aiSparkleIcon

                width: 9
                height: 9
                anchors.verticalCenter: parent.verticalCenter
                onWidthChanged: requestPaint()
                onHeightChanged: requestPaint()

                onPaint: {
                    const ctx = getContext("2d")
                    ctx.clearRect(0, 0, width, height)
                    ctx.fillStyle = root.iconColor
                    ctx.beginPath()
                    ctx.moveTo(width * 0.5, 0)
                    ctx.lineTo(width * 0.62, height * 0.38)
                    ctx.lineTo(width, height * 0.5)
                    ctx.lineTo(width * 0.62, height * 0.62)
                    ctx.lineTo(width * 0.5, height)
                    ctx.lineTo(width * 0.38, height * 0.62)
                    ctx.lineTo(0, height * 0.5)
                    ctx.lineTo(width * 0.38, height * 0.38)
                    ctx.closePath()
                    ctx.fill()
                }

                Connections {
                    target: root
                    function onIconColorChanged() {
                        aiSparkleIcon.requestPaint()
                    }
                }
            }

            Text {
                text: "AI"
                color: root.iconColor
                font.family: "Segoe UI"
                font.pixelSize: 11
                font.weight: Font.DemiBold
                anchors.verticalCenter: parent.verticalCenter
                renderType: Text.NativeRendering
            }
        }
    }
}
