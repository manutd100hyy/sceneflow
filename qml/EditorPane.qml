import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14
import SketchRoad 1.0

Item {
    id: editor

    readonly property bool phone: width < 760
    readonly property bool desktop: width >= 1100
    readonly property var sel: app.selection

    function host() {
        var p = parent
        while (p && !p.openAerialDialog)
            p = p.parent
        return p
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            visible: desktop
            Layout.preferredWidth: 258
            Layout.fillHeight: true
            Layout.margins: 8
            radius: 12
            color: "white"
            border.color: "#d5e4e0"
            clip: true
            LibraryColumn { anchors.fill: parent }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 8

            ScrollView {
                Layout.fillWidth: true
                Layout.preferredHeight: phone ? 64 : 72
                Layout.leftMargin: 8
                Layout.rightMargin: 8
                Layout.topMargin: 8
                clip: true
                Row {
                    spacing: 6
                    leftPadding: 2
                    topPadding: 2
                    Repeater {
                        model: [
                            { tool: 0, text: "选择", glyph: "select" },
                            { tool: 1, text: "平移", glyph: "pan" },
                            { tool: 2, text: "道路", glyph: "road" },
                            { tool: 3, text: "图符", glyph: "symbol" },
                            { tool: 4, text: "痕迹", glyph: "trace" },
                            { tool: 5, text: "散落", glyph: "debris" },
                            { tool: 6, text: "标注", glyph: "dimension" },
                            { tool: 7, text: "文字", glyph: "text" },
                            { tool: 8, text: "横道", glyph: "crosswalk" },
                            { tool: 9, text: "箭头", glyph: "guide" },
                            { tool: 10, text: "环岛", glyph: "circle" },
                            { tool: 11, text: "橡皮", glyph: "eraser" }
                        ]
                        delegate: Button {
                            id: toolBtn
                            highlighted: app.tool === modelData.tool
                            onClicked: {
                                if (modelData.tool === 3)
                                    app.openSymbolLibrary()
                                else
                                    app.tool = modelData.tool
                            }
                            width: 58
                            height: phone ? 56 : 62
                            leftPadding: 2
                            rightPadding: 2
                            contentItem: Column {
                                spacing: 2
                                ToolGlyph {
                                    name: modelData.glyph
                                    ink: toolBtn.highlighted ? "#ffffff" : "#117b70"
                                    width: 18
                                    height: 18
                                    anchors.horizontalCenter: parent.horizontalCenter
                                }
                                Text {
                                    text: modelData.text
                                    color: toolBtn.highlighted ? "#ffffff" : "#203542"
                                    font.pixelSize: 11
                                    font.family: "WenQuanYi Micro Hei"
                                    anchors.horizontalCenter: parent.horizontalCenter
                                }
                            }
                        }
                    }
                    Button {
                        id: calibBtn
                        visible: app.edition === "aerial" || app.hasAerial
                        highlighted: app.tool === 12
                        onClicked: app.tool = 12
                        width: 58
                        height: phone ? 56 : 62
                        contentItem: Column {
                            spacing: 2
                            ToolGlyph {
                                name: "calibrate"
                                ink: calibBtn.highlighted ? "#ffffff" : "#117b70"
                                width: 18
                                height: 18
                                anchors.horizontalCenter: parent.horizontalCenter
                            }
                            Text {
                                text: "标定"
                                color: calibBtn.highlighted ? "#ffffff" : "#203542"
                                font.pixelSize: 11
                                anchors.horizontalCenter: parent.horizontalCenter
                            }
                        }
                    }
                }
            }

            SceneCanvas {
                id: canvas
                Layout.fillWidth: true
                Layout.fillHeight: true
                Component.onCompleted: {
                    app.attachCanvas(canvas)
                    Qt.callLater(function() { app.fit() })
                }
            }

            Rectangle {
                visible: phone
                Layout.fillWidth: true
                Layout.preferredHeight: 58
                color: "white"
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 6
                    spacing: 6
                    Button { text: "图符"; Layout.fillWidth: true; Layout.fillHeight: true; onClicked: app.openSymbolLibrary() }
                    Button { text: "标注"; Layout.fillWidth: true; Layout.fillHeight: true; highlighted: app.tool === 6; onClicked: app.tool = 6 }
                    Button { text: "属性"; Layout.fillWidth: true; Layout.fillHeight: true; onClicked: propDrawer.open() }
                    Button { text: "更多"; Layout.fillWidth: true; Layout.fillHeight: true; onClicked: moreDrawer.open() }
                }
            }
        }

        Rectangle {
            visible: desktop
            Layout.preferredWidth: 292
            Layout.fillHeight: true
            Layout.margins: 8
            radius: 12
            color: "white"
            border.color: "#d5e4e0"
            clip: true
            PropertyColumn { anchors.fill: parent; anchors.margins: 4 }
        }
    }

    Button {
        visible: !desktop && !phone
        text: "库"
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.topMargin: 58
        anchors.leftMargin: 8
        onClicked: libraryOpen = true
        height: 40
        z: 2
    }

    property bool libraryOpen: false

    Connections {
        target: app
        onSymbolLibraryRequested: if (!desktop) libraryOpen = true
        onSymbolLibraryClosed: libraryOpen = false
    }

    Rectangle {
        visible: libraryOpen && !desktop
        anchors.fill: parent
        color: "#66000000"
        z: 4
        MouseArea {
            anchors.fill: parent
            onClicked: libraryOpen = false
        }
    }
    Rectangle {
        visible: libraryOpen && !desktop
        z: 5
        width: Math.min(parent.width * 0.92, 340)
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        color: "white"
        MouseArea { anchors.fill: parent; z: 0 }
        LibraryColumn { anchors.fill: parent; z: 1 }
    }
    Drawer {
        id: propDrawer
        edge: Qt.BottomEdge
        width: editor.width
        height: Math.min(editor.height * 0.62, 420)
        PropertyColumn { anchors.fill: parent }
    }
    Drawer {
        id: moreDrawer
        edge: Qt.BottomEdge
        width: editor.width
        height: 280
        Column {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 8
            Label { text: app.hint; wrapMode: Text.WordWrap; width: parent.width - 32; color: "#203542" }
            Button { text: "适应画面"; onClicked: app.fit() }
            Button { text: "比例化"; onClicked: app.proportionalize() }
            Button { text: "整体按实测缩放"; onClicked: app.scaleToMeasures() }
            Button { text: "导入航拍照片"; onClicked: { var h = editor.host(); if (h) h.openAerialDialog() } }
            Button { text: "清除图面"; onClicked: app.clearDrawings() }
        }
    }
}
