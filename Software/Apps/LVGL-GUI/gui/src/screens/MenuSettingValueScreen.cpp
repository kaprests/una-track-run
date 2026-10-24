/**
 ******************************************************************************
 * @file    MenuSettingValueScreen.cpp
 * @brief   Track setting value lists (see MenuSettingValueScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/MenuSettingValueScreen.hpp"
#include "gui/screens/ScreenManager.hpp"

#include <algorithm>
#include <cmath>

namespace
{
using L = Settings::Limits;

struct Range {
    const char* title;
    const char* format;
    float       first;
    float       step;
    uint16_t    count;
    float     (*get)(const Settings&);
    void      (*set)(Settings&, float);
};

constexpr uint16_t countOf(float first, float last, float step)
{
    return static_cast<uint16_t>((last - first) / step + 1.5f);
}

constexpr Range kRanges[] = {
    { "LAP LENGTH", "%.0f m", L::kLapLengthMinM, L::kLapLengthStepM,
      countOf(L::kLapLengthMinM, L::kLapLengthMaxM, L::kLapLengthStepM),
      [](const Settings& s) { return s.lapLengthM; },
      [](Settings& s, float v) { s.lapLengthM = v; } },
    { "LANE", "%.0f", L::kLaneMin, 1.0f,
      countOf(L::kLaneMin, L::kLaneMax, 1.0f),
      [](const Settings& s) { return static_cast<float>(s.lane); },
      [](Settings& s, float v) { s.lane = static_cast<uint8_t>(v); } },
    { "LANE WIDTH", "%.2f m", L::kLaneWidthMinM, L::kLaneWidthStepM,
      countOf(L::kLaneWidthMinM, L::kLaneWidthMaxM, L::kLaneWidthStepM),
      [](const Settings& s) { return s.laneWidthM; },
      [](Settings& s, float v) { s.laneWidthM = v; } },
    { "MISSED LAP", "%.0f %%", L::kMissedPctMin, L::kMissedPctStep,
      countOf(L::kMissedPctMin, L::kMissedPctMax, L::kMissedPctStep),
      [](const Settings& s) { return static_cast<float>(s.missedLapPct); },
      [](Settings& s, float v) { s.missedLapPct = static_cast<uint8_t>(v); } },
    { "GUARD LEVEL", "%.0f %%", L::kEarlyPctMin, L::kEarlyPctStep,
      countOf(L::kEarlyPctMin, L::kEarlyPctMax, L::kEarlyPctStep),
      [](const Settings& s) { return static_cast<float>(s.earlyPressPct); },
      [](Settings& s, float v) { s.earlyPressPct = static_cast<uint8_t>(v); } },
};

constexpr bool fitsLists()
{
    for (const Range& range : kRanges) {
        if (range.count > MenuSettingValueScreen::kMaxCount) {
            return false;
        }
    }
    return true;
}
static_assert(fitsLists(), "a value list exceeds MenuSettingValueScreen::kMaxCount");

const Range& rangeOf(MenuSettingValueScreen::Kind kind) { return kRanges[static_cast<size_t>(kind)]; }
} // namespace

MenuSettingValueScreen::MenuSettingValueScreen(Model& model, Kind kind)
    : Screen(model)
    , mKind(kind)
{
}

void MenuSettingValueScreen::build()
{
    const Range& range = rangeOf(mKind);
    mCount = range.count;
    for (uint16_t i = 0; i < mCount; ++i) {
        snprintf(mTexts[i], sizeof(mTexts[i]), range.format, range.first + range.step * static_cast<float>(i));
        mItems[i] = { WheelMenu::Item::Style::Simple, mTexts[i] };
    }
    mMenu    = std::make_unique<WheelMenu>(mRoot, mItems, mCount);
    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::NONE, Widgets::Buttons::NONE,
                  Widgets::Buttons::AMBER, Widgets::Buttons::WHITE);
    mTitle   = std::make_unique<Widgets::Title>(mRoot, range.title);
}

void MenuSettingValueScreen::onShow()
{
    const Range& range = rangeOf(mKind);
    const float  index = std::round((range.get(mModel.getSettings()) - range.first) / range.step);
    mMenu->select(static_cast<uint16_t>(std::clamp(index, 0.0f, static_cast<float>(mCount - 1))));
    mModel.resetIdleTimer();
}

void MenuSettingValueScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
        case Btn::L1: mMenu->prev(); break;
        case Btn::L2: mMenu->next(); break;
        case Btn::R1: save(); ScreenManager::instance().goTo(ScreenId::MenuSettings); break;
        case Btn::R2: ScreenManager::instance().goTo(ScreenId::MenuSettings); break;
        default: break;
    }
}

void MenuSettingValueScreen::save()
{
    const Range& range = rangeOf(mKind);
    Settings sett = mModel.getSettings();
    range.set(sett, range.first + range.step * static_cast<float>(mMenu->selected()));
    mModel.saveSettings(sett);
}
