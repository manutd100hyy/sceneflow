import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14

Item {
    id: col

    property Item host: null
    property int libraryTab: 1
    property string category: "常用"
    property var gridModel: []

    function tab() { return host ? host.libraryTab : libraryTab }
    function cat() { return host ? host.libraryCategory : category }
    function setTab(index) {
        if (host) {
            host.libraryTab = index
            host.libraryCategory = "常用"
        } else {
            libraryTab = index
            category = "常用"
        }
    }
    function setCat(name) {
        if (host)
            host.libraryCategory = name
        else
            category = name
    }

    function groupIndex(name) {
        for (var i = 0; i < app.groups.length; ++i) {
            if (app.groups[i].name === name)
                return i
        }
        return 0
    }
    function symbolsOf(names) {
        var out = []
        var seen = {}
        for (var n = 0; n < names.length; ++n) {
            var items = app.searchLibrary("", groupIndex(names[n]))
            for (var i = 0; i < items.length; ++i) {
                var key = String(items[i].name) + "\n" + String(items[i].uuid)
                if (seen[key])
                    continue
                seen[key] = true
                out.push(items[i])
            }
        }
        return out
    }
    function categories() {
        if (tab() === 0)
            return ["常用", "模板", "构造"]
        if (tab() === 1)
            return ["常用", "车辆", "痕迹", "设施", "导向"]
        return ["常用", "工具"]
    }
    function annotationTools() {
        var common = [
            ({ name: "距离标注", tool: 6, style: 0, glyph: "dimension" }),
            ({ name: "文字", tool: 7, style: -1, glyph: "text" }),
            ({ name: "痕迹", tool: 4, style: -1, glyph: "trace" }),
            ({ name: "散落物", tool: 5, style: -1, glyph: "debris" })
        ]
        var tools = [
            ({ name: "直角标注", tool: 6, style: 1, glyph: "dimension" }),
            ({ name: "皮尺", tool: 6, style: 3, glyph: "dimension" }),
            ({ name: "人行横道", tool: 8, style: -1, glyph: "crosswalk" }),
            ({ name: "导向箭头", tool: 9, style: -1, glyph: "guide" }),
            ({ name: "环岛", tool: 10, style: -1, glyph: "circle" }),
            ({ name: "橡皮", tool: 11, style: -1, glyph: "eraser" })
        ]
        if (app.edition === "aerial" || app.hasAerial)
            tools.push(({ name: "航拍标定", tool: 12, style: -1, glyph: "calibrate" }))
        return cat() === "工具" ? tools : common
    }
    function reload() {
        var q = filter.text
        var out = []
        if (tab() === 2) {
            var tools = annotationTools()
            for (var t = 0; t < tools.length; ++t) {
                if (q.length === 0 || String(tools[t].name).indexOf(q) >= 0)
                    out.push(tools[t])
            }
            gridModel = out
            return
        }
        if (tab() === 0 && cat() !== "构造") {
            var commonNames = ["十字路口", "丁字路口", "市内路段", "S型弯路", "弯路", "五岔路口"]
            var src = app.templates
            for (var i = 0; i < src.length; ++i) {
                var name = String(src[i].name)
                if (q.length > 0 && name.indexOf(q) < 0)
                    continue
                if (q.length === 0 && cat() === "常用" && commonNames.indexOf(name) < 0)
                    continue
                var row = src[i]
                row.kind = "template"
                out.push(row)
            }
            if (q.length === 0 && cat() === "常用") {
                out.sort(function(a, b) {
                    return commonNames.indexOf(a.name) - commonNames.indexOf(b.name)
                })
            }
            gridModel = out
            return
        }
        if (q.length > 0) {
            var found = app.searchLibrary(q, 0)
            for (var s = 0; s < found.length; ++s) {
                var hit = found[s]
                hit.kind = "symbol"
                out.push(hit)
            }
            if (tab() === 0) {
                for (var r = 0; r < app.templates.length; ++r) {
                    if (String(app.templates[r].name).indexOf(q) >= 0) {
                        var tpl = app.templates[r]
                        tpl.kind = "template"
                        out.push(tpl)
                    }
                }
            }
            gridModel = out
            return
        }
        var groups = []
        if (tab() === 0)
            groups = ["手绘道路", "手绘道路元素"]
        else if (cat() === "常用")
            groups = ["常用"]
        else if (cat() === "车辆")
            groups = ["交通事故元素"]
        else if (cat() === "痕迹")
            groups = ["痕迹散落物"]
        else if (cat() === "设施")
            groups = ["地面物体图形符号", "道路结构及安全设施"]
        else
            groups = ["交通现象"]
        var rows = symbolsOf(groups)
        for (var k = 0; k < rows.length; ++k) {
            rows[k].kind = "symbol"
            out.push(rows[k])
        }
        gridModel = out
    }

    function activate(item) {
        if (item.kind === "template" || item.id) {
            if (item.id)
                app.placeTemplate(item.id)
            return
        }
        if (item.tool !== undefined && item.notification === undefined) {
            if (item.style >= 0)
                app.setDimensionStyle(item.style)
            app.tool = item.tool
            return
        }
        app.activateLibrary(item.name, item.notification || "", item.flag || 0, item.uuid || "")
    }

    Connections {
        target: app
        onSymbolLibraryRequested: {
            col.setTab(1)
            filter.text = ""
            reload()
        }
    }
    onLibraryTabChanged: reload()
    onCategoryChanged: reload()
    Component.onCompleted: reload()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            spacing: 4
            Repeater {
                model: ["道路", "图符", "标注"]
                Button {
                    text: modelData
                    Layout.fillWidth: true
                    implicitHeight: 36
                    font.pixelSize: 13
                    highlighted: col.tab() === index
                    onClicked: col.setTab(index)
                }
            }
        }
        TextField {
            id: filter
            placeholderText: "搜索图符"
            Layout.fillWidth: true
            selectByMouse: true
            onTextChanged: col.reload()
        }
        Flow {
            Layout.fillWidth: true
            Layout.preferredHeight: childrenRect.height
            spacing: 5
            Repeater {
                model: col.categories()
                Button {
                    text: modelData
                    implicitHeight: 32
                    font.pixelSize: 12
                    highlighted: col.cat() === modelData
                    onClicked: col.setCat(modelData)
                }
            }
        }
        GridView {
            id: symbolGrid
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            cellWidth: width > 0 ? Math.floor(width / 2) : 110
            cellHeight: 94
            model: col.gridModel
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
            delegate: Item {
                width: symbolGrid.cellWidth
                height: symbolGrid.cellHeight
                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 3
                    radius: 5
                    color: cellArea.containsMouse ? "#edf5f1" : "white"
                    border.color: cellArea.containsMouse ? "#117b70" : "#dfe5e3"
                    Column {
                        anchors.fill: parent
                        anchors.margins: 4
                        spacing: 2
                        Item {
                            width: parent.width
                            height: 48
                            IconThumb {
                                anchors.fill: parent
                                visible: (modelData.icon && String(modelData.icon).length > 0) || !modelData.glyph
                                icon: modelData.icon || ""
                                label: modelData.name || ""
                            }
                            ToolGlyph {
                                visible: (!modelData.icon || String(modelData.icon).length === 0) && !!modelData.glyph
                                anchors.centerIn: parent
                                width: 28
                                height: 28
                                name: modelData.glyph || ""
                                ink: "#117b70"
                            }
                        }
                        Label {
                            width: parent.width
                            text: modelData.name || ""
                            horizontalAlignment: Text.AlignHCenter
                            elide: Text.ElideRight
                            font.pixelSize: 12
                            color: "#304553"
                        }
                    }
                    MouseArea {
                        id: cellArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: col.activate(modelData)
                    }
                }
            }
        }
        Label {
            visible: symbolGrid.count === 0
            text: "没有匹配的图符"
            color: "#71818a"
        }
        Rectangle { Layout.fillWidth: true; height: 1; color: "#e2e8e5" }
        Label {
            text: "对象列表  ·  " + app.objectCount
            font.bold: true
            color: "#40545e"
        }
        ListView {
            Layout.fillWidth: true
            Layout.preferredHeight: 104
            clip: true
            model: app.objects
            boundsBehavior: Flickable.StopAtBounds
            delegate: ItemDelegate {
                width: ListView.view.width
                height: 32
                text: modelData.title || modelData.type
                highlighted: app.selection && app.selection.id === modelData.id
                onClicked: app.selectObject(modelData.id)
            }
        }
    }
}
