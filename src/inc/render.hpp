#pragma once

#include <ws2811.h>
#include <vector>
#include <stdexcept>
#include <memory>
#include <spdlog/spdlog.h>

namespace ohtoai::rpi {
    using LedColor = ws2811_led_t;

    enum Led: LedColor {
        White = 0xffff,
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
        IPaintDevice() = default;
        virtual ~IPaintDevice() = default;
        virtual LedColor& led(int x, int y) = 0;
        virtual const LedColor& led(int x, int y) const = 0;
        virtual void set_transparent(bool transparent) {
            transparent_ = transparent;
        }

        virtual void set_background(LedColor color) {
            back_ = color;
        }

        virtual void clear() {
            for (int y = 0; y < height(); ++y) {
                for (int x = 0; x < width(); ++x) {
                    led(x, y) = back_;
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
                    if (transparent_ && src_led == back_) {
                        continue;
                    }
                    led(x + dx, y + dy) = src_led;
                }
            }
        }
        
        virtual void flip(bool flip_x, bool flip_y) {
            for (int y = 0; y < height(); ++y) {
                for (int x = 0; x < width() / 2; ++x) {
                    int dx = flip_x ? width() - x - 1 : x;
                    int dy = flip_y ? height() - y - 1 : y;
                    auto tmp = led(x, y);
                    led(x, y) = led(dx, dy);
                    led(dx, dy) = tmp;
                }
            }
        }

    protected:
        bool transparent_ = false;
        LedColor back_ = Led::Black;
    };

    template <typename StripIndexType = ISnakeStripIndex>
    class IRotatablePaintDevice: public IPaintDevice {
    public:
        IRotatablePaintDevice(int w, int h): width_(w), height_(h), logic_width_(w), logic_height_(h), rotate_helper_(std::make_shared<StripRotateHelper<StripIndexType, 0, false, false>>(width_, height_)) {};
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
        Pixmap(int w, int h)
            : IRotatablePaintDevice<IVectorStripIndex>(w, h),
            leds_(w * h) {}
        LedColor& led(int x, int y) override { return leds_[index(x, y)]; }
        const LedColor& led(int x, int y) const override { return leds_[index(x, y)]; }
    private:
        std::vector<LedColor> leds_;
    };

    class Digit3x5 : public IPaintSource {
    public:
        Digit3x5(int digit, LedColor color = Led::White, LedColor back = Led::Black) : digit(digit), color(color), back(back) {}
        int width() const override { return 3; }
        int height() const override { return 5; }
        const LedColor& led(int x, int y) const override {
            if (digit >= 0 && digit < sizeof(digit3x5font) / sizeof(digit3x5font[0]) && x >= 0 && x < 3 && y >= 0 && y < 5) {
                return (digit3x5font[digit] & (1 << (y * 3 + 2 - x))) ? color : back;
            }
            throw std::out_of_range(fmt::format("Digit3x5::led<{}>({}, {})", digit, x, y));
        }

        int digit;
        LedColor color;
        LedColor back;
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

    class Ascii4x8 : public IPaintSource {
        friend class Text4x8;
    public:
        Ascii4x8(char c, LedColor color = Led::White, LedColor back = Led::Black) : c_(c), color_(color), back_(back) {}
        int width() const override { return 4; }
        int height() const override { return 8; }
        const LedColor& led(int x, int y) const override {
            if (c_ >= 0 && c_ < sizeof(ascii4x8font) / sizeof(ascii4x8font[0]) && x >= 0 && x < 4 && y >= 0 && y < 8) {
                return (ascii4x8font[c_] & (1 << (y * 4 + 3 - x))) ? color_ : back_;
            }
            throw std::out_of_range("Ascii4x8::led");
        }

        auto& set_color(LedColor color) { 
            color_ = color; 
            return *this;
        }

        char& c() { return c_; }
        char c() const { return c_; }
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
        char c_;
        LedColor color_ = Led::White;
        LedColor back_ = Led::Black;
    };

    class WS2811Strip : public IRotatablePaintDevice<ISnakeStripIndex>, protected ws2811_t {
        enum {DMA = 10, GPIO_PIN = 18};
    public:
        WS2811Strip(int w = 8, int h = 8)
            : IRotatablePaintDevice<ISnakeStripIndex>(w, h), ws2811_t() {
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
            return ws2811_init(this);
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
        LedColor& led(int x, int y) override {
            return channel[0].leds[index(x, y)];
        }
        const LedColor& led(int x, int y) const override {
            return channel[0].leds[index(x, y)];
        }
    };

    class Text4x8 : public IPaintSource {
    public:
        Text4x8(const std::string& text, LedColor color = Led::White, LedColor back = Led::Black) : text_(text), color_(color), back_(back) {}
        int width() const override { return text_.size() * 4; }
        int height() const override { return 8; }
        const LedColor& led(int x, int y) const override {
            if (x >= 0 && x < width() && y >= 0 && y < height()) {
                return (Ascii4x8::ascii4x8font[text_[x / 4]] & (1 << (y * 4 + 3 - x % 4))) ? color_ : back_;
            }
            throw std::out_of_range("Text4x8::led");
        }

        auto& set_color(LedColor color) { 
            color_ = color; 
            return *this;
        }

        std::string& text() { return text_; }
        const std::string& text() const { return text_; }
    private:
        std::string text_;
        LedColor color_ = Led::White;
        LedColor back_ = Led::Black;
    };

    class Window : public IRotatablePaintDevice<IVectorStripIndex> {
    public:
        Window(int x, int y, int w, int h, Window* parent = nullptr) 
            : IRotatablePaintDevice<IVectorStripIndex>(w, h),
            x_(x), y_(y),
            leds_(w * h) {
                if (parent) {
                    parent->children_.push_back(this);
                    parent_ = parent;
                }
            }

        LedColor& led(int x, int y) override { return leds_[index(x, y)]; }
        const LedColor& led(int x, int y) const override { 
            for (const auto& child : children_) {
                if (x >= child->x_ && x < child->x_ + child->width_ && y >= child->y_ && y < child->y_ + child->height_) {
                    auto& child_led = child->led(x - child->x_, y - child->y_);
                    if (transparent_ && child_led != back_)
                        return child_led;
                }
            }
            return leds_[index(x, y)]; 
        }

        void move(int x, int y) {
            x_ = x;
            y_ = y;
        }

        int x() const { return x_; }
        int y() const { return y_; }

        ~Window() {
            if (parent_) {
                auto it = std::find(parent_->children_.begin(), parent_->children_.end(), this);
                if (it != parent_->children_.end()) {
                    parent_->children_.erase(it);
                }
            }
        }
    private:
        int x_;
        int y_;
        Window* parent_ = nullptr;
        std::vector<LedColor> leds_;
        std::vector<Window*> children_;
    };

    namespace literal {
        inline Text4x8 operator""_t(const char* text, size_t size) {
            return Text4x8(std::string(text, size));
        }

        inline Digit3x5 operator""_d(unsigned long long digit) {
            return Digit3x5(digit);
        }
    }
}