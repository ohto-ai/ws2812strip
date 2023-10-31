#include "render.hpp"

int main() {
#ifndef RASPBERRY_PI
	spdlog::critical("This program is only for Raspberry Pi.");
	return 0;
#endif
#ifdef _DEBUG
	spdlog::set_level(spdlog::level::debug);
#endif
	using namespace ohtoai::rpi::literal;
	using ohtoai::rpi::Window;
	using ohtoai::rpi::WS2811Strip;

	srand(time(NULL));

	WS2811Strip strip(16, 32);
	Window w(32, 16);
	Window sub_w(8, 8, &w);
	Window sub_w2(16, 8, &w);

	strip.init();
	strip.set_rotate(270);
	strip.set_transparent(true);

	w.draw("Hello"_t.set_color(ohtoai::rpi::Red), 0, 0);
	w.draw("World"_t.set_color(ohtoai::rpi::Green), 8, 8);
	w.set_transparent(true);

	sub_w.draw("!!"_t.set_color(ohtoai::rpi::Blue), 0, 0);
	sub_w.set_rotate(90);

	auto text = "1145"_t;
	sub_w2.add(&text, 0, 0);

    while (true) {
		strip.clear();
		sub_w.move((sub_w.x() + 1) % w.width(), (sub_w.y() + 1)% w.height());
		sub_w2.move((sub_w2.x() + 1) % w.width(), sub_w2.y());

		auto random_color = rand() % 0xffffff;
		text.set_color(random_color);
		auto random_num = rand() % 10000;
		text.text() = std::to_string(random_num);

		strip.draw(w, 0, 0);
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
	return 0;
}