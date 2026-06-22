import QtQuick
import QtQuick.Controls
import QtQuick.Effects
import QtQuick.Controls.Material
import QtQuick.Dialogs
import QtQuick.Layouts

Rectangle {
    id: root

    property var agentController: null
    property var agentKnowledgeController: null
    property string draftText: ""
    property int copiedMessageIndex: -1

    readonly property bool darkTheme: Material.theme === Material.Dark
    readonly property bool agentAvailable: agentController !== null && agentController.available
    readonly property bool agentRunning: agentController !== null && agentController.running
    readonly property var referenceDocuments: agentKnowledgeController !== null ? agentKnowledgeController.references : []
    readonly property bool referenceBusy: agentKnowledgeController !== null && agentKnowledgeController.busy
    readonly property string referenceStatus: agentKnowledgeController !== null ? agentKnowledgeController.status : ""
    readonly property string referenceFailureText: firstReferenceError()
    readonly property bool hasMessages: agentController !== null && agentController.hasMessages
    readonly property string agentError: agentController !== null ? agentController.errorMessage : ""
    readonly property string agentStatus: agentController !== null ? agentController.status : ""
    readonly property string unavailableStatus: agentStatus.length > 0
            ? agentStatus
            : qsTr("Smart analysis is temporarily unavailable.")
    readonly property string statusText: agentError.length > 0
            ? agentError
            : (!agentAvailable ? unavailableStatus : agentStatus)
    readonly property bool canSendPrompt: agentAvailable
            && !agentRunning
            && !referenceBusy
            && draftText.trim().length > 0

    readonly property color pageTopColor: darkTheme ? "#1e1e1e" : "#e8f0fb"
    readonly property color pageBottomColor: darkTheme ? "#171819" : "#f4f8fc"
    readonly property color panelColor: darkTheme ? "#1b1d20" : "#fbfcff"
    readonly property color composerColor: darkTheme ? "#202326" : "#ffffff"
    readonly property color composerBorderColor: darkTheme ? "#353a41" : "#d7dfe9"
    readonly property color composerFocusBorderColor: darkTheme ? "#4b5563" : "#b8c5d8"
    readonly property color userBubbleColor: darkTheme ? "#1f4d78" : "#dbeafe"
    readonly property color assistantBubbleColor: darkTheme ? "#1b1d20" : "#ffffff"
    readonly property color primaryTextColor: darkTheme ? "#e7e7e7" : "#0f172a"
    readonly property color secondaryTextColor: darkTheme ? "#a6a6a6" : "#748094"
    readonly property color mutedTextColor: darkTheme ? "#858b92" : "#8a96a8"
    readonly property color borderColor: darkTheme ? "#34383d" : "#d3dce9"
    readonly property color iconColor: darkTheme ? "#c5cad0" : "#14213d"
    readonly property color accentColor: darkTheme ? "#3f8fd2" : "#5d83f4"
    readonly property color accentHoverColor: darkTheme ? "#52a0df" : "#4e74e4"
    readonly property color accentPressedColor: darkTheme ? "#3379b6" : "#446bdd"
    readonly property color newChatColor: darkTheme ? "#202226" : "#f8fbff"
    readonly property color newChatHoverColor: darkTheme ? "#282b30" : "#ffffff"
    readonly property color newChatBorderColor: darkTheme ? "#3a4047" : "#d7e0ed"
    readonly property real panelShadowOpacity: darkTheme ? 0.18 : 0.13
    readonly property real buttonShadowOpacity: darkTheme ? 0.14 : 0.1
    readonly property int contentGutter: width < 720 ? 18 : (width < 1080 ? 72 : 132)
    readonly property int contentWidth: Math.max(280, Math.min(930, width - contentGutter * 2))

    function firstReferenceError() {
        for (let i = 0; i < referenceDocuments.length; ++i) {
            const item = referenceDocuments[i]
            if (item.state === "failed" && item.error && item.error.length > 0) {
                return item.error
            }
        }
        return ""
    }

    function scheduleChatFollowTail() {
        if (!root.hasMessages) {
            return
        }
        followTailTimer.restart()
    }

    gradient: Gradient {
        GradientStop {
            position: 0
            color: root.pageTopColor
        }
        GradientStop {
            position: 1
            color: root.pageBottomColor
        }
    }

    Button {
        id: newChatButton

        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: 18
        anchors.rightMargin: 24
        width: Math.max(106, newChatContent.implicitWidth + 26)
        height: 34
        padding: 0
        hoverEnabled: true
        enabled: !root.agentRunning && root.hasMessages
        opacity: enabled ? 1.0 : 0.55
        layer.enabled: true
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowBlur: 0.75
            shadowOpacity: root.buttonShadowOpacity
            shadowVerticalOffset: 5
        }

        background: Rectangle {
            radius: height / 2
            color: newChatButton.hovered ? root.newChatHoverColor : root.newChatColor
            border.width: 1
            border.color: root.newChatBorderColor
        }

        contentItem: Row {
            id: newChatContent

            anchors.centerIn: parent
            spacing: 7

            Icon {
                name: "new-chat"
                color: root.primaryTextColor
                width: 13
                height: 13
                strokeWidth: 1.8
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                text: qsTr("New Chat")
                color: root.primaryTextColor
                font.pixelSize: 13
                font.weight: Font.DemiBold
                anchors.verticalCenter: parent.verticalCenter
                renderType: Text.NativeRendering
            }
        }

        onClicked: {
            if (root.agentController !== null) {
                root.agentController.newChat()
            }
            root.draftText = ""
            promptInput.forceActiveFocus()
        }
    }

    Label {
        id: emptyTitle

        width: root.contentWidth
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: composer.top
        anchors.bottomMargin: 18
        visible: !root.hasMessages
        text: qsTr("What would you like to ask?")
        color: root.primaryTextColor
        font.pixelSize: root.width < 520 ? 28 : 34
        font.weight: Font.Bold
        horizontalAlignment: Text.AlignHCenter
    }

    ListView {
        id: chatList

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: composer.top
        anchors.topMargin: 66
        anchors.bottomMargin: 18
        visible: root.hasMessages
        clip: true
        model: root.agentController !== null ? root.agentController.messageModel : null
        spacing: 16
        boundsBehavior: Flickable.StopAtBounds
        cacheBuffer: 1200

        header: Item {
            width: chatList.width
            height: 10
        }

        footer: Item {
            width: chatList.width
            height: 12
        }

        delegate: Item {
            id: messageDelegate

            required property int index
            required property string messageRole
            required property string messageText
            required property string messageState
            required property string messageTime

            readonly property bool userMessage: messageRole === "user"
            readonly property bool streaming: messageState === "streaming"
            readonly property bool activeAssistantMessage: streaming && !userMessage
            readonly property bool copied: root.copiedMessageIndex === index
            readonly property real maxBubbleWidth: messageDelegate.userMessage
                                                   ? Math.max(220, root.contentWidth * 0.78)
                                                   : root.contentWidth

            width: chatList.width
            height: messageColumn.implicitHeight

            ColumnLayout {
                id: messageColumn

                width: root.contentWidth
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 7

                Label {
                    Layout.alignment: messageDelegate.userMessage ? Qt.AlignRight : Qt.AlignLeft
                    text: messageDelegate.userMessage ? qsTr("You") : qsTr("ReArk Agent")
                    color: root.mutedTextColor
                    font.pixelSize: 11
                    font.weight: Font.DemiBold
                }

                Item {
                    id: messageBubble

                    readonly property real horizontalPadding: 30
                    readonly property color bubbleColor: messageDelegate.userMessage ? root.userBubbleColor : root.assistantBubbleColor
                    readonly property real compactWidth: Math.min(
                        Math.max(44, bubbleTextMeasure.implicitWidth + horizontalPadding),
                        messageDelegate.maxBubbleWidth)

                    Layout.alignment: messageDelegate.userMessage ? Qt.AlignRight : Qt.AlignLeft
                    Layout.maximumWidth: messageDelegate.maxBubbleWidth
                    implicitWidth: messageDelegate.streaming && !messageDelegate.userMessage
                                   ? messageDelegate.maxBubbleWidth
                                   : messageDelegate.userMessage
                                     ? compactWidth
                                     : messageDelegate.maxBubbleWidth
                    implicitHeight: messageBody.implicitHeight + 22
                                    + (messageDelegate.activeAssistantMessage ? streamStatus.implicitHeight + 10 : 0)

                    Rectangle {
                        anchors.fill: parent
                        radius: 8
                        color: messageBubble.bubbleColor
                        border.width: messageDelegate.userMessage ? 0 : 1
                        border.color: root.borderColor
                        visible: !messageDelegate.userMessage && !messageDelegate.streaming
                        layer.enabled: visible
                        layer.effect: MultiEffect {
                            shadowEnabled: true
                            shadowBlur: 0.35
                            shadowOpacity: root.darkTheme ? 0.16 : 0.08
                            shadowVerticalOffset: 3
                        }
                    }

                    Rectangle {
                        anchors.fill: parent
                        radius: 8
                        color: messageBubble.bubbleColor
                        border.width: messageDelegate.userMessage ? 0 : 1
                        border.color: messageDelegate.activeAssistantMessage
                                      ? (root.darkTheme ? "#4b667c" : "#9fc7e8")
                                      : root.borderColor
                    }

                    Rectangle {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        anchors.leftMargin: 1
                        anchors.rightMargin: 1
                        anchors.bottomMargin: 1
                        height: streamStatus.implicitHeight + 12
                        radius: 7
                        color: root.darkTheme ? "#161d23" : "#eef6fc"
                        opacity: messageDelegate.activeAssistantMessage ? 1 : 0
                        visible: opacity > 0

                        Behavior on opacity { NumberAnimation { duration: 120 } }
                    }

                    Text {
                        id: bubbleTextMeasure

                        visible: false
                        text: messageDelegate.messageText.length > 0
                              ? messageDelegate.messageText
                              : messageBody.emptyText
                        font.pixelSize: messageBody.textPixelSize
                        wrapMode: Text.NoWrap
                    }

                    MarkdownMessage {
                        id: messageBody

                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.leftMargin: 15
                        anchors.rightMargin: 15
                        anchors.topMargin: 11
                        anchors.bottom: messageDelegate.activeAssistantMessage ? streamStatus.top : parent.bottom
                        anchors.bottomMargin: messageDelegate.activeAssistantMessage ? 8 : 11
                        markdown: messageDelegate.messageText
                        markdownEnabled: !messageDelegate.userMessage
                        streaming: messageDelegate.streaming
                        emptyText: messageDelegate.streaming ? qsTr("Thinking...") : ""
                        darkTheme: root.darkTheme
                        textColor: root.primaryTextColor
                        accentColor: root.accentColor
                        textPixelSize: 13
                        clipboardController: root.agentController
                    }

                    RowLayout {
                        id: streamStatus

                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        anchors.leftMargin: 15
                        anchors.rightMargin: 15
                        anchors.bottomMargin: 9
                        visible: messageDelegate.activeAssistantMessage
                        spacing: 7

                        Item {
                            Layout.preferredWidth: 15
                            Layout.preferredHeight: 15

                            Rectangle {
                                id: pulseDot

                                anchors.centerIn: parent
                                width: 7
                                height: 7
                                radius: 4
                                color: root.accentColor
                                opacity: 0.82
                                scale: 0.82

                                SequentialAnimation on scale {
                                    running: messageDelegate.activeAssistantMessage
                                    loops: Animation.Infinite
                                    NumberAnimation { to: 1.18; duration: 520; easing.type: Easing.OutCubic }
                                    NumberAnimation { to: 0.82; duration: 520; easing.type: Easing.InOutCubic }
                                }

                                SequentialAnimation on opacity {
                                    running: messageDelegate.activeAssistantMessage
                                    loops: Animation.Infinite
                                    NumberAnimation { to: 1.0; duration: 520; easing.type: Easing.OutCubic }
                                    NumberAnimation { to: 0.58; duration: 520; easing.type: Easing.InOutCubic }
                                }
                            }
                        }

                        Label {
                            Layout.fillWidth: true
                            text: root.agentRunning && root.agentStatus.length > 0
                                  ? root.agentStatus
                                  : qsTr("Working...")
                            color: root.secondaryTextColor
                            font.pixelSize: 11
                            elide: Text.ElideRight
                        }
                    }
                }

                RowLayout {
                    Layout.alignment: messageDelegate.userMessage ? Qt.AlignRight : Qt.AlignLeft
                    spacing: 8

                    AbstractButton {
                        id: copyButton

                        Layout.preferredWidth: 16
                        Layout.preferredHeight: 16
                        padding: 0
                        hoverEnabled: true
                        enabled: messageDelegate.messageText.length > 0
                        opacity: enabled ? (hovered || messageDelegate.copied ? 1.0 : 0.68) : 0.34

                        Accessible.name: qsTr("Copy message")
                        ToolTip.text: qsTr("Copy")
                        ToolTip.visible: hovered && enabled && !messageDelegate.copied
                        ToolTip.delay: 450

                        background: Rectangle {
                            radius: 4
                            color: messageDelegate.copied
                                ? (root.darkTheme ? "#243546" : "#dbeafe")
                                : copyButton.hovered
                                ? (root.darkTheme ? "#292d32" : "#e7edf7")
                                : "transparent"
                        }

                        contentItem: Icon {
                            anchors.centerIn: parent
                            name: messageDelegate.copied ? "check" : "copy"
                            width: 10
                            height: 10
                            color: messageDelegate.copied ? root.accentColor : root.mutedTextColor
                        }

                        onClicked: {
                            if (root.agentController !== null) {
                                root.agentController.copyTextToClipboard(messageDelegate.messageText)
                            }
                            root.copiedMessageIndex = messageDelegate.index
                            copiedResetTimer.restart()
                        }
                    }

                    Label {
                        text: messageDelegate.messageTime
                        color: root.mutedTextColor
                        font.pixelSize: 11
                        verticalAlignment: Text.AlignVCenter
                        visible: text.length > 0
                    }
                }
            }
        }

        onCountChanged: root.scheduleChatFollowTail()
        onContentHeightChanged: if (root.agentRunning) root.scheduleChatFollowTail()
    }

    Rectangle {
        id: composer

        width: root.contentWidth
        height: root.hasMessages ? 124 : 130
        anchors.horizontalCenter: parent.horizontalCenter
        radius: 8
        color: root.composerColor
        border.width: 1
        border.color: promptInput.activeFocus ? root.composerFocusBorderColor : root.composerBorderColor
        layer.enabled: true
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowBlur: root.darkTheme ? 0.32 : 0.48
            shadowOpacity: root.darkTheme ? 0.28 : root.panelShadowOpacity
            shadowVerticalOffset: root.darkTheme ? 2 : 5
        }

        states: [
            State {
                when: root.hasMessages
                AnchorChanges {
                    target: composer
                    anchors.bottom: root.bottom
                }
                PropertyChanges {
                    target: composer
                    anchors.bottomMargin: 34
                    anchors.verticalCenterOffset: 0
                }
            },
            State {
                when: !root.hasMessages
                AnchorChanges {
                    target: composer
                    anchors.verticalCenter: root.verticalCenter
                }
                PropertyChanges {
                    target: composer
                    anchors.bottomMargin: 0
                    anchors.verticalCenterOffset: 82
                }
            }
        ]

        Flickable {
            id: promptViewport

            anchors.left: parent.left
            anchors.right: sendButton.left
            anchors.top: referenceFlow.visible ? referenceFlow.bottom : parent.top
            anchors.bottom: statusLabel.visible ? statusLabel.top : toolRow.top
            anchors.leftMargin: 18
            anchors.rightMargin: 16
            anchors.topMargin: referenceFlow.visible ? 10 : 18
            anchors.bottomMargin: 8
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            contentWidth: width
            contentHeight: Math.max(height, promptInput.implicitHeight)
            ScrollBar.vertical: ScrollBar {
                id: promptScrollBar

                policy: promptViewport.contentHeight > promptViewport.height
                        ? ScrollBar.AsNeeded
                        : ScrollBar.AlwaysOff
                width: 7
                rightPadding: 2
                contentItem: Rectangle {
                    implicitWidth: 5
                    radius: 2.5
                    color: root.darkTheme ? "#6c737d" : "#9aa6b2"
                    opacity: promptViewport.moving || promptScrollBar.pressed || promptScrollBar.hovered ? 0.82 : 0.56
                }
                background: Item {
                    implicitWidth: 7
                }
            }

            function keepCursorVisible() {
                const cursorTop = promptInput.cursorRectangle.y
                const cursorBottom = cursorTop + promptInput.cursorRectangle.height
                const visibleBottom = contentY + height

                if (cursorBottom > visibleBottom) {
                    contentY = Math.min(cursorBottom - height, contentHeight - height)
                } else if (cursorTop < contentY) {
                    contentY = Math.max(0, cursorTop)
                }
            }

            TextEdit {
                id: promptInput

                width: Math.max(0, promptViewport.width - 12)
                wrapMode: TextEdit.Wrap
                color: root.primaryTextColor
                selectedTextColor: "#ffffff"
                selectionColor: root.accentColor
                cursorVisible: activeFocus
                font.pixelSize: 13
                enabled: !root.agentRunning
                text: root.draftText

                onTextChanged: {
                    if (root.draftText !== text) {
                        root.draftText = text
                    }
                    if (text.length === 0) {
                        promptViewport.contentY = 0
                    }
                }

                onCursorRectangleChanged: promptViewport.keepCursorVisible()

                Keys.onPressed: function(event) {
                    if (event.key !== Qt.Key_Return && event.key !== Qt.Key_Enter) {
                        return
                    }

                    const modifiers = event.modifiers & ~Qt.KeypadModifier
                    if (modifiers === Qt.ShiftModifier) {
                        promptInput.insert(promptInput.cursorPosition, "\n")
                        event.accepted = true
                        return
                    }

                    if (modifiers !== Qt.NoModifier) {
                        event.accepted = true
                        return
                    }

                    root.submitPrompt()
                    event.accepted = true
                }
            }
        }

        Flow {
            id: referenceFlow

            anchors.left: parent.left
            anchors.right: sendButton.left
            anchors.top: parent.top
            anchors.leftMargin: 18
            anchors.rightMargin: 14
            anchors.topMargin: 10
            spacing: 7
            visible: root.referenceDocuments.length > 0

            Repeater {
                model: root.referenceDocuments

                Rectangle {
                    required property var modelData

                    height: 24
                    width: Math.min(230, chipText.implicitWidth + 54)
                    radius: height / 2
                    color: root.darkTheme ? "#24272c" : "#edf4ff"
                    border.width: 1
                    border.color: modelData.state === "failed"
                                  ? "#ef6f75"
                                  : (root.darkTheme ? "#3b4149" : "#cbd8ee")
                    ToolTip.text: modelData.error || ""
                    ToolTip.visible: chipHover.hovered && ToolTip.text.length > 0
                    ToolTip.delay: 350

                    HoverHandler {
                        id: chipHover
                    }

                    Label {
                        id: chipText

                        anchors.left: parent.left
                        anchors.right: removeReferenceButton.left
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.leftMargin: 10
                        anchors.rightMargin: 5
                        text: modelData.displayName + " · " + modelData.stateLabel
                        color: modelData.state === "failed" ? "#ef6f75" : root.secondaryTextColor
                        font.pixelSize: 11
                        elide: Text.ElideRight
                    }

                    AbstractButton {
                        id: removeReferenceButton

                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.rightMargin: 6
                        width: 14
                        height: 14
                        padding: 0
                        hoverEnabled: true

                        contentItem: Icon {
                            name: "close"
                            color: root.mutedTextColor
                            width: 9
                            height: 9
                            anchors.centerIn: parent
                        }
                        background: Rectangle {
                            radius: 7
                            color: removeReferenceButton.hovered
                                   ? (root.darkTheme ? "#30343a" : "#dbe7fb")
                                   : "transparent"
                        }
                        onClicked: {
                            if (root.agentKnowledgeController !== null) {
                                root.agentKnowledgeController.removeReference(modelData.id)
                            }
                        }
                    }
                }
            }
        }

        Label {
            anchors.left: promptViewport.left
            anchors.top: promptViewport.top
            text: qsTr("Ask anything about this app")
            color: root.secondaryTextColor
            font.pixelSize: 12
            visible: promptInput.text.length === 0
        }

        Label {
            id: statusLabel

            anchors.left: promptViewport.left
            anchors.right: sendButton.left
            anchors.bottom: toolRow.visible ? toolRow.top : parent.bottom
            anchors.bottomMargin: toolRow.visible ? 7 : 14
            text: root.statusText
            color: root.agentError.length > 0 ? "#ef6f75" : root.secondaryTextColor
            font.pixelSize: 11
            elide: Text.ElideRight
            visible: text.length > 0
                && (!root.agentAvailable
                    || (root.agentRunning && !root.hasMessages))
        }

        Row {
            id: toolRow

            anchors.left: parent.left
            anchors.bottom: parent.bottom
            anchors.leftMargin: 26
            anchors.bottomMargin: 23
            spacing: 12
            visible: root.agentAvailable

            AbstractButton {
                id: referenceButton

                width: 18
                height: 18
                padding: 0
                hoverEnabled: true
                enabled: root.agentKnowledgeController !== null && !root.referenceBusy && !root.agentRunning
                opacity: enabled ? 1.0 : 0.42
                ToolTip.text: root.agentRunning
                              ? qsTr("Wait for the current response to finish")
                              : (root.referenceBusy ? root.referenceStatus : qsTr("Add attachment"))
                ToolTip.visible: hovered
                ToolTip.delay: 450

                background: Rectangle {
                    radius: 4
                    color: referenceButton.hovered
                           ? (root.darkTheme ? "#292d32" : "#e7edf7")
                           : "transparent"
                }

                contentItem: Icon {
                    name: "paperclip"
                    color: root.iconColor
                    width: 15
                    height: 15
                    strokeWidth: 1.9
                    anchors.centerIn: parent
                }

                onClicked: referenceFileDialog.open()
            }

            Label {
                width: Math.min(280, implicitWidth)
                anchors.verticalCenter: parent.verticalCenter
                text: root.agentError.length > 0
                    ? root.agentError
                    : qsTr("Reference indexing failed. Check settings.")
                color: "#ef6f75"
                font.pixelSize: 11
                elide: Text.ElideRight
                visible: root.agentError.length > 0 || root.referenceFailureText.length > 0
                ToolTip.text: root.agentError.length > 0 ? root.agentError : root.referenceFailureText
                ToolTip.visible: statusHover.hovered && ToolTip.text.length > 0
                ToolTip.delay: 350

                HoverHandler {
                    id: statusHover
                }
            }
        }

        RoundIconButton {
            id: sendButton

            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.rightMargin: 16
            anchors.bottomMargin: 13
            diameter: 36
            iconSize: 16
            iconName: root.agentRunning ? "stop" : "arrow-up"
            backgroundColor: root.agentRunning
                ? (root.darkTheme ? "#d4d8de" : "#111827")
                : (root.darkTheme ? "#d4d8de" : "#111827")
            hoverColor: root.agentRunning
                ? (root.darkTheme ? "#e0e4ea" : "#030712")
                : (root.darkTheme ? "#e0e4ea" : "#030712")
            pressedColor: root.agentRunning
                ? (root.darkTheme ? "#bfc5ce" : "#374151")
                : (root.darkTheme ? "#bfc5ce" : "#374151")
            disabledColor: root.darkTheme ? "#3b414a" : "#e5e7eb"
            iconColor: enabled
                ? (root.darkTheme ? "#18181b" : "#ffffff")
                : (root.darkTheme ? "#9ca3af" : "#9aa3af")
            borderColor: enabled
                ? (root.darkTheme ? "#c3c8d0" : "#111827")
                : (root.darkTheme ? "#4b5563" : "#d7dce3")
            focusBorderColor: root.darkTheme ? "#eef1f5" : "#7c8aa0"
            iconStrokeWidth: 2.15
            shadowEnabled: enabled
            toolTipText: root.agentRunning
                ? qsTr("Cancel")
                : (root.referenceBusy
                   ? qsTr("Wait for reference indexing to finish")
                   : (!root.agentAvailable ? qsTr("Smart analysis unavailable") : qsTr("Send")))
            enabled: root.agentAvailable && (root.agentRunning || root.canSendPrompt)
            opacity: enabled ? 1.0 : 0.55
            onClicked: {
                if (root.agentRunning) {
                    root.agentController.cancel()
                } else {
                    root.submitPrompt()
                }
            }
        }
    }

    Connections {
        target: root.agentController
        ignoreUnknownSignals: true

        function onMessagesChanged() {
            if (root.hasMessages) {
                root.scheduleChatFollowTail()
            }
        }
    }

    FileDialog {
        id: referenceFileDialog

        title: qsTr("Add Attachment")
        nameFilters: [
            qsTr("Reference documents (*.md *.markdown *.txt *.html *.htm *.rtf *.csv *.json *.pdf *.docx *.pptx *.xlsx)"),
            qsTr("All files (*)")
        ]
        onAccepted: {
            if (root.agentKnowledgeController !== null) {
                root.agentKnowledgeController.addReferenceFile(selectedFile)
            }
        }
    }

    Timer {
        id: copiedResetTimer

        interval: 1800
        repeat: false
        onTriggered: root.copiedMessageIndex = -1
    }

    Timer {
        id: followTailTimer

        interval: 33
        repeat: false
        onTriggered: chatList.positionViewAtEnd()
    }

    function submitPrompt() {
        if (!root.canSendPrompt) {
            return
        }
        const text = root.draftText.trim()
        root.agentController.ask(text)
        root.draftText = ""
        promptInput.text = ""
        root.scheduleChatFollowTail()
    }
}
