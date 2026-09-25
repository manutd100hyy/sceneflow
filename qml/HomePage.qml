import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14

Item {
    id: home

    Rectangle { anchors.fill: parent; color: "#f4f7f6" }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: home.width < 700 ? 16 : 28
        spacing: 16

        RowLayout {
            Layout.fillWidth: true
            Label { text: "SR"; color: "#117b70"; font.pixelSize: 26; font.bold: true }
            Label { text: "SketchRoad"; color: "#203542"; font.pixelSize: 22; font.bold: true; Layout.fillWidth: true }
            Button { text: "使用说明"; flat: true; onClicked: helpDialog.open() }
        }

        Item { Layout.fillHeight: true; Layout.minimumHeight: 8 }

        Label {
            text: "开始一份现场记录"
            color: "#203542"
            font.pixelSize: home.width < 700 ? 28 : 40
            font.bold: true
            Layout.alignment: Qt.AlignHCenter
        }
        Label {
            text: "创建新案例，或打开已有案例继续工作"
            color: "#5c6e78"
            font.pixelSize: 16
            Layout.alignment: Qt.AlignHCenter
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 8
            Button {
                text: "草图版"
                highlighted: app.edition === "sketch"
                onClicked: app.edition = "sketch"
                Layout.preferredHeight: 44
            }
            Button {
                text: "航拍版"
                highlighted: app.edition === "aerial"
                onClicked: app.edition = "aerial"
                Layout.preferredHeight: 44
            }
        }
        Label {
            text: app.edition === "aerial"
                  ? "航拍版：导入无人机照片，标定比例后在照片上绘图。"
                  : "草图版：按道路模板或手绘车道，放置图符、痕迹和标注。"
            color: "#5c6e78"
            wrapMode: Text.WordWrap
            Layout.maximumWidth: 560
            Layout.alignment: Qt.AlignHCenter
            horizontalAlignment: Text.AlignHCenter
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 16
            Button {
                text: "＋ 新建案例"
                highlighted: true
                Layout.preferredWidth: home.width < 700 ? 140 : 200
                Layout.preferredHeight: 52
                onClicked: createDialog.open()
            }
            Button {
                text: "打开已有案例"
                Layout.preferredWidth: home.width < 700 ? 140 : 200
                Layout.preferredHeight: 52
                onClicked: { app.refreshCases(""); app.showCases = true; caseLayer.open() }
            }
        }

        Column {
            Layout.alignment: Qt.AlignHCenter
            spacing: 6
            Label { text: "先体验一下"; color: "#203542"; font.bold: true; anchors.horizontalCenter: parent.horizontalCenter }
            Label { text: "通过示例了解绘图与标注"; color: "#5c6e78"; anchors.horizontalCenter: parent.horizontalCenter }
            Button {
                text: "打开示例"
                anchors.horizontalCenter: parent.horizontalCenter
                onClicked: app.openSample()
            }
        }

        Item { Layout.fillHeight: true }
        Label {
            text: "案例保存在本机  " + app.dataPath
            color: "#7d8d94"
            font.pixelSize: 12
            elide: Text.ElideMiddle
            Layout.fillWidth: true
        }
    }

    Dialog {
        id: createDialog
        modal: true
        title: "新建案例"
        anchors.centerIn: parent
        width: Math.min(home.width - 24, 520)
        standardButtons: Dialog.NoButton
        contentItem: ColumnLayout {
            spacing: 12
            Label { text: "案例名称"; color: "#203542" }
            TextField { id: caseName; text: "未命名案例001"; selectByMouse: true; Layout.fillWidth: true }
            Label { text: "起始画布"; color: "#203542" }
            RowLayout {
                Layout.fillWidth: true
                Repeater {
                    model: ["空白绘图", "十字路口", "丁字路口"]
                    Button {
                        text: modelData
                        Layout.fillWidth: true
                        Layout.preferredHeight: 64
                        highlighted: templatePick.currentIndex === index
                        onClicked: templatePick.currentIndex = index
                    }
                }
            }
            ComboBox {
                id: templatePick
                Layout.fillWidth: true
                model: ["空白绘图", "十字路口", "丁字路口"].concat(extraNames())
                function extraNames() {
                    var names = []
                    for (var i = 0; i < app.templates.length; ++i) {
                        var n = app.templates[i].name
                        if (names.indexOf(n) < 0 && n.indexOf("十字") < 0 && n.indexOf("丁字") < 0)
                            names.push(n)
                    }
                    return names
                }
            }
            Label { text: "时间、地点等信息可进入后补充"; color: "#7d8d94"; font.pixelSize: 12 }
            RowLayout {
                Layout.alignment: Qt.AlignRight
                Button { text: "取消"; onClicked: createDialog.close() }
                Button {
                    text: "创建并绘图"
                    highlighted: true
                    onClicked: {
                        var choice = templatePick.currentText
                        if (choice === "空白绘图")
                            choice = ""
                        app.newCase(caseName.text, choice)
                        createDialog.close()
                    }
                }
            }
        }
    }

    Dialog {
        id: helpDialog
        modal: true
        title: "使用说明"
        x: Math.max(12, (home.width - width) / 2)
        y: Math.max(12, (home.height - height) / 2)
        width: Math.min(home.width - 24, 560)
        standardButtons: Dialog.Ok
        contentItem: Label {
            width: Math.min(home.width - 72, 512)
            wrapMode: Text.WordWrap
            text: "草图版用来画道路交通事故现场图。先放道路模板或手绘车道，再放置车辆、行人和设施，接着画痕迹、散落物，并用标注记下实测距离。填写实测米数后点「比例化」，草图会按实测调整。\n\n航拍版在此基础上导入无人机照片。在照片上点两个已知距离的位置，输入米数完成标定，之后的绘图与导出都使用米。\n\n文书按现行勘察笔录、询问笔录、讯问笔录和简易程序认定书版式导出矢量 PDF，可选 A4 或 A3。现场图同样导出矢量 PDF。"
            color: "#203542"
        }
    }

    Popup {
        id: caseLayer
        modal: true
        width: Math.min(home.width - 16, 640)
        height: Math.min(home.height - 40, 640)
        anchors.centerIn: parent
        padding: 0
        CaseList {
            anchors.fill: parent
            onClose: caseLayer.close()
        }
    }
}
