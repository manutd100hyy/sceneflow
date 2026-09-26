import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14

Rectangle {
    id: root
    color: "white"
    signal close()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 10
        RowLayout {
            Label { text: "已有案例"; font.pixelSize: 20; font.bold: true; color: "#203542"; Layout.fillWidth: true }
            Button { text: "关闭"; onClicked: root.close() }
        }
        TextField {
            id: query
            placeholderText: "搜索案号、时间、地点、当事人、号牌、办案人"
            Layout.fillWidth: true
            selectByMouse: true
            onTextChanged: app.refreshCases(text)
        }
        Label {
            visible: app.cases.length === 0
            text: "没有找到案例。新建一份，或把案例文件夹放回数据目录后点「重建索引」。"
            wrapMode: Text.WordWrap
            color: "#5c6e78"
            Layout.fillWidth: true
        }
        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 8
            model: app.cases
            delegate: Rectangle {
                width: ListView.view.width
                height: 92
                radius: 8
                border.color: "#d5e0df"
                color: "#fbfefd"
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    Label { text: modelData.name || "未命名"; font.bold: true; color: "#203542"; elide: Text.ElideRight; Layout.fillWidth: true }
                    Label {
                        text: (modelData.edition === "aerial" ? "航拍 · " : "草图 · ")
                              + (modelData.accidentTime || "时间未填") + "  "
                              + (modelData.location || "地点未填")
                        color: "#5c6e78"
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                    Label {
                        text: [modelData.caseNumber, modelData.parties, modelData.plates, modelData.officer].filter(function(s){ return s }).join("  ")
                        color: "#7d8d94"
                        font.pixelSize: 12
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }
                Row {
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.margins: 6
                    spacing: 4
                    Button { text: "打开"; onClicked: { app.openCase(modelData.id); root.close() } }
                    Button { text: "复制"; onClicked: app.duplicateCase(modelData.id) }
                    Button { text: "删除"; onClicked: app.deleteCase(modelData.id) }
                }
            }
        }
        Button { text: "从文件夹重建索引"; onClicked: app.rebuildIndex() }
    }
}
