/**
 ******************************************************************************
 * @file    MenuSettingsScreen.cpp
 * @brief   Settings menu (see MenuSettingsScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/MenuSettingsScreen.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/Assets.hpp"

using Style = WheelMenu::Item::Style;

MenuSettingsScreen::MenuSettingsScreen(Model& model)
    : Screen(model)
{
}

void MenuSettingsScreen::build()
{
    mItems[Menu::ID_LAP_LENGTH]  = { Style::Tip, "Lap\nLength", "Lap Length", &poppins_semibold_25, mLapLengthTip };
    mItems[Menu::ID_LANE]        = { Style::Tip, "Lane", nullptr, &poppins_semibold_25, mLaneTip };
    mItems[Menu::ID_LANE_WIDTH]  = { Style::Tip, "Lane\nWidth", "Lane Width", &poppins_semibold_25, mLaneWidthTip };
    mItems[Menu::ID_PHONE_NOTIF] = { Style::Toggle, "Phone\nNotif.", "Phone Notif.", &poppins_semibold_25 };

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
        case Btn::R1:
            switch (mMenu->selected()) {
                case Menu::ID_LAP_LENGTH: ScreenManager::instance().goTo(ScreenId::MenuLapLength); break;
                case Menu::ID_LANE:       ScreenManager::instance().goTo(ScreenId::MenuLane); break;
                case Menu::ID_LANE_WIDTH: ScreenManager::instance().goTo(ScreenId::MenuLaneWidth); break;
                default: togglePhoneNotif(); break;
            }
            break;
        case Btn::R2:
            ScreenManager::instance().goTo(ScreenId::Main);
            break;
        default:
            break;
    }
}

void MenuSettingsScreen::togglePhoneNotif()
{
    Settings sett = mModel.getSettings();
    sett.phoneNotifEn = !sett.phoneNotifEn;
    mModel.saveSettings(sett);
    onSettings(sett);
}

void MenuSettingsScreen::onSettings(const Settings& settings)
{
    snprintf(mLapLengthTip, sizeof(mLapLengthTip), "%.0f m", settings.lapLengthM);
    snprintf(mLaneTip, sizeof(mLaneTip), "%u", static_cast<unsigned>(settings.lane));
    snprintf(mLaneWidthTip, sizeof(mLaneWidthTip), "%.2f m", settings.laneWidthM);
    mItems[Menu::ID_PHONE_NOTIF].toggleState = settings.phoneNotifEn;
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
