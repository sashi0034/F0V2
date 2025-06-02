#pragma once

namespace ZG
{
    /// @brief メンバ関数ポインタからTを取得
    // template <typename T, typename R>
    // T get_class_type(R T::*);
}

namespace ZG
{
    class KeyboardInput
    {
    public:
        KeyboardInput() = default;

        constexpr KeyboardInput(uint8_t code) : m_code(code)
        {
        }

        bool down() const;

        bool pressed() const;

        bool up() const;

    private:
        uint8_t m_code{};
    };

    inline constexpr KeyboardInput KeyCancel{0x03};

    inline constexpr KeyboardInput KeyBackspace{0x08};

    inline constexpr KeyboardInput KeyTab{0x09};

    inline constexpr KeyboardInput KeyClear{0x0C};

    inline constexpr KeyboardInput KeyEnter{0x0D};

    inline constexpr KeyboardInput KeyShift{0x10};

    inline constexpr KeyboardInput KeyControl{0x11};

    inline constexpr KeyboardInput KeyAlt{0x12};

    inline constexpr KeyboardInput KeyPause{0x13};

    inline constexpr KeyboardInput KeyEscape{0x1B};

    inline constexpr KeyboardInput KeySpace{0x20};

    inline constexpr KeyboardInput KeyPageUp{0x21};

    inline constexpr KeyboardInput KeyPageDown{0x22};

    inline constexpr KeyboardInput KeyEnd{0x23};

    inline constexpr KeyboardInput KeyHome{0x24};

    inline constexpr KeyboardInput KeyLeft{0x25};

    inline constexpr KeyboardInput KeyUp{0x26};

    inline constexpr KeyboardInput KeyRight{0x27};

    inline constexpr KeyboardInput KeyDown{0x28};

    inline constexpr KeyboardInput KeyPrintScreen{0x2C};

    inline constexpr KeyboardInput KeyInsert{0x2D};

    inline constexpr KeyboardInput KeyDelete{0x2E};

    inline constexpr KeyboardInput Key0{0x30};

    inline constexpr KeyboardInput Key1{0x31};

    inline constexpr KeyboardInput Key2{0x32};

    inline constexpr KeyboardInput Key3{0x33};

    inline constexpr KeyboardInput Key4{0x34};

    inline constexpr KeyboardInput Key5{0x35};

    inline constexpr KeyboardInput Key6{0x36};

    inline constexpr KeyboardInput Key7{0x37};

    inline constexpr KeyboardInput Key8{0x38};

    inline constexpr KeyboardInput Key9{0x39};

    inline constexpr KeyboardInput KeyA{0x41};

    inline constexpr KeyboardInput KeyB{0x42};

    inline constexpr KeyboardInput KeyC{0x43};

    inline constexpr KeyboardInput KeyD{0x44};

    inline constexpr KeyboardInput KeyE{0x45};

    inline constexpr KeyboardInput KeyF{0x46};

    inline constexpr KeyboardInput KeyG{0x47};

    inline constexpr KeyboardInput KeyH{0x48};

    inline constexpr KeyboardInput KeyI{0x49};

    inline constexpr KeyboardInput KeyJ{0x4A};

    inline constexpr KeyboardInput KeyK{0x4B};

    inline constexpr KeyboardInput KeyL{0x4C};

    inline constexpr KeyboardInput KeyM{0x4D};

    inline constexpr KeyboardInput KeyN{0x4E};

    inline constexpr KeyboardInput KeyO{0x4F};

    inline constexpr KeyboardInput KeyP{0x50};

    inline constexpr KeyboardInput KeyQ{0x51};

    inline constexpr KeyboardInput KeyR{0x52};

    inline constexpr KeyboardInput KeyS{0x53};

    inline constexpr KeyboardInput KeyT{0x54};

    inline constexpr KeyboardInput KeyU{0x55};

    inline constexpr KeyboardInput KeyV{0x56};

    inline constexpr KeyboardInput KeyW{0x57};

    inline constexpr KeyboardInput KeyX{0x58};

    inline constexpr KeyboardInput KeyY{0x59};

    inline constexpr KeyboardInput KeyZ{0x5A};

    inline constexpr KeyboardInput KeyNum0{0x60};

    inline constexpr KeyboardInput KeyNum1{0x61};

    inline constexpr KeyboardInput KeyNum2{0x62};

    inline constexpr KeyboardInput KeyNum3{0x63};

    inline constexpr KeyboardInput KeyNum4{0x64};

