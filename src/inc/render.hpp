#pragma once

#include <ws2811.h>
#include <vector>
#include <stdexcept>
#include <memory>
#include <unordered_map>
#include <spdlog/spdlog.h>

namespace ohtoai::rpi {
    using LedColor = ws2811_led_t;

    enum Led: LedColor {
        White = 0xffffff,
        Red = 0xff0000,
        Green = 0x00ff00,
        Blue = 0x0000ff,
        Yellow = 0xffff00,
        Cyan = 0x00ffff,
        Magenta = 0xff00ff,
        Black = 0x000000,
    };

    class IPaintSource {
    public:
        IPaintSource() = default;
        virtual ~IPaintSource() = default;
        virtual int width() const = 0;
        virtual int height() const = 0;
        virtual const LedColor& led(int x, int y) const = 0;
    };

    class IStripIndex {
    public:
        IStripIndex(const int& w, const int& h) : width_(w), height_(h) {}
        virtual int index(int x, int y) const = 0;
    protected:
        const int& width_;
        const int& height_;
    };

    class ISnakeStripIndex: public IStripIndex {
    public:
        using IStripIndex::IStripIndex;
        virtual int index(int x, int y) const override {
            return y % 2 == 0 ? y * width_ + x : y * width_ + (width_ - x - 1);
        }
    };

    class IVectorStripIndex: public IStripIndex {
    public:
        using IStripIndex::IStripIndex;
        virtual int index(int x, int y) const override {
            return y * width_ + x;
        }
    };

    template <typename StripIndexType = ISnakeStripIndex>
    class IStripRotateHelper: public StripIndexType {
    public:
        using StripIndexType::StripIndexType;
    };

    template <typename StripIndexType = ISnakeStripIndex, int RotateDegree = 0, bool FlipX = false, bool FlipY = false>
    class StripRotateHelper: public IStripRotateHelper<StripIndexType> {
    public:
        using IStripRotateHelper<StripIndexType>::IStripRotateHelper;
        virtual int index(int x, int y) const {
            int ur_x, ur_y;
            if constexpr (RotateDegree == 0) {
                ur_x = x;
                ur_y = y;
            }
            else if constexpr (RotateDegree == 90) {
                ur_x = y;
                ur_y = IStripRotateHelper<StripIndexType>::height_ - x - 1;
            }
            else if constexpr (RotateDegree == 180) {
                ur_x = IStripRotateHelper<StripIndexType>::width_ - x - 1;
                ur_y = IStripRotateHelper<StripIndexType>::height_ - y - 1;
            }
            else if constexpr (RotateDegree == 270) {
                ur_x = IStripRotateHelper<StripIndexType>::width_ - y - 1;
                ur_y = x;
            }
            else {
                static_assert(RotateDegree == 0 || RotateDegree == 90 || RotateDegree == 180 || RotateDegree == 270, "RotateDegree must be 0, 90, 180 or 270");
            }
            if constexpr (FlipX) {
                ur_x = IStripRotateHelper<StripIndexType>::width_ - ur_x - 1;
            }
            if constexpr (FlipY) {
                ur_y = IStripRotateHelper<StripIndexType>::height_ - ur_y - 1;
            }
            return IStripRotateHelper<StripIndexType>::index(ur_x, ur_y);
        }
    };

    class IPaintDevice: public IPaintSource {
    public:
        IPaintDevice(LedColor back = Led::Black) : back(back) {};
        virtual ~IPaintDevice() = default;
        virtual LedColor& led_ref(int x, int y) = 0;
        virtual const LedColor& led(int x, int y) const = 0;
        virtual void set_transparent(bool transparent) {
            transparent_ = transparent;
        }

        virtual void set_background(LedColor color) {
            back = color;
        }

        virtual void clear() {
            for (int y = 0; y < height(); ++y) {
                for (int x = 0; x < width(); ++x) {
                    led_ref(x, y) = back;
                }
            }
        }
        
        virtual void draw(const IPaintSource& src, int x, int y) {
            draw(src, x, y, false, false);
        }
        
