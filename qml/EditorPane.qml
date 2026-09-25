import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14
import SketchRoad 1.0

Item {
    id: editor

    readonly property bool phone: width < 760
    readonly property bool desktop: width >= 1100
    readonly property var sel: app.selection

    function host() {
        var p = parent
        while (p && !p.openAerialDialog)
            p = p.parent
        return p
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            visible: desktop
            Layout.preferredWidth: 250
            Layout.fillHeight: true
            color: "white"
            LibraryColumn { anchors.fill: parent }
        }
        Rectangle { visible: desktop; Layout.preferredWidth: 1; Layout.fillHeight: true; color: "#d5e0df" }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            ScrollView {
                Layout.fillWidth: true
                Layout.preferredHeight: phone ? 46 : 50
                clip: true
                Row {
                    spacing: 4
                    leftPadding: 6
                    topPadding: 6
                    Button { text: "选择"; highlighted: app.tool === 0; onClicked: app.tool = 0; height: 36 }
                    Button { text: "平移"; highlighted: app.tool === 1; onClicked: app.tool = 1; height: 36 }
                    Button { text: "道路"; highlighted: app.tool === 2; onClicked: app.tool = 2; height: 36 }
                    Button { text: "图符"; highlighted: app.tool === 3; onClicked: app.tool = 3; height: 36 }
                    Button { text: "痕迹"; highlighted: app.tool === 4; onClicked: app.tool = 4; height: 36 }
                    Button { text: "散落物"; highlighted: app.tool === 5; onClicked: app.tool = 5; height: 36 }
                    Button { text: "标注"; highlighted: app.tool === 6; onClicked: app.tool = 6; height: 36 }
                    Button { text: "文字"; highlighted: app.tool === 7; onClicked: app.tool = 7; height: 36 }
                    Button { text: "横道"; highlighted: app.tool === 8; onClicked: app.tool = 8; height: 36 }
                    Button { text: "箭头"; highlighted: app.tool === 9; onClicked: app.tool = 9; height: 36 }
                    Button { text: "环岛"; highlighted: app.tool === 10; onClicked: app.tool = 10; height: 36 }
                    Button { text: "橡皮"; highlighted: app.tool === 11; onClicked: app.tool = 11; height: 36 }
                    Button { text: "标定"; visible: app.edition === "aerial" || app.hasAerial; highlighted: app.tool === 12; onClicked: app.tool = 12; height: 36 }
                }
            }

            SceneCanvas {
                id: canvas
                Layout.fillWidth: true
                Layout.fillHeight: true
                Component.onCompleted: app.attachCanvas(canvas)
            }

            Rectangle {
                visible: phone
                Layout.fillWidth: true
                Layout.preferredHeight: 58
                color: "white"
                Row {
                    anchors.centerIn: parent
                    spacing: 8
                    Button { text: "图符"; height: 44; onClicked: libDrawer.open() }
                    Button { text: "标注"; height: 44; highlighted: app.tool === 6; onClicked: app.tool = 6 }
                    Button { text: "属性"; height: 44; onClicked: propDrawer.open() }
                    Button { text: "更多"; height: 44; onClicked: moreDrawer.open() }
                }
            }
        }

        Rectangle { visible: desktop; Layout.preferredWidth: 1; Layout.fillHeight: true; color: "#d5e0df" }
        Rectangle {
            visible: desktop
            Layout.preferredWidth: 280
            Layout.fillHeight: true
            color: "white"
            PropertyColumn { anchors.fill: parent }
        }
    }

    Button {
        visible: !desktop && !phone
        text: "库"
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.topMargin: 58
        anchors.leftMargin: 8
        onClicked: libDrawer.open()
        height: 40
        z: 2
    }

    Drawer {
        id: libDrawer
        edge: Qt.LeftEdge
        width: Math.min(editor.width * 0.86, 320)
        height: editor.height
        LibraryColumn { anchors.fill: parent }
    }
    Drawer {
        id: propDrawer
        edge: Qt.BottomEdge
        width: editor.width
        height: Math.min(editor.height * 0.62, 420)
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
            Button { text: "适应画面"; onClicked: app.fit() }
            Button { text: "比例化"; onClicked: app.proportionalize() }
            Button { text: "整体按实测缩放"; onClicked: app.scaleToMeasures() }
            Button { text: "导入航拍照片"; onClicked: { var h = editor.host(); if (h) h.openAerialDialog() } }
            Button { text: "清除图面"; onClicked: app.clearDrawings() }
        }
    }
}
