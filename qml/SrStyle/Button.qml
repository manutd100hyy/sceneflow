import QtQuick 2.14
import QtQuick.Templates 2.14 as T
import QtQuick.Controls.impl 2.14

T.Button {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)
    padding: 8
    leftPadding: 12
    rightPadding: 12
    spacing: 6
    icon.width: 18
    icon.height: 18
    icon.color: control.highlighted || control.checked ? "#ffffff" : (control.flat ? "#117b70" : "#203542")

    contentItem: IconLabel {
        spacing: control.spacing
        mirrored: control.mirrored
        display: control.display
        icon: control.icon
        text: control.text
        font: control.font
        color: !control.enabled ? "#8aa39e"
             : (control.highlighted || control.checked ? "#ffffff"
             : (control.flat ? "#117b70" : "#203542"))
    }

    background: Rectangle {
        implicitWidth: 84
        implicitHeight: 36
        radius: 8
        visible: !control.flat || control.down || control.checked || control.highlighted
        color: {
            if (!control.enabled)
                return "#e6eeec"
            if (control.highlighted || control.checked)
                return control.down ? "#0c6158" : "#117b70"
            if (control.down)
                return "#d7efe9"
            if (control.hovered)
                return "#f3faf8"
            return "#ffffff"
        }
        border.width: 1
        border.color: (control.highlighted || control.checked) ? "#0c6158" : "#c5d8d4"
    }
}