        virtual void draw(const IPaintSource& src, int x, int y, bool flip_x, bool flip_y) {
            for (int sy = 0; sy < src.height(); ++sy) {
                int dy = flip_y ? src.height() - sy - 1 : sy;
                if (y + dy >= height()) {
                    continue;
                }
                for (int sx = 0; sx < src.width(); ++sx) {
                    int dx = flip_x ? src.width() - sx - 1 : sx;
                    if (x + dx >= width()) {
                        continue;
                    }
                    auto src_led = src.led(sx, sy);
                    if (transparent_ && src_led == back) {
                        continue;
                    }
                    led_ref(x + dx, y + dy) = src_led;
                }
            }
        }
        
        virtual void flip(bool flip_x, bool flip_y) {
            for (int y = 0; y < height(); ++y) {
                for (int x = 0; x < width() / 2; ++x) {
                    int dx = flip_x ? width() - x - 1 : x;
                    int dy = flip_y ? height() - y - 1 : y;
                    auto tmp = led(x, y);
                    led_ref(x, y) = led(dx, dy);
                    led_ref(dx, dy) = tmp;
                }
            }
        }

    protected:
        bool transparent_ = false;
        LedColor back = Led::Black;
    };

    template <typename StripIndexType = ISnakeStripIndex>
    class IRotatablePaintDevice: public IPaintDevice {
    public:
        IRotatablePaintDevice(int w, int h, LedColor back = Led::Black)
            : width_(w), height_(h),
            IPaintDevice(back),
            logic_width_(w), logic_height_(h),
            rotate_helper_(std::make_shared<StripRotateHelper<StripIndexType, 0, false, false>>(width_, height_)) {};
        virtual ~IRotatablePaintDevice() = default;
        virtual int width() const override { return logic_width_; }
        virtual int height() const override { return logic_height_; }
        virtual void set_rotate(int degree, bool flip_x = false, bool flip_y = false) {
            switch (degree) {
                case 0:
                    if (flip_x) {
                        if (flip_y) {
                            rotate_helper_ = std::make_shared<StripRotateHelper<StripIndexType, 0, true, true>>(width_, height_);
                        }
                        else {
                            rotate_helper_ = std::make_shared<StripRotateHelper<StripIndexType, 0, true, false>>(width_, height_);
                        }
                    }
                    else {
                        if (flip_y) {
                            rotate_helper_ = std::make_shared<StripRotateHelper<StripIndexType, 0, false, true>>(width_, height_);
                        }
                        else {
                            rotate_helper_ = std::make_shared<StripRotateHelper<StripIndexType, 0, false, false>>(width_, height_);
                        }
                    }
                    logic_width_ = width_;
                    logic_height_ = height_;
                    break;
                case 90:
                    if (flip_x) {
                        if (flip_y) {
                            rotate_helper_ = std::make_shared<StripRotateHelper<StripIndexType, 90, true, true>>(width_, height_);
                        }
                        else {
                            rotate_helper_ = std::make_shared<StripRotateHelper<StripIndexType, 90, true, false>>(width_, height_);
                        }
                    }
                    else {
                        if (flip_y) {
                            rotate_helper_ = std::make_shared<StripRotateHelper<StripIndexType, 90, false, true>>(width_, height_);
                        }
                        else {
                            rotate_helper_ = std::make_shared<StripRotateHelper<StripIndexType, 90, false, false>>(width_, height_);
                        }
                    }
                    logic_width_ = height_;
                    logic_height_ = width_;
                    break;
                case 180:
                    if (flip_x) {
                        if (flip_y) {
                            rotate_helper_ = std::make_shared<StripRotateHelper<StripIndexType, 180, true, true>>(width_, height_);
                        }
                        else {
                            rotate_helper_ = std::make_shared<StripRotateHelper<StripIndexType, 180, true, false>>(width_, height_);
                        }
                    }
                    else {
                        if (flip_y) {
                            rotate_helper_ = std::make_shared<StripRotateHelper<StripIndexType, 180, false, true>>(width_, height_);
                        }
                        else {
                            rotate_helper_ = std::make_shared<StripRotateHelper<StripIndexType, 180, false, false>>(width_, height_);
                        }
                    }
                    logic_width_ = width_;
                    logic_height_ = height_;
                    break;
                case 270:
                    if (flip_x) {
                        if (flip_y) {
                            rotate_helper_ = std::make_shared<StripRotateHelper<StripIndexType, 270, true, true>>(width_, height_);
                        }
                        else {
                            rotate_helper_ = std::make_shared<StripRotateHelper<StripIndexType, 270, true, false>>(width_, height_);
                        }
                    }
                    else {
                        if (flip_y) {
                            rotate_helper_ = std::make_shared<StripRotateHelper<StripIndexType, 270, false, true>>(width_, height_);
                        }
                        else {
                            rotate_helper_ = std::make_shared<StripRotateHelper<StripIndexType, 270, false, false>>(width_, height_);
                        }
                    }
                    logic_width_ = height_;
                    logic_height_ = width_;
                    break;
                default:
                    throw std::invalid_argument("IRotatablePaintDevice::set_rotate");
            }
        }

