#include "render.hpp"

int main() {
	spdlog::set_level(spdlog::level::debug);
	using namespace ohtoai::rpi::literal;
	using ohtoai::rpi::Window;
	using ohtoai::rpi::WS2811Strip;

	WS2811Strip strip(16, 32);
	Window w(0, 0, 32, 16);
	Window sub_w(0, 0, 8, 8, &w);
	Window sub_w2(0, 4, 16, 8, &w);

	strip.init();
	strip.set_rotate(270);
	strip.set_transparent(true);
	
	w.draw("Hello"_t.set_color(ohtoai::rpi::Red), 0, 0);
	w.draw("World"_t.set_color(ohtoai::rpi::Green), 8, 8);
	w.set_transparent(true);
	
	sub_w.draw("!!"_t.set_color(ohtoai::rpi::Blue), 0, 0);
	sub_w.set_rotate(90);

	sub_w2.draw("1145"_t, 0, 0);

    while (true) {
		strip.clear();
		sub_w.move((sub_w.x() + 1) % w.width(), (sub_w.y() + 1)% w.height());
		sub_w2.move((sub_w2.x() + 1) % w.width(), sub_w2.y());

		strip.draw(w, 0, 0);
		strip.render();
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
	return 0;
}