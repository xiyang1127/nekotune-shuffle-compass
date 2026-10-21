#include "algorithm.h"
#include <QSet>
#include <algorithm>
#include <cmath>
#include <limits>
#include <random>

using namespace nekotune;

namespace compass {
namespace {
QString identity(const sdk::ShuffleTrack &track) {
    return track.songId > 0 ? QStringLiteral("song:%1").arg(track.songId)
                            : QStringLiteral("queue:%1").arg(track.queueId);
}
bool same(const QString &a, const QString &b) {
    return !a.trimmed().isEmpty() && a.simplified().compare(b.simplified(), Qt::CaseInsensitive) == 0;
}
double similarity(const sdk::ShuffleTrack &track, const sdk::ShuffleTrack &current) {
    return (same(track.artist, current.artist) ? .7 : 0) + (same(track.album, current.album) ? .3 : 0);
}
bool number(const QVariant &value) {
    switch (value.typeId()) {
    case QMetaType::Double: case QMetaType::Float: case QMetaType::Int: case QMetaType::UInt:
    case QMetaType::LongLong: case QMetaType::ULongLong: return true;
    default: return false;
    }
}
} // namespace

sdk::Params Preferences::toParams() const { return {{"frequency", frequency}, {"variety", variety}}; }

Result<Preferences> parsePreferences(const sdk::Params &params, Preferences defaults) {
    for (const auto &key : {QStringLiteral("frequency"), QStringLiteral("variety")}) {
        if (!params.contains(key)) continue;
        const auto value = params.value(key);
        const auto amount = value.toDouble();
        if (!number(value) || !std::isfinite(amount) || amount < -1 || amount > 1)
            return failure(key + " must be a number between -1 and 1");
        if (key == "frequency") defaults.frequency = amount;
        else defaults.variety = amount;
    }
    return defaults;
}

Result<sdk::ShuffleSelection> selectTrack(const sdk::ShuffleRequest &request,
                                        const Preferences &preferences) {
    if (request.queue.isEmpty()) return failure("Queue is empty");
    const sdk::ShuffleTrack *current = nullptr;
    for (const auto &track : request.queue)
        if (track.queueId == request.currentId) current = &track;
    QStringList recent = request.state.value("recent_songs").toStringList();
    if (current) {
        recent.removeAll(identity(*current));
        recent.append(identity(*current));
    }
    const QSet<QString> recentIds(recent.cbegin(), recent.cend());
    QList<const sdk::ShuffleTrack *> candidates;
    for (const auto &track : request.queue) {
        if (current && identity(track) == identity(*current)) continue;
        candidates.append(&track);
    }
    // Current-song avoidance is strict when possible; recent songs get a soft penalty so the
    // preference axes still affect small playlists instead of forcing a fixed rotation.
    if (candidates.isEmpty()) {
        for (const auto &track : request.queue)
            if (track.queueId != request.currentId) candidates.append(&track);
        if (candidates.isEmpty()) candidates.append(&request.queue.first());
    }
    double minimum = std::numeric_limits<double>::max(), maximum = 0;
    for (const auto *track : candidates) {
        const double played = std::log1p(static_cast<double>(qMax<qint64>(0, track->playCount)));
        minimum = qMin(minimum, played); maximum = qMax(maximum, played);
    }
    QList<double> weights;
    double total = 0;
    for (const auto *track : candidates) {
        const double played = std::log1p(static_cast<double>(qMax<qint64>(0, track->playCount)));
        const double frequency = maximum > minimum ? 2 * (played - minimum) / (maximum - minimum) - 1 : 0;
        const double variety = current ? 1 - 2 * similarity(*track, *current) : 0;
        const double recency = recentIds.contains(identity(*track)) ? .2 : 1.0;
        const double weight = recency * std::exp(2.8 * (preferences.frequency * frequency + preferences.variety * variety));
        weights.append(weight); total += weight;
    }
    std::mt19937 random(request.seed);
    double draw = static_cast<double>(random()) / (static_cast<double>(std::mt19937::max()) + 1) * total;
    int selected = candidates.size() - 1;
    for (int i = 0; i < candidates.size(); ++i) {
        draw -= weights.at(i);
        if (draw < 0) { selected = i; break; }
    }
    const auto *track = candidates.at(selected);
    recent.removeAll(identity(*track)); recent.append(identity(*track));
    while (recent.size() > 3) recent.removeFirst();
    return sdk::ShuffleSelection{track->queueId, {{"recent_songs", recent}}};
}
} // namespace compass