        virtual int index(int x, int y) const {
            return rotate_helper_->index(x, y);
        }
    protected:
        std::shared_ptr<IStripRotateHelper<StripIndexType>> rotate_helper_ = nullptr;
        int width_ = 0;
        int height_ = 0;
        int logic_width_ = 0;
        int logic_height_ = 0;
    };

    class Pixmap : public IRotatablePaintDevice<IVectorStripIndex> {
    public:
        Pixmap(int w, int h, LedColor back = Led::Black)
            : IRotatablePaintDevice<IVectorStripIndex>(w, h, back),
            leds_(w * h) {}
        LedColor& led_ref(int x, int y) override { return leds_[index(x, y)]; }
        const LedColor& led(int x, int y) const override { return leds_[index(x, y)]; }
    private:
        std::vector<LedColor> leds_;
    };

    class SimpleAlphaGroup : public IPaintSource {
    public:
        SimpleAlphaGroup(LedColor color = Led::White, LedColor back = Led::Black)
            : color(color), back(back) { }
        auto& set_color(LedColor color) {
            this->color = color;
            return *this;
        }
        auto& set_background(LedColor color) {
            this->back = color;
            return *this;
        }
        LedColor color;
        LedColor back;
    };

    class Digit3x5 : public SimpleAlphaGroup {
        friend class DigitGroup3x5;
    public:
        Digit3x5(int digit, LedColor color = Led::White, LedColor back = Led::Black) : digit(digit), SimpleAlphaGroup(color, back) {}
        int width() const override { return 3; }
        int height() const override { return 5; }
        const LedColor& led(int x, int y) const override {
            if (digit >= 0 && digit < sizeof(digit3x5font) / sizeof(digit3x5font[0]) && x >= 0 && x < 3 && y >= 0 && y < 5) {
                return (digit3x5font[digit] & (1 << (y * 3 + 2 - x))) ? color : back;
            }
            throw std::out_of_range(fmt::format("Digit3x5::led<{}>({}, {})", digit, x, y));
        }

        int digit;
    private:
        static inline uint16_t digit3x5font[] = {
            0b0'111'101'101'101'111, // 0
            0b0'010'010'010'010'010, // 1
            0b0'111'001'111'100'111, // 2
            0b0'111'001'111'001'111, // 3
            0b0'101'101'111'001'001, // 4
            0b0'111'100'111'001'111, // 5
            0b0'111'100'111'101'111, // 6
            0b0'111'001'001'001'001, // 7
            0b0'111'101'111'101'111, // 8
            0b0'111'101'111'001'111, // 9
        };
    };

    class Ascii4x8 : public SimpleAlphaGroup {
        friend class Text4x8;
    public:
        Ascii4x8(char c, LedColor color = Led::White, LedColor back = Led::Black) : c(c), SimpleAlphaGroup(color, back) {}
        int width() const override { return 4; }
        int height() const override { return 8; }
        const LedColor& led(int x, int y) const override {
            if (c >= 0 && c < sizeof(ascii4x8font) / sizeof(ascii4x8font[0]) && x >= 0 && x < 4 && y >= 0 && y < 8) {
                return (ascii4x8font[c] & (1 << (y * 4 + 3 - x))) ? color : back;
            }
            throw std::out_of_range("Ascii4x8::led");
        }

