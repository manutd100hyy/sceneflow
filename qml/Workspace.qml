import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14
import Qt.labs.platform 1.1 as Platform
import SketchRoad 1.0

Item {
    id: work

    readonly property bool phone: width < 760
    readonly property bool desktop: width >= 1180
    readonly property var sel: app.selection

    function pathOf(url) {
        var s = decodeURIComponent(String(url))
        if (s.indexOf("file://") === 0)
            s = s.slice(7)
        return s
    }
    function openPhotoDialog(caption) {
        captionField.text = caption || ""
        photoDialog.open()
    }
    function openAerialDialog() { aerialDialog.open() }
    function openScenePdf(paperIndex, scaleIndex) {
        paperBox.currentIndex = paperIndex
        scaleBox.currentIndex = scaleIndex
        scenePdf.open()
    }
    function openFormPdf(paperIndex) {
        formPaperBox.currentIndex = paperIndex
        formPdf.open()
    }

    Rectangle { anchors.fill: parent; color: "#f4f7f6" }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: phone ? 52 : 60
            color: "white"
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                spacing: 6
                Button { text: phone ? "首页" : "‹ 案例首页"; flat: true; onClicked: app.goHome() }
                Label {
                    text: app.caseName
                    font.bold: true
                    font.pixelSize: phone ? 15 : 18
                    color: "#203542"
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
                Button { text: "撤销"; visible: !phone; enabled: app.canUndo; onClicked: app.undo() }
                Button { text: "重做"; visible: !phone; enabled: app.canRedo; onClicked: app.redo() }
                Button { text: "保存"; highlighted: app.dirty; onClicked: app.save() }
                Button { text: "导出"; visible: !phone; onClicked: exportMenu.open() }
                Button { text: "···"; visible: phone; onClicked: exportMenu.open() }
            }
        }
        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#d5e0df" }

        TabBar {
            id: tabs
            Layout.fillWidth: true
            currentIndex: app.activeTab
            onCurrentIndexChanged: if (app.activeTab !== currentIndex) app.activeTab = currentIndex
            TabButton { text: phone ? "信息" : "案例信息" }
            TabButton { text: phone ? "绘图" : "现场绘图" }
            TabButton { text: phone ? "照片" : "照片与记录" }
            TabButton { text: phone ? "文书" : "文书输出" }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabs.currentIndex

            CaseInfoPane { }
            EditorPane { }
            PhotosPane { }
            FormsPane { }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 28
            color: "#eef3f2"
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                Label { text: app.statusText; color: "#5c6e78"; font.pixelSize: 12; Layout.fillWidth: true; elide: Text.ElideRight }
                Label { text: app.zoomText; color: "#203542"; font.pixelSize: 12 }
            }
        }
    }

    Popup {
        id: caseOverlay
        modal: true
        visible: app.showCases
        width: Math.min(work.width - 12, 640)
        height: Math.min(work.height - 24, 640)
        anchors.centerIn: parent
        padding: 0
        onClosed: if (app.showCases) app.showCases = false
        CaseList { anchors.fill: parent; onClose: caseOverlay.close() }
    }

    Menu {
        id: exportMenu
        MenuItem { text: "导出现场图 PDF"; onTriggered: scenePdf.open() }
        MenuItem { text: "导出当前文书 PDF"; onTriggered: formPdf.open() }
        MenuItem { text: "案例列表"; onTriggered: { app.refreshCases(""); app.showCases = true } }
        MenuItem { text: "航拍示例"; onTriggered: app.openAerialSample() }
    }

    Platform.FileDialog {
        id: photoDialog
        title: "选择现场照片"
        nameFilters: ["图片 (*.png *.jpg *.jpeg *.bmp)"]
        onAccepted: app.importPhoto(work.pathOf(file), captionField.text)
    }
    Platform.FileDialog {
        id: aerialDialog
        title: "选择航拍照片"
        nameFilters: ["图片 (*.png *.jpg *.jpeg *.bmp)"]
        onAccepted: app.importAerial(work.pathOf(file))
    }
    Platform.FileDialog {
        id: scenePdf
        title: "导出现场图"
        fileMode: Platform.FileDialog.SaveFile
        nameFilters: ["PDF (*.pdf)"]
        defaultSuffix: "pdf"
        onAccepted: app.exportScene(work.pathOf(file), paperBox.currentText, true, parseInt(scaleBox.currentText))
    }
    Platform.FileDialog {
        id: formPdf
        title: "导出文书"
        fileMode: Platform.FileDialog.SaveFile
        nameFilters: ["PDF (*.pdf)"]
        defaultSuffix: "pdf"
        onAccepted: app.exportForm(work.pathOf(file), formPaperBox.currentText)
    }

    property alias photoDialog: photoDialog
    property alias aerialDialog: aerialDialog
    property alias paperBox: paperBox
    property alias scaleBox: scaleBox
    property alias formPaperBox: formPaperBox
    property alias captionField: captionField

    ComboBox { id: paperBox; model: ["A3", "A4"]; visible: false }
    ComboBox { id: scaleBox; model: ["200", "100", "500", "1000", "0"]; visible: false }
    ComboBox { id: formPaperBox; model: ["A4", "A3"]; visible: false }
    TextField { id: captionField; visible: false }

    Label {
        visible: app.message.length > 0
        text: app.message
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 36
        color: "white"
        padding: 8
        background: Rectangle { color: "#203542"; radius: 6 }
    }
}
