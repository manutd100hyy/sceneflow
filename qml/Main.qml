import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14
import SketchRoad 1.0

ApplicationWindow {
    id: win
    visible: true
    width: 1360
    height: 860
    minimumWidth: 360
    minimumHeight: 560
    title: app.screen === "home" ? "SketchRoad · 事故现场绘图" : ("SketchRoad · " + app.caseName)
    color: "#f4f7f6"
    font.family: "WenQuanYi Micro Hei"
    font.pixelSize: 15

    readonly property bool phone: width < 760
    readonly property bool tablet: width >= 760 && width < 1180

    Shortcut { sequence: "Ctrl+Z"; onActivated: app.undo() }
    Shortcut { sequence: "Ctrl+Y"; onActivated: app.redo() }
    Shortcut { sequence: "Ctrl+S"; onActivated: if (app.screen !== "home") app.save() }
    Shortcut { sequence: "Delete"; onActivated: if (app.screen !== "home") app.removeSelection() }

    Loader {
        anchors.fill: parent
        sourceComponent: app.screen === "home" ? homePage : workspace
    }

    Component { id: homePage; HomePage { } }
    Component { id: workspace; Workspace { } }

    Dialog {
        id: textDialog
        modal: true
        title: "添加文字"
        anchors.centerIn: parent
        width: Math.min(win.width - 32, 420)
        standardButtons: Dialog.Ok | Dialog.Cancel
        property double px: 0
        property double py: 0
        contentItem: TextField {
            id: textInput
            placeholderText: "输入现场文字"
            selectByMouse: true
        }
        onAccepted: app.addTextAt(textDialog.px, textDialog.py, textInput.text)
    }

    Connections {
        target: app
        onTextRequested: {
            textDialog.px = x
            textDialog.py = y
            textInput.text = ""
            textDialog.open()
        }
    }
}
