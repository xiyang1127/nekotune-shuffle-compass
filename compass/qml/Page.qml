import QtQuick
import NekoTune 1.0

Item {
    id: root
    signal dismissRequested()
    ExtensionApi { id: extensionApi }
    Controller { id: compassController; objectName: "compassController"; api: extensionApi }
    CompassPanel {
        objectName: "compassPanel"; anchors.fill: parent; client: compassController
        theme: extensionApi.theme
        onDismissRequested: root.dismissRequested()
    }
}
