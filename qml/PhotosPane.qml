import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14

ScrollView {
    id: pane
    clip: true
    ColumnLayout {
        width: pane.availableWidth
        spacing: 12

        RowLayout {
            Layout.margins: 16
            Layout.fillWidth: true
            TextField { id: cap; placeholderText: "照片说明"; Layout.fillWidth: true }
            Button {
                text: "添加照片"
                highlighted: true
                onClicked: {
                    var p = pane.parent
                    while (p && !p.openPhotoDialog)
                        p = p.parent
                    if (p)
                        p.openPhotoDialog(cap.text)
                }
            }
        }
        Label {
            visible: app.photos.length === 0
            text: "还没有现场照片。可以从相册加入，说明会随案例一起保存。"
            color: "#5c6e78"
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16
        }
        Repeater {
            model: app.photos
            delegate: Rectangle {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.preferredHeight: 64
                radius: 8
                border.color: "#d5e0df"
                color: "white"
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    Label { text: (modelData.caption || "现场照片") + "\n" + modelData.file; color: "#203542"; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                    Button { text: "移除"; onClicked: app.removePhoto(modelData.id) }
                }
            }
        }

        Label { text: "现场记录"; font.bold: true; Layout.leftMargin: 16; Layout.topMargin: 8 }
        RowLayout {
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.fillWidth: true
            TextField { id: note; placeholderText: "写一条现场情况"; Layout.fillWidth: true }
            Button { text: "添加"; onClicked: { app.addRecord(note.text); note.text = "" } }
        }
        Repeater {
            model: app.records
            delegate: Label {
                text: (modelData.time || "") + "  " + (modelData.text || "")
                wrapMode: Text.WordWrap
                color: "#203542"
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
            }
        }
        Item { Layout.preferredHeight: 20 }
    }
}
