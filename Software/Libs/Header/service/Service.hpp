#ifndef SERVICE_HPP
#define SERVICE_HPP

#include <ctime>
#include <optional>

#include "SDK/Kernel/Kernel.hpp"
#include "SDK/SensorLayer/SensorConnection.hpp"
#include "SDK/SensorLayer/SensorDataBatch.hpp"
#include "SDK/Metrics/MonotonicTime.hpp"
#include "SDK/Metrics/MonotonicCounter.hpp"
#include "SDK/Metrics/VariableCounter.hpp"
#include "SDK/Metrics/SpeedSmoother.hpp"
#include "SDK/Metrics/ThrottledSample.hpp"
#include "SDK/Calibration/CadenceStrideModel.hpp"
#include "SDK/Calibration/TreadmillSpeedEstimator.hpp"

#include "CalibrationStore.hpp"
#include "SegmentCorrector.hpp"
#include "StartLine.hpp"
#include "TrackDistance.hpp"
#include "SettingsSerializer.hpp"
#include "ActivitySummarySerializer.hpp"
#include "ActivityWriter.hpp"
#include "Commands.hpp"
#include "WristTiltDetector.hpp"

class Service : public WristTiltDetector::IListener
{
public:
    Service(SDK::Kernel &kernel);

    virtual ~Service();

    void run();

private:
    // -- Constants ------------------------------------------------------------

    static constexpr uint32_t skBacklightTimeout     = 5000;
    static constexpr uint32_t skSamplePeriod         = 1000;
    static constexpr uint32_t skSampleLatency        = 1000;

    static constexpr float    skReminderDistanceM        = 15.0f;
    static constexpr float    skAutoLapMinFraction       = 0.8f;
    static constexpr uint32_t skPhaseEndPromptPeriodSec  = 10;

    static constexpr uint32_t skBatteryLogPeriodMs   = 5 * 60 * 1000;
    static constexpr float    skFusionSampleRateHz   = 100.0f;

    /// Window, in 1 Hz ticks, of the outdoor GPS pace readout.
    static constexpr std::size_t skPaceSmoothingTicks = 10;

    // -- Infrastructure -------------------------------------------------------

    SDK::Kernel&          mKernel;
    bool                  mGuiStarted;

    // -- Settings & persistence -----------------------------------------------

    Settings                  mSettings;
    bool                      mIsImperial = false;
    bool                      mTimeFormat12h = false;
    SettingsSerializer        mSettingsSerializer;
    ActivitySummary           mSummary;
    ActivitySummarySerializer mActivitySummarySerializer;
    ActivityWriter            mActivityWriter;

    // -- Sensors --------------------------------------------------------------

    SDK::Sensor::Connection mSensorGpsLocation;
    SDK::Sensor::Connection mSensorGpsSpeed;
    SDK::Sensor::Connection mSensorGpsDistance;
    SDK::Sensor::Connection mSensorHr;
    SDK::Sensor::Connection mSensorBatteryLevel;
    SDK::Sensor::Connection mSensorBatteryMetrics;
    SDK::Sensor::Connection mSensorWristMotion;
    SDK::Sensor::Connection mSensorFusion;
    SDK::Sensor::Connection mSensorRunningCadence;
    bool                    mIsSensorsConnected = false;

    struct {
        float cadenceSpm      = 0.0f;
        bool  cadenceValid    = false;
    } mRunningCadence{};

    // -- Cadence pace (indoor mode) -------------------------------------------

    // mModel must be declared before mEstimator: the estimator holds a reference to it.
    SDK::Calibration::CadenceStrideModel      mModel;
    SDK::Calibration::TreadmillSpeedEstimator mEstimator;
    float                                     mHeightM     = 1.70f;
    std::time_t                               mLastTickUtc = 0;

    // Known-distance segments (track laps, interval reps) correct the model's distance.
    CalibrationStore     mCalibrationStore;
    SegmentCorrector     mCorrector;
    TrackDistance        mDistance{ Settings{}.missedLapFraction() };
    std::time_t          mSegmentStartActiveSec = 0;
    std::optional<float> mRemindedMarkM;
    bool                 mAnchoredThisSession   = false;
    bool                 mInWarmUp              = false;
    StartLine            mStartLine;

