import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Dialogs
import QtQuick.Layouts
import ReArk as RK

ApplicationWindow {
    id: mainWindow
    width: 1180
    height: 760
    minimumWidth: 900
    minimumHeight: 560
    visible: false
    flags: Qt.Window
           | Qt.FramelessWindowHint
           | Qt.WindowSystemMenuHint
           | Qt.WindowMinimizeButtonHint
           | Qt.WindowMaximizeButtonHint
           | Qt.WindowCloseButtonHint
    title: currentFileName.length > 0 ? "ReArk - " + currentFileName : "ReArk"

    property string currentTheme: "dark"
    property string currentHighlightTheme: "GitHub Dark"
    property url currentFileUrl: ""
    readonly property bool effectiveDarkTheme: currentTheme === "system"
                                               ? Qt.styleHints.colorScheme === Qt.Dark
                                               : currentTheme === "dark"
    readonly property color windowBackgroundColor: effectiveDarkTheme ? "#171a1f" : "#f5f7f8"
    readonly property string currentFilePath: decodeURIComponent(currentFileUrl.toString().replace(/^file:\/+/, ""))
    readonly property string currentFileName: currentFilePath.length > 0 ? currentFilePath.split(/[\\/]/).pop() : ""

    color: windowBackgroundColor
    Material.theme: effectiveDarkTheme ? Material.Dark : Material.Light
    Material.accent: Material.Teal

    onEffectiveDarkThemeChanged: syncHighlightThemeWithAppTheme()

    footer: RK.StatusBar {
        filePath: decompilerController.status
        version: appVersion
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        RK.WindowTitleBar {
            id: titleBar

            Layout.fillWidth: true
            targetWindow: mainWindow
            currentTheme: mainWindow.currentTheme
            currentHighlightTheme: mainWindow.currentHighlightTheme
            onOpenRequested: openFileDialog.open()
            onThemeRequested: function(theme) { mainWindow.applyTheme(theme) }
            onHighlightThemeRequested: function(theme) { mainWindow.currentHighlightTheme = theme }
            onSystemMenuRequested: function(globalPosition) {
                windowChrome.showSystemMenu(mainWindow, globalPosition)
            }
        }

        RK.MainWorkspace {
            Layout.fillWidth: true
            Layout.fillHeight: true
            fileName: mainWindow.currentFileName
            filePath: mainWindow.currentFilePath
            highlightTheme: mainWindow.currentHighlightTheme
            onOpenRequested: openFileDialog.open()
            onFileDropped: function(url) { mainWindow.currentFileUrl = url }
        }
    }

    RK.WindowResizeHandle {
        targetWindow: mainWindow
        edges: Qt.TopEdge | Qt.LeftEdge
        maximized: titleBar.maximized
        cursorShape: Qt.SizeFDiagCursor
        width: 8
        height: 8
        anchors.left: parent.left
        anchors.top: parent.top
    }

    RK.WindowResizeHandle {
        targetWindow: mainWindow
        edges: Qt.TopEdge | Qt.RightEdge
        maximized: titleBar.maximized
        cursorShape: Qt.SizeBDiagCursor
        width: 8
        height: 8
        anchors.right: parent.right
        anchors.top: parent.top
    }

    RK.WindowResizeHandle {
        targetWindow: mainWindow
        edges: Qt.BottomEdge | Qt.LeftEdge
        maximized: titleBar.maximized
        cursorShape: Qt.SizeBDiagCursor
        width: 8
        height: 8
        anchors.left: parent.left
        anchors.bottom: parent.bottom
    }

    RK.WindowResizeHandle {
        targetWindow: mainWindow
        edges: Qt.BottomEdge | Qt.RightEdge
        maximized: titleBar.maximized
        cursorShape: Qt.SizeFDiagCursor
        width: 8
        height: 8
        anchors.right: parent.right
        anchors.bottom: parent.bottom
    }

    RK.WindowResizeHandle {
        targetWindow: mainWindow
        edges: Qt.LeftEdge
        maximized: titleBar.maximized
        cursorShape: Qt.SizeHorCursor
        width: 5
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.topMargin: 8
        anchors.bottomMargin: 8
    }

    RK.WindowResizeHandle {
        targetWindow: mainWindow
        edges: Qt.RightEdge
        maximized: titleBar.maximized
        cursorShape: Qt.SizeHorCursor
        width: 5
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.topMargin: 8
        anchors.bottomMargin: 8
    }

    RK.WindowResizeHandle {
        targetWindow: mainWindow
        edges: Qt.TopEdge
        maximized: titleBar.maximized
        cursorShape: Qt.SizeVerCursor
        height: 5
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.leftMargin: 8
        anchors.rightMargin: 8
    }

    RK.WindowResizeHandle {
        targetWindow: mainWindow
        edges: Qt.BottomEdge
        maximized: titleBar.maximized
        cursorShape: Qt.SizeVerCursor
        height: 5
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: 8
        anchors.rightMargin: 8
    }

    FileDialog {
        id: openFileDialog
        title: qsTr("Open HarmonyOS package or Ark bytecode")
        nameFilters: [
            qsTr("HarmonyOS packages (*.hap *.app *.abc)"),
            qsTr("All files (*)")
        ]
        onAccepted: mainWindow.currentFileUrl = selectedFile
    }

    Component.onCompleted: {
        if (initialFileUrl && initialFileUrl.length > 0) {
            currentFileUrl = initialFileUrl
        }
        show()
    }

    function applyTheme(theme) {
        currentTheme = theme
        syncHighlightThemeWithAppTheme()
    }

    function syncHighlightThemeWithAppTheme() {
        const shouldFollowTheme = currentHighlightTheme === "GitHub Dark"
                || currentHighlightTheme === "GitHub Light"
        if (shouldFollowTheme) {
            currentHighlightTheme = effectiveDarkTheme ? "GitHub Dark" : "GitHub Light"
        }
    }
}
