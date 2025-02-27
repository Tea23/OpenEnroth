#include "GameWrapper.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <utility>

#include "GUI/GUIProgressBar.h"
#include "GUI/GUIWindow.h"
#include "GUI/GUIButton.h"

#include "Engine/SaveLoad.h"

#include "Testing/Engine/TestStateHandle.h"
#include "Testing/Engine/TestProxy.h"
#include "Testing/Extensions/ThrowingAssertions.h"

#include "Library/Application/PlatformApplication.h"
#include "Library/Trace/EventTrace.h"

#include "Platform/PlatformEvents.h"

#include "Utility/Random/Random.h"

GameWrapper::GameWrapper(TestStateHandle state, const std::string &testDataDir):
    state_(std::move(state)),
    testDataDir_(testDataDir) {
    if (!testDataDir_.ends_with('/') && !testDataDir_.ends_with('\\'))
        testDataDir_.push_back('/');
}

GameWrapper::~GameWrapper() {}

void GameWrapper::Reset() {
    GoToMainMenu();
    state_->proxy->reset();
    SeedRandom(0);
}

void GameWrapper::Tick(int count) {
    for (int i = 0; i < count; i++)
        state_.YieldExecution();
}

void GameWrapper::PressKey(PlatformKey key) {
    std::unique_ptr<PlatformKeyEvent> event = std::make_unique<PlatformKeyEvent>();
    event->type = PlatformEvent::KeyPress;
    event->key = key;
    event->mods = 0;
    event->isAutoRepeat = false;
    state_->proxy->postEvent(state_->application->window(), std::move(event));
}

void GameWrapper::ReleaseKey(PlatformKey key) {
    std::unique_ptr<PlatformKeyEvent> event = std::make_unique<PlatformKeyEvent>();
    event->type = PlatformEvent::KeyRelease;
    event->key = key;
    event->mods = 0;
    event->isAutoRepeat = false;
    state_->proxy->postEvent(state_->application->window(), std::move(event));
}

void GameWrapper::PressButton(PlatformMouseButton button, int x, int y) {
    std::unique_ptr<PlatformMouseEvent> event = std::make_unique<PlatformMouseEvent>();
    event->type = PlatformEvent::MouseButtonPress;
    event->button = PlatformMouseButton::Left;
    event->pos = Pointi(x, y);
    event->isDoubleClick = false;
    state_->proxy->postEvent(state_->application->window(), std::move(event));
}

void GameWrapper::ReleaseButton(PlatformMouseButton button, int x, int y) {
    std::unique_ptr<PlatformMouseEvent> event = std::make_unique<PlatformMouseEvent>();
    event->type = PlatformEvent::MouseButtonRelease;
    event->button = PlatformMouseButton::Left;
    event->buttons = PlatformMouseButton::Left;
    event->pos = Pointi(x, y);
    event->isDoubleClick = false;
    state_->proxy->postEvent(state_->application->window(), std::move(event));
}

void GameWrapper::PressAndReleaseKey(PlatformKey key) {
    PressKey(key);
    ReleaseKey(key);
}

void GameWrapper::PressAndReleaseButton(PlatformMouseButton button, int x, int y) {
    PressButton(button, x, y);
    ReleaseButton(button, x, y);
}

void GameWrapper::PressGuiButton(std::string_view buttonId) {
    GUIButton *button = AssertButton(buttonId);

    PressAndReleaseButton(PlatformMouseButton::Left, button->uX + button->uWidth / 2, button->uY + button->uHeight / 2);
}

void GameWrapper::GoToMainMenu() {
    if (GetCurrentMenuID() == MENU_MAIN)
        return;

    if (GetCurrentMenuID() == MENU_CREATEPARTY) {
        PressAndReleaseKey(PlatformKey::Escape);
        Tick(2);
        ASSERT_EQ(GetCurrentMenuID(), MENU_MAIN);
        return;
    }

    if (GetCurrentMenuID() == -1) {
        PressAndReleaseKey(PlatformKey::Escape);
        Tick(1);
        PressGuiButton("GameMenu_Quit");
        Tick(1);
        PressGuiButton("GameMenu_Quit");
        while (GetCurrentMenuID() != MENU_MAIN)
            Tick(1);
        return;
    }

    ASSERT_TRUE(false); // TODO(captainurist)
}

void GameWrapper::LoadGame(const std::string &name) {
    std::string saveName = "!!!test.mm7";
    std::string savePath = MakeDataPath("saves", saveName);
    if (std::filesystem::exists(savePath))
        std::filesystem::remove(savePath);
    std::filesystem::copy_file(testDataDir_ + name, savePath);

    // TODO(captainurist): these tricks might fail if we have more than 45 save files.

    GoToMainMenu();
    PressGuiButton("MainMenu_LoadGame");
    Tick(3);

    // Using asserts here as if we can't load the save then there's usually little reason to continue the test.
    ASSERT_TRUE(pSavegameUsedSlots[0]);
    ASSERT_EQ(pSavegameList->pFileList[0], saveName);

    PressGuiButton("LoadMenu_Slot0");
    Tick(2);
    PressGuiButton("LoadMenu_Load");
    SkipLoadingScreen();

    SeedRandom(0);
}

void GameWrapper::SkipLoadingScreen() {
    while (!pGameLoadingUI_ProgressBar->IsActive())
        Tick(1);
    while (pGameLoadingUI_ProgressBar->IsActive())
        Tick(1);
    while (dword_6BE364_game_settings_1 & GAME_SETTINGS_0080_SKIP_USER_INPUT_THIS_FRAME)
        Tick(1);
}

GUIButton *GameWrapper::AssertButton(std::string_view buttonId) {
    auto findButton = [](std::string_view buttonId) -> GUIButton * {
        for (GUIWindow *window : lWindowList)
            for (GUIButton *button : window->vButtons)
                if (button->id == buttonId)
                    return button;
        return nullptr;
    };

    GUIButton *result = findButton(buttonId);
    ASSERT_NE(result, nullptr) << "Button '" << buttonId << "' not found.";

    auto checkButton = [](GUIButton *button) {
        Pointi point = Pointi(button->uX + button->uWidth / 2, button->uY + button->uHeight / 2);

        for (GUIWindow *window : lWindowList) {
            for (GUIButton *otherButton : window->vButtons) {
                if (otherButton->Contains(point.x, point.y)) {
                    ASSERT_EQ(button, otherButton) << "Button '" << button->id << "' is hidden by another button.";
                    return;
                }
            }
        }
    };

    checkButton(result);

    return result;
}

void GameWrapper::PlayTrace(const std::string &name) {
    auto trace = EventTrace::loadFromFile(testDataDir_ + name).takeEvents();

    for (std::unique_ptr<PlatformEvent> &event : trace) {
        if (event->type == EventTrace::PaintEvent) {
            Tick(1);
        } else {
            state_->proxy->postEvent(state_->application->window(), std::move(event));
        }
    }
}
