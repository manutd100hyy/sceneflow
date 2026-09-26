import QtQuick 2.14

Item {
    id: thumb
    property string icon: ""
    property string label: ""
    implicitWidth: 44
    implicitHeight: 44

    Image {
        id: img
        anchors.fill: parent
        anchors.margins: 2
        source: thumb.icon.length > 0 ? thumb.icon : ""
        fillMode: Image.PreserveAspectFit
        smooth: true
        asynchronous: false
        visible: thumb.icon.length > 0 && status === Image.Ready
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: 2
        visible: !img.visible
        radius: 6
        color: "#e7f4f1"
        border.color: "#117b70"
        border.width: 1
        Text {
            anchors.centerIn: parent
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: thumb.label.length > 0 ? thumb.label.charAt(0) : ""
            color: "#117b70"
            font.pixelSize: 16
            font.family: "WenQuanYi Micro Hei"
        }
    }
}
