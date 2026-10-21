#include "controllers/app_controllers.h"
#include "extensions/extension_view.h"
#include "i18n.h"
#include "runtime/backend_runtime.h"
#include <QQmlComponent>
#include <QQmlContext>
#include <QQuickWindow>
#include <QImage>
#include <QTemporaryDir>
#include <QtTest>

class HostUiTest final : public QObject {
    Q_OBJECT
  private slots:
    void installedPageCanSaveAndActivate_data() {
        QTest::addColumn<bool>("compact");
        QTest::newRow("queue") << false;
        QTest::newRow("drawer") << true;
    }
    void installedPageCanSaveAndActivate() {
        QFETCH(bool, compact);
        QTemporaryDir profile;
        qputenv("NEKOTUNE_HOME", profile.filePath("profile").toUtf8());
        qputenv("NEKOTUNE_SOCKET", profile.filePath("backend.sock").toUtf8());
        nekotune::BackendRuntime runtime;
        QVERIFY2(runtime.start(), qPrintable(runtime.errorString()));
        IpcClient client;
        AppControllers controllers(client);
        I18n translator;
        client.connectBackend();
        QTRY_VERIFY(client.connected());
        auto call = [&](const QString &method, const QJsonObject &params) {
            QJsonObject response;
            QString error;
            bool finished = false;
            client.request(method, params, [&](const auto &data, const QString &message) {
                response = data; error = message; finished = true;
            }, false, 5000);
            if (!QTest::qWaitFor([&] { return finished; }, 6000)) error = "Timed out";
            return qMakePair(response, error);
        };
        const auto installed = call("extensions.install", {{"path", QStringLiteral(COMPASS_PACKAGE)}});
        QVERIFY2(installed.second.isEmpty(), qPrintable(installed.second));
        const auto enabled = call("extensions.enable", {{"id", "nekotune.shuffle-compass"}, {"trusted", true}});
        QVERIFY2(enabled.second.isEmpty(), qPrintable(enabled.second));
        QTRY_COMPARE(controllers.extensions->settingsPages().size(), 1);
        QCOMPARE(controllers.extensions->pages().size(), 0);
        QVERIFY(call("player.set_shuffle_strategy", {{"id", "nekotune.shuffle-compass/compass"}}).second.isEmpty());
        QVERIFY(call("player.set_playback_mode", {{"mode", "shuffle"}}).second.isEmpty());
        QTRY_COMPARE(controllers.playback->effectiveShuffleStrategy(), "nekotune.shuffle-compass/compass");
        controllers.theme->setMode("dark");
        QTRY_COMPARE(controllers.theme->mode(), "dark");

        registerExtensionQmlTypes();
        QQuickWindow window;
        window.resize(800, 640); window.show();
        QQmlEngine engine;
        engine.rootContext()->setContextProperty("testApp", &controllers);
        engine.rootContext()->setContextProperty("controllers", &controllers);
        engine.rootContext()->setContextProperty("testI18n", &translator);
        engine.rootContext()->setContextProperty("i18n", &translator);
        engine.rootContext()->setContextProperty("testCompact", compact);
        QQmlComponent component(&engine);
        component.setData("import QtQuick; import QtQuick.Controls; import \"qrc:/qml/components\"; "
                          "Rectangle { width:800; height:640; color:Theme.bgCanvas; QueuePanel { anchors.fill:parent; anchors.margins:20; "
                          "compact:testCompact; appControllers:testApp; playbackController:testApp.playback; "
                          "playbackMode:testApp.playback.playbackMode } }", QUrl());
        QScopedPointer<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto *scene = qobject_cast<QQuickItem *>(object.data());
        QVERIFY(scene);
        scene->setParentItem(window.contentItem());
        auto *button = scene->findChild<QQuickItem *>("playbackModeButton");
        QVERIFY(button);
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QTRY_VERIFY(button->width() > 0);
        QTest::qWait(50);
        auto *menu = button->findChild<QObject *>("playbackModeMenu");
        auto *popup = button->findChild<QObject *>("shuffleSettingsPopup");
        QVERIFY(menu); QVERIFY(popup);
        auto openSettings = [&] {
            if (!QMetaObject::invokeMethod(button, "clicked")) return false;
            if (!QTest::qWaitFor([&] { return menu->property("opened").toBool(); })) return false;
            const int last = button->property("entryCount").toInt() - 1;
            return QMetaObject::invokeMethod(menu, "choose", Q_ARG(QVariant, last));
        };
        QVERIFY(openSettings());
        QTRY_VERIFY(popup->property("opened").toBool());
        QVERIFY2(popup->property("x").toDouble() >= 8, qPrintable(QString("x=%1 width=%2").arg(popup->property("x").toDouble()).arg(popup->property("width").toDouble())));
        QVERIFY(popup->property("y").toDouble() >= 8);
        QVERIFY(popup->property("x").toDouble() + popup->property("width").toDouble() <= window.width() - 8);
        QVERIFY(popup->property("y").toDouble() + popup->property("height").toDouble() <= window.height() - 8);
        auto *view = button->findChild<ExtensionView *>("shuffleSettingsView");
        QVERIFY(view);
        QTRY_VERIFY2(view->ready(), qPrintable(view->error()));
        auto *controller = view->item()->findChild<QObject *>("compassController");
        QVERIFY(controller);
        QTRY_VERIFY(controller->property("ready").toBool());
        QTRY_VERIFY(controller->property("active").toBool());
        auto *panel = view->item()->findChild<QObject *>("compassPanel");
        QVERIFY(panel);
        panel->setProperty("draftFrequency", -0.7);
        panel->setProperty("draftVariety", 0.8);
        if (const auto directory = qEnvironmentVariable("NEKOTUNE_TEST_SCREENSHOT_DIR"); !directory.isEmpty()) {
            QTest::qWait(100);
            QVERIFY(window.grabWindow().save(directory + "/shuffle-compass-popup-" + (compact ? "drawer" : "queue") + ".png"));
        }
        const auto darkColor = panel->property("color").value<QColor>();
        controllers.theme->setMode("light");
        QTRY_VERIFY(!controllers.theme->isDark());
        QTRY_VERIFY(panel->property("color").value<QColor>() != darkColor);
        QCOMPARE(panel->property("draftFrequency").toDouble(), -0.7);
        QVERIFY(popup->property("visible").toBool());
        if (const auto directory = qEnvironmentVariable("NEKOTUNE_TEST_SCREENSHOT_DIR"); !directory.isEmpty()) {
            QTest::qWait(100);
            QVERIFY(window.grabWindow().save(directory + "/shuffle-compass-popup-light-" + (compact ? "drawer" : "queue") + ".png"));
        }
        controllers.theme->setMode("dark");
        QTRY_COMPARE(panel->property("color").value<QColor>(), darkColor);
        auto *save = view->item()->findChild<QQuickItem *>("compassSave");
        QVERIFY(save);
        QVERIFY(QMetaObject::invokeMethod(save, "clicked"));
        QTRY_VERIFY(!popup->property("visible").toBool());
        QTRY_VERIFY(!view->ready());
        QVERIFY(openSettings());
        QTRY_VERIFY(view->ready());
        controller = view->item()->findChild<QObject *>("compassController");
        QTRY_VERIFY(controller->property("ready").toBool());
        QCOMPARE(controller->property("config").toMap().value("frequency").toDouble(), -0.7);
        panel = view->item()->findChild<QObject *>("compassPanel");
        panel->setProperty("draftFrequency", 0.9);
        auto *cancel = view->item()->findChild<QObject *>("compassCancel");
        QVERIFY(QMetaObject::invokeMethod(cancel, "clicked"));
        QTRY_VERIFY(!popup->property("visible").toBool());
        const auto settings = call("extensions.call", {{"id", "nekotune.shuffle-compass"}, {"service", "settings"}});
        QCOMPARE(settings.first.value("frequency").toDouble(), -0.7);
        QVERIFY(openSettings());
        QTRY_VERIFY(view->ready());
        QVERIFY(call("extensions.disable", {{"id", "nekotune.shuffle-compass"}}).second.isEmpty());
        QTRY_VERIFY(!popup->property("visible").toBool());
        QTRY_VERIFY(!view->ready());
        runtime.stop();
    }
};
QTEST_MAIN(HostUiTest)
#include "host_ui_test.moc"
