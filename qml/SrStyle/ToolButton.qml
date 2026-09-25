import QtQuick 2.14
import QtQuick.Templates 2.14 as T
import QtQuick.Controls.impl 2.14

T.ToolButton {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)
    padding: 6
    spacing: 4

    contentItem: IconLabel {
        spacing: control.spacing
        mirrored: control.mirrored
        display: control.display
        icon: control.icon
        text: control.text
        font: control.font
        color: control.highlighted || control.checked ? "#ffffff" : "#203542"
    }

    background: Rectangle {
        implicitWidth: 36
        implicitHeight: 36
        radius: 8
        color: control.highlighted || control.checked ? "#117b70" : (control.down ? "#d7efe9" : "transparent")
        border.width: control.highlighted || control.checked ? 0 : 1
        border.color: control.down ? "#117b70" : "transparent"
    }
}
