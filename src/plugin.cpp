#include "algorithm.h"
#include <nekotune/sdk/extension_plugin.h>
#include <QObject>
#include <vector>

using namespace nekotune;

class CompassProvider final : public sdk::IShuffleProvider {
  public:
    compass::Preferences preferences;
    Result<sdk::ShuffleSelection> select(const sdk::ShuffleRequest &request) override {
        return compass::selectTrack(request, preferences);
    }
};

class CompassPlugin final : public QObject, public sdk::IExtensionPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID NEKOTUNE_EXTENSION_IID)
    Q_INTERFACES(nekotune::sdk::IExtensionPlugin)
  public:
    void activate(sdk::IExtensionContext &context, std::function<void(Result<void>)> done) override {
        auto *capability = sdk::shuffleContext(context);
        if (!capability) { done(failure("Shuffle Compass requires a host with SDK 2.1 shuffle support")); return; }
        const auto settings = compass::parsePreferences(context.config());
        if (!settings) { done(settings.error()); return; }
        m_context = &context;
        m_shuffle = capability;
        m_provider.preferences = settings.value();
        if (!registerStrategy()) { done(failure("Unable to register Shuffle Compass")); return; }
        m_handles.push_back(context.registerService("settings", [this](const sdk::Params &, sdk::Reply reply) {
            reply(m_provider.preferences.toParams());
        }));
        m_handles.push_back(context.on("extension.config_changed", [this](const sdk::Params &event) {
            const auto settings = compass::parsePreferences(event.value("config").toMap());
            if (!settings) { m_context->log(QtWarningMsg, settings.error().message); return; }
            if (settings.value() != m_provider.preferences) {
                m_provider.preferences = settings.value();
                // A new registration invalidates pending selections/preloads using old preferences.
                if (!registerStrategy()) m_context->log(QtWarningMsg, "Unable to refresh Shuffle Compass");
            }
            m_context->publish("settings_changed", m_provider.preferences.toParams());
        }));
        m_handles.push_back(context.registerService("configure", [this](const sdk::Params &params, sdk::Reply reply) {
            const auto settings = compass::parsePreferences(params, m_provider.preferences);
            if (!settings) { reply(settings.error()); return; }
            auto config = m_context->config();
            const auto values = settings.value().toParams();
            for (auto it = values.cbegin(); it != values.cend(); ++it) config.insert(it.key(), it.value());
            m_context->updateConfig(config, [this, reply](Result<void> saved) {
                if (!saved) { reply(saved.error()); return; }
                reply(m_provider.preferences.toParams());
            });
        }));
        m_handles.push_back(context.registerService("activate", [this](const sdk::Params &, sdk::Reply reply) {
            m_context->hostCall("player.set_shuffle_strategy", {{"id", m_context->id() + "/compass"}},
                [this, reply](Result<sdk::Params> result) {
                    if (!result) { reply(result.error()); return; }
                    m_context->hostCall("player.set_playback_mode", {{"mode", "shuffle"}}, reply);
                });
        }));
        done({});
    }
    void deactivate() override {
        m_strategy.reset();
        m_handles.clear();
        m_context = nullptr; m_shuffle = nullptr;
        m_provider.preferences = {};
    }
  private:
    bool registerStrategy() {
        m_strategy.reset();
        m_strategy = m_shuffle->registerShuffleStrategy("compass", &m_provider,
            {{"name", "漫游罗盘 · Shuffle Compass"}});
        return bool(m_strategy);
    }
    sdk::IExtensionContext *m_context = nullptr;
    sdk::IShuffleExtensionContext *m_shuffle = nullptr;
    CompassProvider m_provider;
    sdk::RegistrationPtr m_strategy;
    std::vector<sdk::RegistrationPtr> m_handles;
};

#include "plugin.moc"
