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
    property bool reasoningDetailsCopied: false

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
    readonly property bool hasReasoningDetails: agentController !== null && agentController.hasReasoningDetails
    readonly property string reasoningResultJson: agentController !== null ? agentController.reasoningResultJson : ""
    readonly property string reasoningTraceJson: agentController !== null ? agentController.reasoningTraceJson : ""
    readonly property string reasoningUsageJson: agentController !== null ? agentController.reasoningUsageJson : ""
    property int detailsTabIndex: 0
    readonly property string activeDetailsJson: detailsTabIndex === 0
            ? reasoningResultJson
            : (detailsTabIndex === 1 ? reasoningTraceJson : reasoningUsageJson)
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

    Button {
        id: detailsButton

        anchors.top: parent.top
        anchors.right: newChatButton.left
        anchors.topMargin: 18
        anchors.rightMargin: 10
        width: Math.max(104, detailsButtonText.implicitWidth + 28)
        height: 34
        padding: 0
        hoverEnabled: true
        visible: root.hasReasoningDetails
        layer.enabled: true
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowBlur: 0.75
            shadowOpacity: root.buttonShadowOpacity
            shadowVerticalOffset: 5
        }

        background: Rectangle {
            radius: height / 2
            color: detailsButton.hovered ? root.newChatHoverColor : root.newChatColor
            border.width: 1
            border.color: root.newChatBorderColor
        }

        contentItem: Text {
            id: detailsButtonText

            text: qsTr("Run Details")
            color: root.primaryTextColor
            font.pixelSize: 13
            font.weight: Font.DemiBold
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            renderType: Text.NativeRendering
        }

        onClicked: {
            root.detailsTabIndex = 0
            reasoningDetailsDialog.open()
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
            readonly property bool copied: root.copiedMessageIndex === index
            readonly property real maxBubbleWidth: Math.min(
                root.contentWidth,
                messageDelegate.userMessage ? Math.max(220, root.contentWidth * 0.78) : 760)

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
                        border.color: root.borderColor
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
        color: root.panelColor
        border.width: 1
        border.color: promptInput.activeFocus ? root.accentColor : root.borderColor
        layer.enabled: true
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowBlur: 0.55
            shadowOpacity: root.panelShadowOpacity
            shadowVerticalOffset: 5
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

        TextEdit {
            id: promptInput

            anchors.left: parent.left
            anchors.right: sendButton.left
            anchors.top: referenceFlow.visible ? referenceFlow.bottom : parent.top
            anchors.bottom: statusLabel.visible ? statusLabel.top : toolRow.top
            anchors.leftMargin: 18
            anchors.rightMargin: 16
            anchors.topMargin: referenceFlow.visible ? 10 : 18
            anchors.bottomMargin: 8
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
            }

            Keys.onPressed: function(event) {
                if (event.key !== Qt.Key_Return && event.key !== Qt.Key_Enter) {
                    return
                }
                if (event.modifiers & Qt.ControlModifier) {
                    promptInput.insert(promptInput.cursorPosition, "\n")
                    event.accepted = true
                    return
                }
                root.submitPrompt()
                event.accepted = true
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
            anchors.left: promptInput.left
            anchors.top: promptInput.top
            text: qsTr("Ask anything about this app")
            color: root.secondaryTextColor
            font.pixelSize: 12
            visible: promptInput.text.length === 0
        }

        Label {
            id: statusLabel

            anchors.left: promptInput.left
            anchors.right: sendButton.left
            anchors.bottom: toolRow.visible ? toolRow.top : parent.bottom
            anchors.bottomMargin: toolRow.visible ? 7 : 14
            text: root.statusText
            color: root.agentError.length > 0 ? "#ef6f75" : root.secondaryTextColor
            font.pixelSize: 11
            elide: Text.ElideRight
            visible: text.length > 0
                && (!root.agentAvailable
                    || root.agentRunning
                    || !toolRow.visible)
        }

        Row {
            id: toolRow

            anchors.left: parent.left
            anchors.bottom: parent.bottom
            anchors.leftMargin: 26
            anchors.bottomMargin: 23
            spacing: 12
            visible: root.agentAvailable && !root.agentRunning

            AbstractButton {
                id: referenceButton

                width: 18
                height: 18
                padding: 0
                hoverEnabled: true
                enabled: root.agentKnowledgeController !== null && !root.referenceBusy
                opacity: enabled ? 1.0 : 0.42
                ToolTip.text: root.referenceBusy ? root.referenceStatus : qsTr("Add attachment")
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
            diameter: 38
            iconSize: 17
            iconName: root.agentRunning ? "close" : "arrow-up"
            backgroundColor: root.accentColor
            hoverColor: root.accentHoverColor
            pressedColor: root.accentPressedColor
            disabledColor: root.darkTheme ? "#667181" : "#b8c2d3"
            iconColor: "#ffffff"
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

    Dialog {
        id: reasoningDetailsDialog

        modal: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        padding: 0
        width: Math.min(880, root.width - 96)
        height: Math.min(620, root.height - 120)
        x: Math.round((root.width - width) / 2)
        y: Math.round((root.height - height) / 2)

        Overlay.modal: Rectangle {
            color: root.darkTheme ? "#a0080d14" : "#b8dbe4ee"
        }

        background: Rectangle {
            radius: 10
            color: root.darkTheme ? "#1b1d20" : "#fbfcff"
            border.width: 1
            border.color: root.darkTheme ? "#3a4047" : "#ccd7e6"
            layer.enabled: true
            layer.effect: MultiEffect {
                shadowEnabled: true
                shadowBlur: 0.55
                shadowColor: "#000000"
                shadowOpacity: root.darkTheme ? 0.24 : 0.16
                shadowVerticalOffset: 14
            }
        }

        header: Rectangle {
            implicitHeight: 60
            radius: 10
            color: root.darkTheme ? "#202226" : "#f6f8fc"

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 1
                color: root.borderColor
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 22
                anchors.rightMargin: 14
                spacing: 12

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    Label {
                        text: qsTr("Run Details")
                        color: root.primaryTextColor
                        font.pixelSize: 20
                        font.weight: Font.DemiBold
                    }

                    Label {
                        text: qsTr("Reasoning result, trace, and usage captured from Wuwe.")
                        color: root.secondaryTextColor
                        font.pixelSize: 12
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }

                ToolButton {
                    Layout.preferredWidth: 32
                    Layout.preferredHeight: 32
                    text: "×"
                    font.pixelSize: 20
                    focusPolicy: Qt.NoFocus
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Close")
                    onClicked: reasoningDetailsDialog.close()

                    contentItem: Label {
                        text: parent.text
                        color: parent.hovered ? root.primaryTextColor : root.secondaryTextColor
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font: parent.font
                    }

                    background: Rectangle {
                        radius: 6
                        color: parent.hovered
                               ? (root.darkTheme ? "#2a2e34" : "#edf3fb")
                               : "transparent"
                    }
                }
            }
        }

        contentItem: ColumnLayout {
            spacing: 14

            Row {
                Layout.fillWidth: true
                Layout.leftMargin: 22
                Layout.rightMargin: 22
                Layout.topMargin: 18
                spacing: 4

                Repeater {
                    model: [qsTr("Result"), qsTr("Trace"), qsTr("Usage")]

                    delegate: Rectangle {
                        width: Math.max(86, tabLabel.implicitWidth + 28)
                        height: 34
                        radius: 6
                        color: root.detailsTabIndex === index
                               ? (root.darkTheme ? "#2a3038" : "#e8f0ff")
                               : "transparent"
                        border.width: root.detailsTabIndex === index ? 1 : 0
                        border.color: root.darkTheme ? "#414851" : "#c9d7ee"

                        Label {
                            id: tabLabel

                            anchors.centerIn: parent
                            text: modelData
                            color: root.detailsTabIndex === index ? root.primaryTextColor : root.secondaryTextColor
                            font.pixelSize: 13
                            font.weight: root.detailsTabIndex === index ? Font.DemiBold : Font.Normal
                        }

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.detailsTabIndex = index
                        }
                    }
                }
            }

            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.leftMargin: 22
                Layout.rightMargin: 22
                clip: true

                TextArea {
                    id: detailsText

                    readOnly: true
                    selectByMouse: true
                    wrapMode: TextEdit.NoWrap
                    text: root.activeDetailsJson.length > 0
                            ? root.activeDetailsJson
                            : qsTr("No details were recorded for this section.")
                    color: root.primaryTextColor
                    selectedTextColor: "#ffffff"
                    selectionColor: root.accentColor
                    leftPadding: 14
                    rightPadding: 14
                    topPadding: 14
                    bottomPadding: 14
                    font.family: "Cascadia Mono, Consolas, Courier New, monospace"
                    font.pixelSize: 12
                    background: Rectangle {
                        radius: 6
                        color: root.darkTheme ? "#17191c" : "#f7f9fd"
                        border.width: 1
                        border.color: root.darkTheme ? "#34383d" : "#d6e0ee"
                    }
                }
            }
        }

        footer: Rectangle {
            implicitHeight: 64
            color: root.darkTheme ? "#1b1d20" : "#fbfcff"
            radius: 10

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                height: 1
                color: root.borderColor
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 22
                anchors.rightMargin: 22
                spacing: 12

                Label {
                    Layout.fillWidth: true
                    text: qsTr("Structured JSON for debugging, audit, and issue reports.")
                    color: root.secondaryTextColor
                    font.pixelSize: 12
                    elide: Text.ElideRight
                }

                Button {
                    Layout.preferredWidth: 118
                    Layout.preferredHeight: 34
                    text: root.reasoningDetailsCopied ? qsTr("Copied") : qsTr("Copy JSON")
                    enabled: root.activeDetailsJson.length > 0
                    onClicked: {
                        if (root.agentController !== null) {
                            root.agentController.copyTextToClipboard(root.activeDetailsJson)
                            root.reasoningDetailsCopied = true
                            reasoningDetailsCopiedTimer.restart()
                        }
                    }

                    contentItem: Label {
                        text: parent.text
                        color: parent.enabled ? "#ffffff" : root.mutedTextColor
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: 13
                        font.weight: Font.DemiBold
                    }

                    background: Rectangle {
                        radius: 7
                        color: !parent.enabled
                               ? (root.darkTheme ? "#25282d" : "#e7edf6")
                               : (parent.hovered ? root.accentHoverColor : root.accentColor)
                    }
                }

                Button {
                    Layout.preferredWidth: 82
                    Layout.preferredHeight: 34
                    text: qsTr("Close")
                    onClicked: reasoningDetailsDialog.close()

                    contentItem: Label {
                        text: parent.text
                        color: parent.hovered ? root.primaryTextColor : root.secondaryTextColor
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: 13
                    }

                    background: Rectangle {
                        radius: 7
                        color: parent.hovered
                               ? (root.darkTheme ? "#2a2e34" : "#edf3fb")
                               : "transparent"
                        border.width: 1
                        border.color: root.borderColor
                    }
                }
            }
        }

        onOpened: root.reasoningDetailsCopied = false
    }

    Timer {
        id: copiedResetTimer

        interval: 1800
        repeat: false
        onTriggered: root.copiedMessageIndex = -1
    }

    Timer {
        id: reasoningDetailsCopiedTimer

        interval: 1600
        repeat: false
        onTriggered: root.reasoningDetailsCopied = false
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
