#include "fake_context.h"
#include <nekotune/sdk/extension_plugin.h>
#include <QPluginLoader>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QJsonDocument>
#include <QtTest>

using namespace nekotune;

class PluginTest final : public QObject {
    Q_OBJECT
    QPluginLoader m_loader;
    sdk::IExtensionPlugin *m_plugin = nullptr;
  private slots:
    void initTestCase() {
        m_loader.setFileName(QCoreApplication::arguments().at(1));
        m_loader.setLoadHints(QLibrary::PreventUnloadHint);
        m_plugin = qobject_cast<sdk::IExtensionPlugin *>(m_loader.instance());
        QVERIFY2(m_plugin, qPrintable(m_loader.errorString()));
        QFile manifest(QFileInfo(m_loader.fileName()).dir().absoluteFilePath("../extension.json"));
        QVERIFY(manifest.open(QIODevice::ReadOnly));
        const auto data = QJsonDocument::fromJson(manifest.readAll()).object();
        QCOMPARE(data.value("qt").toString(), QStringLiteral("%1.%2").arg(QT_VERSION_MAJOR).arg(QT_VERSION_MINOR));
    }
    void lifecycle() {
        std::optional<Result<void>> result;
        FakeContext context;
        for (int i = 0; i < 2; ++i) {
            m_plugin->activate(context, [&](Result<void> value) { result = value; });
            QVERIFY(result && *result);
            QVERIFY(context.provider);
            QVERIFY(context.call("settings"));
            m_plugin->deactivate();
            QVERIFY(!context.provider);
            QVERIFY(context.serviceHandlers.isEmpty());
            QVERIFY(context.listeners.empty());
        }
    }
    void settingsSaveValidationAndInvalidation() {
        FakeContext context;
        bool activated = false;
        m_plugin->activate(context, [&](Result<void> value) { activated = bool(value); });
        QVERIFY(activated);
        const auto initial = context.registrations;
        QVERIFY(!context.call("configure", {{"frequency", "bad"}}));
        QCOMPARE(context.registrations, initial);
        context.saveFails = true;
        QVERIFY(!context.call("configure", {{"frequency", -1}}));
        QCOMPARE(context.call("settings").value().value("frequency").toDouble(), 0);
        QCOMPARE(context.registrations, initial);
        context.saveFails = false;
        const auto saved = context.call("configure", {{"frequency", -1}, {"variety", 1}});
        QVERIFY(saved);
        QCOMPARE(saved.value().value("frequency").toDouble(), -1);
        QCOMPARE(saved.value().value("variety").toDouble(), 1);
        QCOMPARE(context.registrations, initial + 1);
        QVERIFY(context.provider);
        const auto again = context.call("configure", {{"frequency", -1}});
        QVERIFY(again);
        QCOMPARE(context.registrations, initial + 1);
        m_plugin->deactivate();
        m_plugin->activate(context, [&](Result<void> value) { activated = bool(value); });
        QVERIFY(activated);
        QCOMPARE(context.call("settings").value().value("frequency").toDouble(), -1);
        m_plugin->deactivate();
    }
    void enableUsesHostAndPropagatesFailures() {
        FakeContext context;
        m_plugin->activate(context, [](Result<void>) {});
        QVERIFY(context.call("activate"));
        QCOMPARE(context.hostCalls, QStringList({"player.set_shuffle_strategy", "player.set_playback_mode"}));
        context.hostCalls.clear();
        context.failHost = "player.set_shuffle_strategy";
        QVERIFY(!context.call("activate"));
        QCOMPARE(context.hostCalls.size(), 1);
        context.failHost = "player.set_playback_mode";
        QVERIFY(!context.call("activate"));
        m_plugin->deactivate();
    }
};

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    if (app.arguments().size() != 2) return 1;
    PluginTest test;
    return QTest::qExec(&test, 1, argv);
}
#include "plugin_test.moc"
