#include <QQmlContext>
#include <QQmlEngine>
#include <QtQuickTest/quicktest.h>

class Setup final : public QObject {
    Q_OBJECT
  public:
    Q_INVOKABLE QString screenshotPath(const QString &name) const {
        const auto directory = qEnvironmentVariable("NEKOTUNE_TEST_SCREENSHOT_DIR");
        return directory.isEmpty() ? QString() : directory + '/' + name + ".png";
    }
  public slots:
    void qmlEngineAvailable(QQmlEngine *engine) { engine->rootContext()->setContextProperty("testFixtures", this); }
};
QUICK_TEST_MAIN_WITH_SETUP(compass, Setup)
#include "quick_test.moc"
