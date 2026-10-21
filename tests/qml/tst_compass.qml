import QtQuick
import QtQuick.Controls
import QtTest
import "../../compass/qml" as Plugin

Rectangle {
    id: scene
    width: 680; height: 860; color: "#F6F8FC"
    QtObject {
        id: fakeTheme
        property bool dark: false
        property color bgRaised: dark ? "#211C2F" : "#FDFBFF"
        property color bgHover: dark ? "#30263F" : "#EDE6F7"
        property color borderSubtle: dark ? "#40334F" : "#DED5E9"
        property color textPrimary: dark ? "#F4EEFF" : "#20162F"
        property color textSecondary: dark ? "#CDC0E0" : "#514260"
        property color textMuted: dark ? "#AA97BF" : "#736480"
        property color textDisabled: dark ? "#766782" : "#94879F"
        property color textOnAccent: "#24143A"
        property color accentPrimary: "#C5A7F4"
        property color accentHover: "#D6BEFA"
        property color accentPressed: "#AE88E4"
        property color statusError: "#EE829B"
        property color statusSuccess: "#7EC7A9"
        property string fontFamily: "Sans Serif"
    }
    QtObject {
        id: fakeApi
        property string ownerId: "nekotune.shuffle-compass"
        property var config: ({frequency: 0, variety: 0})
        property var calls: []
        property bool saveFails: false
        property bool activateFails: false
        property bool loadFails: false
        signal event(var event)
        function invoke(service, params) {
            calls = calls.concat([{service: service, params: params || {}}])
            if (service === "settings") return loadFails ? Promise.reject(new Error("load refused")) : Promise.resolve(config)
            if (service === "configure") {
                if (saveFails) return Promise.reject(new Error("save refused"))
                config = params
                return Promise.resolve(config)
            }
            if (service === "activate") return activateFails ? Promise.reject(new Error("activate refused"))
                : Promise.resolve({playback_mode: "shuffle", effective_shuffle_strategy: ownerId + "/compass"})
            return Promise.reject(new Error("Unknown service"))
        }
        function call(method, params) {
            return Promise.resolve({playback_mode: "sequential", effective_shuffle_strategy: "shuffle_bag"})
        }
    }
    Component { id: controllerComponent; Plugin.Controller { api: fakeApi } }
    Component { id: panelComponent; Plugin.CompassPanel { anchors.fill: parent; client: tests.controller } }
    TestCase {
        id: tests
        name: "ShuffleCompass"
        when: windowShown
        property var controller
        property var panel
        SignalSpy { id: dismissed; signalName: "dismissRequested" }
        function init() {
            failOnWarning(/TypeError|ReferenceError|Binding loop/)
            scene.width = 680; scene.height = 860
            fakeApi.config = {frequency: 0, variety: 0}; fakeApi.calls = []
            fakeApi.saveFails = false; fakeApi.activateFails = false; fakeApi.loadFails = false
            controller = createTemporaryObject(controllerComponent, scene)
            verify(controller !== null); tryCompare(controller, "ready", true)
            panel = createTemporaryObject(panelComponent, scene)
            verify(panel !== null)
            dismissed.target = panel; dismissed.clear()
            waitForRendering(panel)
        }
        function cleanup() { panel.destroy(); wait(0); controller.destroy(); wait(0) }
        function child(name) { return findChild(panel, name) }
        function screenshot(name) {
            const path = testFixtures.screenshotPath(name)
            if (path) { waitForRendering(panel); grabImage(scene).save(path) }
        }
        function test_pointerChangesDraftOnlyAndCancel() {
            const plot = child("compassDragArea")
            mouseClick(plot, plot.width - 1, 1)
            verify(panel.draftFrequency > .98); verify(panel.draftVariety > .98)
            verify(panel.dirty)
            compare(controller.config.frequency, 0)
            compare(fakeApi.calls.length, 1)
            mouseClick(child("compassCancel"))
            compare(panel.draftFrequency, 0); compare(panel.draftVariety, 0)
            verify(!panel.dirty)
            tryCompare(dismissed, "count", 1)
        }
        function test_dragAndKeyboardClamping() {
            const area = child("compassDragArea")
            mousePress(area, area.width / 2, area.height / 2)
            mouseMove(area, area.width + 80, -50)
            mouseRelease(area, area.width + 80, -50)
            compare(panel.draftFrequency, 1); compare(panel.draftVariety, 1)
            const pad = child("compassBiasPad")
            pad.forceActiveFocus()
            keyClick(Qt.Key_Right); keyClick(Qt.Key_Up)
            compare(panel.draftFrequency, 1); compare(panel.draftVariety, 1)
            keyClick(Qt.Key_Home)
            compare(panel.draftFrequency, 0); compare(panel.draftVariety, 0)
            keyClick(Qt.Key_Left); keyClick(Qt.Key_Down)
            fuzzyCompare(panel.draftFrequency, -0.05, .00001)
            fuzzyCompare(panel.draftVariety, -0.05, .00001)
        }
        function test_saveAndFailurePreserveDraft() {
            panel.preset(-.8, .65)
            verify(panel.dirty)
            fakeApi.saveFails = true
            mouseClick(child("compassSave"))
            tryCompare(controller, "saving", false)
            verify(controller.failed); verify(panel.dirty)
            compare(dismissed.count, 0)
            compare(controller.config.frequency, 0)
            compare(panel.draftFrequency, -0.8)
            fakeApi.saveFails = false
            mouseClick(child("compassSave"))
            tryCompare(controller, "saving", false)
            compare(controller.config.frequency, -0.8)
            compare(controller.config.variety, .65)
            verify(!panel.dirty)
            tryCompare(dismissed, "count", 1)
        }
        function test_enableAndExternalPlaybackEvents() {
            fakeApi.activateFails = true
            mouseClick(child("compassActivate"))
            tryCompare(controller, "activating", false)
            verify(controller.failed); verify(!controller.active)
            fakeApi.activateFails = false
            mouseClick(child("compassActivate"))
            tryCompare(controller, "active", true)
            verify(!child("compassActivate").visible)
            fakeApi.event({event: "player.shuffle_strategy_changed", effective_shuffle_strategy: "shuffle_bag"})
            verify(!controller.active)
        }
        function test_externalConfigKeepsUnsavedDraft() {
            panel.preset(-0.8, 0.65)
            fakeApi.event({event: "extension.nekotune.shuffle-compass.settings_changed", data: {frequency: .9, variety: -.7}})
            compare(panel.draftFrequency, -.8)
            mouseClick(child("compassCancel"))
            compare(panel.draftFrequency, .9); compare(panel.draftVariety, -.7)
            verify(!panel.dirty)
        }
        function test_loadingFailureCanRetry() {
            fakeApi.loadFails = true
            controller.refresh()
            tryCompare(controller, "failed", true)
            verify(!controller.ready)
            fakeApi.loadFails = false
            controller.refresh()
            tryCompare(controller, "ready", true)
        }
        function test_referenceLayoutAndNarrowWidth() {
            scene.width = 404; scene.height = 444
            screenshot("shuffle-compass-desktop")
            scene.width = 360; scene.height = 800
            waitForRendering(panel)
            const pad = child("compassBiasPad"), plot = child("compassPlot")
            verify(plot.width > 0 && plot.height > 0)
            verify(pad.mapToItem(panel, pad.width, 0).x <= panel.width)
            panel.preset(-.8, .65)
            screenshot("shuffle-compass-narrow")
        }
        function test_themeColorsUpdateLive() {
            panel.theme = fakeTheme
            scene.width = 404; scene.height = 444
            fakeTheme.dark = false
            compare(panel.color, fakeTheme.bgRaised)
            compare(child("compassPuckDot").color, fakeTheme.accentPrimary)
            tryCompare(child("compassSave").contentItem, "color", fakeTheme.textOnAccent)
            fakeTheme.dark = true
            compare(panel.color, fakeTheme.bgRaised)
            compare(panel.colors.text, fakeTheme.textPrimary)
            screenshot("shuffle-compass-dark")
            fakeTheme.accentPrimary = "#82CCD5"
            compare(child("compassPuckDot").color, fakeTheme.accentPrimary)
            fakeTheme.dark = false
            compare(panel.color, fakeTheme.bgRaised)
            fakeTheme.accentPrimary = "#C5A7F4"
        }
    }
}
