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
using Lap = Settings::Lap;

struct Range {
    const char* title;
    const char* format;
    float       first;
    float       step;
    uint16_t    count;
};

constexpr uint16_t countOf(float first, float last, float step)
{
    return static_cast<uint16_t>((last - first) / step + 1.5f);
}

constexpr Range kRanges[] = {
    { "LAP LENGTH", "%.0f m", Lap::kLengthMinM,    Lap::kLengthStepM,    countOf(Lap::kLengthMinM, Lap::kLengthMaxM, Lap::kLengthStepM) },
    { "LANE",       "%.0f",   Lap::kLaneMin,       1.0f,                 countOf(Lap::kLaneMin, Lap::kLaneMax, 1.0f) },
    { "LANE WIDTH", "%.2f m", Lap::kLaneWidthMinM, Lap::kLaneWidthStepM, countOf(Lap::kLaneWidthMinM, Lap::kLaneWidthMaxM, Lap::kLaneWidthStepM) },
};

const Range& rangeOf(MenuSettingValueScreen::Kind kind) { return kRanges[static_cast<size_t>(kind)]; }

float valueOf(const Settings& s, MenuSettingValueScreen::Kind kind)
{
    switch (kind) {
        case MenuSettingValueScreen::Kind::LapLength: return s.lapLengthM;
        case MenuSettingValueScreen::Kind::Lane:      return static_cast<float>(s.lane);
        default:                                      return s.laneWidthM;
    }
}

void assign(Settings& s, MenuSettingValueScreen::Kind kind, float value)
{
    switch (kind) {
        case MenuSettingValueScreen::Kind::LapLength: s.lapLengthM = value; break;
        case MenuSettingValueScreen::Kind::Lane:      s.lane       = static_cast<uint8_t>(value); break;
        default:                                      s.laneWidthM = value; break;
    }
}
} // namespace

static_assert(kRanges[0].count <= 81 && kRanges[1].count <= 81 && kRanges[2].count <= 81,
              "value lists are sized for 81 entries");

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
    const float  index = std::round((valueOf(mModel.getSettings(), mKind) - range.first) / range.step);
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
    assign(sett, mKind, range.first + range.step * static_cast<float>(mMenu->selected()));
    mModel.saveSettings(sett);
}
