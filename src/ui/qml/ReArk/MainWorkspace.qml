import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Rectangle {
    id: root

    property string fileName: ""
    property string filePath: ""
    property string highlightTheme: "GitHub Dark"
    readonly property bool hasPackage: filePath.length > 0
    readonly property bool darkTheme: Material.theme === Material.Dark
    readonly property color pageColor: darkTheme ? "#171a1f" : "#f5f7f8"
    readonly property color sidebarColor: darkTheme ? "#20242b" : "#ffffff"
    readonly property color editorColor: darkTheme ? "#111419" : "#ffffff"
    readonly property color dividerColor: darkTheme ? "#3a404a" : "#d5dcdf"
    readonly property color hoverColor: darkTheme ? "#2b313a" : "#e8eef0"
    readonly property color selectedColor: darkTheme ? "#33424a" : "#d6e8e7"
    readonly property color secondaryTextColor: darkTheme ? "#aab2bd" : "#5f6872"
    readonly property string activeKind: decompilerController.tabsModel.activeKind
    readonly property bool activeIsText: decompilerController.tabsModel.hasTabs
                                         && decompilerController.tabsModel.activeContentMode === "text"
    readonly property bool activeIsJson: activeIsText && root.isJsonKind(activeKind)
    readonly property bool activeIsResourceIndex: activeIsText
                                                  && activeKind === "RESOURCE_INDEX"
                                                  && decompilerController.tabsModel.activeHasBinary
    readonly property bool fileToolsVisible: activeIsJson || activeIsResourceIndex
    property string textViewMode: "raw"
    property string formattedJsonContent: ""

    signal openRequested()
    signal fileDropped(url fileUrl)

    color: pageColor

    onFilePathChanged: {
        if (filePath.length > 0) {
            decompilerController.decompileFile(filePath)
        } else {
            decompilerController.clear()
        }
    }

    Connections {
        target: decompilerController.tabsModel

        function onActiveTabChanged() {
            root.textViewMode = "raw"
            root.refreshFormattedJson()
        }
    }

    Connections {
        target: decompilerController

        function onSelectedContentChanged() {
            root.refreshFormattedJson()
        }
    }

    DropArea {
        id: dropArea
        anchors.fill: parent

        onDropped: function(drop) {
            if (drop.hasUrls && drop.urls.length > 0) {
                root.fileDropped(drop.urls[0])
                drop.accept()
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.preferredWidth: 320
            Layout.fillHeight: true
            color: sidebarColor

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                Label {
                    Layout.fillWidth: true
                    Layout.leftMargin: 12
                    Layout.rightMargin: 12
                    Layout.topMargin: 12
                    Layout.bottomMargin: 8
                    text: hasPackage ? root.fileName : qsTr("Drop a package to start decompiling")
                    color: hasPackage ? Material.foreground : secondaryTextColor
                    font.pixelSize: 12
                    elide: Text.ElideMiddle
                }

                BusyIndicator {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.topMargin: 18
                    running: decompilerController.busy
                    visible: decompilerController.busy
                }

                ListView {
                    id: fileTree

                    property string contextName: ""
                    property string contextPath: ""

                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds
                    model: hasPackage ? decompilerController.treeModel : 0
                    currentIndex: decompilerController.selectedIndex

                    delegate: ItemDelegate {
                        id: fileTreeDelegate

                        width: fileTree.width
                        height: 28
                        leftPadding: 8 + model.depth * 16
                        rightPadding: 10
                        text: model.name
                        highlighted: index === decompilerController.selectedIndex
                        hoverEnabled: true
                        background: Rectangle {
                            color: index === decompilerController.selectedIndex
                                   ? selectedColor
                                   : parent.hovered ? hoverColor : "transparent"
                        }
                        contentItem: RowLayout {
                            spacing: 5

                            TreeDisclosureIndicator {
                                Layout.preferredWidth: 14
                                Layout.preferredHeight: 16
                                expanded: model.expanded
                                directory: model.isDirectory
                                placeholder: model.isPlaceholder
                            }

                            FileTreeIcon {
                                Layout.preferredWidth: 16
                                Layout.preferredHeight: 16
                                name: model.name
                                kind: model.kind
                                directory: model.isDirectory
                                placeholder: model.isPlaceholder
                            }

                            Label {
                                Layout.fillWidth: true
                                text: model.name
                                color: model.isPlaceholder ? secondaryTextColor : Material.foreground
                                opacity: model.isPlaceholder ? 0.75 : 1.0
                                font.pixelSize: 12
                                font.italic: model.isPlaceholder
                                elide: Text.ElideMiddle
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                        onClicked: decompilerController.activateIndex(index)

                        MouseArea {
                            anchors.fill: parent
                            acceptedButtons: Qt.RightButton
                            onClicked: function(mouse) {
                                fileTree.currentIndex = index
                                fileTree.contextName = model.name
                                fileTree.contextPath = model.path
                                treeContextMenu.popup(fileTreeDelegate, mouse.x, mouse.y)
                            }
                        }
                    }

                    CompactMenu {
                        id: treeContextMenu
                        minimumItemWidth: 160

                        Action {
                            text: qsTr("Copy Internal Path")
                            enabled: fileTree.contextPath.length > 0
                            onTriggered: decompilerController.copyTextToClipboard(fileTree.contextPath)
                        }

                        Action {
                            text: qsTr("Copy Name")
                            enabled: fileTree.contextName.length > 0
                            onTriggered: decompilerController.copyTextToClipboard(fileTree.contextName)
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.preferredWidth: 1
            Layout.fillHeight: true
            color: dividerColor
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: decompilerController.tabsModel.hasTabs ? 36 : 0
                visible: decompilerController.tabsModel.hasTabs
                color: darkTheme ? "#171a1f" : "#eef2f4"
                clip: true

                ListView {
                    id: openTabs
                    anchors.fill: parent
                    orientation: ListView.Horizontal
                    boundsBehavior: Flickable.StopAtBounds
                    clip: true
                    model: decompilerController.tabsModel
                    currentIndex: decompilerController.tabsModel.activeIndex

                    delegate: Rectangle {
                        id: tabDelegate

                        width: Math.min(220, Math.max(136, tabTitle.implicitWidth + 52))
                        height: openTabs.height
                        color: model.active ? editorColor : (darkTheme ? "#20242b" : "#e4eaed")
                        ToolTip.text: model.path.length > 0 ? model.path : model.name
                        ToolTip.visible: tabMouse.containsMouse && !tabMenu.visible
                        ToolTip.delay: 500

                        Rectangle {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: parent.top
                            height: 2
                            color: model.active ? "#4aa3ff" : "transparent"
                        }

                        Rectangle {
                            anchors.right: parent.right
                            width: 1
                            height: parent.height
                            color: dividerColor
                        }

                        Label {
                            id: tabTitle
                            anchors.left: parent.left
                            anchors.right: closeButton.left
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.leftMargin: 12
                            anchors.rightMargin: 4
                            text: model.loading ? model.name + " ..." : model.name
                            color: model.active ? Material.foreground : secondaryTextColor
                            font.pixelSize: 12
                            elide: Text.ElideRight
                            verticalAlignment: Text.AlignVCenter
                        }

                        Rectangle {
                            id: closeButton
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            width: 28
                            height: 28
                            radius: 4
                            color: closeMouse.containsMouse
                                   ? (darkTheme ? "#3a4048" : "#ccd5da")
                                   : "transparent"
                            ToolTip.text: qsTr("Close")
                            ToolTip.visible: closeMouse.containsMouse

                            Label {
                                anchors.centerIn: parent
                                text: "×"
                                color: closeMouse.containsMouse ? Material.foreground : secondaryTextColor
                                font.pixelSize: 16
                                font.weight: Font.Normal
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }

                            MouseArea {
                                id: closeMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: decompilerController.tabsModel.closeTab(index)
                            }
                        }

                        CompactMenu {
                            id: tabMenu
                            minimumItemWidth: 190

                            Action {
                                text: qsTr("Close")
                                onTriggered: decompilerController.tabsModel.closeTab(index)
                            }

                            CompactMenuSeparator {}

                            Action {
                                text: qsTr("Close Others")
                                enabled: openTabs.count > 1
                                onTriggered: decompilerController.tabsModel.closeOtherTabs(index)
                            }

                            Action {
                                text: qsTr("Close Tabs to the Left")
                                enabled: index > 0
                                onTriggered: decompilerController.tabsModel.closeTabsToLeft(index)
                            }

                            Action {
                                text: qsTr("Close Tabs to the Right")
                                enabled: index < openTabs.count - 1
                                onTriggered: decompilerController.tabsModel.closeTabsToRight(index)
                            }

                            CompactMenuSeparator {}

                            Action {
                                text: qsTr("Close All")
                                onTriggered: decompilerController.tabsModel.clear()
                            }
                        }

                        MouseArea {
                            id: tabMouse
                            anchors.fill: parent
                            anchors.rightMargin: closeButton.width
                            acceptedButtons: Qt.LeftButton | Qt.RightButton
                            hoverEnabled: true
                            onClicked: function(mouse) {
                                if (mouse.button === Qt.LeftButton) {
                                    decompilerController.tabsModel.activeIndex = index
                                }
                            }
                            onPressed: function(mouse) {
                                if (mouse.button === Qt.RightButton) {
                                    decompilerController.tabsModel.activeIndex = index
                                    tabMenu.popup(tabDelegate, mouse.x, mouse.y)
                                }
                            }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: editorColor

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        CodeView {
                            anchors.fill: parent
                            visible: decompilerController.tabsModel.hasTabs
                                     && decompilerController.tabsModel.activeContentMode === "text"
                                     && root.textViewMode !== "binary"
                            code: root.textViewMode === "formatted"
                                  ? root.formattedJsonContent
                                  : decompilerController.selectedContent
                            highlightTheme: root.highlightTheme
                            syntax: root.activeIsJson
                                    ? "JSON"
                                    : decompilerController.tabsModel.activePath
                        }

                        HexView {
                            anchors.fill: parent
                            visible: decompilerController.tabsModel.hasTabs
                                     && (decompilerController.tabsModel.activeContentMode === "hex"
                                         || root.textViewMode === "binary")
                            hexModel: decompilerController.hexModel
                        }

                        ImageView {
                            anchors.fill: parent
                            visible: decompilerController.tabsModel.hasTabs
                                     && decompilerController.tabsModel.activeContentMode === "image"
                            sourceData: visible ? decompilerController.selectedContent : ""
                        }

                        MediaView {
                            anchors.fill: parent
                            visible: decompilerController.tabsModel.hasTabs
                                     && decompilerController.tabsModel.activeContentMode === "media"
                            sourceUrl: visible ? decompilerController.selectedContent : ""
                            fileName: decompilerController.tabsModel.activeName
                        }

                        Label {
                            anchors.centerIn: parent
                            visible: !decompilerController.tabsModel.hasTabs && !decompilerController.busy
                            text: !hasPackage
                                  ? (dropArea.containsDrag
                                     ? qsTr("Release to open package")
                                     : qsTr("Open or drop a .hap, .app, or .abc file"))
                                  : qsTr("Select a file from the tree")
                            color: secondaryTextColor
                            font.pixelSize: 15
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: root.fileToolsVisible ? 30 : 0
                        visible: root.fileToolsVisible
                        color: darkTheme ? "#171a1f" : "#eef2f4"
                        border.width: 1
                        border.color: dividerColor
                        clip: true

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            spacing: 8

                            Label {
                                text: root.activeIsJson ? qsTr("JSON") : qsTr("Resource index")
                                color: secondaryTextColor
                                font.pixelSize: 11
                                font.weight: Font.DemiBold
                                verticalAlignment: Text.AlignVCenter
                            }

                            Item {
                                Layout.preferredWidth: 4
                            }

                            ButtonGroup {
                                id: fileViewGroup
                            }

                            ToolButton {
                                ButtonGroup.group: fileViewGroup
                                checkable: true
                                checked: root.textViewMode === "raw"
                                text: root.activeIsJson ? qsTr("Raw") : qsTr("Text")
                                implicitWidth: 58
                                implicitHeight: 24
                                padding: 0
                                onClicked: root.textViewMode = "raw"
                            }

                            ToolButton {
                                ButtonGroup.group: fileViewGroup
                                checkable: true
                                checked: root.textViewMode === "formatted"
                                visible: root.activeIsJson
                                text: qsTr("Formatted")
                                implicitWidth: 92
                                implicitHeight: 24
                                padding: 0
                                onClicked: {
                                    root.refreshFormattedJson()
                                    root.textViewMode = "formatted"
                                }
                            }

                            ToolButton {
                                ButtonGroup.group: fileViewGroup
                                checkable: true
                                checked: root.textViewMode === "binary"
                                visible: root.activeIsResourceIndex
                                text: qsTr("Binary")
                                implicitWidth: 72
                                implicitHeight: 24
                                padding: 0
                                onClicked: root.textViewMode = "binary"
                            }

                            Item {
                                Layout.fillWidth: true
                            }

                            Label {
                                text: decompilerController.tabsModel.activePath
                                color: secondaryTextColor
                                font.pixelSize: 11
                                elide: Text.ElideMiddle
                                horizontalAlignment: Text.AlignRight
                                verticalAlignment: Text.AlignVCenter
                                Layout.maximumWidth: 420
                            }
                        }
                    }
                }

            }
        }
    }

    function isJsonKind(kind) {
        return kind === "JSON"
    }

    function refreshFormattedJson() {
        if (activeIsJson) {
            formattedJsonContent = decompilerController.formatJson(decompilerController.selectedContent)
        } else {
            formattedJsonContent = ""
        }
    }
}
