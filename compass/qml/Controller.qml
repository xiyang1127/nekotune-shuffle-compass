pragma ComponentBehavior: Bound
import QtQuick

QtObject {
    id: root
    required property var api
    property var config: ({frequency: 0, variety: 0})
    property bool ready: false
    property bool saving: false
    property bool activating: false
    property bool active: false
    property bool failed: false
    property string message: ""
    property string mode: "sequential"
    property string effectiveStrategy: "shuffle_bag"
    property int generation: 0
    property bool disposed: false
    readonly property string strategyId: api.ownerId + "/compass"
    readonly property bool working: saving || activating
    function acceptPlayback(value) {
        if (value.playback_mode !== undefined) mode = value.playback_mode
        if (value.effective_shuffle_strategy !== undefined) effectiveStrategy = value.effective_shuffle_strategy
        active = mode === "shuffle" && effectiveStrategy === strategyId
    }
    function refresh() {
        const revision = ++generation
        ready = false; failed = false; message = ""
        return api.invoke("settings").then(value => {
            if (!root || root.disposed || revision !== root.generation) return
            root.config = value; root.ready = true
            return root.api.call("player.status", {}).then(status => {
                if (root && !root.disposed && revision === root.generation) root.acceptPlayback(status)
            })
        }).catch(error => {
            if (!root || root.disposed || revision !== root.generation) return
            root.failed = true; root.message = "读取设置失败：" + String(error)
        })
    }
    function save(frequency, variety) {
        if (!ready || working) return
        saving = true; failed = false; message = ""
        return api.invoke("configure", {frequency: frequency, variety: variety}).then(value => {
            if (!root || root.disposed) return
            root.config = value; root.saving = false
            root.message = root.active ? "已保存，接下来的选曲会使用新偏好。" : "偏好已保存，启用漫游罗盘后生效。"
            return true
        }).catch(error => {
            if (!root || root.disposed) return
            root.saving = false; root.failed = true; root.message = "保存失败：" + String(error)
            return false
        })
    }
    function activate() {
        if (!ready || working) return
        activating = true; failed = false; message = ""
        return api.invoke("activate").then(value => {
            if (!root || root.disposed) return
            root.acceptPlayback(value); root.activating = false
            root.message = "漫游罗盘已启用，当前歌曲继续播放。"
        }).catch(error => {
            if (!root || root.disposed) return
            root.activating = false; root.failed = true; root.message = "启用失败：" + String(error)
        })
    }
    property Connections events: Connections {
        target: root.api
        function onEvent(event) {
            if (event.event === "extension." + root.api.ownerId + ".settings_changed") root.config = event.data
            else if (String(event.event).startsWith("player.")) root.acceptPlayback(event)
        }
    }
    Component.onCompleted: refresh()
    Component.onDestruction: { disposed = true; ++generation }
}
