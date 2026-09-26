import QtQuick 2.14
import QtQuick.Templates 2.14 as T
import QtQuick.Controls.impl 2.14

T.ItemDelegate {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)
    padding: 10
    leftPadding: 12
    spacing: 8

    contentItem: IconLabel {
        spacing: control.spacing
        mirrored: control.mirrored
        display: control.display
        alignment: Qt.AlignLeft
        icon: control.icon
        text: control.text
        font: control.font
        color: control.highlighted ? "#0c6158" : "#203542"
    }

    background: Rectangle {
        implicitWidth: 100
        implicitHeight: 40
        radius: 8
        color: control.highlighted ? "#d7efe9" : (control.down ? "#eef6f4" : "transparent")
    }
}
