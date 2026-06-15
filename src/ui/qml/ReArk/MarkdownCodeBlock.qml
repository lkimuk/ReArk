import QtQuick
import QtQuick.Controls
import com.reark.app

Rectangle {
    id: root

    property var block: ({})
    property bool darkTheme: true
    property color accentColor: "#3f8fd2"
    property var clipboardController: null

    readonly property string code: block.code || ""
    readonly property string language: block.language || ""
    readonly property string languageLabel: block.languageLabel || ""
    readonly property bool compact: block.compact === true
    readonly property color chromeColor: darkTheme ? "#151719" : "#f5f7fa"
    readonly property color headerTextColor: darkTheme ? "#929aa3" : "#66758a"
    readonly property color hoverColor: darkTheme ? "#24282d" : "#e7edf5"
    readonly property color hairlineColor: darkTheme ? "#2b3036" : "#d9e1ec"
    readonly property real viewportHeight: compact
                                     ? 36
                                     : Math.min(Math.max(74, editor.documentHeight), 430)

    property bool copied: false

    implicitHeight: header.height + codeViewport.height + bottomPad.height
    radius: 5
    color: chromeColor
    clip: true

    function syntaxName() {
        if (language === "ts" || language === "ets") {
            return "TypeScript"
        }
        if (language === "js") {
            return "JavaScript"
        }
        if (language === "json") {
            return "JSON"
        }
        if (language === "cpp") {
            return "C++"
        }
        if (language === "py") {
            return "Python"
        }
        if (language === "sh" || language === "bash") {
            return "Shell"
        }
        if (language === "cmake") {
            return "CMake"
        }
        return languageLabel
    }

    function copyWholeBlock() {
        if (clipboardController !== null && code.length > 0) {
            clipboardController.copyTextToClipboard(code)
            copied = true
            copiedTimer.restart()
        }
    }

    Item {
        id: header

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: compact ? 30 : 42

        Row {
            anchors.left: parent.left
            anchors.leftMargin: 20
            anchors.verticalCenter: parent.verticalCenter
            spacing: 8

            Repeater {
                model: ["#ff5f57", "#ffbd2e", "#28c840"]

                delegate: Rectangle {
                    width: 10
                    height: 10
                    radius: 5
                    color: modelData
                    border.width: 1
                    border.color: root.darkTheme ? "#00000044" : "#00000022"
                }
            }
        }

        Label {
            anchors.right: copyButton.left
            anchors.rightMargin: 10
            anchors.verticalCenter: parent.verticalCenter
            text: root.languageLabel
            visible: text.length > 0 && !root.compact
            color: root.headerTextColor
            font.pixelSize: 11
            elide: Text.ElideRight
        }

        AbstractButton {
            id: copyButton

            anchors.right: parent.right
            anchors.rightMargin: 14
            anchors.verticalCenter: parent.verticalCenter
            width: 26
            height: 24
            hoverEnabled: true
            focusPolicy: Qt.NoFocus
            Accessible.name: qsTr("Copy code")
            opacity: hovered || root.copied ? 1.0 : 0.58
            ToolTip.visible: hovered
            ToolTip.text: root.copied ? qsTr("Copied") : qsTr("Copy code")
            ToolTip.delay: 450
            onClicked: root.copyWholeBlock()

            contentItem: Icon {
                anchors.centerIn: parent
                name: root.copied ? "check" : "copy"
                width: 12
                height: 12
                color: root.copied ? root.accentColor : root.headerTextColor
            }

            background: Rectangle {
                radius: 4
                color: copyButton.hovered || root.copied ? root.hoverColor : "transparent"
            }

            Behavior on opacity {
                NumberAnimation {
                    duration: 90
                }
            }
        }

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 1
            color: root.hairlineColor
            opacity: root.compact ? 0 : 1
        }
    }

    Flickable {
        id: codeViewport

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: header.bottom
        height: root.viewportHeight
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        contentWidth: Math.max(editor.documentWidth, width)
        contentHeight: Math.max(editor.documentHeight, height)

        CodeEditorItem {
            id: editor

            x: codeViewport.contentX
            y: codeViewport.contentY
            width: codeViewport.width
            height: codeViewport.height
            text: root.code
            darkTheme: root.darkTheme
            highlightTheme: root.darkTheme ? "GitHub Dark" : "GitHub Light"
            syntax: root.syntaxName()
            showGutter: false
            scrollX: codeViewport.contentX
            scrollY: codeViewport.contentY
            fastScrolling: codeViewport.moving || codeViewport.flicking
        }

        ScrollBar.horizontal: ScrollBar {
            policy: codeViewport.contentWidth > codeViewport.width ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
        }

        ScrollBar.vertical: ScrollBar {
            policy: codeViewport.contentHeight > codeViewport.height ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
        }
    }

    Item {
        id: bottomPad

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: codeViewport.bottom
        height: compact ? 8 : 16
    }

    Timer {
        id: copiedTimer

        interval: 1400
        repeat: false
        onTriggered: root.copied = false
    }
}