        char c;
    private:
        constexpr static inline uint32_t ascii4x8_unkown = ~0b0000'0100'1010'0010'0100'0000'0100'0000;
        static inline const uint32_t ascii4x8font[] = {
            ascii4x8_unkown, // NUL (0x00)
            ascii4x8_unkown, // SOH (0x01)
            ascii4x8_unkown, // STX (0x02)
            ascii4x8_unkown, // ETX (0x03)
            ascii4x8_unkown, // EOT (0x04)
            ascii4x8_unkown, // ENQ (0x05)
            ascii4x8_unkown, // ACK (0x06)
            ascii4x8_unkown, // BEL (0x07)
            ascii4x8_unkown, // BS  (0x08)
            ascii4x8_unkown, // TAB (0x09)
            ascii4x8_unkown, // LF  (0x0A)
            ascii4x8_unkown, // VT  (0x0B)
            ascii4x8_unkown, // FF  (0x0C)
            ascii4x8_unkown, // CR  (0x0D)
            ascii4x8_unkown, // SO  (0x0E)
            ascii4x8_unkown, // SI  (0x0F)
            ascii4x8_unkown, // DLE (0x10)
            ascii4x8_unkown, // DC1 (0x11)
            ascii4x8_unkown, // DC2 (0x12)
            ascii4x8_unkown, // DC3 (0x13)
            ascii4x8_unkown, // DC4 (0x14)
            ascii4x8_unkown, // NAK (0x15)
            ascii4x8_unkown, // SYN (0x16)
            ascii4x8_unkown, // ETB (0x17)
            ascii4x8_unkown, // CAN (0x18)
            ascii4x8_unkown, // EM  (0x19)
            ascii4x8_unkown, // SUB (0x1A)
            ascii4x8_unkown, // ESC (0x1B)
            ascii4x8_unkown, // FS  (0x1C)
            ascii4x8_unkown, // GS  (0x1D)
            ascii4x8_unkown, // RS  (0x1E)
            ascii4x8_unkown, // US  (0x1F)
            0b0000'0000'0000'0000'0000'0000'0000'0000, // SPACE (0x20)
            0b0000'0100'0100'0100'0100'0000'0100'0000, // ! (0x21)
            0b0000'1010'1010'0000'0000'0000'0000'0000, // " (0x22)
            0b0000'1010'1110'1010'1010'1110'1010'0000, // # (0x23)
            0b0000'0100'1110'1100'0110'0110'1100'0100, // $ (0x24)
            0b0000'0000'1010'0010'0100'1000'1010'0000, // % (0x25)
            0b0000'0100'1010'0100'1010'1000'0110'0000, // & (0x26)
            0b0000'0000'0100'0100'0000'0000'0000'0000, // ' (0x27)
            0b0000'0010'0100'0100'0100'0100'0100'0010, // ( (0x28)
            0b0000'1000'0100'0100'0100'0100'0100'1000, // ) (0x29)
            0b0000'0000'0100'1110'0100'1010'0000'0000, // * (0x2A)
            0b0000'0000'0000'0100'1110'0100'0000'0000, // + (0x2B)
            0b0000'0000'0000'0000'0000'0000'0100'1000, // , (0x2C)
            0b0000'0000'0000'0000'1110'0000'0000'0000, // - (0x2D)
            0b0000'0000'0000'0000'0000'0000'0100'0000, // . (0x2E)
            0b0000'0010'0010'0100'0100'0100'1000'1000, // / (0x2F)

            // 0-9
            0b0000'0100'1010'1110'1010'1010'0100'0000, // 0 (0x30)
            0b0000'0100'1100'0100'0100'0100'1110'0000, // 1 (0x31)
            0b0000'1100'0010'0010'0100'1000'1110'0000, // 2 (0x32)
            0b0000'1100'0010'0100'0010'0010'1100'0000, // 3 (0x33)
            0b0000'0010'0110'1010'1010'1110'0010'0000, // 4 (0x34)
            0b0000'1110'1000'1100'0010'0010'1100'0000, // 5 (0x35)
            0b0000'0110'1000'1100'1010'1010'0100'0000, // 6 (0x36)
            0b0000'1110'0010'0010'0100'0100'0100'0000, // 7 (0x37)
            0b0000'0100'1010'0100'1010'1010'0100'0000, // 8 (0x38)
            0b0000'0100'1010'1010'0110'0010'1100'0000, // 9 (0x39)

            0b0000'0000'0000'0100'0000'0000'0100'0000, // : (0x3A)
            0b0000'0000'0000'0100'0000'0000'0100'1000, // ; (0x3B)
            0b0000'0000'0010'0100'1000'0100'0010'0000, // < (0x3C)
            0b0000'0000'0000'1110'0000'1110'0000'0000, // = (0x3D)
            0b0000'0000'1000'0100'0010'0100'1000'0000, // > (0x3E)
            0b0000'0100'1010'0010'0100'0000'0100'0000, // ? (0x3F)
            0b0000'0100'1010'1110'1110'1000'0110'0000, // @ (0x40)

            // A-Z
            0b0000'0100'1010'1010'1110'1010'1010'0000, // 'A'
            0b0000'1100'1010'1100'1010'1010'1100'0000, // 'B'
            0b0000'0110'1000'1000'1000'1000'0110'0000, // 'C'
            0b0000'1100'1010'1010'1010'1010'1100'0000, // 'D'
            0b0000'1110'1000'1110'1000'1000'1110'0000, // 'E'
            0b0000'1110'1000'1110'1000'1000'1000'0000, // 'F'
            0b0000'0110'1000'1000'1010'1010'0110'0000, // 'G'
            0b0000'1010'1010'1110'1010'1010'1010'0000, // 'H'
            0b0000'1110'0100'0100'0100'0100'1110'0000, // 'I'
            0b0000'0010'0010'0010'0010'1010'0100'0000, // 'J'
            0b0000'1010'1010'1100'1010'1010'1010'0000, // 'K'
            0b0000'1000'1000'1000'1000'1000'1110'0000, // 'L'
            0b0000'1010'1110'1110'1010'1010'1010'0000, // 'M'
            0b0000'1100'1010'1010'1010'1010'1010'0000, // 'N'
            0b0000'0100'1010'1010'1010'1010'0100'0000, // 'O'
            0b0000'1100'1010'1010'1100'1000'1000'0000, // 'P'
            0b0000'0100'1010'1010'1010'1010'0100'0010, // 'Q'
            0b0000'1100'1010'1010'1100'1010'1010'0000, // 'R'
            0b0000'0110'1000'0100'0010'0010'1100'0000, // 'S'
            0b0000'1110'0100'0100'0100'0100'0100'0000, // 'T'
            0b0000'1010'1010'1010'1010'1010'1110'0000, // 'U'
            0b0000'1010'1010'1010'1010'1100'1000'0000, // 'V'
            0b0000'1010'1010'1010'1110'1110'1010'0000, // 'W'
            0b0000'1010'1010'0100'0100'1010'1010'0000, // 'X'
            0b0000'1010'1010'1010'0100'0100'0100'0000, // 'Y'
            0b0000'1110'0010'0100'0100'1000'1110'0000, // 'Z'

            0b0000'0110'0100'0100'0100'0100'0100'0110, // [ (0x5B)
            0b0000'1000'1000'0100'0100'0100'0010'0010, // \ (0x5C)
            0b0000'0110'0010'0010'0010'0010'0010'0110, // ] (0x5D)
            0b0000'0100'1010'0000'0000'0000'0000'0000, // ^ (0x5E)
            0b0000'0000'0000'0000'0000'0000'0000'1110, // _ (0x5F)
            0b0000'0100'0010'0000'0000'0000'0000'0000, // ` (0x60)

            // a-z
            0b0000'0000'0000'0110'1010'1010'0110'0000, // 'a'
            0b0000'1000'1000'1100'1010'1010'1100'0000, // 'b'
            0b0000'0000'0000'0110'1000'1000'0110'0000, // 'c'
            0b0000'0010'0010'0110'1010'1010'0110'0000, // 'd'
            0b0000'0000'0000'0110'1010'1100'0110'0000, // 'e'
            0b0000'0010'0100'1110'0100'0100'0100'0000, // 'f'
            0b0000'0000'0000'0110'1010'0110'0010'1100, // 'g'
            0b0000'1000'1000'1100'1010'1010'1010'0000, // 'h'
            0b0000'0100'0000'1100'0100'0100'1110'0000, // 'i'
            0b0000'0010'0000'1110'0010'0010'0010'1100, // 'j'
            0b0000'1000'1000'1010'1100'1010'1010'0000, // 'k'
            0b0000'1100'0100'0100'0100'0100'0110'0000, // 'l'
            0b0000'0000'0000'1100'1110'1110'1010'0000, // 'm'
            0b0000'0000'0000'1100'1010'1010'1010'0000, // 'n'
            0b0000'0000'0000'0100'1010'1010'0100'0000, // 'o'
            0b0000'0000'0000'1100'1010'1010'1100'1000, // 'p'
            0b0000'0000'0000'0110'1010'1010'0110'0010, // 'q'
            0b0000'0000'0000'1010'1100'1000'1000'0000, // 'r'
            0b0000'0000'0000'0110'1100'0110'1100'0000, // 's'
            0b0000'0000'0100'1110'0100'0100'0010'0000, // 't'
            0b0000'0000'0000'1010'1010'1010'0110'0000, // 'u'
            0b0000'0000'0000'1010'1010'1100'1000'0000, // 'v'
            0b0000'0000'0000'1010'1010'1110'1110'0000, // 'w'
            0b0000'0000'0000'1010'0100'1010'1010'0000, // 'x'
            0b0000'0000'0000'1010'1010'0110'0010'1100, // 'y'
            0b0000'0000'0000'1110'0100'1000'1110'0000, // 'z'

            0b0000'0010'0100'0100'1000'0100'0100'0010, // { (0x7B)
            0b0000'0100'0100'0100'0100'0100'0100'0100, // | (0x7C)
            0b0000'1000'0100'0100'0010'0100'0100'1000, // } (0x7D)
            0b0000'0000'0000'1100'0110'0000'0000'0000, // ~ (0x7E)
            ascii4x8_unkown, // DEL (0x7F)
        };
    };

