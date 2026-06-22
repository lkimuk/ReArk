import QtQuick
import QtQuick.Controls
import QtQuick.Effects

AbstractButton {
    id: root

    property int diameter: 38
    property int iconSize: 17
    property string iconName: ""
    property string toolTipText: ""
    property color backgroundColor: "#3f8fd2"
    property color hoverColor: "#52a0df"
    property color pressedColor: "#3379b6"
    property color disabledColor: "#b9c4d8"
    property color iconColor: "#ffffff"
    property color borderColor: "transparent"
    property color focusBorderColor: "#9db5ff"
    property real iconStrokeWidth: 2.2
    property bool shadowEnabled: false

    implicitWidth: diameter
    implicitHeight: diameter
    padding: 0
    hoverEnabled: true
    focusPolicy: Qt.TabFocus

    Accessible.name: toolTipText
    scale: down ? 0.96 : 1.0

    Behavior on scale {
        NumberAnimation { duration: 90; easing.type: Easing.OutCubic }
    }

    background: Rectangle {
        radius: width / 2
        color: !root.enabled
            ? root.disabledColor
            : root.down ? root.pressedColor
            : root.hovered ? root.hoverColor : root.backgroundColor
        border.width: root.visualFocus ? 2 : (root.borderColor === "transparent" ? 0 : 1)
        border.color: root.visualFocus ? root.focusBorderColor : root.borderColor
        layer.enabled: root.shadowEnabled
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowBlur: 0.42
            shadowOpacity: 0.14
            shadowVerticalOffset: 2
        }
    }

    contentItem: Item {
        Icon {
            anchors.centerIn: parent
            name: root.iconName
            color: root.iconColor
            width: root.iconSize
            height: root.iconSize
            visible: root.iconName !== "stop" && root.iconName !== "arrow-up"
        }

        Canvas {
            id: arrowCanvas

            anchors.centerIn: parent
            width: root.iconSize
            height: root.iconSize
            visible: root.iconName === "arrow-up"
            opacity: root.enabled ? 1.0 : 0.82
            antialiasing: true

            onPaint: {
                const ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)
                ctx.lineWidth = root.iconStrokeWidth
                ctx.lineCap = "round"
                ctx.lineJoin = "round"
                ctx.strokeStyle = root.iconColor

                const cx = width / 2
                const top = height * 0.27
                const bottom = height * 0.75
                const wing = height * 0.23

                ctx.beginPath()
                ctx.moveTo(cx, bottom)
                ctx.lineTo(cx, top)
                ctx.moveTo(cx, top)
                ctx.lineTo(cx - wing, top + wing)
                ctx.moveTo(cx, top)
                ctx.lineTo(cx + wing, top + wing)
                ctx.stroke()
            }

            Connections {
                target: root
                function onIconColorChanged() { arrowCanvas.requestPaint() }
                function onIconStrokeWidthChanged() { arrowCanvas.requestPaint() }
                function onIconSizeChanged() { arrowCanvas.requestPaint() }
            }
        }

        Rectangle {
            anchors.centerIn: parent
            width: Math.max(8, Math.round(root.iconSize * 0.58))
            height: width
            radius: Math.max(1, Math.round(width * 0.14))
            color: root.iconColor
            visible: root.iconName === "stop"
        }
    }

    ToolTip.text: toolTipText
    ToolTip.visible: hovered && toolTipText.length > 0
    ToolTip.delay: 500
}
