/**
 ******************************************************************************
 * @file    ScreenshotTour.cpp
 * @brief   Simulator-only screenshot tour (see ScreenshotTour.hpp).
 *
 * Each step presses a key, waits, then saves the active screen as raw RGB565
 * (<name>.rgb565, 240 x 240); tools/screenshots.py turns them into PNGs.
 ******************************************************************************
 */

#include "ScreenshotTour.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "lvgl.h"
#include "SDK/GUI/Button.hpp"

namespace
{
namespace Btn = SDK::GUI::Button;

struct Step {
    uint32_t    key;    ///< 0 = no key
    uint32_t    waitMs;
    const char* shot;   ///< nullptr = no screenshot
};

constexpr uint32_t kSlide = 1000;   ///< a wheel slide and its redraw

constexpr Step kIndoor[] = {
    {0,       2000,   "01-main"},
    {Btn::L2, kSlide, nullptr},
    {Btn::L2, kSlide, nullptr},
    {Btn::R1, kSlide, "02-settings"},
    {Btn::L2, kSlide, nullptr},
    {Btn::L2, kSlide, nullptr},
    {Btn::R1, kSlide, "03-lap-length"},
    {Btn::R2, kSlide, nullptr},
    {Btn::R2, kSlide, nullptr},
    {Btn::L1, kSlide, nullptr},
    {Btn::L1, kSlide, nullptr},
    {Btn::R1, 8000,   "04-track"},       // live pace after a few cadence samples
    {Btn::L2, kSlide, "05-track-lap"},
    {Btn::L2, kSlide, "06-track-status"},
    {Btn::L2, 52000,  nullptr},          // back to the first face; run most of a 200 m lap
    {Btn::R2, 1500,   "07-lap"},
    {0,       5000,   nullptr},          // the lap popup dismisses itself
    {Btn::R1, kSlide, "08-pause"},
    {Btn::L2, kSlide, nullptr},
    {Btn::R1, kSlide, "09-summary"},
};

constexpr Step kOutdoor[] = {
    {0,       7000,   "10-main-gps"},    // after the GPS fix
    {Btn::L2, kSlide, nullptr},
    {Btn::L2, kSlide, nullptr},
    {Btn::R1, kSlide, "11-settings-outdoor"},
    {Btn::R2, kSlide, nullptr},
    {Btn::L1, kSlide, nullptr},
    {Btn::L1, kSlide, nullptr},
    {Btn::R1, 10000,  "12-track-gps"},
};

constexpr int32_t kSize = 240;

LV_DRAW_BUF_DEFINE_STATIC(sBuf, kSize, kSize, LV_COLOR_FORMAT_RGB565);

std::string sDir;
const Step* sNext = nullptr;
const Step* sEnd  = nullptr;
const char* sShot = nullptr;

void save(const char* name)
{
    if (lv_snapshot_take_to_draw_buf(lv_screen_active(), LV_COLOR_FORMAT_RGB565, &sBuf) != LV_RESULT_OK) {
        std::fprintf(stderr, "Screenshot %s failed\n", name);
        return;
    }
    const std::string path = sDir + "/" + name + ".rgb565";
    if (FILE* f = std::fopen(path.c_str(), "wb")) {
        for (int32_t y = 0; y < kSize; ++y) {
            std::fwrite(sBuf.data + y * sBuf.header.stride, 2, kSize, f);
        }
        std::fclose(f);
    }
}

void tick(lv_timer_t* timer)
{
    if (sShot) {
        save(sShot);
    }
    if (sNext == sEnd) {
        // Skip the simulator's shutdown: it is slow and not needed here.
        std::fflush(nullptr);
        std::_Exit(0);
    }
    const Step& step = *sNext++;
    if (step.key) {
        uint32_t key = step.key;
        lv_obj_send_event(lv_screen_active(), LV_EVENT_KEY, &key);
    }
    sShot = step.shot;
    lv_timer_set_period(timer, step.waitMs);
}

template <std::size_t N>
void run(const Step (&steps)[N])
{
    sNext = steps;
    sEnd  = steps + N;
    LV_DRAW_BUF_INIT_STATIC(sBuf);
    lv_timer_create(&tick, 1, nullptr);
}
} // namespace

void ScreenshotTour::startIfRequested()
{
    const char* dir = std::getenv("TRACKRUNCE_SCREENSHOT_DIR");
    if (!dir) {
        return;
    }
    sDir = dir;
    const char* tour = std::getenv("TRACKRUNCE_SCREENSHOT_TOUR");
    if (tour && std::strcmp(tour, "outdoor") == 0) {
        run(kOutdoor);
    } else {
        run(kIndoor);
    }
}
