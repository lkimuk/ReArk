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
    property string activeView: "start"
    property var settingsWindow: null
    readonly property bool effectiveDarkTheme: currentTheme === "system"
                                               ? Qt.styleHints.colorScheme === Qt.Dark
                                               : currentTheme === "dark"
    readonly property color windowBackgroundColor: effectiveDarkTheme ? "#171a1f" : "#f5f7f8"
    readonly property bool hasPackage: decompilerController.hasPackage
    readonly property string currentFilePath: decompilerController.packagePath
    readonly property string currentFileName: currentFilePath.length > 0 ? currentFilePath.split(/[\\/]/).pop() : ""

    color: windowBackgroundColor
    Material.theme: effectiveDarkTheme ? Material.Dark : Material.Light
    Material.accent: Material.Teal

    onEffectiveDarkThemeChanged: syncHighlightThemeWithAppTheme()
    onHasPackageChanged: {
        if (activeView !== "smartAnalysis") {
            activeView = hasPackage ? "workspace" : "start"
        }
    }

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
            smartAnalysisActive: mainWindow.activeView === "smartAnalysis"
            onOpenRequested: openFileDialog.open()
            onRecentFileRequested: function(filePath) { mainWindow.openRecentFilePath(filePath) }
            onSettingsRequested: mainWindow.openSettingsWindow()
            onThemeRequested: function(theme) { mainWindow.applyTheme(theme) }
            onHighlightThemeRequested: function(theme) { mainWindow.currentHighlightTheme = theme }
            onSmartAnalysisRequested: mainWindow.toggleSmartAnalysisView()
            onSystemMenuRequested: function(globalPosition) {
                windowChrome.showSystemMenu(mainWindow, globalPosition)
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            RK.StartPage {
                anchors.fill: parent
                visible: mainWindow.activeView === "start" && !mainWindow.hasPackage
                busy: decompilerController.busy
                status: decompilerController.status
                onOpenRequested: openFileDialog.open()
                onFileDropped: function(fileUrl) { mainWindow.openFileUrl(fileUrl) }
                onRecentFileRequested: function(filePath) { mainWindow.openRecentFilePath(filePath) }
            }

            RK.MainWorkspace {
                anchors.fill: parent
                visible: mainWindow.activeView === "workspace" && mainWindow.hasPackage
                fileName: mainWindow.currentFileName
                filePath: mainWindow.currentFilePath
                appIconUrl: decompilerController.appIconUrl
                appIconPath: decompilerController.appIconPath
                appIconLayered: decompilerController.appIconLayered
                highlightTheme: mainWindow.currentHighlightTheme
                onOpenRequested: openFileDialog.open()
                onFileDropped: function(fileUrl) { mainWindow.openFileUrl(fileUrl) }
            }

            RK.SmartAnalysisPage {
                anchors.fill: parent
                visible: mainWindow.activeView === "smartAnalysis"
                agentController: rearkAgentController
                agentKnowledgeController: rearkAgentKnowledgeController
            }
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
        title: qsTr("Open HAP package or ABC file")
        nameFilters: [
            qsTr("HarmonyOS packages (*.hap *.app *.abc)"),
            qsTr("All files (*)")
        ]
        onAccepted: mainWindow.openFileUrl(selectedFile)
    }

    Connections {
        target: decompilerController

        function onPackageOpened(filePath, appIconDataUrl) {
            recentFilesModel.addFile(filePath, appIconDataUrl)
        }
    }

    Timer {
        id: automaticUpdateCheckTimer
        interval: 3000
        repeat: false
        onTriggered: updateController.checkForUpdatesIfDue()
    }

    Component.onCompleted: {
        if (initialFileUrl && initialFileUrl.length > 0) {
            openFileUrl(initialFileUrl)
        }
        automaticUpdateCheckTimer.start()
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

    function toggleSmartAnalysisView() {
        if (activeView === "smartAnalysis") {
            activeView = hasPackage ? "workspace" : "start"
            return
        }

        activeView = "smartAnalysis"
    }

    function openSettingsWindow() {
        if (settingsWindow !== null) {
            settingsWindow.currentTheme = mainWindow.currentTheme
            settingsWindow.settingsController = rearkSettingsController
            settingsWindow.show()
            settingsWindow.raise()
            settingsWindow.requestActivate()
            return
        }

        var factory = Qt.createComponent("qrc:/ReArk/SettingsWindow.qml")
        if (factory.status === Component.Ready) {
            var window = factory.createObject(mainWindow, {
                "currentTheme": mainWindow.currentTheme,
                "settingsController": rearkSettingsController,
                "closeCallback": function() {
                    if (mainWindow.settingsWindow === window) {
                        mainWindow.settingsWindow = null
                    }
                }
            })
            if (window === null) {
                console.error(factory.errorString())
                return
            }
            settingsWindow = window
            window.show()
            window.raise()
            window.requestActivate()
        } else {
            console.error(factory.errorString())
        }
    }

    function openFileUrl(fileUrl) {
        openFilePath(filePathFromUrl(fileUrl))
    }

    function openFilePath(filePath) {
        if (!filePath || filePath.length <= 0) {
            return
        }
        decompilerController.decompileFile(filePath)
    }

    function openRecentFilePath(filePath) {
        if (!filePath || filePath.length <= 0) {
            return
        }
        if (!recentFilesModel.fileExists(filePath)) {
            recentFilesModel.removeFile(filePath)
            decompilerController.showStatusMessage(qsTr("Recent file no longer exists: %1").arg(filePath))
            return
        }
        openFilePath(filePath)
    }

    function filePathFromUrl(fileUrl) {
        const text = fileUrl && fileUrl.toString ? fileUrl.toString() : String(fileUrl)
        if (text.indexOf("file:") === 0) {
            return decodeURIComponent(text.replace(/^file:\/+/, ""))
        }
        return text
    }
}
