import QtQuick
import QtQuick.Controls

Item {
    id: root

    property string iconUrl: ""
    property string fileKind: "FILE"
    property bool exists: true
    property bool darkTheme: true
    property color elevatedColor: darkTheme ? "#202226" : "#ffffff"

    implicitWidth: 32
    implicitHeight: 32

    Image {
        anchors.fill: parent
        visible: root.exists && root.iconUrl.length > 0
        source: root.iconUrl
        sourceSize.width: 64
        sourceSize.height: 64
        fillMode: Image.PreserveAspectFit
        smooth: true
        mipmap: true
    }

    Rectangle {
        anchors.centerIn: parent
        width: 25
        height: 32
        visible: !root.exists || root.iconUrl.length <= 0
        radius: 2
        color: root.darkTheme ? "#1b1d20" : "#f2f6f8"
        border.width: 1
        border.color: root.darkTheme ? "#747b84" : "#607080"

        Rectangle {
            anchors.right: parent.right
            anchors.top: parent.top
            width: 8
            height: 8
            color: root.elevatedColor
            border.width: 1
            border.color: parent.border.color
        }

        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 4
            width: 20
            height: 13
            radius: 2
            color: root.exists ? "#3f8fd2" : "#5b6570"

            Label {
                anchors.centerIn: parent
                text: root.fileKind
                color: root.exists ? "#ffffff" : "#d6dce2"
                font.pixelSize: 8
                font.weight: Font.Bold
            }
        }
    }
}
