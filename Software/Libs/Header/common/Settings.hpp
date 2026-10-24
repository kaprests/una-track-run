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
    static constexpr uint8_t kVersion = 3;

    /// Indoor: no GPS, pace from cadence. Outdoor: GPS is connected as well.
    enum class Mode : uint8_t { Indoor, Outdoor };

    /// Ranges of the track settings.
    struct Limits {
        static constexpr float   kLapLengthMinM  = 100.0f;
        static constexpr float   kLapLengthMaxM  = 500.0f;
        static constexpr float   kLapLengthStepM = 5.0f;
        static constexpr uint8_t kLaneMin        = 1;
        static constexpr uint8_t kLaneMax        = 8;
        static constexpr float   kLaneWidthMinM  = 0.80f;
        static constexpr float   kLaneWidthMaxM  = 1.30f;
        static constexpr float   kLaneWidthStepM = 0.05f;
        static constexpr uint8_t kMissedPctMin   = 10;
        static constexpr uint8_t kMissedPctMax   = 50;
        static constexpr uint8_t kMissedPctStep  = 5;
        static constexpr uint8_t kEarlyPctMin    = 30;
        static constexpr uint8_t kEarlyPctMax    = 90;
        static constexpr uint8_t kEarlyPctStep   = 10;
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


    uint32_t version = kVersion;

    bool     autoPauseEn       = false;         ///< Serialized only; auto-pause is not implemented.
    bool     phoneNotifEn      = true;          ///< Phone notifications while the app runs.

    Mode     mode              = Mode::Indoor;
    float    lapLengthM        = 200.0f;        ///< Length of the inner lane.
    uint8_t  lane              = 1;             ///< Lane run in; 1 is the inner lane.
    float    laneWidthM        = 1.0f;

    bool     lapReminderEn     = true;          ///< Vibrate shortly before the lap or rep mark.
    uint8_t  missedLapPct      = 20;            ///< Past the mark by this share of the lap, the press counts as forgotten.
    bool     earlyPressGuardEn = true;          ///< Ignore a press made well before the mark.
    uint8_t  earlyPressPct     = 50;            ///< Share of the lap a press must have covered to count.

    Intervals intervals {};

    /// Distance of one lap in the chosen lane (each lane outward adds a full circle of its width).
    float laneLapLengthM() const
    {
        constexpr float kTwoPi = 6.2831853f;
        return lapLengthM + kTwoPi * laneWidthM * static_cast<float>(lane - 1);
    }

    float missedLapFraction() const  { return static_cast<float>(missedLapPct) / 100.0f; }
    float earlyPressFraction() const { return static_cast<float>(earlyPressPct) / 100.0f; }
};

#endif // SETTINGS_HPP
