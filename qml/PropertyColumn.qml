import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14

ScrollView {
    id: col
    clip: true

    ColumnLayout {
        width: Math.max(0, col.availableWidth - 8)
        spacing: 8

        Item { Layout.preferredHeight: 8; Layout.fillWidth: true }
        Label {
            text: "画布"
            font.bold: true
            font.pixelSize: 16
            color: "#117b70"
            Layout.leftMargin: 8
            Layout.topMargin: 4
        }
        RowLayout {
            Label { text: "网格" }
            ComboBox {
                Layout.fillWidth: true
                model: ["1", "2", "5", "10"]
                currentIndex: Math.max(0, ["1", "2", "5", "10"].indexOf(String(app.gridMetres)))
                onActivated: app.gridMetres = parseInt(currentText)
            }
        }
        CheckBox { text: "吸附"; checked: app.snapOn; onToggled: app.snapOn = checked }
        Button { text: "适应画面"; onClicked: app.fit() }
        Label { text: app.hint; wrapMode: Text.WordWrap; color: "#5c6e78"; font.pixelSize: 12; Layout.fillWidth: true }

        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#d5e0df" }

        Label {
            visible: !app.selection.id
            text: "选中对象后，这里显示长度、宽度、旋转，以及复制和锁定。"
            wrapMode: Text.WordWrap
            color: "#5c6e78"
            Layout.fillWidth: true
        }

        ColumnLayout {
            visible: !!app.selection.id
            Layout.fillWidth: true
            spacing: 8
            Label { text: app.selection.typeLabel || "对象"; font.bold: true; font.pixelSize: 18; color: "#203542" }
            Label { text: app.selection.name || ""; color: "#5c6e78" }

            Label { text: "编号" }
            TextField {
                Layout.fillWidth: true
                text: app.selection.label || ""
                selectByMouse: true
                onEditingFinished: app.setProp("label", text)
            }
            Label { text: app.selection.type === "debris" ? "面积 (m²)" : "长度 (m)" }
            TextField {
                Layout.fillWidth: true
                text: app.selection.length !== undefined ? Number(app.selection.length).toFixed(2) : ""
                enabled: app.selection.type === "symbol" || app.selection.type === "parking"
                selectByMouse: true
                onEditingFinished: app.setProp("length", text)
            }
            Label { text: "宽度 (m)"; visible: app.selection.type === "symbol" || app.selection.type === "parking" || app.selection.type === "crosswalk" }
            TextField {
                visible: parent.visible && (app.selection.type === "symbol" || app.selection.type === "parking" || app.selection.type === "crosswalk")
                Layout.fillWidth: true
                text: app.selection.width !== undefined ? Number(app.selection.width).toFixed(2) : ""
                selectByMouse: true
                onEditingFinished: app.setProp("width", text)
            }
            Label { text: "旋转 (°)" }
            TextField {
                Layout.fillWidth: true
                text: app.selection.rotation !== undefined ? Number(app.selection.rotation).toFixed(1) : "0"
                selectByMouse: true
                onEditingFinished: app.setProp("rotation", text)
            }
            Label { text: "实测 (m)"; visible: app.selection.type === "dimension" }
            TextField {
                visible: app.selection.type === "dimension"
                Layout.fillWidth: true
                placeholderText: "留空则使用图面长度"
                text: app.selection.measured >= 0 ? String(app.selection.measured) : ""
                selectByMouse: true
                onEditingFinished: app.setProp("measured", text)
            }
            ComboBox {
                visible: app.selection.type === "dimension"
                Layout.fillWidth: true
                model: ["直线", "直角", "混合", "皮尺"]
                currentIndex: app.selection.subType || 0
                onActivated: app.setProp("subType", currentIndex)
            }
            ComboBox {
                visible: app.selection.type === "symbol" && app.selection.styles && app.selection.styles.length > 1
                Layout.fillWidth: true
                model: app.selection.styles || []
                currentIndex: app.selection.styleIndex || 0
                onActivated: app.setProp("styleIndex", currentIndex)
            }
            ComboBox {
                visible: app.selection.type === "roadline"
                Layout.fillWidth: true
                model: ["空线", "单实线", "单虚线", "双实线", "双虚线", "绿化带", "不规则绿化", "隔离桩", "水沟", "干涸水沟", "人行道", "路肩", "停车港湾"]
                currentIndex: Math.max(0, app.selection.lineStyle || 0)
                onActivated: app.setProp("lineStyle", currentIndex)
            }
            CheckBox { text: "基准线"; visible: app.selection.type === "roadline"; checked: !!app.selection.isDatum; onToggled: app.setProp("isDatum", checked) }
            CheckBox { text: "锁定"; checked: !!app.selection.locked; onToggled: app.setProp("locked", checked) }
            RowLayout {
                Button { text: "复制"; onClicked: app.duplicateSelection(); Layout.fillWidth: true }
                Button { text: "删除"; onClicked: app.removeSelection(); Layout.fillWidth: true }
            }
        }

        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#d5e0df" }
        Label { text: "比例化"; font.bold: true; color: "#203542" }
        Label { text: "给标注填写实测米数后，按实测移动关联对象，或按全部实测做整体缩放。"; wrapMode: Text.WordWrap; color: "#5c6e78"; font.pixelSize: 12; Layout.fillWidth: true }
        Button { text: "按实测比例化"; highlighted: true; onClicked: app.proportionalize(); Layout.fillWidth: true }
        Button { text: "整体缩放"; onClicked: app.scaleToMeasures(); Layout.fillWidth: true }

        Label { text: "航拍"; font.bold: true; color: "#203542"; visible: app.edition === "aerial" || app.hasAerial }
        Label { text: app.aerialStatus; wrapMode: Text.WordWrap; color: "#5c6e78"; font.pixelSize: 12; Layout.fillWidth: true; visible: app.edition === "aerial" || app.hasAerial }
        Button {
            visible: app.edition === "aerial" || app.hasAerial
            text: "导入航拍照片"
            onClicked: {
                var p = col.parent
                while (p && !p.openAerialDialog)
                    p = p.parent
                if (p) p.openAerialDialog()
            }
        }
        RowLayout {
            visible: app.hasAerial
            Label { text: "透明度" }
            Slider { from: 0.2; to: 1; value: 0.9; Layout.fillWidth: true; onMoved: app.setOpacity(value) }
        }
        RowLayout {
            visible: app.hasAerial
            TextField { id: known; placeholderText: "两点距离（米）"; Layout.fillWidth: true; selectByMouse: true }
            Button { text: "标定"; onClicked: app.confirmCalibration(parseFloat(known.text)) }
        }
        Button { text: "清除图面（保留底图）"; visible: app.hasAerial; onClicked: app.clearDrawings() }
        Item { Layout.preferredHeight: 12 }
    }
}
