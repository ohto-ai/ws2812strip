# WS2812 Matrix


## Build on raspberrypi
```shell
cmake -Bbuild -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j8 --target ws2812_client
```

## Run on raspberrypi
```shell
sudo ./build/src/ws2812_client
```

## Use rpc client to control you matrix or strip
```cpp
#include <rest_rpc.hpp>

// client for WS2812 LED trip
int main() {
	rest_rpc::rpc_client client("192.168.31.200", 9000);
	client.connect();
	auto width = client.call<int>("width");
	auto height = client.call<int>("height");
	std::cout << "width: " << width << ", height: " << height << std::endl;
	client.call<void>("clear");
	client.call<void>("set_auto_render", true);
	client.call<void>("set_rotate", 270, false,  false);
	client.call<void>("draw_text", 0, height / 2, "Hello", 0x0000ff, 0);
	client.call<void>("draw_text", 0, 0, "World", 0xff0000, 0);
	// client.call<void>("render"); // no need to call render due to auto render
	client.run();
	return 0;
}
```
