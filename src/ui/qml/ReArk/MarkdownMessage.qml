import QtQuick
import QtQuick.Controls

Item {
    id: root

    property string markdown: ""
    property bool markdownEnabled: true
    property bool darkTheme: true
    property color textColor: "#eef5ff"
    property color accentColor: "#6f8cff"
    property int textPixelSize: 13
    property string emptyText: ""
    property var clipboardController: null
    property bool streaming: false

    readonly property string displayText: markdown.length > 0 ? markdown : emptyText
    readonly property bool hasRenderer: typeof markdownRenderer !== "undefined"
    readonly property bool richReady: markdownEnabled && renderedBlocks.length > 0
    readonly property bool waitingForRichRender: markdownEnabled && hasRenderer && !richReady

    property var renderedBlocks: []
    property int renderRequestId: 0
    property bool renderInFlight: false
    property bool renderDirty: false

    implicitWidth: Math.max(1, plainBody.implicitWidth)
    implicitHeight: Math.max(1, richReady ? blockColumn.implicitHeight : plainBody.implicitHeight)

    function renderNow() {
        if (markdownEnabled && hasRenderer) {
            if (renderInFlight) {
                renderDirty = true
                return
            }
            renderInFlight = true
            renderDirty = false
            renderRequestId = markdownRenderer.renderBlocksAsync(displayText, darkTheme)
        }
    }

    function scheduleRender() {
        if (markdownEnabled && hasRenderer) {
            renderTimer.restart()
        } else {
            renderTimer.stop()
            renderRequestId = 0
            renderInFlight = false
            renderDirty = false
            renderedBlocks = []
        }
    }

    onDisplayTextChanged: scheduleRender()
    onStreamingChanged: scheduleRender()
    onMarkdownEnabledChanged: {
        if (markdownEnabled) {
            renderTimer.stop()
            renderNow()
        } else {
            scheduleRender()
        }
    }
    onDarkThemeChanged: scheduleRender()
    Component.onCompleted: renderNow()

    Timer {
        id: renderTimer

        interval: root.streaming ? 180 : 45
        repeat: false
        onTriggered: root.renderNow()
    }

    Connections {
        target: root.hasRenderer ? markdownRenderer : null

        function onBlocksReady(requestId, blocks) {
            if (requestId === root.renderRequestId && root.markdownEnabled) {
                root.renderedBlocks = blocks
                root.renderInFlight = false
                if (root.renderDirty) {
                    root.renderDirty = false
                    root.scheduleRender()
                }
            }
        }
    }

    TextEdit {
        id: plainBody

        width: Math.max(1, root.width)
        visible: !root.richReady
        readOnly: true
        selectByMouse: true
        text: root.displayText
        color: root.textColor
        selectedTextColor: "#ffffff"
        selectionColor: root.accentColor
        wrapMode: TextEdit.Wrap
        textFormat: TextEdit.PlainText
        font.pixelSize: root.textPixelSize
        renderType: Text.NativeRendering
        opacity: root.displayText.length > 0 ? (root.waitingForRichRender ? 0.88 : 1.0) : 0.66
    }

    Column {
        id: blockColumn

        width: Math.max(1, root.width)
        visible: root.richReady
        spacing: 10

        Repeater {
            model: root.renderedBlocks

            delegate: Loader {
                id: blockLoader

                property var blockData: modelData

                width: blockColumn.width
                sourceComponent: blockData.type === "code"
                    ? codeBlockComponent
                    : blockData.type === "table" ? tableBlockComponent : htmlBlockComponent
                height: item ? item.implicitHeight : 0
            }
        }
    }

    Component {
        id: htmlBlockComponent

        Item {
            id: htmlBlock

            readonly property var block: parent ? parent.blockData : ({})

            width: parent ? parent.width : root.width
            implicitHeight: htmlText.contentHeight

            TextEdit {
                id: htmlText

                width: parent.width
                height: contentHeight
                readOnly: true
                selectByMouse: true
                wrapMode: TextEdit.Wrap
                textFormat: TextEdit.RichText
                text: htmlBlock.block.html || ""
                color: root.textColor
                selectedTextColor: "#ffffff"
                selectionColor: root.accentColor
                font.pixelSize: root.textPixelSize
                renderType: Text.NativeRendering

                onLinkActivated: function(link) {
                    Qt.openUrlExternally(link)
                }
            }
        }
    }

    Component {
        id: codeBlockComponent

        MarkdownCodeBlock {
            block: parent ? parent.blockData : ({})
            width: parent ? parent.width : root.width
            darkTheme: root.darkTheme
            accentColor: root.accentColor
            clipboardController: root.clipboardController
        }
    }

    Component {
        id: tableBlockComponent

        MarkdownTableBlock {
            block: parent ? parent.blockData : ({})
            width: parent ? parent.width : root.width
            darkTheme: root.darkTheme
            textColor: root.textColor
            accentColor: root.accentColor
            textPixelSize: root.textPixelSize
        }
    }
}
