#ifndef CALIBRATION_STORE_HPP
#define CALIBRATION_STORE_HPP

#include <cstdint>
#include <vector>

#include "SDK/Kernel/Kernel.hpp"

#include "SegmentCorrector.hpp"

/**
 * The converged correction per track and distance source, kept in the app's
 * own file so the shared outdoor stride model is never touched.
 */
class CalibrationStore {
public:
    static constexpr std::size_t kCapacity          = 8;
    static constexpr float       kReturningVariance = 4.0e-4f; ///< a saved k is trusted to about 2 %

    enum class Source : uint8_t { IndoorCadence, OutdoorCadence, OutdoorGps };

    struct Key {
        float   lapLengthM;
        uint8_t lane;
        Source  source;
    };

    CalibrationStore(const SDK::Kernel& kernel, const char* path) : mKernel(kernel), mPath(path) {}

    bool load();
    bool save() const;

    /// The starting state for a session on this track.
    SegmentCorrector::State prior(const Key& key) const;
    void remember(const Key& key, SegmentCorrector::State state);

private:
    struct Entry {
        Key                     key;
        SegmentCorrector::State state;
    };

    std::vector<Entry>::const_iterator find(const Key& key) const;

    const SDK::Kernel& mKernel;
    const char*        mPath;
    std::vector<Entry> mEntries;
};

#endif // CALIBRATION_STORE_HPP
