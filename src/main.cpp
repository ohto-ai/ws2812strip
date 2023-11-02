#include "render.hpp"
#include <rest_rpc.hpp>

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
	std::atomic_bool auto_render = false;

	WS2811Strip strip(16, 32);
	strip.init();
	rest_rpc::rpc_service::rpc_server server(9000, std::thread::hardware_concurrency());

	server.register_handler("render", [&](rest_rpc::rpc_service::rpc_conn conn){
		strip.render();
	});
	server.register_handler("draw_pixel", [&](rest_rpc::rpc_service::rpc_conn conn, int x, int y, ohtoai::rpi::LedColor color){
		strip.led_ref(x, y) = color;
	});
	server.register_handler("draw_text", [&](rest_rpc::rpc_service::rpc_conn conn, int x, int y,
		std::string text, ohtoai::rpi::LedColor color, ohtoai::rpi::LedColor background){
		strip.draw(ohtoai::rpi::Text4x8(text).set_color(color).set_background(background), x, y);
	});
	server.register_handler("draw_number", [&](rest_rpc::rpc_service::rpc_conn conn, int x, int y,
		int number, ohtoai::rpi::LedColor color, ohtoai::rpi::LedColor background){
		strip.draw(ohtoai::rpi::DigitGroup3x5(number).set_color(color), x, y);
	});
	server.register_handler("clear", [&](rest_rpc::rpc_service::rpc_conn conn){
		strip.clear();
	});
	server.register_handler("width", [&](rest_rpc::rpc_service::rpc_conn conn){
		return strip.width();
	});
	server.register_handler("height", [&](rest_rpc::rpc_service::rpc_conn conn){
		return strip.height();
	});
	server.register_handler("pixel", [&](rest_rpc::rpc_service::rpc_conn conn, int x, int y){
		return strip.led(x, y);
	});
	server.register_handler("set_rotate", [&](rest_rpc::rpc_service::rpc_conn conn, int degree, bool flip_x, bool flip_y){
		return strip.set_rotate(degree, flip_x, flip_y);
	});
	server.register_handler("set_transparent", [&](rest_rpc::rpc_service::rpc_conn conn, bool transparent){
		return strip.set_transparent(transparent);
	});
	server.register_handler("set_background", [&](rest_rpc::rpc_service::rpc_conn conn, ohtoai::rpi::LedColor color){
		return strip.set_background(color);
	});
	server.register_handler("set_auto_render", [&](rest_rpc::rpc_service::rpc_conn conn, bool auto_render_){
		return auto_render = auto_render_;
	});

	std::thread render_thread = std::thread([&]{
		while (true) {
			if (auto_render)
				strip.render();
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	});

	server.run();
	return 0;
}