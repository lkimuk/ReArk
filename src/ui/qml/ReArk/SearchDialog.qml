import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Popup {
    id: root

    readonly property bool darkTheme: Material.theme === Material.Dark
    property var candidates: []

    modal: true
    focus: true
    dim: false
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    width: Math.min(760, Math.max(380, Overlay.overlay.width - 120))
    height: Math.min(460, Math.max(280, Overlay.overlay.height - 160))
    x: Math.round((Overlay.overlay.width - width) / 2)
    y: 72
    padding: 0

    onOpened: {
        queryField.forceActiveFocus()
        queryField.selectAll()
        refresh()
    }

    background: Rectangle {
        color: root.darkTheme ? "#1c2027" : "#ffffff"
        radius: 6
        border.width: 1
        border.color: root.darkTheme ? "#4a515d" : "#cbd4d9"
    }

    contentItem: ColumnLayout {
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 48
            color: "transparent"

            TextField {
                id: queryField

                anchors.fill: parent
                anchors.leftMargin: 14
                anchors.rightMargin: 14
                anchors.topMargin: 8
                anchors.bottomMargin: 8
                placeholderText: qsTr("Search")
                selectByMouse: true
                font.pixelSize: 14

                onTextChanged: root.refresh()

                Keys.onPressed: function(event) {
                    if (event.key === Qt.Key_Down) {
                        resultList.currentIndex = Math.min(resultList.count - 1, resultList.currentIndex + 1)
                        event.accepted = true
                    } else if (event.key === Qt.Key_Up) {
                        resultList.currentIndex = Math.max(0, resultList.currentIndex - 1)
                        event.accepted = true
                    } else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                        root.acceptCurrent()
                        event.accepted = true
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: root.darkTheme ? "#343a45" : "#dce3e7"
        }

        ListView {
            id: resultList

            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            model: root.candidates
            currentIndex: -1

            delegate: ItemDelegate {
                id: row

                width: resultList.width
                height: 58
                highlighted: index === resultList.currentIndex
                hoverEnabled: true
                background: Rectangle {
                    color: row.highlighted
                           ? (root.darkTheme ? "#33424a" : "#d6e8e7")
                           : row.hovered ? (root.darkTheme ? "#2b313a" : "#e8eef0") : "transparent"
                }
                contentItem: RowLayout {
                    spacing: 8

                    FileTreeIcon {
                        Layout.preferredWidth: 16
                        Layout.preferredHeight: 16
                        name: modelData.name
                        kind: modelData.kind
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Label {
                                Layout.preferredWidth: 180
                                text: modelData.name
                                color: Material.foreground
                                font.pixelSize: 13
                                elide: Text.ElideRight
                            }

                            Label {
                                Layout.fillWidth: true
                                text: modelData.path
                                color: Material.hintTextColor
                                font.pixelSize: 11
                                elide: Text.ElideMiddle
                            }
                        }

                        Label {
                            Layout.fillWidth: true
                            text: modelData.subtitle
                            color: Material.hintTextColor
                            font.pixelSize: 11
                            elide: Text.ElideRight
                        }
                    }
                }
                onClicked: {
                    resultList.currentIndex = index
                    root.acceptCurrent()
                }
            }

            Label {
                anchors.centerIn: parent
                visible: resultList.count === 0
                text: qsTr("No matches found")
                color: Material.hintTextColor
                font.pixelSize: 13
            }
        }
    }

    function openWithFocus() {
        queryField.text = ""
        open()
    }

    function refresh() {
        candidates = decompilerController.searchCandidates(queryField.text)
        resultList.currentIndex = candidates.length > 0 ? 0 : -1
    }

    function acceptCurrent() {
        if (resultList.currentIndex < 0 || resultList.currentIndex >= candidates.length) {
            return
        }
        decompilerController.navigateToNode(candidates[resultList.currentIndex].nodeIndex)
        close()
    }
}
