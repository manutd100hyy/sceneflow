import QtQuick 2.14
import QtQuick.Templates 2.14 as T
import QtQuick.Controls.impl 2.14

T.TextField {
    id: control

    implicitWidth: implicitBackgroundWidth + leftInset + rightInset
                   || Math.max(contentWidth, placeholder.implicitWidth) + leftPadding + rightPadding
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding,
                             placeholder.implicitHeight + topPadding + bottomPadding)
    padding: 8
    leftPadding: 12
    color: "#203542"
    selectionColor: "#117b70"
    selectedTextColor: "#ffffff"
    placeholderTextColor: "#8aa39e"
    verticalAlignment: TextInput.AlignVCenter

    PlaceholderText {
        id: placeholder
        x: control.leftPadding
        y: control.topPadding
        width: control.width - (control.leftPadding + control.rightPadding)
        height: control.height - (control.topPadding + control.bottomPadding)
        text: control.placeholderText
        font: control.font
        color: control.placeholderTextColor
        verticalAlignment: control.verticalAlignment
        visible: !control.length && !control.preeditText && (!control.activeFocus || control.horizontalAlignment !== Qt.AlignHCenter)
        elide: Text.ElideRight
        renderType: control.renderType
    }

    background: Rectangle {
        implicitWidth: 180
        implicitHeight: 38
        radius: 8
        color: control.enabled ? "#ffffff" : "#f4f7f6"
        border.width: control.activeFocus ? 2 : 1
        border.color: control.activeFocus ? "#117b70" : "#c5d8d4"
    }
}
