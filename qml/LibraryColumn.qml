import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14

Item {
    id: col

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 6
        TextField {
            id: filter
            placeholderText: "搜索图符或模板"
            Layout.fillWidth: true
            selectByMouse: true
        }
        TabBar {
            id: kind
            Layout.fillWidth: true
            TabButton { text: "道路"; width: implicitWidth }
            TabButton { text: "图符"; width: implicitWidth }
            TabButton { text: "对象"; width: implicitWidth }
        }
        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: kind.currentIndex

            ListView {
                clip: true
                model: app.templates
                spacing: 4
                delegate: ItemDelegate {
                    width: ListView.view.width
                    visible: filter.text.length === 0 || modelData.name.indexOf(filter.text) >= 0
                    height: visible ? implicitHeight : 0
                    text: modelData.name
                    onClicked: app.placeTemplate(modelData.id)
                }
            }

            ColumnLayout {
                ComboBox {
                    id: groupBox
                    Layout.fillWidth: true
                    model: app.groups
                    textRole: "name"
                }
                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    spacing: 2
                    model: groupBox.currentIndex >= 0 && app.groups.length > groupBox.currentIndex
                           ? app.groups[groupBox.currentIndex].items : []
                    delegate: ItemDelegate {
                        width: ListView.view.width
                        text: modelData.name
                        visible: filter.text.length === 0 || modelData.name.indexOf(filter.text) >= 0
                        height: visible ? 40 : 0
                        onClicked: app.activateLibrary(modelData.name, modelData.notification, modelData.flag)
                    }
                }
            }

            ListView {
                clip: true
                model: app.objects
                spacing: 2
                delegate: ItemDelegate {
                    width: ListView.view.width
                    text: (modelData.locked ? "锁 " : "") + modelData.title
                    highlighted: app.selection.id === modelData.id
                    onClicked: app.selectObject(modelData.id)
                }
            }
        }
        Label {
            text: "车道 " + app.laneCount + " × " + app.laneWidth.toFixed(1) + " m"
            color: "#5c6e78"
            font.pixelSize: 12
        }
        RowLayout {
            SpinBox {
                from: 1; to: 8; value: app.laneCount
                onValueModified: app.laneCount = value
                Layout.fillWidth: true
            }
            SpinBox {
                from: 25; to: 50; value: Math.round(app.laneWidth * 10)
                onValueModified: app.laneWidth = value / 10
                Layout.fillWidth: true
            }
        }
    }
}