    // -- GPS speed and distance (outdoor mode) --------------------------------

    float       mGpsSpeedMs       = 0.0f; ///< Latest raw GPS speed (instantaneous source).
    bool        mGpsSpeedValid    = false;
    bool        mGpsSpeedFresh    = false; ///< A speed sample arrived since the last track tick.
    bool        mGpsDeadReckoning = false;
    SDK::Metric::MonotonicCounter<float> mGpsDistance; ///< Active GPS distance, raw source outdoors.

    // -- Metrics --------------------------------------------------------------

    SDK::Metric::MonotonicTime<SDK::Interface::ISystem> mTimeTracker;
    SDK::Metric::MonotonicCounter<std::time_t>          mTimeCounter;
    SDK::Metric::MonotonicCounter<float>                mDistanceCounter;
    SDK::Metric::VariableCounter                        mSpeedCounter;
    /// Smooths the outdoor GPS speed for the live readout only.
    SDK::Metric::SpeedSmoother<skPaceSmoothingTicks>    mSpeedSmoother;
    SDK::Metric::VariableCounter                        mHrCounter;
    uint8_t                                             mHrSource = 0;      ///< Latest HR source (HeartRateEx::Source) for the icon + FIT hr_source.
    uint8_t                                             mHrOpticalBpm = 0;  ///< Latest raw optical (PPG) bpm, for the FIT hr_optical series.
    uint8_t                                             mHrExternalBpm = 0; ///< Latest raw external (strap) bpm, for the FIT hr_external series.

    // Battery SoC and voltage are sampled independently;
    // a FIT record is written only when both are due.
    SDK::Metric::ThrottledSample<float, SDK::Interface::ISystem> mBatterySoc;     ///< State of charge, percent
    SDK::Metric::ThrottledSample<float, SDK::Interface::ISystem> mBatteryVoltage; ///< Voltage, volts

    // -- GPS state ------------------------------------------------------------

    struct {
        bool     fix;       // Actual GPS fix
        float    latitude;  // degrees
        float    longitude; // degrees
        uint32_t timestamp; // ms

        void reset()
        {
            fix       = false;
            latitude  = 0.0f;
            longitude = 0.0f;
            timestamp = 0;
        }
    } mGps{};

    // -- Track state ----------------------------------------------------------

    Track::State mTrackState          = Track::State::INACTIVE;
    bool         mPreviousGpsFixState = false;
    bool         mGpsInitialConnectFailed = false;  ///< GPS_LOCATION subscribe lost the startup ack race; the retry logs the recovery.
    bool         mGpsWanted = false;                 ///< GPS_LOCATION should stay connected (pre-activity + active track); cleared in disconnect() so the retry never re-wakes the GNSS post-activity.
    bool         mSessionNotEmpty     = false;
    bool         mLapNotEmpty         = false;
    Track::Data  mTrackData{};

    // -- Interval training state ----------------------------------------------

    bool        mIntervalsMode        = false;
    bool        mIntervalsCompleted   = false; ///< Set after workout completed; blocks further phase processing
    std::time_t mPhaseStartActiveSec  = 0;     ///< mTimeCounter.getValueActive() at phase start
    float       mPhaseStartActiveDist = 0.0f;  ///< mDistanceCounter.getValueActive() at phase start

    /// Maps interval phases to workout_step message_index values for the FIT
    /// workout description (0xFFFF = no associated step).
    struct IntervalsStepMap {
        bool     valid       = false;
        uint16_t warmUpIdx   = 0xFFFF;
        uint16_t runIdx      = 0xFFFF;
        uint16_t restIdx     = 0xFFFF;
        uint16_t finalRunIdx = 0xFFFF; ///< last RUN step when the final rest is skipped
        uint16_t coolDownIdx = 0xFFFF;
    };
    IntervalsStepMap mIntervalsStepMap;

    // -- Wrist tilt -----------------------------------------------------------

