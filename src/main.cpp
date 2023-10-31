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
	using ohtoai::rpi::Pixmap;

	srand(time(NULL));

	// Window strip(16, 32);
	WS2811Strip strip(16, 32);
	// WS2811StripFakeDeviceDebug strip(16, 32);
	Window w(32, 16);
	Window sub_w(8, 8, &w);
	Window sub_w2(16, 8, &w);

	strip.init();
	strip.set_rotate(270);
	strip.set_transparent(true);

	w.draw("hello"_d.set_color(ohtoai::rpi::Green), 0, 0);
	w.draw("world"_d.set_color(ohtoai::rpi::Magenta), 8, 8);
	w.set_transparent(true);

	sub_w.draw("!!"_d.set_color(ohtoai::rpi::Red), 0, 0);
	sub_w.set_rotate(90);

	auto text = 1111_d;
	text.color = ohtoai::rpi::Yellow;
	sub_w2.add(&text, 0, 0);
	sub_w2.set_transparent(true);

	while (true) {
		strip.clear();
		sub_w.move((sub_w.x() + 1) % w.width(), (sub_w.y() + 1)% w.height());
		sub_w2.move((sub_w2.x() + 1) % w.width(), sub_w2.y());

		text.set_digits(rand() % 10000);

		strip.draw(w, 0, 0);
		strip.render();
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
	return 0;
}