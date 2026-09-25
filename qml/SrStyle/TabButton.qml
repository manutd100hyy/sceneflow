import QtQuick 2.14
import QtQuick.Templates 2.14 as T
import QtQuick.Controls.impl 2.14

T.TabButton {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)
    padding: 8
    font.bold: control.checked
    font.pixelSize: 14

    contentItem: IconLabel {
        spacing: control.spacing
        mirrored: control.mirrored
        display: control.display
        icon: control.icon
        text: control.text
        font: control.font
        color: control.checked ? "#117b70" : "#5c6e78"
    }

    background: Rectangle {
        implicitHeight: 42
        color: control.checked ? "#e7f4f1" : (control.down ? "#f4f7f6" : "transparent")
        Rectangle {
            visible: control.checked
            height: 3
            radius: 1.5
            width: Math.max(24, parent.width - 20)
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            color: "#117b70"
        }
    }
}
