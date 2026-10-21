#pragma once
#include <nekotune/sdk/shuffle_provider.h>

namespace compass {
struct Preferences {
    double frequency = 0; // -1: least played; +1: most played.
    double variety = 0;   // -1: similar artist/album; +1: different artist/album.
    bool operator==(const Preferences &) const = default;
    nekotune::sdk::Params toParams() const;
};
nekotune::Result<Preferences> parsePreferences(const nekotune::sdk::Params &params,
                                              Preferences defaults = {});
nekotune::Result<nekotune::sdk::ShuffleSelection> selectTrack(
    const nekotune::sdk::ShuffleRequest &request, const Preferences &preferences);
} // namespace compass