    class WS2811Strip : public IRotatablePaintDevice<ISnakeStripIndex>, protected ws2811_t {
        enum {DMA = 10, GPIO_PIN = 18};
    public:
        WS2811Strip(int w = 8, int h = 8, LedColor back = Led::Black)
            : IRotatablePaintDevice<ISnakeStripIndex>(w, h, back), ws2811_t() {
            freq = WS2811_TARGET_FREQ;
            dmanum = DMA;
            channel[0].gpionum = GPIO_PIN;
            channel[0].count = width_ * height_;
            channel[0].invert = 0;
            channel[0].brightness = 255;
            channel[0].strip_type = WS2811_STRIP_GRB;
            channel[1].gpionum = 0;
            channel[1].count = 0;
            channel[1].invert = 0;
            channel[1].brightness = 0;
            channel[1].strip_type = 0;

            spdlog::debug("WS2811Strip::WS2811Strip {} {}x{}, DMA: {}, GPIO: {}, Brightness: {}",
                (void*)this, width_, height_, dmanum, channel[0].gpionum, (int)channel[0].brightness);
        }

        ~WS2811Strip() {
            spdlog::debug("WS2811Strip::~WS2811Strip {}", (void*)this);
            ws2811_fini(this);
        }

        auto init() {
            auto ret = ws2811_init(this);
            spdlog::debug("WS2811Strip::init {} {}", (void*)this, ws2811_get_return_t_str(ret));
        }

