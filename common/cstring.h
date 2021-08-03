#pragma once
#include <string>
#include <sstream>
#include<unordered_map>
#include<type_traits>
#include<glog/logging.h>
#include<memory>
#include<iostream>
#include<list>
class CString {
	char* _beg, * _end;
	char* _cur;
public:
	CString(const char* beg, const char* end);
	CString(char* beg, char* end);
	CString(std::string& s);
	int size() { return _end - _beg + 1; };
	void seekg(int steps, int base = std::ios::beg);
	int tellg() { return _cur - _beg; };
	int find(std::string s, int pos = -1, int max_pos = -1) {
		return find(&s[0], s.size(), pos, max_pos);
	};
	int find(const char* target, int size, int pos = -1, int max_pos = -1);
	int next_line(std::string term = "\r\n");
	std::string read_until(std::string s, bool includetail = false, bool ignore = true);
	int str_count(std::string term);
	std::list<std::string> split(std::string by);
	std::unordered_map<std::string, std::string> parse_kv(std::string term = ":", std::string end = "\r\n", int maxpos = -1);
	std::string to_string();
	bool operator ==(std::string& v);
};

