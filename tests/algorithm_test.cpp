#include "algorithm.h"
#include <QtTest>
#include <limits>

using namespace nekotune;

class AlgorithmTest final : public QObject {
    Q_OBJECT
    sdk::ShuffleRequest request() const {
        return {{{10, 1, "Current", "Artist", "Album", 1000, 20},
                 {20, 2, "Rare", "Artist", "Album", 1000, 0},
                 {30, 3, "Frequent", "Other", "Other album", 1000, 1000}}, 10, 42, {}};
    }
    int count(int wanted, compass::Preferences preferences) const {
        auto input = request();
        int count = 0;
        for (quint32 seed = 0; seed < 3000; ++seed) {
            input.seed = seed;
            const auto selected = compass::selectTrack(input, preferences);
            if (selected && selected.value().queueId == wanted) ++count;
        }
        return count;
    }
  private slots:
    void frequencyBiasAndNeutral() {
        QVERIFY(count(20, {-1, 0}) > 2900);
        QVERIFY(count(30, {1, 0}) > 2900);
        const auto neutral = count(20, {});
        QVERIFY(neutral > 1300 && neutral < 1700);
    }
    void styleBiasAndCombinedAxes() {
        QVERIFY(count(20, {0, -1}) > 2900);
        QVERIFY(count(30, {0, 1}) > 2900);
        QVERIFY(count(20, {-1, -1}) > 2980);
        QVERIFY(count(30, {1, 1}) > 2980);
    }
    void deterministicAndTransactionalState() {
        const auto input = request();
        const auto first = compass::selectTrack(input, {});
        QVERIFY(first);
        for (int i = 0; i < 10; ++i) {
            const auto repeated = compass::selectTrack(input, {});
            QVERIFY(repeated);
            QCOMPARE(repeated.value().queueId, first.value().queueId);
            QCOMPARE(repeated.value().state, first.value().state);
        }
        QVERIFY(input.state.isEmpty());
        auto next = input;
        next.currentId = first.value().queueId;
        next.state = first.value().state;
        const auto second = compass::selectTrack(next, {});
        QVERIFY(second);
        QVERIFY(second.value().queueId != first.value().queueId);
        QVERIFY(second.value().state.value("recent_songs").toStringList().size() <= 3);
    }
    void duplicateSongsAndSmallQueues() {
        auto input = request();
        input.queue.insert(1, {11, 1, "Duplicate current", "Artist", "Album"});
        for (quint32 seed = 0; seed < 100; ++seed) {
            input.seed = seed;
            const auto selected = compass::selectTrack(input, {});
            QVERIFY(selected);
            QVERIFY(selected.value().queueId == 20 || selected.value().queueId == 30);
        }
        input.state = {{"recent_songs", QStringList{"song:1", "song:2", "song:3"}}};
        QVERIFY(compass::selectTrack(input, {})); // Recency never makes the candidate set empty.
        input.queue = {input.queue.at(0), input.queue.at(1)};
        QCOMPARE(compass::selectTrack(input, {}).value().queueId, 11);
        input.queue.removeLast();
        QCOMPARE(compass::selectTrack(input, {}).value().queueId, 10);
        input.queue.clear();
        QVERIFY(!compass::selectTrack(input, {}));
    }
    void recentPenaltyStillAllowsPreferenceControl() {
        auto input = request();
        input.state = {{"recent_songs", QStringList{"song:2"}}};
        int neutral = 0, rareBias = 0;
        for (quint32 seed = 0; seed < 3000; ++seed) {
            input.seed = seed;
            if (compass::selectTrack(input, {}).value().queueId == 20) ++neutral;
            if (compass::selectTrack(input, {-1, 0}).value().queueId == 20) ++rareBias;
        }
        QVERIFY(neutral > 350 && neutral < 650);
        QVERIFY(rareBias > 2800);
    }
    void incompleteMetadataAndExtremeCounts() {
        auto input = request();
        input.currentId = 0;
        input.queue[0].artist.clear(); input.queue[0].album.clear();
        input.queue[1].playCount = -10;
        input.queue[2].playCount = std::numeric_limits<qint64>::max();
        for (auto options : {compass::Preferences{-1, -1}, compass::Preferences{1, 1}})
            QVERIFY(compass::selectTrack(input, options));
        input.queue[0].songId = 0; input.queue[1].songId = 0;
        input.currentId = input.queue.first().queueId;
        const auto chosen = compass::selectTrack(input, {});
        QVERIFY(chosen);
        QVERIFY(chosen.value().queueId != input.currentId);
    }
    void strictPreferenceValidation() {
        for (const auto &bad : {QVariant("0.5"), QVariant(true), QVariant(), QVariant(2),
                                QVariant(std::numeric_limits<double>::quiet_NaN()),
                                QVariant(std::numeric_limits<double>::infinity())})
            QVERIFY(!compass::parsePreferences({{"frequency", bad}}));
        const auto options = compass::parsePreferences({{"frequency", -1}}, {.3, .7});
        QVERIFY(options);
        QCOMPARE(options.value().frequency, -1);
        QCOMPARE(options.value().variety, .7);
    }
};
QTEST_GUILESS_MAIN(AlgorithmTest)
#include "algorithm_test.moc"
