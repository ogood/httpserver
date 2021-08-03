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