import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14

Item {
    id: col

    property var symbolModel: []
    property var templateModel: []

    function reloadSymbols() {
        symbolModel = app.searchLibrary(filter.text, Math.max(0, groupBox.currentIndex))
    }
    function reloadTemplates() {
        var src = app.templates
        var q = filter.text
        var out = []
        for (var i = 0; i < src.length; ++i) {
            if (q.length === 0 || String(src[i].name).indexOf(q) >= 0)
                out.push(src[i])
        }
        templateModel = out
    }

    Connections {
        target: app
        onSymbolLibraryRequested: {
            kind.currentIndex = 1
            groupBox.currentIndex = 0
            filter.text = ""
            reloadSymbols()
            reloadTemplates()
        }
    }

    Component.onCompleted: {
        reloadSymbols()
        reloadTemplates()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 6
        TextField {
            id: filter
            placeholderText: "搜索图符或模板"
            Layout.fillWidth: true
            selectByMouse: true
            onTextChanged: {
                reloadSymbols()
                reloadTemplates()
            }
        }
        TabBar {
            id: kind
            Layout.fillWidth: true
            currentIndex: 1
            TabButton { text: "道路"; width: implicitWidth }
            TabButton { text: "图符"; width: implicitWidth }
            TabButton { text: "对象"; width: implicitWidth }
        }
        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: kind.currentIndex

            GridView {
                id: templateGrid
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                cellWidth: width > 0 ? Math.floor(width / (width >= 220 ? 3 : 2)) : 80
                cellHeight: 98
                model: col.templateModel
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                delegate: Item {
                    width: templateGrid.cellWidth
                    height: templateGrid.cellHeight
                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: 3
                        radius: 8
                        color: tplMouse.pressed ? "#d7efe9" : "transparent"
                    }
                    Column {
                        anchors.fill: parent
                        anchors.margins: 4
                        spacing: 2
                        IconThumb {
                            icon: modelData.icon || ""
                            label: modelData.name || ""
                            width: 40
                            height: 40
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                        Text {
                            width: parent.width
                            text: modelData.name || ""
                            color: "#203542"
                            font.pixelSize: 11
                            font.family: "WenQuanYi Micro Hei"
                            horizontalAlignment: Text.AlignHCenter
                            wrapMode: Text.WordWrap
                            maximumLineCount: 2
                            elide: Text.ElideRight
                        }
                    }
                    MouseArea {
                        id: tplMouse
                        anchors.fill: parent
                        onClicked: app.placeTemplate(modelData.id)
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                ComboBox {
                    id: groupBox
                    Layout.fillWidth: true
                    model: app.groups
                    textRole: "name"
                    onCurrentIndexChanged: col.reloadSymbols()
                }
                GridView {
                    id: symbolGrid
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds
                    cellWidth: width > 0 ? Math.floor(width / (width >= 220 ? 3 : 2)) : 80
                    cellHeight: 98
                    model: col.symbolModel
                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                    delegate: Item {
                        width: symbolGrid.cellWidth
                        height: symbolGrid.cellHeight
                        Rectangle {
                            anchors.fill: parent
                            anchors.margins: 3
                            radius: 8
                            color: symMouse.pressed ? "#d7efe9" : "transparent"
                        }
                        Column {
                            anchors.fill: parent
                            anchors.margins: 4
                            spacing: 2
                            IconThumb {
                                icon: modelData.icon || ""
                                label: modelData.name || ""
                                width: 40
                                height: 40
                                anchors.horizontalCenter: parent.horizontalCenter
                            }
                            Text {
                                width: parent.width
                                text: modelData.name || ""
                                color: "#203542"
                                font.pixelSize: 11
                                font.family: "WenQuanYi Micro Hei"
                                horizontalAlignment: Text.AlignHCenter
                                wrapMode: Text.WordWrap
                                maximumLineCount: 2
                                elide: Text.ElideRight
                            }
                        }
                        MouseArea {
                            id: symMouse
                            anchors.fill: parent
                            onClicked: app.activateLibrary(modelData.name || "", modelData.notification || "", modelData.flag || 0, modelData.uuid || "")
                        }
                    }
                }
            }

            ListView {
                clip: true
                model: app.objects
                spacing: 2
                Layout.fillWidth: true
                Layout.fillHeight: true
                delegate: ItemDelegate {
                    width: ListView.view.width
                    text: (modelData.locked ? "锁 " : "") + modelData.title
                    highlighted: app.selection.id === modelData.id
                    onClicked: app.selectObject(modelData.id)
                }
            }
        }
        Label {
            text: "车道 " + app.laneCount + " × " + app.laneWidth.toFixed(1) + " m"
            color: "#5c6e78"
            font.pixelSize: 12
        }
        RowLayout {
            SpinBox {
                from: 1; to: 8; value: app.laneCount
                onValueModified: app.laneCount = value
                Layout.fillWidth: true
            }
            SpinBox {
                from: 25; to: 50; value: Math.round(app.laneWidth * 10)
                onValueModified: app.laneWidth = value / 10
                Layout.fillWidth: true
            }
        }
    }
}
