/**
 ******************************************************************************
 * @file    Settings.hpp
 * @date    08-04-2025
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Application settings structure and alert sub-types.
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include <cstdint>

/**
 * @brief Application settings persisted to storage.
 */
struct Settings {
    /// Settings version.
    static constexpr uint8_t kVersion = 2;

    /// Indoor: no GPS, pace from cadence. Outdoor: GPS is connected as well.
    enum class Mode : uint8_t { Indoor, Outdoor };

    /// The lap the track mode corrects its pace against.
    struct Lap {
        static constexpr float   kLengthMinM  = 100.0f;
        static constexpr float   kLengthMaxM  = 500.0f;
        static constexpr float   kLengthStepM = 5.0f;
        static constexpr uint8_t kLaneMin     = 1;
        static constexpr uint8_t kLaneMax     = 8;
        static constexpr float   kLaneWidthMinM  = 0.80f;
        static constexpr float   kLaneWidthMaxM  = 1.30f;
        static constexpr float   kLaneWidthStepM = 0.05f;
    };

    /**
     * @struct Intervals
     * @brief Interval training configuration (WARM_UP-RUN-REST-COOL_DOWN cycles).
     */
    struct Intervals {

        static constexpr uint8_t kRepeatsMax = 20; ///< Maximum number of RUN-REST repetitions.

        /**
         * @brief Metric used to measure the duration of a phase.
         */
        enum Metric {
            OPEN = 0,   ///< Phase has no duration limit.
            TIME,       ///< Phase duration is measured by time.
            DISTANCE,   ///< Phase duration is measured by distance.
        };

        uint8_t  repeatsNum    = 0;     ///< Number of repetitions of RUN-REST cycles. 0 - 'Open'

        Metric   runMetric     = Metric::OPEN; ///< Metric used to measure the RUN phase duration.
        uint32_t runTime       = 0;     ///< Duration of the RUN phase in seconds. 0 - 'Open'
        float    runDistance   = 0.0f;  ///< Duration of the RUN phase in meters. 0 - 'Open'

        Metric   restMetric    = Metric::OPEN; ///< Metric used to measure the REST phase duration.
        uint32_t restTime      = 0;     ///< Duration of the REST phase in seconds. 0 - 'Open'
        float    restDistance  = 0.0f;  ///< Duration of the REST phase in meters. 0 - 'Open'

        bool     warmUp        = true;  ///< Enable WARM_UP phase.
        bool     coolDown      = true;  ///< Enable COOL_DOWN phase.
        bool     lastRest      = true;  ///< Include the REST after the final interval; false -> skip straight to COOL_DOWN.
    };


    // Fields
    uint32_t version = kVersion;    ///< Settings version.

    bool autoPauseEn = false; ///< Serialized only; not on settings wheel until auto-pause is implemented.
    bool     phoneNotifEn  = true;  ///< Flag to enable receiving phone notification when app is run.

    Mode     mode          = Mode::Indoor;
    float    lapLengthM    = 200.0f; ///< Length of the track's inner lane.
    uint8_t  lane          = 1;      ///< Lane the user runs in; 1 is the inner lane.
    float    laneWidthM    = 1.0f;

    Intervals intervals {};  ///< Interval training settings.

    /// Distance of one lap in the chosen lane (each lane outward adds a full circle of its width).
    float laneLapLengthM() const
    {
        constexpr float kTwoPi = 6.2831853f;
        return lapLengthM + kTwoPi * laneWidthM * static_cast<float>(lane - 1);
    }
};

#endif // SETTINGS_HPP
