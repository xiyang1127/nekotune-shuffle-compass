pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    required property var client
    property var theme: null
    readonly property CompassPalette colors: CompassPalette { source: root.theme }
    property real draftFrequency: 0
    property real draftVariety: 0
    property real savedFrequency: 0
    property real savedVariety: 0
    readonly property bool dirty: Math.abs(draftFrequency - savedFrequency) > 0.0001 || Math.abs(draftVariety - savedVariety) > 0.0001
    signal dismissRequested()
    color: colors.surface
    implicitWidth: 400; implicitHeight: 420
    function syncDraft() {
        savedFrequency = client.config.frequency || 0; savedVariety = client.config.variety || 0
        draftFrequency = savedFrequency; draftVariety = savedVariety
    }
    function preset(frequency, variety) { draftFrequency = frequency; draftVariety = variety }
    function complete() {
        if (!dirty) { dismissRequested(); return }
        client.save(draftFrequency, draftVariety).then(success => {
            if (root && success) root.dismissRequested()
        })
    }
    Connections {
        target: root.client
        function onConfigChanged() { if (!root.dirty || root.client.saving) root.syncDraft() }
        function onReadyChanged() { if (root.client.ready && !root.dirty) root.syncDraft() }
    }
    Component.onCompleted: syncDraft()

    ScrollView {
        id: scroll
        anchors.fill: parent; contentWidth: availableWidth; clip: true
        ColumnLayout {
            width: Math.min(400, scroll.availableWidth)
            x: (scroll.availableWidth - width) / 2
            spacing: 8
            Label {
                Layout.fillWidth: true; Layout.topMargin: 8
                horizontalAlignment: Text.AlignHCenter
                text: "随机选曲偏好"; color: root.colors.text; font.pixelSize: 13; font.family: root.colors.fontFamily
            }
            BiasPad {
                objectName: "compassBiasPad"
                colors: root.colors
                Layout.fillWidth: true
                Layout.leftMargin: 8; Layout.rightMargin: 8
                Layout.preferredHeight: implicitHeight
                frequency: root.draftFrequency; variety: root.draftVariety
                enabled: root.client.ready && !root.client.working
                onEdited: (frequency, variety) => root.preset(frequency, variety)
            }
            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 12; Layout.rightMargin: 12
                CompassButton {
                    objectName: "compassCancel"
                    colors: root.colors
                    text: "取消"
                    enabled: !root.client.working
                    onClicked: { root.syncDraft(); root.dismissRequested() }
                }
                Item { Layout.fillWidth: true }
                CompassButton {
                    objectName: "compassSave"
                    colors: root.colors
                    text: root.client.saving ? "保存中…" : "完成"
                    enabled: root.client.ready && !root.client.working
                    highlighted: true
                    onClicked: root.complete()
                }
            }
            RowLayout {
                visible: root.client.ready && !root.client.active
                Layout.fillWidth: true
                Layout.leftMargin: 12; Layout.rightMargin: 12
                Label {
                    Layout.fillWidth: true
                    text: "启用后将使用此偏好选曲"; color: root.colors.muted; font.pixelSize: 12
                    wrapMode: Text.WordWrap
                }
                CompassButton {
                    objectName: "compassActivate"
                    colors: root.colors
                    text: root.client.activating ? "启用中…" : "启用算法"
                    enabled: !root.client.working
                    onClicked: root.client.activate()
                }
            }
            Label {
                objectName: "compassMessage"
                Layout.fillWidth: true
                Layout.leftMargin: 12; Layout.rightMargin: 12
                visible: root.client.message.length > 0
                text: root.client.message; textFormat: Text.PlainText; wrapMode: Text.WordWrap
                color: root.client.failed ? root.colors.error : root.colors.success; font.pixelSize: 12
            }
            CompassButton {
                colors: root.colors
                Layout.alignment: Qt.AlignHCenter
                visible: !root.client.ready && root.client.failed
                text: "重新读取"; onClicked: root.client.refresh()
            }
            Label {
                Layout.fillWidth: true
                Layout.leftMargin: 12; Layout.rightMargin: 12; Layout.bottomMargin: 8
                text: "少听／常听按播放次数；风格按歌手与专辑。方向键微调，Home 回到中心。"
                wrapMode: Text.WordWrap; color: root.colors.muted; font.pixelSize: 11
            }
        }
    }
}
