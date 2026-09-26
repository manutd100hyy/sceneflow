import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14
import SketchRoad 1.0

Item {
    id: editor

    readonly property bool phone: width < 760
    readonly property bool desktop: width >= 1100
    property bool libraryOpen: false
    property int libraryTab: 1
    property string libraryCategory: "常用"

    function host() {
        var p = parent
        while (p && !p.openAerialDialog)
            p = p.parent
        return p
    }
    function toolModel() {
        return [
            ({ tool: 0, text: "选择", glyph: "select" }),
            ({ tool: 1, text: "平移", glyph: "pan" }),
            ({ tool: 2, text: "道路", glyph: "road", library: 0 }),
            ({ tool: 3, text: "图符", glyph: "symbol", library: 1 }),
            ({ tool: 6, text: "标注", glyph: "dimension", library: 2 }),
            ({ tool: 4, text: "痕迹", glyph: "trace" }),
            ({ tool: 5, text: "散落", glyph: "debris" }),
            ({ tool: 7, text: "文字", glyph: "text" }),
            ({ tool: 8, text: "横道", glyph: "crosswalk" }),
            ({ tool: 9, text: "箭头", glyph: "guide" }),
            ({ tool: 10, text: "环岛", glyph: "circle" }),
            ({ tool: 11, text: "橡皮", glyph: "eraser" })
        ]
    }
    function showLibrary(tab) {
        libraryTab = tab
        libraryCategory = "常用"
        if (!desktop)
            libraryOpen = true
    }

    Item {
        id: frame
        anchors.fill: parent

        Rectangle {
            id: toolBar
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: phone ? 52 : 58
            color: "white"
            RowLayout {
                anchors.fill: parent
                anchors.margins: 7
                spacing: 6
                Button {
                    text: "图符库"
                    highlighted: !desktop && libraryOpen
                    visible: !desktop
                    onClicked: libraryOpen = !libraryOpen
                }
                Flickable {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    contentWidth: tools.width
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds
                    Row {
                        id: tools
                        spacing: 5
                        anchors.verticalCenter: parent.verticalCenter
                        Repeater {
                            // 括号里的对象字面量。Qt 5.14 的 QML 解析器会把裸的 { 当成对象，逗号处报 Expected token。
                            model: editor.toolModel()
                            delegate: Button {
                                id: toolBtn
                                highlighted: modelData.library === undefined && app.tool === modelData.tool
                                onClicked: {
                                    if (modelData.library !== undefined)
                                        editor.showLibrary(modelData.library)
                                    else if (modelData.tool === 3)
                                        app.openSymbolLibrary()
                                    else
                                        app.tool = modelData.tool
                                }
                                width: phone ? 52 : 64
                                height: phone ? 40 : 44
                                contentItem: Row {
                                    spacing: 4
                                    anchors.centerIn: parent
                                    ToolGlyph {
                                        name: modelData.glyph
                                        ink: toolBtn.highlighted ? "#ffffff" : "#117b70"
                                        width: 16
                                        height: 16
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                    Text {
                                        text: modelData.text
                                        color: toolBtn.highlighted ? "#ffffff" : "#203542"
                                        font.pixelSize: 12
                                        font.family: "WenQuanYi Micro Hei"
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                }
                            }
                        }
                        Button {
                            id: calibBtn
                            visible: app.edition === "aerial" || app.hasAerial
                            highlighted: app.tool === 12
                            text: "标定"
                            onClicked: app.tool = 12
                            height: phone ? 40 : 44
                        }
                    }
                }
                Button {
                    text: "属性"
                    highlighted: propDrawer.visible
                    onClicked: {
                        if (desktop)
                            showProperties = !showProperties
                        else
                            propDrawer.open()
                    }
                }
            }
        }
        Rectangle {
            anchors.top: toolBar.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: 1
            color: "#e1e7e5"
        }

        RowLayout {
            anchors.top: toolBar.bottom
            anchors.topMargin: 1
            anchors.bottom: zoomBar.top
            anchors.left: parent.left
            anchors.right: parent.right
            spacing: 0
            clip: true

            Rectangle {
                visible: desktop && showLibrary
                Layout.preferredWidth: 260
                Layout.fillHeight: true
                color: "white"
                LibraryColumn {
                    anchors.fill: parent
                    host: editor
                    libraryTab: editor.libraryTab
                    category: editor.libraryCategory
                }
            }

            SceneCanvas {
                id: canvas
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumHeight: 80
                Component.onCompleted: {
                    app.attachCanvas(canvas)
                    Qt.callLater(function() { app.fit() })
                }
            }

            Rectangle {
                visible: desktop && showProperties
                Layout.preferredWidth: 250
                Layout.fillHeight: true
                color: "white"
                PropertyColumn { anchors.fill: parent }
            }
        }

        Rectangle {
            id: phoneBar
            visible: phone
            z: 3
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: phone ? 52 : 0
            color: "white"
            RowLayout {
                anchors.fill: parent
                anchors.margins: 6
                spacing: 6
                Button { text: "图符"; Layout.fillWidth: true; Layout.fillHeight: true; onClicked: { editor.showLibrary(1); app.openSymbolLibrary() } }
                Button { text: "标注"; Layout.fillWidth: true; Layout.fillHeight: true; onClicked: editor.showLibrary(2) }
                Button { text: "属性"; Layout.fillWidth: true; Layout.fillHeight: true; onClicked: propDrawer.open() }
                Button { text: "更多"; Layout.fillWidth: true; Layout.fillHeight: true; onClicked: moreDrawer.open() }
            }
        }

        Rectangle {
            id: zoomBar
            z: 3
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: phone ? phoneBar.top : parent.bottom
            height: 50
            color: "white"
            Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: "#e1e7e5" }
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 8
                spacing: 4
                Label {
                    text: app.statusText
                    color: "#71818a"
                    font.pixelSize: 12
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }
                Button { text: "−"; implicitWidth: 44; onClicked: app.zoomOut() }
                Label { text: app.zoomText; color: "#40545e"; Layout.minimumWidth: 48; horizontalAlignment: Text.AlignHCenter }
                Button { text: "+"; implicitWidth: 44; onClicked: app.zoomIn() }
                Button { text: "适合"; onClicked: app.fit() }
            }
        }
    }

    property bool showLibrary: true
    property bool showProperties: true

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
        MouseArea { anchors.fill: parent; onClicked: libraryOpen = false }
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
        LibraryColumn {
            anchors.fill: parent
            z: 1
            host: editor
            libraryTab: editor.libraryTab
            category: editor.libraryCategory
        }
    }

    Drawer {
        id: propDrawer
        edge: Qt.BottomEdge
        width: editor.width
        height: Math.min(editor.height * 0.7, 460)
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
            Button { text: "适合画面"; onClicked: app.fit() }
            Button { text: "按实测比例化"; onClicked: app.proportionalize() }
            Button { text: "整体按实测缩放"; onClicked: app.scaleToMeasures() }
            Button { text: "导入航拍照片"; onClicked: { var h = editor.host(); if (h) h.openAerialDialog() } }
            Button { text: "清除图面"; onClicked: app.clearDrawings() }
        }
    }
}
