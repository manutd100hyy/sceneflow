import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14

Item {
    id: pane

    RowLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 12

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredWidth: 360
            ComboBox {
                id: formBox
                Layout.fillWidth: true
                model: app.formCatalog()
                textRole: "title"
                onActivated: app.formId = model[currentIndex].id
            }
            RowLayout {
                Button { text: "上一页"; enabled: app.formPage > 0; onClicked: app.formPage = app.formPage - 1 }
                Label { text: "第 " + (app.formPage + 1) + " 页"; Layout.fillWidth: true; horizontalAlignment: Text.AlignHCenter }
                Button { text: "下一页"; onClicked: app.formPage = app.formPage + 1 }
            }
            ComboBox {
                id: paper
                Layout.fillWidth: true
                model: ["A4", "A3"]
            }
            Button {
                text: "导出矢量 PDF"
                highlighted: true
                onClicked: {
                    var p = pane.parent
                    while (p && !p.openFormPdf)
                        p = p.parent
                    if (p)
                        p.openFormPdf(paper.currentIndex)
                }
            }
            Label { text: "版式沿用现行勘察笔录、询问/讯问笔录和简易程序认定书。导出的文字是矢量，底图为现行扫描表格。签名板不在本版范围内，签名位可改在预览中留空。"; wrapMode: Text.WordWrap; color: "#5c6e78"; font.pixelSize: 12; Layout.fillWidth: true }
            ListView {
                id: fields
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                spacing: 8
                model: app.currentFields()
                delegate: Column {
                    width: fields.width
                    spacing: 2
                    Label { text: modelData.label || modelData.key; color: "#203542"; font.pixelSize: 13 }
                    ComboBox {
                        visible: modelData.options && modelData.options.length > 0 && modelData.kind !== "check"
                        width: parent.width
                        model: modelData.options
                        editable: true
                        editText: modelData.value || ""
                        onActivated: app.setFormValue(modelData.key, currentText)
                        onAccepted: app.setFormValue(modelData.key, editText)
                    }
                    CheckBox {
                        visible: modelData.kind === "check"
                        text: "勾选"
                        checked: modelData.value === "√"
                        onToggled: app.setFormValue(modelData.key, checked ? "√" : "")
                    }
                    TextField {
                        visible: modelData.kind !== "check" && !(modelData.options && modelData.options.length > 0)
                        width: parent.width
                        text: modelData.value || ""
                        selectByMouse: true
                        onEditingFinished: app.setFormValue(modelData.key, text)
                    }
                }
            }
        }

        Rectangle {
            visible: pane.width > 800
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#e7eeed"
            border.color: "#d5e0df"
            Image {
                anchors.fill: parent
                anchors.margins: 12
                fillMode: Image.PreserveAspectFit
                source: "image://formpage/" + app.formId + "/" + app.formPage + "/" + app.formRevision
                cache: false
            }
        }
    }
}
