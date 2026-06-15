import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic as Basic
import QtQuick.Controls.Material
import QtQuick.Layouts
import com.reark.app

Rectangle {
    id: root

    property string code: ""
    property string highlightTheme: "GitHub Dark"
    property string syntax: ""
    property bool findVisible: false
    readonly property bool darkTheme: Material.theme === Material.Dark
    readonly property color editorColor: darkTheme ? "#171819" : "#ffffff"
    readonly property color panelColor: darkTheme ? "#1b1d20" : "#f7fafb"
    readonly property color panelBorderColor: darkTheme ? "#34383d" : "#cbd6dc"
    readonly property color fieldColor: darkTheme ? "#17191c" : "#ffffff"
    readonly property color fieldBorderColor: darkTheme ? "#424952" : "#b9c7cf"
    readonly property color accentBorderColor: darkTheme ? "#3f8fd2" : "#2f80c1"
    readonly property color mutedTextColor: darkTheme ? "#9299a1" : "#5f6872"

    color: editorColor

    function openFind() {
        findVisible = true
        Qt.callLater(function() {
            findField.forceActiveFocus()
            findField.selectAll()
            runFind()
        })
    }

    function closeFind() {
        findVisible = false
        findDebounce.stop()
        editor.clearSearch()
        editor.clearSelection()
        editor.forceActiveFocus()
    }

    function scheduleFind() {
        if (!findVisible) {
            return
        }
        findDebounce.restart()
    }

    function runFind() {
        if (!findVisible) {
            return
        }
        revealBounds(editor.updateSearch(
                         findField.text,
                         matchCaseButton.checked,
                         wholeWordButton.checked,
                         regexButton.checked))
    }

    function findNext(direction) {
        if (!findVisible) {
            openFind()
            return
        }
        if (findDebounce.running) {
            findDebounce.stop()
            runFind()
        }
        revealBounds(editor.moveSearchResult(direction))
    }

    function revealBounds(bounds) {
        if (!bounds || !bounds.valid) {
            return
        }

        var maxY = Math.max(0, flickable.contentHeight - flickable.height)
        var maxX = Math.max(0, flickable.contentWidth - flickable.width)
        if (bounds.y < flickable.contentY + 12 || bounds.y + bounds.height > flickable.contentY + flickable.height - 12) {
            flickable.contentY = Math.max(0, Math.min(maxY, bounds.y - flickable.height / 2 + bounds.height / 2))
        }
        if (bounds.x < flickable.contentX + 16) {
            flickable.contentX = Math.max(0, Math.min(maxX, bounds.x - 24))
        } else if (bounds.x + bounds.width > flickable.contentX + flickable.width - 16) {
            flickable.contentX = Math.max(0, Math.min(maxX, bounds.x + bounds.width - flickable.width + 32))
        }
    }

    function findStatusText() {
        if (!findVisible || findField.text.length === 0) {
            return ""
        }
        if (!editor.searchPatternValid) {
            return qsTr("Invalid")
        }
        if (editor.searchResultCount === 0) {
            return qsTr("No results")
        }

        var total = editor.searchLimited ? qsTr("%1+").arg(editor.searchResultCount) : editor.searchResultCount
        return qsTr("%1/%2").arg(editor.activeSearchResult + 1).arg(total)
    }

    onCodeChanged: {
        if (findVisible) {
            scheduleFind()
        } else {
            editor.clearSearch()
        }
    }

    Timer {
        id: findDebounce
        interval: 120
        repeat: false
        onTriggered: root.runFind()
    }

    Shortcut {
        sequences: [StandardKey.Find]
        context: Qt.WindowShortcut
        onActivated: root.openFind()
    }

    Shortcut {
        sequence: "F3"
        context: Qt.WindowShortcut
        enabled: root.findVisible
        onActivated: root.findNext(1)
    }

    Shortcut {
        sequence: "Shift+F3"
        context: Qt.WindowShortcut
        enabled: root.findVisible
        onActivated: root.findNext(-1)
    }

    component FindIconButton: ToolButton {
        id: button

        property string iconSource: ""
        property int buttonSize: 24
        property int imageSize: 14

        Layout.preferredWidth: buttonSize
        Layout.preferredHeight: buttonSize
        padding: 0
        hoverEnabled: true

        background: Rectangle {
            radius: 3
            color: button.checked
                   ? (root.darkTheme ? "#243546" : "#d5edf0")
                   : button.hovered ? (root.darkTheme ? "#282b30" : "#e8eef0") : "transparent"
            border.width: button.checked ? 1 : 0
            border.color: root.accentBorderColor
        }

        contentItem: Item {
            Image {
                anchors.centerIn: parent
                width: button.imageSize
                height: button.imageSize
                source: button.iconSource
                sourceSize.width: 32
                sourceSize.height: 32
                fillMode: Image.PreserveAspectFit
                opacity: button.enabled ? (button.checked ? 1.0 : 0.78) : 0.32
                smooth: true
            }
        }
    }

    Flickable {
        id: flickable
        anchors.fill: parent
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        contentWidth: Math.max(editor.documentWidth, width)
        contentHeight: Math.max(editor.documentHeight, height)

        CodeEditorItem {
            id: editor
            x: flickable.contentX
            y: flickable.contentY
            width: flickable.width
            height: flickable.height
            text: root.code
            darkTheme: root.darkTheme
            highlightTheme: root.highlightTheme
            syntax: root.syntax
            fastScrolling: flickable.moving || flickable.flicking || verticalScrollBar.pressed || horizontalScrollBar.pressed
            scrollX: flickable.contentX
            scrollY: flickable.contentY
        }

        MouseArea {
            x: flickable.contentX
            y: flickable.contentY
            width: flickable.width
            height: flickable.height
            acceptedButtons: Qt.RightButton
            onClicked: function(mouse) {
                contextMenu.popup(editor, mouse.x, mouse.y)
            }
        }

        CompactMenu {
            id: contextMenu
            minimumItemWidth: 136

            Action {
                text: qsTr("Copy")
                enabled: editor.hasSelection
                shortcut: "Ctrl+C"
                onTriggered: editor.copySelection()
            }

            Action {
                text: qsTr("Select All")
                shortcut: "Ctrl+A"
                onTriggered: editor.selectAll()
            }

            Action {
                text: qsTr("Select Line")
                onTriggered: editor.selectCurrentLine()
            }

            Action {
                text: qsTr("Clear Selection")
                enabled: editor.hasSelection
                onTriggered: editor.clearSelection()
            }
        }

        ScrollBar.vertical: ScrollBar {
            id: verticalScrollBar
            policy: ScrollBar.AsNeeded
        }

        ScrollBar.horizontal: ScrollBar {
            id: horizontalScrollBar
            policy: ScrollBar.AsNeeded
        }
    }

    Rectangle {
        id: findPanel
        width: Math.min(parent.width - 32, 376)
        height: 40
        anchors.top: parent.top
        anchors.topMargin: 8
        anchors.right: parent.right
        anchors.rightMargin: 20
        visible: root.findVisible
        z: 10
        radius: 4
        color: root.panelColor
        border.width: 1
        border.color: root.panelBorderColor

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 8
            anchors.rightMargin: 6
            spacing: 4

            Rectangle {
                Layout.preferredWidth: 280
                Layout.preferredHeight: 28
                radius: 4
                color: root.fieldColor
                border.width: 1
                border.color: findField.activeFocus ? root.accentBorderColor : root.fieldBorderColor

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 3
                    spacing: 2

                    Basic.TextField {
                        id: findField

                        Layout.fillWidth: true
                        Layout.preferredHeight: parent.height
                        placeholderText: qsTr("Find")
                        selectByMouse: true
                        color: Material.foreground
                        placeholderTextColor: root.mutedTextColor
                        selectedTextColor: root.darkTheme ? "#ffffff" : "#000000"
                        selectionColor: root.darkTheme ? "#1f4d78" : "#b8daf0"
                        leftPadding: 0
                        rightPadding: 4
                        topPadding: 0
                        bottomPadding: 0
                        verticalAlignment: TextInput.AlignVCenter
                        font.pixelSize: 13
                        background: null

                        onTextChanged: root.scheduleFind()

                        Keys.onPressed: function(event) {
                            if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                                root.findNext(event.modifiers & Qt.ShiftModifier ? -1 : 1)
                                event.accepted = true
                            } else if (event.key === Qt.Key_Escape) {
                                root.closeFind()
                                event.accepted = true
                            }
                        }
                    }

                    Label {
                        readonly property bool hasStatus: text.length > 0

                        Layout.preferredWidth: hasStatus ? 48 : 0
                        visible: hasStatus
                        horizontalAlignment: Text.AlignRight
                        text: root.findStatusText()
                        color: (!editor.searchPatternValid || (editor.searchResultCount === 0 && findField.text.length > 0))
                               ? "#d26a6a" : root.mutedTextColor
                        font.pixelSize: 11
                        elide: Text.ElideRight
                    }

                    FindIconButton {
                        id: matchCaseButton
                        checkable: true
                        iconSource: "qrc:/icons/find-match-case.svg"
                        ToolTip.text: qsTr("Match Case")
                        ToolTip.visible: hovered
                        onToggled: root.scheduleFind()
                    }

                    FindIconButton {
                        id: wholeWordButton
                        checkable: true
                        iconSource: "qrc:/icons/find-whole-word.svg"
                        ToolTip.text: qsTr("Match Whole Word")
                        ToolTip.visible: hovered
                        onToggled: root.scheduleFind()
                    }

                    FindIconButton {
                        id: regexButton
                        checkable: true
                        iconSource: "qrc:/icons/find-regex.svg"
                        ToolTip.text: qsTr("Use Regular Expression")
                        ToolTip.visible: hovered
                        onToggled: root.scheduleFind()
                    }
                }
            }

            FindIconButton {
                iconSource: "qrc:/icons/chevron-up.svg"
                enabled: editor.searchResultCount > 0
                ToolTip.text: qsTr("Previous Match")
                ToolTip.visible: hovered
                onClicked: root.findNext(-1)
            }

            FindIconButton {
                iconSource: "qrc:/icons/chevron-down.svg"
                enabled: editor.searchResultCount > 0
                ToolTip.text: qsTr("Next Match")
                ToolTip.visible: hovered
                onClicked: root.findNext(1)
            }

            FindIconButton {
                iconSource: "qrc:/icons/close-small.svg"
                ToolTip.text: qsTr("Close")
                ToolTip.visible: hovered
                onClicked: root.closeFind()
            }
        }
    }
}
