#include "http.h"
#include<common/utility.h>
#include <string_view>
#include <iostream>
#include<ctime>
#include <common/cstring.h>
#include <fstream>
#include<cstring>
namespace Http {
	
	void Client::parse_request(char* beg, int size)
	{
		CString cs(beg, beg+size);
		auto pos = cs.find(std::string("\r\n"), 0, max_url + 10);
		LOG_IF(ERROR, pos < 0) << "failed to find first line";
		CString firstline(beg, beg+pos);
		auto cmd = firstline.split(" ");
		if (cmd.size() != 3 || cmd.front() != "GET")
		{
			DLOG(INFO) << "No support method other than get.";
			ctx.status = Status::BadRequest;
			return;
		}

		ctx.method = Http::Method::GET;
		cmd.pop_front();
		ctx.url = cmd.front();
		ctx.protocol = cmd.back();
		cs.seekg(pos + 2);//next line
		request.head = cs.parse_kv();
		ctx.status = Http::Status::Ok;
	}
	void Client::set_head(std::string k, std::string v)
	{
		reply.head[std::move(k)] = std::move(v);
	}
	void Client::make_default_head()
	{
		reply.head["Connection"] = "close";

		auto f = reply.head.find("Content-Type");
		if(f== reply.head.end())
			reply.head["Content-Type"] = "text/html; charset=UTF-8";
		f = reply.head.find("Content-Length");
		if (f == reply.head.end())
			reply.head["Content-Length"] = std::to_string(reply.body.size());

		
		time_t now = std::time(0);//seconds passed since 1970
		tm* ltm = std::gmtime(&now);
		std::string date;
		date.resize(30);
		std::strftime(&date[0], date.size(), "%a, %d %b %Y %H:%M:%S GMT", ltm);
		while(date.back()=='\0')
		date.resize(date.size()-1);
		reply.head["Date"] = date;
		reply.head["Expires"] = std::move(date);

	}

	void Client::make_head_str()
	{
		reply.head_str.reserve(chunk_size);
		reply.head_str += ctx.protocol;
		reply.head_str += " ";
		reply.head_str += std::to_string(int(ctx.status));
		reply.head_str += " ";
		reply.head_str += "OK\r\n";
		for (auto& i : reply.head) {
			
			reply.head_str += i.first;
			reply.head_str += ": ";
			reply.head_str += i.second;
			reply.head_str += "\r\n";
		}
		reply.head_str += "\r\n";
	}

	void Client::bytes_from_head(std::string* v)
	{
		*v += ctx.protocol;
		*v += " ";
		*v += std::to_string(int(ctx.status));
		*v += " ";
		*v += "OK\r\n";
		for (auto& i : reply.head) {
			*v += i.first;
			*v += ": ";
			*v += i.second;
			*v += "\r\n";
		}
		*v += "\r\n";
	}


	std::string Router::detect_filetype(std::filesystem::path p)
	{	
		if (p.empty())
			return std::string();
		auto ext = p.extension().string().substr(1);

		if (ext == "html")
			return "text/html";
		if (ext == "txt"|| ext == "cpp")
			return "text/plain";
		if (ext == "gif")
			return "image/gif";
		if (ext == "png")
			return "image/png ";
		if (ext == "jpg" || ext == "jpeg")
			return "image/jpeg ";

		return "application/octet-stream";
	}

	Router::Router()
	{
	}
	void Router::set_static(std::string url, std::string path)
	{
		static_dir.first = std::move(url);
		if (path.back() != '/')
			path += '/';
		static_dir.second = std::filesystem::path(path);
	}
	void Router::add_handler(std::string url, std::function<void(Client*)> f)
	{
		handlers[url] = std::move(f);
	}
	void Router::handle(Client* c)
	{
		bool usestatic=false;
		if (c->ctx.url.size() >= static_dir.first.size()) {
			CString cs(&c->ctx.url[0], &c->ctx.url[static_dir.first.size()]);
			if (cs == static_dir.first)
				usestatic = true;
		}
		
		if (usestatic)
		{
			std::filesystem::path p(static_dir.second);
			p += c->ctx.url.substr(static_dir.first.size());
			if (std::filesystem::exists(p))
			{
				int filesize = std::filesystem::file_size(p);
				if (filesize <= c->max_filesize)
				{
					c->reply.body.resize(filesize);
					c->set_head("Content-Type", detect_filetype(p));
					std::ifstream file;
					file.open(p, std::ios::binary | std::ios::in);
					file.read((char*)(c->reply.body.data()), c->reply.body.size());
					file.close();
				}
				else {
					c->reply.body = "file too large. Max size : " + std::to_string(c->max_filesize / 1024000) + "MB";
				}

			}
			else
			{
				c->reply.body = "static file not found";
				
			}
		}
		else {

			auto f = handlers.find(c->ctx.url);
			if (f == handlers.end())
			{

				c->reply.body = "no handler for this url";

			}
			else
				f->second(c);
		}
		c->make_default_head();
		c->make_head_str();
		return;
	}

	Http::Client* Service::process_request(char* in, int insize)
	{

		Http::Client* c = new Http::Client;
		c->parse_request(in, insize);
		router->handle(c);
		return c;
	}

	std::pair<char*, int> Service::reply_callback(Client* client, int sig)
	{

		if (sig < 0) {
			delete client;
		}
		else if (sig == 0) {//query pending data
			if (client->replied_size >= client->reply.body.size() + client->reply.head_str.size())
			{
				delete client;
			}
			else
				return next_send(client, client->replied_size);

		}
		else if (sig > 0) {
			client->replied_size += sig;
			return next_send(client, client->replied_size);
		}
		else
			LOG(FATAL) << "sended bytes > reply data size?";


		return std::pair<char*, int>(nullptr, 0);//nothing to send
	}

	int Service::request_in(char* in, int insize, Reply_CB* cb)
	{
		Client* client=process_request(in,insize);
		if(client->ctx.status!=Http::Status::Ok)
		return -1;
		*cb = [this, client](int sig)->std::pair<char*, int> {
			return this->reply_callback(client, sig);

		};
		return 0;

	}

	std::pair<char*, int> Service::next_send(Client* c,int pos)
	{
		std::pair<char*, int> v;
		if (pos >= c->reply.head_str.size() + c->reply.body.size())//all data sended
		{
			v.first = nullptr;
			v.second = 0;
		}
		else if (pos < c->reply.head_str.size())//to send head
		{
			int withbodysize = c->reply.head_str.size() - pos + c->reply.body.size();
			if (withbodysize <= c->chunk_size) {//with body
				c->buff.resize(withbodysize);
				memcpy(&c->buff[0], &c->reply.head_str[pos], c->reply.head_str.size() - pos);
				memcpy(&c->buff[c->reply.head_str.size() - pos], &c->reply.body[0], withbodysize- c->reply.head_str.size() + pos);
				v.first = &c->buff[0];
				v.second = c->buff.size();
			}
			else//without body
			{
				v.first = &c->reply.head_str[pos];
				v.second = c->reply.head_str.size() - pos;
			}
		}
		else {//to send body
			int ofst = pos - c->reply.head_str.size();
			int size = c->reply.body.size() - ofst;
			if (size > c->chunk_size)
				size = c->chunk_size;
			v.first = &c->reply.body[ofst];
			v.second = size;
		}
		return v;
	}

}