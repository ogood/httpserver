#pragma once
#include<unordered_map> 
#include<string>
#include<common/enum_http.h>
#include<functional>
#include"tcp_service.h"
#include <filesystem>
namespace Http {
	struct Context {
		Status status;
		Method method;
		//uint8_t version;
		std::string url;
		std::string protocol;
		void print();
	};
	struct Request {
		

		std::unordered_map<std::string, std::string> head;
		
		void print();
	};
	struct Reply {
	//	std::string line;
		
		//int sended_size=0;
		std::unordered_map<std::string, std::string> head;
		std::string body;
		std::string head_str;
		

	};
	struct  Client {
		Client() { buff.reserve(chunk_size); };
		Context ctx;
		Request request;
		Reply reply;
		//std::string sendbuf;
		//int sendcnt=0;
		static const int max_url = 200;
		static const int max_filesize = 1024000;//1MB
		static const int chunk_size = 1024;//bytes
		void parse_request(char* beg,int size);
		void set_head(std::string,std::string);
		void make_default_head();
		void make_head_str();
		void bytes_from_head(std::string* v);
		std::string buff;
		int replied_size = 0;
		//std::pair<char*,int> get_sendbuf();


	};

	class Router {
		//std::unordered_map<std::string, std::unique_ptr<Router>> handler;
		std::unordered_map < std::string , std::function<void(Client*)> >handlers;
		std::pair<std::string, std::filesystem::path> static_dir;
		std::string detect_filetype(std::filesystem::path p);
		public:
			Router();
			void set_static(std::string url, std::string path);
			void add_handler(std::string url, std::function<void(Client*)> f);
			
			void handle(Client* c);
	};

	class Service :public TCPService{
		
		Router* router = nullptr;
		virtual int request_in(char* in, int insize, Reply_CB* cb);
		std::pair<char*, int> next_send(Client* ,int startpos);
	public:
		Service(Router* r,int port=8080) :TCPService(port),router(r) {};
		~Service() { delete router; };
		Client* process_request(char* in, int insize);
		std::pair<char*, int> reply_callback(Client* c,int sig);
	};
}
