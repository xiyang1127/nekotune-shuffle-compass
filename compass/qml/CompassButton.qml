import QtQuick
import QtQuick.Controls

Button {
    id: control
    property CompassPalette colors: CompassPalette {}
    implicitWidth: Math.max(68, implicitContentWidth + 28)
    implicitHeight: 34
    hoverEnabled: true
    leftPadding: 14; rightPadding: 14
    font.pixelSize: 13
    font.family: colors.fontFamily
    contentItem: Text {
        text: control.text; textFormat: Text.PlainText
        font: control.font; elide: Text.ElideRight
        horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
        color: !control.enabled ? control.colors.disabled : control.highlighted ? control.colors.accentText : control.colors.secondary
    }
    background: Rectangle {
        radius: 17
        color: !control.enabled ? (control.flat ? "transparent" : control.colors.surface)
            : control.highlighted ? (control.down ? control.colors.accentPressed : control.hovered ? control.colors.accentHover : control.colors.accent)
            : control.hovered ? control.colors.hover : control.flat ? "transparent" : control.colors.surface
        border.width: control.activeFocus ? 2 : 1
        border.color: control.activeFocus ? control.colors.accent : control.highlighted || control.flat ? "transparent" : control.colors.border
    }
}
