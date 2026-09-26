import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14

Item {
    id: home

    function templateIcon(name) {
        for (var i = 0; i < app.templates.length; ++i) {
            if (app.templates[i].name === name)
                return app.templates[i].icon || ""
        }
        return ""
    }
    function extraTemplateNames() {
        var names = []
        for (var i = 0; i < app.templates.length; ++i) {
            var n = app.templates[i].name
            if (names.indexOf(n) < 0)
                names.push(n)
        }
        return names
    }

    Rectangle { anchors.fill: parent; color: "#f5f7f6" }

    Rectangle {
        id: bar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 68
        color: "white"
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: "#dce3e3" }
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 12
            spacing: 10
            Label { text: "SR"; color: "#117b70"; font.bold: true; font.pixelSize: 24 }
            Label {
                text: "SketchRoad"
                color: "#203542"
                font.pixelSize: home.width < 600 ? 16 : 20
                font.bold: true
                Layout.fillWidth: true
            }
            Button {
                text: app.edition === "aerial" ? "航拍版" : "草图版"
                flat: true
                onClicked: app.edition = app.edition === "aerial" ? "sketch" : "aerial"
            }
            Button { text: "使用说明"; flat: true; onClicked: helpDialog.open() }
        }
    }

    Flickable {
        anchors.top: bar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        contentWidth: width
        contentHeight: 720
        clip: true

        ColumnLayout {
            id: column
            width: Math.min(home.width - 40, 820)
            x: (home.width - width) / 2
            spacing: 22

            Item { Layout.preferredHeight: home.width < 600 ? 20 : 56 }
            Label {
                text: "开始一份现场记录"
                color: "#203542"
                font.pixelSize: home.width < 600 ? 26 : 34
                font.bold: true
            }
            Label {
                text: app.edition === "aerial"
                      ? "航拍版：导入无人机照片，标定比例后在照片上绘图。"
                      : "创建新案例，或打开已有案例继续工作"
                color: "#71818a"
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            GridLayout {
                columns: home.width < 600 ? 1 : 2
                columnSpacing: 20
                rowSpacing: 12
                Layout.fillWidth: true
                Button {
                    text: "＋   新建案例"
                    highlighted: true
                    Layout.fillWidth: true
                    Layout.preferredHeight: 86
                    font.pixelSize: 18
                    onClicked: createDialog.open()
                }
                Button {
                    text: "打开已有案例"
                    Layout.fillWidth: true
                    Layout.preferredHeight: 86
                    font.pixelSize: 18
                    onClicked: { app.refreshCases(""); app.showCases = true; caseLayer.open() }
                }
            }
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 116
                color: "white"
                radius: 8
                border.color: "#dce3e3"
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 20
                    spacing: 16
                    Image {
                        visible: home.width > 600
                        source: home.templateIcon("十字路口")
                        Layout.preferredWidth: 120
                        Layout.preferredHeight: 72
                        fillMode: Image.PreserveAspectFit
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4
                        Label { text: "先体验一下"; font.bold: true; font.pixelSize: 18; color: "#203542" }
                        Label { text: "通过示例了解绘图与标注"; color: "#71818a"; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                    }
                    Button { text: "打开示例 ›"; onClicked: app.openSample() }
                }
            }
            Item { Layout.preferredHeight: 12 }
        }
    }

    Dialog {
        id: createDialog
        modal: true
        title: "新建案例"
        anchors.centerIn: parent
        width: Math.min(home.width - 32, 560)
        implicitHeight: 560
        standardButtons: Dialog.Cancel
        contentItem: ColumnLayout {
            spacing: 14
            Label { text: "案例名称"; color: "#203542" }
            TextField { id: caseName; text: "未命名案例 001"; selectByMouse: true; Layout.fillWidth: true }
            Label { text: "起始画布"; color: "#203542" }
            RowLayout {
                Layout.fillWidth: true
                spacing: 10
                Repeater {
                    model: ["空白绘图", "十字路口", "丁字路口"]
                    Item {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 138
                        Rectangle {
                            anchors.fill: parent
                            radius: 6
                            color: templatePick.currentIndex === index && moreBox.currentIndex === 0 ? "#f0f8f5" : "white"
                            border.color: templatePick.currentIndex === index && moreBox.currentIndex === 0 ? "#117b70" : "#dce3e3"
                            border.width: templatePick.currentIndex === index && moreBox.currentIndex === 0 ? 2 : 1
                        }
                        Column {
                            anchors.fill: parent
                            anchors.margins: 8
                            spacing: 8
                            Item {
                                width: parent.width
                                height: 78
                                Rectangle {
                                    visible: index === 0
                                    anchors.centerIn: parent
                                    width: parent.width * 0.72
                                    height: 68
                                    color: "white"
                                    border.color: "#cdd7d2"
                                }
                                Image {
                                    visible: index !== 0
                                    anchors.fill: parent
                                    source: home.templateIcon(modelData)
                                    fillMode: Image.PreserveAspectFit
                                }
                            }
                            Label {
                                text: modelData
                                width: parent.width
                                horizontalAlignment: Text.AlignHCenter
                                font.pixelSize: 13
                                color: "#304553"
                            }
                        }
                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                templatePick.currentIndex = index
                                moreBox.currentIndex = 0
                            }
                        }
                    }
                }
            }
            ComboBox {
                id: moreBox
                Layout.fillWidth: true
                model: ["使用上方画布"].concat(home.extraTemplateNames())
            }
            Label { text: "时间、地点等信息可进入后补充。"; color: "#71818a"; wrapMode: Text.WordWrap; Layout.fillWidth: true }
            Button {
                text: "创建并绘图"
                highlighted: true
                Layout.fillWidth: true
                Layout.preferredHeight: 44
                onClicked: {
                    var choice = ""
                    if (moreBox.currentIndex > 0)
                        choice = moreBox.currentText
                    else if (templatePick.currentIndex > 0)
                        choice = ["空白绘图", "十字路口", "丁字路口"][templatePick.currentIndex]
                    app.newCase(caseName.text, choice)
                    createDialog.close()
                }
            }
        }
        QtObject { id: templatePick; property int currentIndex: 0 }
    }

    Dialog {
        id: helpDialog
        modal: true
        title: "使用说明"
        implicitWidth: 520
        implicitHeight: 420
        width: Math.min(home.width - 24, 560)
        height: Math.min(home.height - 48, 440)
        x: Math.round((home.width - width) / 2)
        y: Math.round((home.height - height) / 2)
        standardButtons: Dialog.Ok
        padding: 16
        contentItem: Item {
            implicitWidth: 480
            implicitHeight: 280
            Flickable {
                anchors.fill: parent
                contentWidth: width
                contentHeight: helpText.implicitHeight
                clip: true
                Label {
                    id: helpText
                    width: Math.max(160, Math.min(480, home.width - 80))
                    wrapMode: Text.WordWrap
                    text: "首页新建案例或打开示例。现场绘图页左侧是道路、图符和标注，右侧可改画布尺寸（米）和对象属性。画布 100% 时 1 米等于 20 像素，网格为 5 米粗线、1 米细线。右键、中键、双指或平移工具可拖动，滚轮以光标为中心缩放。\n\n草图版按道路模板或手绘车道放置车辆、行人和设施，再画痕迹、散落物，并用标注记下实测距离。填写实测米数后点「按实测比例化」。\n\n航拍版导入无人机照片，点两个已知距离的位置并输入米数完成标定。文书和现场图可导出 A4 或 A3 矢量 PDF。案例保存在本机。"
                    color: "#203542"
                }
            }
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

    Connections {
        target: app
        onCreateDialogRequested: createDialog.open()
        onDismissPopups: {
            createDialog.close()
            helpDialog.close()
            caseLayer.close()
        }
    }
}
