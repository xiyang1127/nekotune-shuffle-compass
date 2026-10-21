pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls

Item {
    id: root
    property CompassPalette colors: CompassPalette {}
    property real frequency: 0
    property real variety: 0
    signal edited(real frequency, real variety)
    implicitWidth: 420
    implicitHeight: Math.min(360, width * 0.82)
    activeFocusOnTab: true
    Accessible.role: Accessible.Slider
    Accessible.name: "随机选曲偏好"
    Accessible.description: "左右调整少听与常听，上下调整风格跳跃与相似；Home 恢复均衡。"
    function clamp(value) { return Math.max(-1, Math.min(1, value)) }
    function move(dx, dy) { edited(clamp(frequency + dx), clamp(variety + dy)) }
    Keys.onLeftPressed: event => move(event.modifiers & Qt.ShiftModifier ? -0.01 : -0.05, 0)
    Keys.onRightPressed: event => move(event.modifiers & Qt.ShiftModifier ? 0.01 : 0.05, 0)
    Keys.onUpPressed: event => move(0, event.modifiers & Qt.ShiftModifier ? 0.01 : 0.05)
    Keys.onDownPressed: event => move(0, event.modifiers & Qt.ShiftModifier ? -0.01 : -0.05)
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Home) { edited(0, 0); event.accepted = true }
    }

    Label {
        anchors.horizontalCenter: plot.horizontalCenter; y: 8
        text: "风格跳跃"; color: root.colors.text; font.pixelSize: 14; font.weight: Font.DemiBold
    }
    Label {
        anchors.horizontalCenter: plot.horizontalCenter; anchors.bottom: parent.bottom; anchors.bottomMargin: 8
        text: "风格相似"; color: root.colors.text; font.pixelSize: 14; font.weight: Font.DemiBold
    }
    Label {
        x: 0; width: 66; anchors.verticalCenter: plot.verticalCenter
        text: "极少听"; horizontalAlignment: Text.AlignHCenter
        color: root.colors.text; font.pixelSize: 14; font.weight: Font.DemiBold
    }
    Label {
        anchors.right: parent.right; width: 66; anchors.verticalCenter: plot.verticalCenter
        text: "最常听"; horizontalAlignment: Text.AlignHCenter
        color: root.colors.text; font.pixelSize: 14; font.weight: Font.DemiBold
    }
    Item {
        id: plot
        objectName: "compassPlot"
        x: 72; y: 44
        width: Math.max(40, root.width - 144)
        height: Math.max(40, root.height - 88)
        Rectangle {
            anchors.fill: parent; anchors.margins: -8
            color: "transparent"; radius: 14
            border.color: root.activeFocus ? root.colors.accent : "transparent"
        }
        Repeater {
            model: 9
            Rectangle {
                required property int index
                x: index * plot.width / 8; width: 1; height: plot.height
                color: root.colors.border
                opacity: index === 4 ? 1 : 0.45
            }
        }
        Repeater {
            model: 9
            Rectangle {
                required property int index
                y: index * plot.height / 8; width: plot.width; height: 1
                color: root.colors.border
                opacity: index === 4 ? 1 : 0.45
            }
        }
        Rectangle {
            objectName: "compassPuck"
            x: (root.frequency + 1) / 2 * plot.width - width / 2
            y: (1 - root.variety) / 2 * plot.height - height / 2
            width: 44; height: 44; radius: 22
            color: Qt.rgba(root.colors.accent.r, root.colors.accent.g, root.colors.accent.b, 0.16)
            Rectangle {
                objectName: "compassPuckDot"
                anchors.centerIn: parent; width: 26; height: 26; radius: 13
                color: root.enabled ? root.colors.accent : root.colors.disabled
                border.color: root.colors.surface; border.width: 1
            }
        }
        MouseArea {
            objectName: "compassDragArea"
            anchors.fill: parent
            preventStealing: true
            cursorShape: pressed ? Qt.ClosedHandCursor : Qt.PointingHandCursor
            function update(mouse) {
                root.edited(root.clamp(mouse.x / width * 2 - 1), root.clamp(1 - mouse.y / height * 2))
            }
            onPressed: mouse => { root.forceActiveFocus(); update(mouse) }
            onPositionChanged: mouse => { if (pressed) update(mouse) }
        }
    }
}
