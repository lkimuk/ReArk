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
    property color focusBorderColor: "#9db5ff"
    property bool shadowEnabled: false

    implicitWidth: diameter
    implicitHeight: diameter
    padding: 0
    hoverEnabled: true
    focusPolicy: Qt.TabFocus

    Accessible.name: toolTipText

    background: Rectangle {
        radius: width / 2
        color: !root.enabled
            ? root.disabledColor
            : root.down ? root.pressedColor
            : root.hovered ? root.hoverColor : root.backgroundColor
        border.width: root.visualFocus ? 2 : 0
        border.color: root.focusBorderColor
        layer.enabled: root.shadowEnabled
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowBlur: 0.55
            shadowOpacity: 0.16
            shadowVerticalOffset: 3
        }
    }

    contentItem: Item {
        Icon {
            anchors.centerIn: parent
            name: root.iconName
            color: root.iconColor
            width: root.iconSize
            height: root.iconSize
        }
    }

    ToolTip.text: toolTipText
    ToolTip.visible: hovered && toolTipText.length > 0
    ToolTip.delay: 500
}
