import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import com.reark.app

Rectangle {
    id: root

    property string currentTheme: "dark"
    property string currentHighlightTheme: "GitHub Dark"
    property bool embedded: false
    property bool menuNavigationActive: false
    property var entryPointItems: []
    readonly property bool darkTheme: Material.theme === Material.Dark
    readonly property string tutorialUrl: "https://www.cppmore.com/category/ReArk/"
    readonly property string issueUrl: "https://github.com/lkimuk/ReArk/issues"

    signal openRequested()
    signal recentFileRequested(string filePath)
    signal themeRequested(string theme)
    signal highlightThemeRequested(string theme)

    implicitWidth: menuRow.implicitWidth
    implicitHeight: embedded ? 32 : 30
    height: implicitHeight
    color: embedded ? "transparent" : (darkTheme ? "#3f3f3f" : "#f3f3f3")

    function showMenu(source, menu, toggle) {
        if (toggle && menu.visible) {
            menu.close()
            return
        }

        menuNavigationActive = true
        if (fileMenu !== menu) {
            fileMenu.close()
        }
        if (viewMenu !== menu) {
            viewMenu.close()
        }
        if (navigationMenu !== menu) {
            navigationMenu.close()
        }
        if (helpMenu !== menu) {
            helpMenu.close()
        }
        if (navigationMenu === menu) {
            entryPointItems = decompilerController.entryPointCandidates()
        }
        if (!menu.visible) {
            menu.popup(source, 0, source.height)
        }
    }

    function anyMenuVisible() {
        return fileMenu.visible || viewMenu.visible || navigationMenu.visible || helpMenu.visible
    }

    function leaveMenuNavigationWhenClosed() {
        Qt.callLater(function() {
            if (!root.anyMenuVisible()) {
                root.menuNavigationActive = false
            }
        })
    }

    RowLayout {
        id: menuRow

        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        spacing: 0

        MenuBarButton {
            id: fileButton

            text: qsTr("File")
            menu: fileMenu
            embedded: root.embedded
            menuNavigationActive: root.menuNavigationActive
            onMenuRequested: function(source, menu, toggle) { root.showMenu(source, menu, toggle) }
        }

        MenuBarButton {
            id: viewButton

            text: qsTr("View")
            menu: viewMenu
            embedded: root.embedded
            menuNavigationActive: root.menuNavigationActive
            onMenuRequested: function(source, menu, toggle) { root.showMenu(source, menu, toggle) }
        }

        MenuBarButton {
            id: navigationButton

            text: qsTr("Navigation")
            menu: navigationMenu
            embedded: root.embedded
            menuNavigationActive: root.menuNavigationActive
            onMenuRequested: function(source, menu, toggle) { root.showMenu(source, menu, toggle) }
        }

        MenuBarButton {
            id: helpButton

            text: qsTr("Help")
            menu: helpMenu
            embedded: root.embedded
            menuNavigationActive: root.menuNavigationActive
            onMenuRequested: function(source, menu, toggle) { root.showMenu(source, menu, toggle) }
        }
    }

    Shortcut {
        sequence: "Alt+F"
        onActivated: root.showMenu(fileButton, fileMenu, false)
    }

    Shortcut {
        sequence: "Alt+V"
        onActivated: root.showMenu(viewButton, viewMenu, false)
    }

    Shortcut {
        sequence: "Alt+N"
        onActivated: root.showMenu(navigationButton, navigationMenu, false)
    }

    Shortcut {
        sequence: "Alt+H"
        onActivated: root.showMenu(helpButton, helpMenu, false)
    }

    Shortcut {
        sequence: "Ctrl+P"
        onActivated: quickOpenDialog.openWithFocus()
    }

    Shortcut {
        sequence: "Ctrl+Shift+F"
        onActivated: searchDialog.openWithFocus()
    }

    CompactMenu {
        id: fileMenu
        minimumItemWidth: 200
        onClosed: root.leaveMenuNavigationWhenClosed()

        Action {
            text: qsTr("New Window")
            shortcut: "Ctrl+Shift+N"
            onTriggered: applicationController.openNewWindow()
        }

        CompactMenuSeparator {}

        Action {
            text: qsTr("Open...")
            shortcut: StandardKey.Open
            onTriggered: root.openRequested()
        }

        CompactMenu {
            id: recentFilesMenu

            title: qsTr("Open Recent")
            enabled: recentFilesModel.count > 0
            minimumItemWidth: Math.min(
                                  maximumItemWidth,
                                  Math.max(baseItemWidth,
                                           recentLongestNameMetrics.width + 24,
                                           clearRecentMetrics.width + 24))
            property int baseItemWidth: 168
            property int maximumItemWidth: 360

            TextMetrics {
                id: recentLongestNameMetrics
                font.pixelSize: 13
                text: recentFilesModel.longestDisplayName
            }

            TextMetrics {
                id: clearRecentMetrics
                font.pixelSize: 13
                text: qsTr("Clear Recent Files")
            }

            delegate: MenuItem {
                id: recentMenuItem

                implicitWidth: recentFilesMenu.minimumItemWidth
                implicitHeight: 28
                padding: 12
                leftPadding: 12
                rightPadding: 12
                verticalPadding: 4
                font.pixelSize: 13
                hoverEnabled: true
                ToolTip.text: action && action.fullPath ? action.fullPath : text
                ToolTip.visible: hovered && ToolTip.text.length > 0
                ToolTip.delay: 450

                contentItem: Label {
                    text: recentMenuItem.text
                    color: recentMenuItem.enabled ? Material.foreground : Material.hintTextColor
                    font: recentMenuItem.font
                    elide: Text.ElideRight
                    verticalAlignment: Text.AlignVCenter
                }
            }

            Instantiator {
                model: recentFilesModel

                delegate: Action {
                    required property string name
                    required property string path
                    required property bool exists
                    property string fullPath: path

                    text: exists ? name : qsTr("%1 (missing)").arg(name)
                    onTriggered: root.recentFileRequested(path)
                }

                onObjectAdded: function(index, object) {
                    recentFilesMenu.insertAction(index, object)
                }

                onObjectRemoved: function(index, object) {
                    recentFilesMenu.removeAction(object)
                }
            }

            CompactMenuSeparator {}

            Action {
                text: qsTr("Clear Recent Files")
                onTriggered: recentFilesModel.clear()
            }
        }

        CompactMenuSeparator {}

        CompactMenu {
            title: qsTr("Preferences")
            minimumItemWidth: 184

            CompactMenu {
                title: qsTr("Theme")
                minimumItemWidth: 150
                delegate: MenuItem {
                    id: themeItem

                    implicitHeight: 28
                    padding: 12
                    verticalPadding: 4
                    spacing: 12
                    font.pixelSize: 13
                    indicator: null

                    contentItem: RowLayout {
                        spacing: 12

                        Label {
                            Layout.fillWidth: true
                            text: themeItem.text
                            color: themeItem.enabled ? Material.foreground : Material.hintTextColor
                            font: themeItem.font
                            elide: Text.ElideRight
                            verticalAlignment: Text.AlignVCenter
                        }

                        Label {
                            Layout.preferredWidth: 18
                            text: themeItem.checked ? "✓" : ""
                            color: themeItem.enabled ? Material.foreground : Material.hintTextColor
                            font: themeItem.font
                            horizontalAlignment: Text.AlignRight
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                }

                Action {
                    text: qsTr("Dark")
                    checkable: true
                    checked: root.currentTheme === "dark"
                    onTriggered: root.themeRequested("dark")
                }

                Action {
                    text: qsTr("Light")
                    checkable: true
                    checked: root.currentTheme === "light"
                    onTriggered: root.themeRequested("light")
                }

                Action {
                    text: qsTr("System")
                    checkable: true
                    checked: root.currentTheme === "system"
                    onTriggered: root.themeRequested("system")
                }
            }

            CompactMenu {
                title: qsTr("Language")
                minimumItemWidth: 150
                delegate: MenuItem {
                    id: languageItem

                    implicitHeight: 28
                    padding: 12
                    verticalPadding: 4
                    spacing: 12
                    font.pixelSize: 13
                    indicator: null

                    contentItem: RowLayout {
                        spacing: 12

                        Label {
                            Layout.fillWidth: true
                            text: languageItem.text
                            color: languageItem.enabled ? Material.foreground : Material.hintTextColor
                            font: languageItem.font
                            elide: Text.ElideRight
                            verticalAlignment: Text.AlignVCenter
                        }

                        Label {
                            Layout.preferredWidth: 18
                            text: languageItem.checked ? "✓" : ""
                            color: languageItem.enabled ? Material.foreground : Material.hintTextColor
                            font: languageItem.font
                            horizontalAlignment: Text.AlignRight
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                }

                Action {
                    text: qsTr("English")
                    checkable: true
                    checked: languageController.hasUserLanguage
                             && languageController.currentLanguage === "en_US"
                    onTriggered: languageController.switchLanguage("en_US")
                }

                Action {
                    text: qsTr("Simple Chinese")
                    checkable: true
                    checked: languageController.hasUserLanguage
                             && languageController.currentLanguage === "zh_CN"
                    onTriggered: languageController.switchLanguage("zh_CN")
                }

                Action {
                    text: qsTr("System")
                    checkable: true
                    checked: !languageController.hasUserLanguage
                    onTriggered: languageController.clearLanguageOverride()
                }
            }
        }

        CompactMenuSeparator {}

        Action {
            text: qsTr("Exit")
            shortcut: StandardKey.Quit
            onTriggered: Qt.quit()
        }
    }

    CompactMenu {
        id: viewMenu
        onClosed: root.leaveMenuNavigationWhenClosed()

        SyntaxThemeProvider {
            id: syntaxThemeProvider
        }

        CompactMenu {
            id: syntaxHighlightMenu

            title: qsTr("Syntax Highlight")
            minimumItemWidth: 210
            delegate: MenuItem {
                id: syntaxThemeItem

                implicitHeight: 28
                padding: 12
                verticalPadding: 4
                spacing: 12
                font.pixelSize: 13
                indicator: null

                contentItem: RowLayout {
                    spacing: 12

                    Label {
                        Layout.fillWidth: true
                        text: syntaxThemeItem.text
                        color: syntaxThemeItem.enabled ? Material.foreground : Material.hintTextColor
                        font: syntaxThemeItem.font
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                    }

                    Label {
                        Layout.preferredWidth: 18
                        text: syntaxThemeItem.checked ? "✓" : ""
                        color: syntaxThemeItem.enabled ? Material.foreground : Material.hintTextColor
                        font: syntaxThemeItem.font
                        horizontalAlignment: Text.AlignRight
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }

            Instantiator {
                model: syntaxThemeProvider.themes

                delegate: Action {
                    text: modelData
                    checkable: true
                    checked: root.currentHighlightTheme === modelData
                    onTriggered: root.highlightThemeRequested(modelData)
                }

                onObjectAdded: function(index, object) {
                    syntaxHighlightMenu.insertAction(index, object)
                }

                onObjectRemoved: function(index, object) {
                    syntaxHighlightMenu.removeAction(object)
                }
            }
        }
    }

    CompactMenu {
        id: navigationMenu
        minimumItemWidth: 210
        onClosed: root.leaveMenuNavigationWhenClosed()

        Action {
            text: qsTr("Quick Open...")
            shortcut: "Ctrl+P"
            onTriggered: quickOpenDialog.openWithFocus()
        }

        Action {
            text: qsTr("Search...")
            shortcut: "Ctrl+Shift+F"
            onTriggered: searchDialog.openWithFocus()
        }

        CompactMenuSeparator {}

        CompactMenu {
            id: entryPointsMenu

            title: qsTr("Entry Points")
            enabled: root.entryPointItems.length > 0
            minimumItemWidth: 210

            Instantiator {
                model: root.entryPointItems

                delegate: Action {
                    text: modelData.subtitle
                    enabled: modelData.nodeIndex !== undefined
                    onTriggered: decompilerController.navigateToNode(modelData.nodeIndex)
                }

                onObjectAdded: function(index, object) {
                    entryPointsMenu.insertAction(index, object)
                }

                onObjectRemoved: function(index, object) {
                    entryPointsMenu.removeAction(object)
                }
            }
        }
    }

    CompactMenu {
        id: helpMenu
        onClosed: root.leaveMenuNavigationWhenClosed()

        Action {
            text: qsTr("User Guide")
            onTriggered: Qt.openUrlExternally(root.tutorialUrl)
        }

        Action {
            text: qsTr("Report Issue")
            onTriggered: Qt.openUrlExternally(root.issueUrl)
        }

        Action {
            text: qsTr("View License")
            enabled: false
        }

        CompactMenuSeparator {}

        Action {
            text: updateController.checking ? qsTr("Checking for Updates...") : qsTr("Check for Updates")
            enabled: !updateController.checking
            onTriggered: updateController.checkForUpdates(false)
        }

        CompactMenuSeparator {}

        Action {
            text: qsTr("About ReArk")
            onTriggered: {
                var factory = Qt.createComponent("qrc:/ReArk/AboutWindow.qml")
                if (factory.status === Component.Ready) {
                    var aboutWindow = factory.createObject(null, {
                        "currentTheme": root.currentTheme
                    })
                    aboutWindow.show()
                } else {
                    console.error(factory.errorString())
                }
            }
        }
    }

    QuickOpenDialog {
        id: quickOpenDialog
    }

    SearchDialog {
        id: searchDialog
    }

    Dialog {
        id: updateStatusDialog

        property string dialogTitle: ""
        property string dialogText: ""

        title: dialogTitle
        modal: true
        standardButtons: Dialog.Ok
        width: 360
        x: Overlay.overlay ? Math.round((Overlay.overlay.width - width) / 2) : 0
        y: Overlay.overlay ? Math.round((Overlay.overlay.height - height) / 2) : 0

        contentItem: Label {
            text: updateStatusDialog.dialogText
            color: Material.foreground
            wrapMode: Text.WordWrap
            font.pixelSize: 13
        }
    }

    Connections {
        target: updateController

        function onUpdateAvailable(version, changelog, releaseUrl, releaseDate) {
            var factory = Qt.createComponent("qrc:/ReArk/UpdateWindow.qml")
            if (factory.status === Component.Ready) {
                var updateWindow = factory.createObject(null, {
                    "currentTheme": root.currentTheme,
                    "version": version,
                    "changelog": changelog,
                    "releaseUrl": releaseUrl,
                    "releaseDate": releaseDate
                })
                updateWindow.show()
            } else {
                console.error(factory.errorString())
            }
        }

        function onNoUpdateAvailable() {
            updateStatusDialog.dialogTitle = qsTr("Software Update")
            updateStatusDialog.dialogText = qsTr("ReArk is up to date.")
            updateStatusDialog.open()
        }

        function onCheckFailed(message) {
            updateStatusDialog.dialogTitle = qsTr("Update Check Failed")
            updateStatusDialog.dialogText = message
            updateStatusDialog.open()
        }
    }
}
