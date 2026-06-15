import QtQuick
import QtQuick.Controls

Item {
    id: root

    property var block: ({})
    property bool darkTheme: true
    property color textColor: "#e7e7e7"
    property color accentColor: "#3f8fd2"
    property int textPixelSize: 13

    readonly property int columnCount: Math.max(1, Number(block.columnCount || 1))
    readonly property var headers: block.headers || []
    readonly property var rows: block.rows || []
    readonly property var alignments: block.alignments || []
    readonly property var columnWeights: block.columnWeights || []
    readonly property real totalColumnWeight: {
        let total = 0
        for (let i = 0; i < columnCount; ++i) {
            total += columnWeight(i)
        }
        return Math.max(1, total)
    }
    readonly property color borderColor: darkTheme ? "#343a42" : "#d0dae8"
    readonly property color headerBackground: darkTheme ? "#22262b" : "#eef4fb"
    readonly property color rowBackground: darkTheme ? "#181b1f" : "#ffffff"
    readonly property color alternateRowBackground: darkTheme ? "#1d2024" : "#f8fafd"
    readonly property color hoverBackground: darkTheme ? "#252a30" : "#f1f6fd"
    readonly property color mutedTextColor: darkTheme ? "#9da5ad" : "#5f7085"
    readonly property color headerAccent: darkTheme ? "#3f8fd2" : "#2b8a8a"
    readonly property real minimumColumnWidth: 120
    readonly property real maximumColumnWidth: 980
    readonly property real averageCharacterWidth: Math.max(7, textPixelSize * 0.62)
    readonly property real preferredTableWidth: Math.max(width, naturalTableWidth())

    implicitWidth: tableFlick.implicitWidth
    implicitHeight: tableFlick.implicitHeight

    function cellAlignment(column) {
        const alignment = column < alignments.length ? alignments[column] : "left"
        if (alignment === "right") {
            return Text.AlignRight
        }
        if (alignment === "center") {
            return Text.AlignHCenter
        }
        return Text.AlignLeft
    }

    function rowDataAt(index) {
        if (index === 0) {
            return headers
        }
        return index - 1 < rows.length ? rows[index - 1] : []
    }

    function columnWeight(column) {
        const value = column < columnWeights.length ? Number(columnWeights[column]) : 12
        return Math.max(8, Math.min(120, value || 12))
    }

    function naturalColumnWidth(column) {
        return Math.max(minimumColumnWidth,
                        Math.min(maximumColumnWidth,
                                 columnWeight(column) * averageCharacterWidth + 34))
    }

    function naturalTableWidth() {
        let total = 0
        for (let i = 0; i < columnCount; ++i) {
            total += naturalColumnWidth(i)
        }
        return Math.max(columnCount * minimumColumnWidth, total)
    }

    function columnWidth(column) {
        const naturalWidth = naturalColumnWidth(column)
        const extraWidth = Math.max(0, preferredTableWidth - naturalTableWidth())
        return naturalWidth + extraWidth * columnWeight(column) / totalColumnWeight
    }

    Flickable {
        id: tableFlick

        width: parent.width
        height: tableColumn.implicitHeight + (horizontalBar.visible ? horizontalBar.height + 5 : 0)
        implicitWidth: Math.max(1, root.width)
        implicitHeight: height
        contentWidth: tableFrame.width
        contentHeight: tableFrame.height
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        Rectangle {
            id: tableFrame

            width: Math.max(root.width, root.preferredTableWidth)
            height: tableColumn.implicitHeight
            color: root.rowBackground
            border.color: root.borderColor
            border.width: 1
            radius: 6
            clip: true

            Column {
                id: tableColumn

                width: parent.width
                spacing: 0

                Repeater {
                    model: root.rows.length + 1

                    delegate: Item {
                        id: tableRow

                        required property int index

                        width: tableColumn.width
                        height: Math.max(34, cellsRepeater.maxImplicitHeight + 17)

                        readonly property bool header: index === 0
                        readonly property var rowCells: root.rowDataAt(index)

                        Rectangle {
                            anchors.fill: parent
                            color: tableRow.header
                                ? root.headerBackground
                                : (tableRow.index % 2 === 0 ? root.alternateRowBackground : root.rowBackground)
                        }

                        Rectangle {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: parent.top
                            height: tableRow.header ? 2 : 0
                            color: root.headerAccent
                            opacity: tableRow.header ? 0.52 : 0
                        }

                        Row {
                            anchors.fill: parent

                            Repeater {
                                id: cellsRepeater

                                model: root.columnCount

                                property real maxImplicitHeight: 0
                                function recomputeMaxImplicitHeight() {
                                    let value = 0
                                    for (let i = 0; i < count; ++i) {
                                        const item = itemAt(i)
                                        if (item && item.cellTextItem) {
                                            value = Math.max(value, item.cellTextItem.contentHeight)
                                        }
                                    }
                                    maxImplicitHeight = value
                                }

                                delegate: Item {
                                    id: cellFrame

                                    required property int index

                                    property alias cellTextItem: cellText

                                    width: root.columnWidth(index)
                                    height: tableRow.height

                                    readonly property var cell: index < tableRow.rowCells.length ? tableRow.rowCells[index] : ({})

                                    TextEdit {
                                        id: cellText

                                        x: 11
                                        y: 7
                                        width: parent.width - 22
                                        height: contentHeight
                                        readOnly: true
                                        selectByMouse: true
                                        wrapMode: TextEdit.WrapAnywhere
                                        textFormat: TextEdit.RichText
                                        text: cellFrame.cell.html || ""
                                        color: tableRow.header ? root.textColor : root.textColor
                                        selectedTextColor: "#ffffff"
                                        selectionColor: root.accentColor
                                        horizontalAlignment: root.cellAlignment(cellFrame.index)
                                        font.pixelSize: root.textPixelSize
                                        font.weight: tableRow.header ? Font.DemiBold : Font.Normal
                                        renderType: Text.NativeRendering

                                        Component.onCompleted: cellsRepeater.recomputeMaxImplicitHeight()
                                        onContentHeightChanged: cellsRepeater.recomputeMaxImplicitHeight()
                                    }

                                    onWidthChanged: cellsRepeater.recomputeMaxImplicitHeight()

                                    Rectangle {
                                        anchors.left: parent.left
                                        anchors.top: parent.top
                                        anchors.bottom: parent.bottom
                                        width: cellFrame.index === 0 ? 0 : 1
                                        color: root.borderColor
                                        opacity: 0.62
                                    }
                                }
                            }
                        }

                        Rectangle {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                            height: tableRow.index === root.rows.length ? 0 : 1
                            color: root.borderColor
                            opacity: 0.66
                        }
                    }
                }
            }
        }

        ScrollBar.horizontal: ScrollBar {
            id: horizontalBar

            policy: tableFlick.contentWidth > tableFlick.width ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
        }
    }
}
