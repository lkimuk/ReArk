import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Controls.Material

ApplicationWindow {
    id: settingsWindow

    width: 980
    height: 640
    minimumWidth: 780
    minimumHeight: 480
    visible: false
    title: qsTr("Settings")
    modality: Qt.ApplicationModal
    flags: Qt.WindowCloseButtonHint | Qt.CustomizeWindowHint | Qt.Dialog | Qt.WindowTitleHint

    property string currentTheme: "dark"
    property var settingsController: null
    property var closeCallback: null
    readonly property bool darkTheme: currentTheme === "system"
                                      ? Qt.styleHints.colorScheme === Qt.Dark
                                      : currentTheme === "dark"

    Material.theme: darkTheme ? Material.Dark : Material.Light
    Material.accent: "#3f8fd2"
    onClosing: {
        if (closeCallback) {
            closeCallback()
        }
        destroy()
    }

    SettingsPage {
        anchors.fill: parent
        settingsController: settingsWindow.settingsController
    }
}
