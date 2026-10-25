/**
 ******************************************************************************
 * @file    MenuSettingsScreen.cpp
 * @brief   Settings menu (see MenuSettingsScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/MenuSettingsScreen.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/Assets.hpp"

#include <algorithm>
#include <iterator>

namespace
{
using Style = WheelMenu::Item::Style;
using Menu  = App::MenuNav::Root::Settings;

struct Entry {
    uint16_t    id;
    Style       style;
    const char* text;
    const char* itemText;
    bool        outdoorOnly;
};

struct Page {
    uint16_t id;
    ScreenId screen;
};

struct Toggle {
    uint16_t id;
    bool Settings::* field;
};

struct Choice {
    uint16_t id;
    void (*next)(Settings&);
};

constexpr Entry kEntries[] = {
    { Menu::ID_MODE,         Style::Tip,    "Mode",          nullptr,        false },
    { Menu::ID_PACE_SOURCE,  Style::Tip,    "Pace Source",   nullptr,        true  },
    { Menu::ID_AUTO_LAP,     Style::Toggle, "Auto\nLap",     "Auto Lap",     true  },
    { Menu::ID_WARM_UP,      Style::Toggle, "Warm-up\nLap",  "Warm-up Lap",  false },
    { Menu::ID_LAP_LENGTH,   Style::Tip,    "Lap Length",    nullptr,        false },
    { Menu::ID_LANE,         Style::Tip,    "Lane",          nullptr,        false },
    { Menu::ID_LANE_WIDTH,   Style::Tip,    "Lane Width",    nullptr,        false },
    { Menu::ID_LAP_REMINDER, Style::Toggle, "Lap\nReminder", "Lap Reminder", false },
    { Menu::ID_MISSED_LAP,   Style::Tip,    "Missed Lap",    nullptr,        false },
    { Menu::ID_EARLY_GUARD,  Style::Toggle, "Press\nGuard",  "Press Guard",  false },
    { Menu::ID_EARLY_MIN,    Style::Tip,    "Guard Level",   nullptr,        false },
    { Menu::ID_PHONE_NOTIF,  Style::Toggle, "Phone\nNotif.", "Phone Notif.", false },
};

constexpr Page kPages[] = {
    { Menu::ID_LAP_LENGTH, ScreenId::MenuLapLength },
    { Menu::ID_LANE,       ScreenId::MenuLane },
    { Menu::ID_LANE_WIDTH, ScreenId::MenuLaneWidth },
    { Menu::ID_MISSED_LAP, ScreenId::MenuMissedLap },
    { Menu::ID_EARLY_MIN,  ScreenId::MenuEarlyPress },
};

constexpr Toggle kToggles[] = {
    { Menu::ID_AUTO_LAP,     &Settings::autoLapEn },
    { Menu::ID_WARM_UP,      &Settings::warmUpLapEn },
    { Menu::ID_LAP_REMINDER, &Settings::lapReminderEn },
    { Menu::ID_EARLY_GUARD,  &Settings::earlyPressGuardEn },
    { Menu::ID_PHONE_NOTIF,  &Settings::phoneNotifEn },
};

constexpr Choice kChoices[] = {
    { Menu::ID_MODE, [](Settings& s) {
          s.setMode(s.usesGps() ? Settings::Mode::Indoor : Settings::Mode::Outdoor);
      } },
    { Menu::ID_PACE_SOURCE, [](Settings& s) {
          s.paceSource = s.paceSource == Settings::PaceSource::Gps ? Settings::PaceSource::Cadence
                                                                   : Settings::PaceSource::Gps;
      } },
};

template <typename T, size_t N>
const T* findById(const T (&table)[N], uint16_t id)
{
    const auto it = std::find_if(std::begin(table), std::end(table), [id](const T& t) { return t.id == id; });
    return it == std::end(table) ? nullptr : it;
}
} // namespace

MenuSettingsScreen::MenuSettingsScreen(Model& model)
    : Screen(model)
{
}

void MenuSettingsScreen::build()
{
    const bool outdoor = mModel.usesGps();
    for (const Entry& entry : kEntries) {
        if (!entry.outdoorOnly || outdoor) {
            mIds[mCount] = entry.id;
            mItems[mCount++] = { entry.style, entry.text, entry.itemText, &poppins_semibold_25, mTips[entry.id] };
        }
    }

    mMenu      = std::make_unique<WheelMenu>(mRoot, mItems, mCount);
    mButtons   = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::NONE, Widgets::Buttons::NONE,
                  Widgets::Buttons::AMBER, Widgets::Buttons::WHITE);
    mTitle     = std::make_unique<Widgets::Title>(mRoot, "SETTINGS");
    mSensorRow = std::make_unique<Widgets::SensorStatusRow>(mRoot, 0, 52, 240, 24);
}

void MenuSettingsScreen::onShow()
{
    const auto position = std::find(mIds, mIds + mCount, mModel.menu().settings.get());
    mMenu->select(static_cast<uint16_t>(position == mIds + mCount ? 0 : position - mIds));
    mModel.resetIdleTimer();
    onSettings(mModel.getSettings());
    onGpsFix(mModel.hasGpsFix());
    onAccessoryStatus(mModel.getAccessoryState(), "");
}

void MenuSettingsScreen::onHide()
{
    mModel.menu().settings.set(mIds[mMenu->selected()]);
}

void MenuSettingsScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
        case Btn::L1: mMenu->prev(); break;
        case Btn::L2: mMenu->next(); break;
        case Btn::R1: activate(mIds[mMenu->selected()]); break;
        case Btn::R2: ScreenManager::instance().goTo(ScreenId::Main); break;
        default: break;
    }
}

void MenuSettingsScreen::activate(uint16_t id)
{
    if (const Page* page = findById(kPages, id)) {
        ScreenManager::instance().goTo(page->screen);
        return;
    }

    Settings sett = mModel.getSettings();
    if (const Toggle* toggle = findById(kToggles, id)) {
        sett.*toggle->field = !(sett.*toggle->field);
    } else if (const Choice* choice = findById(kChoices, id)) {
        choice->next(sett);
    }
    mModel.saveSettings(sett);

    if (id == Menu::ID_MODE) {
        ScreenManager::instance().goTo(ScreenId::MenuSettings);
    } else {
        onSettings(sett);
    }
}

void MenuSettingsScreen::onSettings(const Settings& settings)
{
    snprintf(mTips[Menu::ID_MODE], kTipSize, "%s", settings.usesGps() ? "Outdoor" : "Indoor");
    snprintf(mTips[Menu::ID_PACE_SOURCE], kTipSize, "%s",
             settings.paceSource == Settings::PaceSource::Gps ? "GPS" : "Cadence");
    snprintf(mTips[Menu::ID_LAP_LENGTH], kTipSize, "%.0f m", settings.lapLengthM);
    snprintf(mTips[Menu::ID_LANE], kTipSize, "%u", static_cast<unsigned>(settings.lane));
    snprintf(mTips[Menu::ID_LANE_WIDTH], kTipSize, "%.2f m", settings.laneWidthM);
    snprintf(mTips[Menu::ID_MISSED_LAP], kTipSize, "%u %%", static_cast<unsigned>(settings.missedLapPct));
    snprintf(mTips[Menu::ID_EARLY_MIN], kTipSize, "%u %%", static_cast<unsigned>(settings.earlyPressPct));
    for (uint16_t i = 0; i < mCount; ++i) {
        if (const Toggle* toggle = findById(kToggles, mIds[i])) {
            mItems[i].toggleState = settings.*toggle->field;
        }
    }
    mMenu->refresh();
}

void MenuSettingsScreen::onGpsFix(bool acquired)
{
    mSensorRow->setGps(Widgets::SensorStatusRow::gpsState(mModel.usesGps(), acquired));
}

void MenuSettingsScreen::onAccessoryStatus(uint8_t state, const char* /*name*/)
{
    mSensorRow->setHr(Widgets::SensorStatusRow::hrState(state));
}
