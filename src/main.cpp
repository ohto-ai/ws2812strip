#include "render.hpp"
#include <ctime>
#include <cstdio>
#include <unistd.h>

int main() {
	spdlog::set_level(spdlog::level::debug);
	ohtoai::rpi::WS2811Strip strip(16, 32);
	strip.init();
	strip.clear();
	strip.set_rotate(270);

	int lasttime = 0;
    while (true)
    {
		time_t t = time(NULL);
		struct tm tm = *localtime(&t);
		ohtoai::rpi::Digit3x5 digit3x5[] {
			ohtoai::rpi::Digit3x5(tm.tm_hour / 10, 0xff0000),
			ohtoai::rpi::Digit3x5(tm.tm_hour % 10, 0xff0000),
			ohtoai::rpi::Digit3x5(1, 0xffffff),
			ohtoai::rpi::Digit3x5(tm.tm_min / 10, 0x00ff00),
			ohtoai::rpi::Digit3x5(tm.tm_min % 10, 0x00ff00),
			ohtoai::rpi::Digit3x5(1, 0xffffff),
			ohtoai::rpi::Digit3x5(tm.tm_sec / 10, 0x0000ff),
			ohtoai::rpi::Digit3x5(tm.tm_sec % 10, 0x0000ff),
		};
		// strip.draw(digit3x5[0], 2, 5);
		// strip.draw(digit3x5[1], 6, 5);
		// // strip.draw(digit3x5[2], 9, 5);
		// strip.draw(digit3x5[3], 12, 5);
		// strip.draw(digit3x5[4], 16, 5);
		// // strip.draw(digit3x5[5], 19, 5);
		// strip.draw(digit3x5[6], 22, 5);
		// strip.draw(digit3x5[7], 26, 5);
		ohtoai::rpi::Pixmap pixelmap(8, 8);
		pixelmap.draw(ohtoai::rpi::Ascii4x8('(', 0xff0000), 0, 0);
		pixelmap.draw(ohtoai::rpi::Ascii4x8(')', 0x00ff00), 4, 0);
		auto copy = pixelmap;
		copy.draw(ohtoai::rpi::Ascii4x8('A', 0x0000ff), 0, 4);
		strip.draw(copy, 0, 8);

		if (lasttime != tm.tm_sec) {
			lasttime = tm.tm_sec;
			auto idx = (lasttime % ('`' - '[' + 1));
			auto random_color = rand()%256 << 16 | rand()%256 << 8 | rand()%256;
			// strip.draw(ohtoai::rpi::Ascii4x8('[' + idx, random_color), idx*4 % 32, (idx*8)/64 * 8);
		}
		strip.draw(pixelmap, 0, 0);
		// strip.draw(ohtoai::rpi::Ascii4x8('A'), 26, 4);
		strip.render();
        usleep(1000000 / 2);
    }
	return 0;
}