        auto wait() {
            return ws2811_wait(this);
        }

        auto render() {
            return ws2811_render(this);
        }

        auto uninit() {
            spdlog::debug("WS2811Strip::uninit {}", (void*)this);
            return ws2811_fini(this);
        }
    public:
        LedColor& led_ref(int x, int y) override {
            return channel[0].leds[index(x, y)];
        }
        const LedColor& led(int x, int y) const override {
            return channel[0].leds[index(x, y)];
        }
    };

    /// @brief 3x5 digits group, contains variable number of digits
    class DigitGroup3x5 : public SimpleAlphaGroup {
    public:
        DigitGroup3x5(int digits, LedColor color = Led::White, LedColor back = Led::Black)
            : SimpleAlphaGroup(color, back) {
                set_digits(digits);
        }
        int width() const override { 
            return digits_width_ * 3;
         }
        int height() const override { return 5; }
        const LedColor& led(int x, int y) const override {
            if (x >= 0 && x < width() && y >= 0 && y < height()) {
                auto digit = (digits_ / (int)std::pow(10, digits_width_ - x / 3 - 1)) % 10;
                return (Digit3x5::digit3x5font[digit] & (1 << (y * 3 + 2 - x % 3))) ? color : back;
            }
            throw std::out_of_range("DigitGroup3x5::led");
        }

        int digits() const { return digits_; }
        DigitGroup3x5& set_digits(int digits) {
            if (digits < 0)
                digits = -digits;
            digits_ = digits;
            digits_width_ = 0;
            while (digits != 0) {
                digits_width_++;
                digits /= 10;
            }
            return *this;
        }
        
    protected:
        int digits_;
        mutable int digits_width_;
    };

