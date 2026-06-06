import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Window

Rectangle {
    id: root

    property Window targetWindow
    property string currentTheme: "dark"
    property string currentHighlightTheme: "GitHub Dark"
    property bool maximized: false
    readonly property bool darkTheme: Material.theme === Material.Dark
    readonly property bool windowActive: !targetWindow || targetWindow.active

    signal openRequested()
    signal themeRequested(string theme)
    signal highlightThemeRequested(string theme)
    signal systemMenuRequested(point globalPosition)

    implicitHeight: 32
    color: darkTheme
           ? (windowActive ? "#3f3f3f" : "#363636")
           : (windowActive ? "#f3f3f3" : "#eeeeee")

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 1
        color: darkTheme ? "#4a4a4a" : "#d8d8d8"
    }

    onTargetWindowChanged: syncMaximized()

    Component.onCompleted: syncMaximized()

    Connections {
        target: root.targetWindow
        ignoreUnknownSignals: true

        function onVisibilityChanged() {
            root.syncMaximizedFromVisibility()
        }

        function onWindowStateChanged() {
            root.syncMaximizedFromVisibility()
        }
    }

    MouseArea {
        anchors.fill: parent
        anchors.rightMargin: windowButtons.width
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onDoubleClicked: root.toggleMaximized()
        onPressed: function(mouse) {
            if (!root.targetWindow) {
                return
            }
            if (mouse.button === Qt.RightButton) {
                root.systemMenuRequested(root.mapToGlobal(Qt.point(mouse.x, mouse.y)))
                return
            }
            if (mouse.button === Qt.LeftButton) {
                root.beginSystemMove(mouse.x, mouse.y)
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Image {
            id: icon

            readonly property int iconLogicalSize: 16
            readonly property int iconPhysicalSize: Math.max(
                                                       16,
                                                       Math.round(iconLogicalSize * Screen.devicePixelRatio))

            Layout.leftMargin: 12
            Layout.preferredWidth: iconLogicalSize
            Layout.preferredHeight: iconLogicalSize
            Layout.alignment: Qt.AlignVCenter
            source: "qrc:/images/app_icon.ico"
            sourceSize.width: iconPhysicalSize
            sourceSize.height: iconPhysicalSize
            fillMode: Image.PreserveAspectFit
            smooth: false
            mipmap: false
            opacity: root.windowActive ? 1.0 : 0.72

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                onClicked: root.systemMenuRequested(
                               root.mapToGlobal(Qt.point(icon.x, root.height)))
                onDoubleClicked: if (root.targetWindow) root.targetWindow.close()
            }
        }

        AppMenuBar {
            Layout.leftMargin: 8
            Layout.preferredWidth: implicitWidth
            Layout.fillHeight: true
            embedded: true
            currentTheme: root.currentTheme
            currentHighlightTheme: root.currentHighlightTheme
            onOpenRequested: root.openRequested()
            onThemeRequested: function(theme) { root.themeRequested(theme) }
            onHighlightThemeRequested: function(theme) { root.highlightThemeRequested(theme) }
        }

        Item {
            Layout.fillWidth: true
        }

        Row {
            id: windowButtons

            Layout.fillHeight: true
            Layout.preferredWidth: minimizeButton.width + maximizeButton.width + closeButton.width

            WindowTitleButton {
                id: minimizeButton
                height: parent.height
                buttonType: "minimize"
                ToolTip.text: qsTr("Minimize")
                ToolTip.visible: hovered
                onClicked: if (root.targetWindow) root.targetWindow.showMinimized()
            }

            WindowTitleButton {
                id: maximizeButton
                height: parent.height
                buttonType: root.maximized ? "restore" : "maximize"
                ToolTip.text: root.maximized ? qsTr("Restore") : qsTr("Maximize")
                ToolTip.visible: hovered
                onClicked: root.toggleMaximized()
            }

            WindowTitleButton {
                id: closeButton
                height: parent.height
                buttonType: "close"
                ToolTip.text: qsTr("Close")
                ToolTip.visible: hovered
                onClicked: if (root.targetWindow) root.targetWindow.close()
            }
        }
    }

    function toggleMaximized() {
        if (!targetWindow) {
            return
        }
        if (maximized) {
            maximized = false
            targetWindow.showNormal()
        } else {
            maximized = true
            targetWindow.showMaximized()
        }
    }

    function beginSystemMove(localX, localY) {
        if (!targetWindow) {
            return
        }

        const globalPosition = root.mapToGlobal(Qt.point(localX, localY))
        if (maximized) {
            const xRatio = Math.max(0, Math.min(1, localX / Math.max(1, root.width)))
            maximized = false
            targetWindow.showNormal()
            targetWindow.x = Math.round(globalPosition.x - targetWindow.width * xRatio)
            targetWindow.y = Math.round(globalPosition.y - Math.min(localY, 16))
        }

        targetWindow.startSystemMove()
    }

    function isWindowMaximized() {
        return targetWindow && windowChrome.isMaximized(targetWindow)
    }

    function syncMaximized() {
        maximized = isWindowMaximized()
    }

    function syncMaximizedFromVisibility() {
        if (!targetWindow) {
            maximized = false
            return
        }
        if (targetWindow.visibility === Window.Maximized) {
            maximized = true
        }
    }
}
