import QtQuick
import QtQuick.Window

MouseArea {
    id: root

    required property Window targetWindow
    required property int edges
    required property bool maximized

    acceptedButtons: Qt.LeftButton
    cursorShape: Qt.ArrowCursor
    enabled: targetWindow && !maximized
    hoverEnabled: true
    visible: enabled

    onPressed: function(mouse) {
        if (mouse.button === Qt.LeftButton && targetWindow) {
            targetWindow.startSystemResize(edges)
        }
    }
}
