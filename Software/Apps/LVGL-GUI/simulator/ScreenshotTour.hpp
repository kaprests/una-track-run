/**
 ******************************************************************************
 * @file    ScreenshotTour.hpp
 * @brief   Simulator-only: press through the app and save screenshots.
 *
 * Runs only when TRACKRUNCE_SCREENSHOT_DIR is set (see `mise run screenshots`).
 * TRACKRUNCE_SCREENSHOT_TOUR selects the script: "indoor" (default) or "outdoor".
 ******************************************************************************
 */

#ifndef SCREENSHOT_TOUR_HPP
#define SCREENSHOT_TOUR_HPP

namespace ScreenshotTour
{
/// Start the tour on the LVGL thread if TRACKRUNCE_SCREENSHOT_DIR is set.
void startIfRequested();
} // namespace ScreenshotTour

#endif // SCREENSHOT_TOUR_HPP