    inline constexpr KeyboardInput KeyNum5{0x65};

    inline constexpr KeyboardInput KeyNum6{0x66};

    inline constexpr KeyboardInput KeyNum7{0x67};

    inline constexpr KeyboardInput KeyNum8{0x68};

    inline constexpr KeyboardInput KeyNum9{0x69};

    inline constexpr KeyboardInput KeyNumMultiply{0x6A};

    inline constexpr KeyboardInput KeyNumAdd{0x6B};

    inline constexpr KeyboardInput KeyNumEnter{0x6C};

    inline constexpr KeyboardInput KeyNumSubtract{0x6D};

    inline constexpr KeyboardInput KeyNumDecimal{0x6E};

    inline constexpr KeyboardInput KeyNumDivide{0x6F};

    inline constexpr KeyboardInput KeyF1{0x70};

    inline constexpr KeyboardInput KeyF2{0x71};

    inline constexpr KeyboardInput KeyF3{0x72};

    inline constexpr KeyboardInput KeyF4{0x73};

    inline constexpr KeyboardInput KeyF5{0x74};

    inline constexpr KeyboardInput KeyF6{0x75};

    inline constexpr KeyboardInput KeyF7{0x76};

    inline constexpr KeyboardInput KeyF8{0x77};

    inline constexpr KeyboardInput KeyF9{0x78};

    inline constexpr KeyboardInput KeyF10{0x79};

    inline constexpr KeyboardInput KeyF11{0x7A};

    inline constexpr KeyboardInput KeyF12{0x7B};

    inline constexpr KeyboardInput KeyF13{0x7C};

    inline constexpr KeyboardInput KeyF14{0x7D};

    inline constexpr KeyboardInput KeyF15{0x7E};

    inline constexpr KeyboardInput KeyF16{0x7F};

    inline constexpr KeyboardInput KeyF17{0x80};

    inline constexpr KeyboardInput KeyF18{0x81};

    inline constexpr KeyboardInput KeyF19{0x82};

    inline constexpr KeyboardInput KeyF20{0x83};

    inline constexpr KeyboardInput KeyF21{0x84};

    inline constexpr KeyboardInput KeyF22{0x85};

    inline constexpr KeyboardInput KeyF23{0x86};

    inline constexpr KeyboardInput KeyF24{0x87};

    inline constexpr KeyboardInput KeyNumLock{0x90};

    inline constexpr KeyboardInput KeyLShift{0xA0};

    inline constexpr KeyboardInput KeyRShift{0xA1};

    inline constexpr KeyboardInput KeyLControl{0xA2};

    inline constexpr KeyboardInput KeyRControl{0xA3};

    inline constexpr KeyboardInput KeyLAlt{0xA4};

    inline constexpr KeyboardInput KeyRAlt{0xA5};

    inline constexpr KeyboardInput KeyNextTrack{0xB0};

    inline constexpr KeyboardInput KeyPreviousTrack{0xB1};

    inline constexpr KeyboardInput KeyStopMedia{0xB2};

    inline constexpr KeyboardInput KeyPlayPauseMedia{0xB3};

    inline constexpr KeyboardInput KeyColon_JIS{0xBA};

    inline constexpr KeyboardInput KeySemicolon_US{0xBA};

    inline constexpr KeyboardInput KeySemicolon_JIS{0xBB};

    inline constexpr KeyboardInput KeyEqual_US{0xBB};

    inline constexpr KeyboardInput KeyComma{0xBC};

    inline constexpr KeyboardInput KeyMinus{0xBD};

    inline constexpr KeyboardInput KeyPeriod{0xBE};

    inline constexpr KeyboardInput KeySlash{0xBF};

    inline constexpr KeyboardInput KeyGraveAccent{0xC0};

    inline constexpr KeyboardInput KeyCommand{0xD8};

    inline constexpr KeyboardInput KeyLeftCommand{0xD9};

    inline constexpr KeyboardInput KeyRightCommand{0xDA};

    inline constexpr KeyboardInput KeyLBracket{0xDB};

    inline constexpr KeyboardInput KeyYen_JIS{0xDC};

    inline constexpr KeyboardInput KeyBackslash_US{0xDC};

    inline constexpr KeyboardInput KeyRBracket{0xDD};

    inline constexpr KeyboardInput KeyCaret_JIS{0xDE};

    inline constexpr KeyboardInput KeyApostrophe_US{0xDE};

    inline constexpr KeyboardInput KeyUnderscore_JIS{0xE2};
}
