/**
 ******************************************************************************
 * @file    MenuSettingValueScreen.hpp
 * @brief   Value list for one of the track settings, chosen by Kind.
 ******************************************************************************
 */

#ifndef MENU_SETTING_VALUE_SCREEN_HPP
#define MENU_SETTING_VALUE_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"
#include "gui/widgets/WheelMenu.hpp"

class MenuSettingValueScreen : public Screen
{
public:
    enum class Kind : uint8_t { LapLength, Lane, LaneWidth, MissedLap, EarlyPress };

    static constexpr uint16_t kMaxCount = 81;

    MenuSettingValueScreen(Model& model, Kind kind);

    void onShow() override;
    void onKey(uint8_t code) override;
    /// Idle on a menu screen leaves the app, as the TouchGFX presenter does.
    void onIdleTimeout() override { mModel.exitApp(); }

protected:
    void build() override;

private:
    void save();

    Kind mKind;
    uint16_t mCount = 0;
    WheelMenu::Item mItems[kMaxCount] {};
    char mTexts[kMaxCount][12] {};

    std::unique_ptr<Widgets::Title>   mTitle;
    std::unique_ptr<Widgets::Buttons> mButtons;
    std::unique_ptr<WheelMenu>        mMenu;
};

#endif // MENU_SETTING_VALUE_SCREEN_HPP
