import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14

ScrollView {
    id: pane
    clip: true
    contentWidth: availableWidth

    ColumnLayout {
        width: pane.availableWidth
        spacing: 12

        GridLayout {
            columns: pane.width < 700 ? 1 : 2
            columnSpacing: 16
            rowSpacing: 8
            Layout.fillWidth: true
            Layout.margins: 16

            Label { text: "案例名称" }
            TextField { text: app.meta.name || ""; Layout.fillWidth: true; onEditingFinished: app.setMetaField("name", text) }
            Label { text: "案件编号" }
            TextField { text: app.meta.caseNumber || ""; Layout.fillWidth: true; onEditingFinished: app.setMetaField("caseNumber", text) }
            Label { text: "事故时间" }
            TextField { text: app.meta.accidentTime || ""; placeholderText: "2026-09-25 14:30"; Layout.fillWidth: true; onEditingFinished: app.setMetaField("accidentTime", text) }
            Label { text: "事故地点" }
            TextField { text: app.meta.location || ""; Layout.fillWidth: true; onEditingFinished: app.setMetaField("location", text) }
            Label { text: "天气" }
            ComboBox {
                Layout.fillWidth: true
                model: ["晴", "阴", "雨", "雪", "雾", "大风"]
                currentIndex: Math.max(0, model.indexOf(app.meta.weather || "晴"))
                onActivated: app.setMetaField("weather", currentText)
            }
            Label { text: "路面" }
            ComboBox {
                Layout.fillWidth: true
                model: ["沥青", "水泥", "沙石", "土路", "其他"]
                currentIndex: Math.max(0, model.indexOf(app.meta.roadSurface || "沥青"))
                onActivated: app.setMetaField("roadSurface", currentText)
            }
            Label { text: "办案民警" }
            TextField { text: app.meta.officer || ""; Layout.fillWidth: true; onEditingFinished: app.setMetaField("officer", text) }
            Label { text: "绘图人" }
            TextField { text: app.meta.drawer || ""; Layout.fillWidth: true; onEditingFinished: app.setMetaField("drawer", text) }
            Label { text: "勘察单位" }
            TextField { text: app.meta.surveyOrg || ""; Layout.fillWidth: true; onEditingFinished: app.setMetaField("surveyOrg", text) }
        }

        Label { text: "当事人"; font.bold: true; Layout.leftMargin: 16 }
        Repeater {
            model: app.parties
            delegate: GridLayout {
                columns: pane.width < 700 ? 1 : 4
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                TextField { placeholderText: "姓名"; text: modelData.name || ""; Layout.fillWidth: true; onEditingFinished: app.setParty(index, "name", text) }
                TextField { placeholderText: "号牌"; text: modelData.plate || ""; Layout.fillWidth: true; onEditingFinished: app.setParty(index, "plate", text) }
                TextField { placeholderText: "电话"; text: modelData.phone || ""; Layout.fillWidth: true; onEditingFinished: app.setParty(index, "phone", text) }
                TextField { placeholderText: "证件号"; text: modelData.idNo || ""; Layout.fillWidth: true; onEditingFinished: app.setParty(index, "idNo", text) }
            }
        }
        Button { text: "添加当事人"; Layout.leftMargin: 16; enabled: app.parties.length < 5; onClicked: app.addParty() }
        Label { text: "这些字段会写入案例索引，并同步到勘察笔录和认定书。"; color: "#7d8d94"; wrapMode: Text.WordWrap; Layout.fillWidth: true; Layout.leftMargin: 16; Layout.rightMargin: 16; Layout.bottomMargin: 20 }
    }
}
