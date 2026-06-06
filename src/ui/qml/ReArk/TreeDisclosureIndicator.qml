import QtQuick
import QtQuick.Controls.Material

Item {
    id: root

    property bool expanded: false
    property bool directory: false
    property bool placeholder: false
    readonly property color strokeColor: placeholder ? Material.hintTextColor : Material.foreground
    onStrokeColorChanged: canvas.requestPaint()

    implicitWidth: 14
    implicitHeight: 16
    visible: directory

    Canvas {
        id: canvas

        anchors.centerIn: parent
        width: 10
        height: 10
        opacity: root.placeholder ? 0.75 : 1.0

        onPaint: {
            const ctx = getContext("2d")
            ctx.reset()
            ctx.strokeStyle = root.strokeColor
            ctx.lineWidth = 1.7
            ctx.lineCap = "round"
            ctx.lineJoin = "round"
            ctx.beginPath()
            if (root.expanded) {
                ctx.moveTo(1.5, 3.0)
                ctx.lineTo(5.0, 6.5)
                ctx.lineTo(8.5, 3.0)
            } else {
                ctx.moveTo(3.0, 1.5)
                ctx.lineTo(6.5, 5.0)
                ctx.lineTo(3.0, 8.5)
            }
            ctx.stroke()
        }

        Connections {
            target: root
            function onExpandedChanged() { canvas.requestPaint() }
            function onDirectoryChanged() { canvas.requestPaint() }
            function onPlaceholderChanged() { canvas.requestPaint() }
        }

    }
}
