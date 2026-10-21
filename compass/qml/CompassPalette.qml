import QtQuick

QtObject {
    property var source: null
    readonly property color surface: source ? source.bgRaised : "#FFFFFF"
    readonly property color hover: source ? source.bgHover : "#EDF4FF"
    readonly property color border: source ? source.borderSubtle : "#E1E7F0"
    readonly property color text: source ? source.textPrimary : "#3D4960"
    readonly property color secondary: source ? source.textSecondary : "#60708A"
    readonly property color muted: source ? source.textMuted : "#778297"
    readonly property color disabled: source ? source.textDisabled : "#A5AFBF"
    readonly property color accentText: source ? source.textOnAccent : "#FFFFFF"
    readonly property color accent: source ? source.accentPrimary : "#4798FA"
    readonly property color accentHover: source ? source.accentHover : "#5BA4FF"
    readonly property color accentPressed: source ? source.accentPressed : "#347DD4"
    readonly property color error: source ? source.statusError : "#C34858"
    readonly property color success: source ? source.statusSuccess : "#438972"
    readonly property string fontFamily: source ? source.fontFamily : "Sans Serif"
}
