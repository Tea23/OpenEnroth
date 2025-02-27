#pragma once

#include <memory>
#include <string>

#include "Platform/PlatformEnums.h"
#include "Io/IKeyboardController.h"
#include "Io/KeyboardActionMapping.h"

enum class WindowInputStatus : int32_t {
    WINDOW_INPUT_NONE = 0,
    WINDOW_INPUT_IN_PROGRESS = 1,
    WINDOW_INPUT_CONFIRMED = 2,
    WINDOW_INPUT_CANCELLED = 3,
};

class GUIWindow;
namespace Io {
    // Handles events from OSWindow through GameWindowHandler/IKeyboardController
    //      and maps it to game actions/events using KeyboardActionMapping
    class KeyboardInputHandler {
     public:
        KeyboardInputHandler(IKeyboardController *controller, std::shared_ptr<KeyboardActionMapping> actionMapping) {
            this->controller = controller;
            this->actionMapping = actionMapping;

            lastKeyPressed = PlatformKey::None;
            inputType = TextInputType::None;
            window = nullptr;
            uNumKeysPressed = 0;
            ResetKeys();
        }

        bool IsKeyboardPickingOutlineToggled() const;
        bool IsRunKeyToggled() const;
        bool IsTurnStrafingToggled() const;
        bool IsStealingToggled() const;
        bool IsTakeAllToggled() const;
        bool IsAdventurerBackcycleToggled() const;
        bool IsSpellBackcycleToggled() const;
        bool IsCastOnClickToggled() const;
        bool IsKeyHeld(PlatformKey key) const;

        void GenerateInputActions();

        void SetWindowInputStatus(WindowInputStatus status);
        inline PlatformKey LastPressedKey() const {
            return lastKeyPressed;
        }
        void ResetKeys();

        void StartTextInput(TextInputType type, int max_string_len, GUIWindow* pWindow);
        bool ProcessTextInput(PlatformKey key, int c);
        void EndTextInput();

        std::string GetTextInput() const;
        void SetTextInput(const std::string& text);
        void SetTextInput(const char* text);

     private:
        void GeneratePausedActions();
        void GenerateGameplayActions();

        IKeyboardController *controller;
        std::shared_ptr<KeyboardActionMapping> actionMapping;

        PlatformKey lastKeyPressed;
        int keydelaytimer;
        int max_input_string_len;
        char pPressedKeysBuffer[257];
        uint8_t uNumKeysPressed;
        TextInputType inputType;
        GUIWindow *window;
    };
}  // namespace Io


bool UI_OnKeyDown(PlatformKey key);


extern std::shared_ptr<Io::KeyboardInputHandler> keyboardInputHandler;