    class Text4x8 : public SimpleAlphaGroup {
    public:
        Text4x8(const std::string& text, LedColor color = Led::White, LedColor back = Led::Black)
            : text(text), SimpleAlphaGroup(color, back) {}
        int width() const override { return text.size() * 4; }
        int height() const override { return 8; }
        const LedColor& led(int x, int y) const override {
            if (x >= 0 && x < width() && y >= 0 && y < height()) {
                return (Ascii4x8::ascii4x8font[text[x / 4]] & (1 << (y * 4 + 3 - x % 4))) ? color : back;
            }
            throw std::out_of_range("Text4x8::led");
        }

        std::string text;
    private:
    };

    class Window : public IRotatablePaintDevice<IVectorStripIndex> {
    public:
        Window(int w, int h, Window* parent = nullptr)
            : Window(w, h, parent, 0, 0) {}
        Window(int w, int h, Window* parent, int x, int y)
            : IRotatablePaintDevice<IVectorStripIndex>(w, h),
            leds_(w * h) {
                spdlog::debug("Window::Window {} {}x{} @ {}, {}", (void*)this, width(), height(), x, y);
                if (parent) {
                    parent->add(this, x, y);
                }
            }

        Window& add(IPaintSource* child, int x, int y) {
            spdlog::debug("Window::add {}->{} {}x{} @ {}, {}", (void*)this, (void*)child, width(), height(), x, y);
            children_[child] = {x, y};
            if (auto window = dynamic_cast<Window*>(child)) {
                if (window->parent_) {
                    window->parent_->remove(window);
                }
                window->parent_ = this;
            }
            return *this;
        }

        LedColor& led_ref(int x, int y) override { return leds_[index(x, y)]; }
        const LedColor& led(int x, int y) const override {
            for (const auto& child : children_) {
                if (x >= child.second.x && x < child.second.x + child.first->width() && y >= child.second.y && y < child.second.y + child.first->height()) {
                    auto& child_led = child.first->led(x - child.second.x, y - child.second.y);
                    if (transparent_ && child_led == back)
                        continue;
                    return child_led;
                }
            }
            return leds_[index(x, y)];
        }

        void move(int x, int y) {
            if (parent_) {
                parent_->children_[this] = {x, y};
            }
        }

        Window& remove(IPaintSource* child) {
            spdlog::debug("Window::remove {}->{}", (void*)this, (void*)child);
            children_.erase(child);
            if (auto window = dynamic_cast<Window*>(child)) {
                window->parent_ = nullptr;
            }
            return *this;
        }

        int x() const {
            if (parent_) {
                auto it = parent_->children_.find(this);
                if (it != parent_->children_.end()) {
                    return it->second.x;
                }
            }
            return 0;
         }
        int y() const {
            if (parent_) {
                auto it = parent_->children_.find(this);
                if (it != parent_->children_.end()) {
                    return it->second.y;
                }
            }
            return 0;
         }

         size_t children_count() const {
             return children_.size();
         }

        ~Window() {
            if (parent_) {
                parent_->remove(this);
            }
        }
    private:
        Window* parent_ = nullptr;
        std::vector<LedColor> leds_;
        using PaintSourcePos = struct {int x; int y;};
        std::unordered_map<const IPaintSource*, PaintSourcePos> children_;
    };

    class WS2811StripFakeDeviceDebug : public Window {
    public:
        WS2811StripFakeDeviceDebug(int w = 8, int h = 8)
            : Window(w, h) {}
        ~WS2811StripFakeDeviceDebug() {
            spdlog::debug("WS2811StripFakeDeviceDebug::~WS2811StripFakeDeviceDebug {}", (void*)this);
        }

        auto init() {
            spdlog::debug("WS2811StripFakeDeviceDebug::init {}", (void*)this);
            return 0;
        }

        auto render() {
            spdlog::debug("WS2811StripFakeDeviceDebug::render {} {}x{}", (void*)this, width(), height());
            for (int y = 0; y < height(); y++) {
                std::string line;
                for (int x = 0; x < width(); x++) {
                    line += led(x, y) == Led::Black ? ' ' : '*';
                }
                spdlog::debug("WS2811StripFakeDeviceDebug::render {}", line);
            }
            return 0;
        }
    };

    namespace literal {
        inline Text4x8 operator""_d(const char* text, size_t size) {
            return Text4x8(std::string(text, size));
        }

        inline DigitGroup3x5 operator""_d(unsigned long long digits) {
            return DigitGroup3x5(digits);
        }
    }
}
