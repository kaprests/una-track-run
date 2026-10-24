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

struct Page {
    uint16_t item;
    ScreenId screen;
};

struct Toggle {
    uint16_t item;
    bool Settings::* field;
};

constexpr Page kPages[] = {
    { Menu::ID_LAP_LENGTH, ScreenId::MenuLapLength },
    { Menu::ID_LANE,       ScreenId::MenuLane },
    { Menu::ID_LANE_WIDTH, ScreenId::MenuLaneWidth },
    { Menu::ID_MISSED_LAP, ScreenId::MenuMissedLap },
    { Menu::ID_EARLY_MIN,  ScreenId::MenuEarlyPress },
};

constexpr Toggle kToggles[] = {
    { Menu::ID_LAP_REMINDER, &Settings::lapReminderEn },
    { Menu::ID_EARLY_GUARD,  &Settings::earlyPressGuardEn },
    { Menu::ID_PHONE_NOTIF,  &Settings::phoneNotifEn },
};
} // namespace

MenuSettingsScreen::MenuSettingsScreen(Model& model)
    : Screen(model)
{
}

void MenuSettingsScreen::build()
{
    const auto font = &poppins_semibold_25;
    mItems[Menu::ID_LAP_LENGTH]   = { Style::Tip,    "Lap Length",      nullptr,          font, mTips[Menu::ID_LAP_LENGTH] };
    mItems[Menu::ID_LANE]         = { Style::Tip,    "Lane",            nullptr,          font, mTips[Menu::ID_LANE] };
    mItems[Menu::ID_LANE_WIDTH]   = { Style::Tip,    "Lane Width",      nullptr,          font, mTips[Menu::ID_LANE_WIDTH] };
    mItems[Menu::ID_LAP_REMINDER] = { Style::Toggle, "Lap\nReminder",   "Lap Reminder",   font };
    mItems[Menu::ID_MISSED_LAP]   = { Style::Tip,    "Missed Lap",      nullptr,          font, mTips[Menu::ID_MISSED_LAP] };
    mItems[Menu::ID_EARLY_GUARD]  = { Style::Toggle, "Press\nGuard",    "Press Guard",    font };
    mItems[Menu::ID_EARLY_MIN]    = { Style::Tip,    "Guard Level",     nullptr,          font, mTips[Menu::ID_EARLY_MIN] };
    mItems[Menu::ID_PHONE_NOTIF]  = { Style::Toggle, "Phone\nNotif.",   "Phone Notif.",   font };

    mMenu      = std::make_unique<WheelMenu>(mRoot, mItems, Menu::ID_COUNT);
    mButtons   = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::NONE, Widgets::Buttons::NONE,
                  Widgets::Buttons::AMBER, Widgets::Buttons::WHITE);
    mTitle     = std::make_unique<Widgets::Title>(mRoot, "SETTINGS");
    mSensorRow = std::make_unique<Widgets::SensorStatusRow>(mRoot, 0, 52, 240, 24, mModel.usesGps());
}

void MenuSettingsScreen::onShow()
{
    mMenu->select(mModel.menu().settings.get());
    mModel.resetIdleTimer();
    onSettings(mModel.getSettings());
    onGpsFix(mModel.hasGpsFix());
    onAccessoryStatus(mModel.getAccessoryState(), "");
}

void MenuSettingsScreen::onHide()
{
    mModel.menu().settings.set(mMenu->selected());
}

void MenuSettingsScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
        case Btn::L1: mMenu->prev(); break;
        case Btn::L2: mMenu->next(); break;
        case Btn::R1: activate(mMenu->selected()); break;
        case Btn::R2: ScreenManager::instance().goTo(ScreenId::Main); break;
        default: break;
    }
}

void MenuSettingsScreen::activate(uint16_t item)
{
    const auto page = std::find_if(std::begin(kPages), std::end(kPages),
                                   [item](const Page& p) { return p.item == item; });
    if (page != std::end(kPages)) {
        ScreenManager::instance().goTo(page->screen);
        return;
    }

    const auto toggle = std::find_if(std::begin(kToggles), std::end(kToggles),
                                     [item](const Toggle& t) { return t.item == item; });
    if (toggle != std::end(kToggles)) {
        Settings sett = mModel.getSettings();
        sett.*toggle->field = !(sett.*toggle->field);
        mModel.saveSettings(sett);
        onSettings(sett);
    }
}

void MenuSettingsScreen::onSettings(const Settings& settings)
{
    snprintf(mTips[Menu::ID_LAP_LENGTH], sizeof(mTips[0]), "%.0f m", settings.lapLengthM);
    snprintf(mTips[Menu::ID_LANE], sizeof(mTips[0]), "%u", static_cast<unsigned>(settings.lane));
    snprintf(mTips[Menu::ID_LANE_WIDTH], sizeof(mTips[0]), "%.2f m", settings.laneWidthM);
    snprintf(mTips[Menu::ID_MISSED_LAP], sizeof(mTips[0]), "%u %%", static_cast<unsigned>(settings.missedLapPct));
    snprintf(mTips[Menu::ID_EARLY_MIN], sizeof(mTips[0]), "%u %%", static_cast<unsigned>(settings.earlyPressPct));
    for (const Toggle& toggle : kToggles) {
        mItems[toggle.item].toggleState = settings.*toggle.field;
    }
    mMenu->refresh();
}

void MenuSettingsScreen::onGpsFix(bool acquired)
{
    mSensorRow->setGps(Widgets::SensorStatusRow::gpsState(acquired));
}

void MenuSettingsScreen::onAccessoryStatus(uint8_t state, const char* /*name*/)
{
    mSensorRow->setHr(Widgets::SensorStatusRow::hrState(state));
}
