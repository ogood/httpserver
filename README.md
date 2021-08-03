### about

simple c++ http server. use epoll and threads pool.  
**supoorts GET Method only.**

### denpends

C++ 17 std  
CMake >=3.16

### build

build and run example server:

```
$git clone https://github.com/ogood/httpserver.git
$cd httpserver
$mkdir build && cd build && cmake .. && make
$./server  
```

### usage
simple as the example (/test.cpp):
```
#include <string>
#include <common/utility.h>
#include <server_side/tcp_service/http.h>
#include <filesystem>

int main(int argc, char** argv) {
	Http::Router router;
	router.set_static("/static/", std::filesystem::current_path().string());
	router.add_handler("/hello", [](Http::Client* c) {
		c->set_head("Token", "123456");
		c->reply.body = "hello world";
		});
	Http::Service s(&router,8080);
	s.start();
	return 0;
}
```

