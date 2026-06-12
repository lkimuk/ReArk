import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Rectangle {
    id: root

    property var settingsController: null

    readonly property bool darkTheme: Material.theme === Material.Dark
    readonly property color pageColor: darkTheme ? "#1e1e1e" : "#f3f3f3"
    readonly property color sidebarColor: darkTheme ? "#181818" : "#f8f8f8"
    readonly property color rowHoverColor: darkTheme ? "#2a2d2e" : "#e8e8e8"
    readonly property color rowSelectedColor: darkTheme ? "#37373d" : "#e4e6f1"
    readonly property color primaryTextColor: darkTheme ? "#cccccc" : "#1f1f1f"
    readonly property color titleTextColor: darkTheme ? "#e7e7e7" : "#1f1f1f"
    readonly property color secondaryTextColor: darkTheme ? "#a6a6a6" : "#5f5f5f"
    readonly property color subtleTextColor: darkTheme ? "#8a8a8a" : "#6f6f6f"
    readonly property color borderColor: darkTheme ? "#3c3c3c" : "#d0d0d0"
    readonly property color inputColor: darkTheme ? "#2b2b2b" : "#ffffff"
    readonly property color inputFocusColor: darkTheme ? "#007fd4" : "#006ab1"
    readonly property color buttonColor: darkTheme ? "#0e639c" : "#007acc"
    readonly property color buttonHoverColor: darkTheme ? "#1177bb" : "#0062a3"
    readonly property color dangerTextColor: darkTheme ? "#f48771" : "#a1260d"

    property bool showApiKey: false
    property bool showEmbeddingApiKey: false
    property string searchQuery: ""
    property string saveMessage: ""
    property string activeProviderDraftId: ""
    property var providerDrafts: ({})
    property bool loadingDraft: false
    readonly property var providerOptions: settingsController !== null
            ? settingsController.agentProviders
            : []
    readonly property string validationMessage: settingsController !== null
            ? settingsController.agentValidationMessage
            : ""

    color: pageColor

    onVisibleChanged: {
        if (visible) {
            loadDraft()
        }
    }

    Component.onCompleted: loadDraft()

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 52
            color: root.pageColor
            border.width: 0

            Rectangle {
                id: searchBox
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin: 26
                anchors.rightMargin: 26
                height: 28
                radius: 2
                color: root.darkTheme ? "#2b2b2b" : "#ffffff"
                border.width: 1
                border.color: searchInput.activeFocus ? root.inputFocusColor : root.borderColor

                Text {
                    anchors.left: parent.left
                    anchors.leftMargin: 9
                    anchors.verticalCenter: parent.verticalCenter
                    visible: searchInput.text.length === 0
                    text: qsTr("Search settings")
                    color: root.subtleTextColor
                    font.pixelSize: 13
                }

                TextInput {
                    id: searchInput
                    anchors.fill: parent
                    anchors.leftMargin: 9
                    anchors.rightMargin: 9
                    clip: true
                    selectByMouse: true
                    color: root.primaryTextColor
                    selectionColor: root.inputFocusColor
                    selectedTextColor: "#ffffff"
                    font.pixelSize: 13
                    verticalAlignment: TextInput.AlignVCenter
                    onTextChanged: root.searchQuery = text
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: root.borderColor
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            Rectangle {
                Layout.preferredWidth: 150
                Layout.fillHeight: true
                color: root.sidebarColor

                ColumnLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 22
                    anchors.rightMargin: 12
                    anchors.topMargin: 16
                    anchors.bottomMargin: 16
                    spacing: 0

                    ListView {
                        id: settingsNavigation

                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        currentIndex: 0
                        clip: true
                        boundsBehavior: Flickable.StopAtBounds
                        interactive: contentHeight > height
                        model: ListModel {
                            ListElement {
                                title: "Agent"
                            }
                            ListElement {
                                title: "Knowledge"
                            }
                        }

                        delegate: ItemDelegate {
                            id: navDelegate

                            required property int index
                            required property string title

                            width: settingsNavigation.width
                            height: 26
                            padding: 0
                            hoverEnabled: true
                            onClicked: settingsNavigation.currentIndex = index

                            background: Rectangle {
                                radius: 3
                                color: settingsNavigation.currentIndex === navDelegate.index
                                       ? root.rowSelectedColor
                                       : (navDelegate.hovered ? root.rowHoverColor : "transparent")
                            }

                            contentItem: Label {
                                leftPadding: 10
                                rightPadding: 8
                                verticalAlignment: Text.AlignVCenter
                                text: navDelegate.title === "Agent"
                                      ? qsTr("Agent")
                                      : (navDelegate.title === "Knowledge" ? qsTr("Knowledge") : navDelegate.title)
                                color: settingsNavigation.currentIndex === navDelegate.index
                                       ? root.titleTextColor
                                       : root.secondaryTextColor
                                font.pixelSize: 13
                                font.weight: settingsNavigation.currentIndex === navDelegate.index
                                             ? Font.DemiBold
                                             : Font.Normal
                                elide: Text.ElideRight
                            }
                        }
                    }
                }

                Rectangle {
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.right: parent.right
                    width: 1
                    color: root.borderColor
                }
            }

            StackLayout {
                id: settingsStack

                currentIndex: settingsNavigation.currentIndex
                Layout.fillWidth: true
                Layout.fillHeight: true

                Item {
                    id: agentPage

                    Flickable {
                        id: agentFlickable

                        anchors.fill: parent
                        clip: true
                        contentWidth: width
                        contentHeight: agentContent.implicitHeight + 44
                        boundsBehavior: Flickable.StopAtBounds

                        ColumnLayout {
                            id: agentContent

                            width: Math.min(980, Math.max(560, agentFlickable.width - agentScrollBar.width - 72))
                            x: 38
                            y: 28
                            spacing: 0

                            Label {
                                Layout.fillWidth: true
                                text: qsTr("Agent Runtime")
                                color: root.titleTextColor
                                font.pixelSize: 26
                                font.weight: Font.DemiBold
                            }

                            Label {
                                Layout.fillWidth: true
                                Layout.topMargin: 8
                                Layout.bottomMargin: 24
                                text: qsTr("Configure the model provider and endpoint used by ReArk smart analysis.")
                                color: root.secondaryTextColor
                                font.pixelSize: 13
                                wrapMode: Text.WordWrap
                            }

                            SettingRow {
                                id: providerRow

                                title: qsTr("Agent: Provider")
                                description: qsTr("Wuwe LLM provider preset or OpenAI-compatible endpoint.")

                                SettingsComboBox {
                                    id: providerField
                                    Layout.preferredWidth: 260
                                    model: root.providerOptions
                                    textRole: "displayName"
                                    valueRole: "id"
                                    onActivated: root.switchProviderDraft(root.providerIdAt(currentIndex))
                                }
                            }

                            SettingRow {
                                id: baseUrlRow

                                title: qsTr("Agent: Base URL")
                                description: qsTr("Provider endpoint. Keep provider defaults here, or override for compatible gateways.")

                                SettingsTextField {
                                    id: baseUrlField
                                    Layout.preferredWidth: 460
                                }
                            }

                            SettingRow {
                                id: modelRow

                                title: qsTr("Agent: Model")
                                description: qsTr("Model name sent with each Agent request.")

                                SettingsTextField {
                                    id: modelField
                                    Layout.preferredWidth: 460
                                }
                            }

                            SettingRow {
                                id: apiKeyRow

                                title: qsTr("Agent: API Key")
                                description: qsTr("Leave empty for local OpenAI-compatible services that do not require authentication.")

                                ColumnLayout {
                                    spacing: 8

                                    RowLayout {
                                        spacing: 12

                                        SettingsTextField {
                                            id: apiKeyField
                                            Layout.preferredWidth: 460
                                            echoMode: root.showApiKey ? TextInput.Normal : TextInput.Password
                                        }

                                        CheckBox {
                                            id: showApiKeyBox
                                            text: qsTr("Show")
                                            checked: root.showApiKey
                                            onToggled: root.showApiKey = checked
                                            font.pixelSize: 13
                                        }
                                    }
                                }
                            }

                            SettingRow {
                                id: requireApiKeyRow

                                title: qsTr("Agent: Require API Key")
                                description: qsTr("Require an API key for remote model endpoints.")

                                CheckBox {
                                    id: requireApiKeyBox
                                    text: qsTr("Require API key")
                                    font.pixelSize: 13
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 1
                                Layout.topMargin: 8
                                visible: root.anyAgentSettingVisible()
                                color: root.borderColor
                            }

                            Label {
                                Layout.fillWidth: true
                                Layout.topMargin: 12
                                Layout.bottomMargin: 18
                                visible: !root.anyAgentSettingVisible()
                                text: qsTr("No settings found")
                                color: root.secondaryTextColor
                                font.pixelSize: 13
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                Layout.topMargin: 18
                                spacing: 10
                                visible: root.anyAgentSettingVisible()

                                Button {
                                    id: agentSaveButton

                                    text: qsTr("Save")
                                    hoverEnabled: true
                                    onClicked: root.saveAgentSettings()
                                    contentItem: Label {
                                        text: agentSaveButton.text
                                        color: "#ffffff"
                                        font.pixelSize: 13
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                    background: Rectangle {
                                        radius: 2
                                        color: agentSaveButton.hovered ? root.buttonHoverColor : root.buttonColor
                                    }
                                }

                                Button {
                                    text: qsTr("Reset")
                                    flat: true
                                    onClicked: {
                                        if (root.settingsController !== null) {
                                            root.settingsController.resetAgentRuntimeSettings()
                                        }
                                        root.loadDraft()
                                        root.saveMessage = qsTr("Agent settings reset.")
                                    }
                                }

                                Label {
                                    Layout.fillWidth: true
                                    text: root.validationMessage.length > 0 ? root.validationMessage : root.saveMessage
                                    color: root.validationMessage.length > 0 ? root.dangerTextColor : root.subtleTextColor
                                    font.pixelSize: 12
                                    elide: Text.ElideRight
                                }
                            }
                        }

                        ScrollBar.vertical: ScrollBar {
                            id: agentScrollBar

                            policy: ScrollBar.AsNeeded
                        }
                    }
                }

                Item {
                    id: knowledgePage

                    Flickable {
                        id: knowledgeFlickable

                        anchors.fill: parent
                        clip: true
                        contentWidth: width
                        contentHeight: knowledgeContent.implicitHeight + 44
                        boundsBehavior: Flickable.StopAtBounds

                        ColumnLayout {
                            id: knowledgeContent

                            width: Math.min(980, Math.max(560, knowledgeFlickable.width - knowledgeScrollBar.width - 72))
                            x: 38
                            y: 28
                            spacing: 0

                            Label {
                                Layout.fillWidth: true
                                text: qsTr("Knowledge")
                                color: root.titleTextColor
                                font.pixelSize: 26
                                font.weight: Font.DemiBold
                            }

                            Label {
                                Layout.fillWidth: true
                                Layout.topMargin: 8
                                Layout.bottomMargin: 24
                                text: qsTr("Configure reference knowledge indexing for RAG-assisted analysis.")
                                color: root.secondaryTextColor
                                font.pixelSize: 13
                                wrapMode: Text.WordWrap
                            }

                            SettingRow {
                                id: embeddingBaseUrlRow

                                title: qsTr("Knowledge: Embedding Base URL")
                                description: qsTr("OpenAI-compatible embedding endpoint used to index reference knowledge.")

                                SettingsTextField {
                                    id: embeddingBaseUrlField
                                    Layout.preferredWidth: 460
                                }
                            }

                            SettingRow {
                                id: embeddingModelRow

                                title: qsTr("Knowledge: Embedding Model")
                                description: qsTr("Embedding model used by the reference knowledge index.")

                                SettingsTextField {
                                    id: embeddingModelField
                                    Layout.preferredWidth: 460
                                }
                            }

                            SettingRow {
                                id: embeddingApiKeyRow

                                title: qsTr("Knowledge: Embedding API Key")
                                description: qsTr("Leave empty for local embedding services that do not require authentication.")

                                RowLayout {
                                    spacing: 12

                                    SettingsTextField {
                                        id: embeddingApiKeyField
                                        Layout.preferredWidth: 460
                                        echoMode: root.showEmbeddingApiKey ? TextInput.Normal : TextInput.Password
                                    }

                                    CheckBox {
                                        id: showEmbeddingApiKeyBox
                                        text: qsTr("Show")
                                        checked: root.showEmbeddingApiKey
                                        onToggled: root.showEmbeddingApiKey = checked
                                        font.pixelSize: 13
                                    }
                                }
                            }

                            SettingRow {
                                id: embeddingRequireApiKeyRow

                                title: qsTr("Knowledge: Embedding API Key Required")
                                description: qsTr("Require an API key before indexing reference knowledge.")

                                CheckBox {
                                    id: embeddingRequireApiKeyBox
                                    text: qsTr("Require embedding API key")
                                    font.pixelSize: 13
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 1
                                Layout.topMargin: 8
                                visible: root.anyKnowledgeSettingVisible()
                                color: root.borderColor
                            }

                            Label {
                                Layout.fillWidth: true
                                Layout.topMargin: 12
                                Layout.bottomMargin: 18
                                visible: !root.anyKnowledgeSettingVisible()
                                text: qsTr("No settings found")
                                color: root.secondaryTextColor
                                font.pixelSize: 13
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                Layout.topMargin: 18
                                spacing: 10
                                visible: root.anyKnowledgeSettingVisible()

                                Button {
                                    id: knowledgeSaveButton

                                    text: qsTr("Save")
                                    hoverEnabled: true
                                    onClicked: root.saveAgentSettings()
                                    contentItem: Label {
                                        text: knowledgeSaveButton.text
                                        color: "#ffffff"
                                        font.pixelSize: 13
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                    background: Rectangle {
                                        radius: 2
                                        color: knowledgeSaveButton.hovered ? root.buttonHoverColor : root.buttonColor
                                    }
                                }

                                Button {
                                    text: qsTr("Reset")
                                    flat: true
                                    onClicked: {
                                        if (root.settingsController !== null) {
                                            root.settingsController.resetKnowledgeSettings()
                                        }
                                        root.loadDraft()
                                        root.saveMessage = qsTr("Knowledge settings reset.")
                                    }
                                }

                                Label {
                                    Layout.fillWidth: true
                                    text: root.validationMessage.length > 0 ? root.validationMessage : root.saveMessage
                                    color: root.validationMessage.length > 0 ? root.dangerTextColor : root.subtleTextColor
                                    font.pixelSize: 12
                                    elide: Text.ElideRight
                                }
                            }
                        }

                        ScrollBar.vertical: ScrollBar {
                            id: knowledgeScrollBar

                            policy: ScrollBar.AsNeeded
                        }
                    }
                }
            }
        }
    }

    component SettingRow: ColumnLayout {
        id: rowRoot

        property string title: ""
        property string description: ""
        default property alias controls: controlSlot.data

        Layout.fillWidth: true
        visible: root.matchesSetting(title, description)
        spacing: 8

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: root.borderColor
        }

        Label {
            Layout.fillWidth: true
            Layout.topMargin: 14
            text: rowRoot.title
            color: root.primaryTextColor
            font.pixelSize: 13
            font.weight: Font.DemiBold
        }

        Label {
            Layout.fillWidth: true
            text: rowRoot.description
            color: root.secondaryTextColor
            font.pixelSize: 12
            wrapMode: Text.WordWrap
        }

        RowLayout {
            id: controlSlot

            Layout.fillWidth: true
            Layout.bottomMargin: 16
        }
    }

    component SettingsTextField: Rectangle {
        id: fieldRoot

        property alias text: input.text
        property alias echoMode: input.echoMode

        implicitWidth: 460
        implicitHeight: 32
        radius: 2
        color: root.inputColor
        border.width: 1
        border.color: input.activeFocus ? root.inputFocusColor : root.borderColor

        TextInput {
            id: input

            anchors.fill: parent
            anchors.leftMargin: 8
            anchors.rightMargin: 8
            clip: true
            selectByMouse: true
            color: root.primaryTextColor
            selectionColor: root.inputFocusColor
            selectedTextColor: "#ffffff"
            font.pixelSize: 13
            verticalAlignment: TextInput.AlignVCenter
        }
    }

    component SettingsComboBox: ComboBox {
        id: comboRoot

        implicitWidth: 260
        implicitHeight: 32
        font.pixelSize: 13
        hoverEnabled: true

        contentItem: Text {
            leftPadding: 8
            rightPadding: 26
            text: comboRoot.displayText
            color: root.primaryTextColor
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
            font: comboRoot.font
        }

        background: Rectangle {
            radius: 2
            color: root.inputColor
            border.width: 1
            border.color: comboRoot.activeFocus ? root.inputFocusColor : root.borderColor
        }
    }

    Connections {
        target: root.settingsController
        ignoreUnknownSignals: true

        function onAgentSettingsChanged() {
            if (root.visible) {
                root.loadDraft()
            }
        }
    }

    function loadDraft() {
        if (settingsController === null) {
            return
        }

        loadingDraft = true
        providerDrafts = ({})
        root.setProviderCurrent(settingsController.agentProvider)
        activeProviderDraftId = settingsController.agentProvider
        baseUrlField.text = settingsController.agentBaseUrl
        modelField.text = settingsController.agentModel
        apiKeyField.text = settingsController.agentApiKey
        requireApiKeyBox.checked = settingsController.agentRequireApiKey
        embeddingBaseUrlField.text = settingsController.agentEmbeddingBaseUrl
        embeddingModelField.text = settingsController.agentEmbeddingModel
        embeddingApiKeyField.text = settingsController.agentEmbeddingApiKey
        embeddingRequireApiKeyBox.checked = settingsController.agentEmbeddingRequireApiKey
        saveMessage = ""
        loadingDraft = false
    }

    function matchesSetting(title, description) {
        const query = searchQuery.trim().toLowerCase()
        if (query.length === 0) {
            return true
        }

        return title.toLowerCase().indexOf(query) !== -1
            || description.toLowerCase().indexOf(query) !== -1
    }

    function anyAgentSettingVisible() {
        return providerRow.visible
            || baseUrlRow.visible
            || modelRow.visible
            || apiKeyRow.visible
            || requireApiKeyRow.visible
    }

    function anyKnowledgeSettingVisible() {
        return embeddingBaseUrlRow.visible
            || embeddingModelRow.visible
            || embeddingApiKeyRow.visible
            || embeddingRequireApiKeyRow.visible
    }

    function saveAgentSettings() {
        if (settingsController === null) {
            return false
        }

        root.captureProviderDraft(root.providerIdAt(providerField.currentIndex))
        const saved = settingsController.saveAgentSettings(
            root.providerIdAt(providerField.currentIndex),
            baseUrlField.text,
            apiKeyField.text,
            modelField.text,
            requireApiKeyBox.checked,
            embeddingBaseUrlField.text,
            embeddingApiKeyField.text,
            embeddingModelField.text,
            embeddingRequireApiKeyBox.checked)
        saveMessage = saved ? qsTr("Agent settings saved.") : ""
        return saved
    }

    function providerIdAt(index) {
        if (index < 0 || index >= providerOptions.length) {
            return ""
        }
        const item = providerOptions[index]
        if (item === undefined || item === null) {
            return ""
        }
        return item.id !== undefined ? item.id : String(item)
    }

    function setProviderCurrent(providerId) {
        for (let i = 0; i < providerOptions.length; ++i) {
            if (providerIdAt(i) === providerId) {
                providerField.currentIndex = i
                activeProviderDraftId = providerId
                return
            }
        }
        providerField.currentIndex = providerOptions.length > 0 ? 0 : -1
        activeProviderDraftId = providerIdAt(providerField.currentIndex)
    }

    function switchProviderDraft(providerId) {
        if (loadingDraft || settingsController === null || providerId.length === 0) {
            return
        }

        root.captureProviderDraft(activeProviderDraftId)
        root.applyProviderSettings(providerId)
        activeProviderDraftId = providerId
    }

    function captureProviderDraft(providerId) {
        if (providerId === undefined || providerId === null || providerId.length === 0) {
            return
        }

        const drafts = Object.assign({}, providerDrafts)
        drafts[providerId] = {
            baseUrl: baseUrlField.text,
            model: modelField.text,
            apiKey: apiKeyField.text,
            requireApiKey: requireApiKeyBox.checked
        }
        providerDrafts = drafts
    }

    function providerDraft(providerId) {
        if (providerDrafts === undefined || providerDrafts === null) {
            return undefined
        }
        return providerDrafts[providerId]
    }

    function applyProviderSettings(providerId) {
        if (settingsController === null || providerId.length === 0) {
            return
        }

        const draft = root.providerDraft(providerId)
        const settings = draft !== undefined
                ? draft
                : settingsController.agentProviderSettings(providerId)
        baseUrlField.text = settings.baseUrl !== undefined ? settings.baseUrl : ""
        modelField.text = settings.model !== undefined ? settings.model : ""
        apiKeyField.text = settings.apiKey !== undefined ? settings.apiKey : ""
        requireApiKeyBox.checked = settings.requireApiKey !== undefined
                ? settings.requireApiKey
                : settings.apiKeyRequired === true
    }
}