    WristTiltDetector mWristTiltDetector;

    // -- Lifecycle ------------------------------------------------------------

    void connectGps();
    void connectSensors(); // All except GPS
    void disconnect();
    void onStartGUI();
    void onStopGUI();

    // -- Sensor data dispatch -------------------------------------------------

    void handleSensorsData(uint16_t handle, SDK::Sensor::DataBatch& data);

    // -- Event handlers -------------------------------------------------------

    void handleEvent(const CustomMessage::TrackStart& event);
    void handleEvent(const CustomMessage::TrackStop& event);
    void handleEvent(const CustomMessage::SettingsSave& event);
    void handleEvent(const CustomMessage::TrackPause& event);
    void handleEvent(const CustomMessage::TrackResume& event);
    void handleEvent(const CustomMessage::ManualLap& event);
    void handleEvent(const CustomMessage::IntervalsNextPhase& event);

    // -- Track control --------------------------------------------------------

    struct LapEnd {
        ActivityWriter::LapTrigger trigger;
        bool                       anchored  = false;
        SDK::Fit::Intensity        intensity = SDK::Fit::Intensity::Active;
    };

    void sendInitialInfoToGui();
    void startTrack(std::time_t utc);
    void processTrack();
    Track::Lap saveLap(const LapEnd& end);
    /// Ends the current segment at its mark and corrects the pace with it. Returns the length it covered, when known.
    std::optional<float> endSegment();
    /// Ends a free-run lap (or the warm-up lap) and starts the next one.
    void completeLap(ActivityWriter::LapTrigger trigger);
    std::optional<TrackDistance::Segment> lapSegment() const;
    bool isEarlyPress() const;
    void rejectPress();
    void remindOfMark();
    /// Outdoors: sets the start line where measured laps begin, if it is not set yet.
    void placeStartLine();
    void detectAutoLap();
    void stopTrack(bool discard);
    void pauseTrack(bool pause);
    void buildPartialSummary();
    ActivityWriter::RecordData prepareRecordData();
    void applyMode();

    bool  usesGps() const { return mSettings.usesGps(); }
    /// The uncorrected distance and speed of the selected source.
    float rawDistanceM() const;
    float rawSpeedMps() const;
    CalibrationStore::Key calibrationKey() const;

    // -- Interval training ----------------------------------------------------

    void startIntervalsPhase(Track::IntervalsPhase phase);
    void advanceIntervalsPhase(bool manual = false);
    void processIntervals();
    void onIntervalsPhaseChange(bool alert, bool manual);

    /// The segment of a set distance phase on a track, which ends on the runner's press.
    std::optional<TrackDistance::Segment> phaseSegment() const;
    /// Ends the phase as its own lap, anchoring the pace to the phase distance when it has one.
    void closeIntervalsPhase(bool manual);
    SDK::Fit::Intensity phaseIntensity() const;
    /// Repeated prompt while a distance phase awaits the press.
    void promptPhaseEnd();

    /// Build the workout_step list from the intervals config, emit the workout /
    /// workout_step messages, and populate mIntervalsStepMap for lap referencing.
    void emitIntervalsWorkout();
    /// workout_step message_index for the current interval phase (0xFFFF = none).
    uint16_t intervalsWktStepIndex() const;

    // -- Notifications --------------------------------------------------------

    void setCapabilities();
    void requestAccessoryPrepare();   // opt in to external HR (pre-warm at GUI start)
    void requestAccessoryRelease();
    void notifyFirstFix();
    void notifyLapEnd();
    void notifyNewActivity();
    void backlightOn(uint32_t timeoutMs = skBacklightTimeout);
    void playBuzzerPattern(uint16_t beepMs, uint8_t count = 1, uint16_t silenceMs = 100);
    void playVibroPattern(SDK::Message::RequestVibroPlay::Effect effect, uint8_t count = 1, uint16_t silenceMs = 100);

    // -- WristTilt callback ---------------------------------------------------

    virtual void onWristTilt(uint32_t timestampMs) override;
};

#endif // SERVICE_HPP
