#include "cstring.h"
#include <filesystem>
#include <fstream>
#include<cstdlib>
#include<functional>
#include<cstring>


CString::CString(const char* beg, const char* end) :_beg(const_cast<char*>(beg)), _end(const_cast<char*>(end) - 1)
{
	_cur = _beg;
}
CString::CString(char* beg, char* end) : _beg(beg), _end(end - 1)
{
	_cur = _beg;
}
CString::CString(std::string& s)
{
	_beg = &s[0];
	_cur = _beg;
	_end = &s[s.size() - 1];
}



void CString::seekg(int steps, int base)
{
	if (base == std::ios::beg)
		_cur = steps + _beg;
	else if (base == std::ios::end)
		_cur = steps + _end;
	else
		_cur = steps + _cur;

	if (_cur < _beg)
		_cur = _beg;
	if (_cur > _end)
		_cur = _end;
}

int CString::find(const char* target, int size, int pos, int max_pos)
{
	target = const_cast<char*>(target);
	if (_cur > _end)
		return -1;
	if (pos < 0)
		pos = _cur - _beg;
	if (max_pos < 0)
		max_pos = _end - _beg;


	bool stopfind = false, found = false;
	//DLOG(INFO) << "start finding loop";
	while (stopfind == false) {
		//LOG_IF(INFO, pos > 25) << *(_beg + pos)<<",int:" << int(*(_beg + pos)) << " vs " << int(*target);
		if (_beg + pos + size - 1 > _beg + max_pos)
		{
			//DLOG(INFO) << "out of range pos,size:"<<pos<<","<<size;
			stopfind = true;
		}
		else if (*(_beg + pos) == *target)
		{
			int i = 1;
			for (; i < size; i++) {
				if (*(_beg + pos + i) == *(target + i))
					continue;
				else
					break;
			}
			if (i == size)
			{
				stopfind = true;
				found = true;
				break;
			}
			else
				pos++;

		}
		else
			pos++;
	}
	//DLOG(INFO) << "out finding loop,found?"<<found;
	if (found)
		return pos;
	else
		return -1;

}

int CString::next_line(std::string term)
{
	int pos = find(term, _cur - _beg);
	if (pos < 0)
		return -1;
	_cur = _beg + pos + 1;
	return pos + 1;
}

std::string CString::read_until(std::string s, bool includetail, bool ignore)
{
	std::string v;
	if (_cur > _end)
		return v;
	int pos = find(s.data(), s.size());
	if (pos < 0) {
		std::string v(_cur, _end - _cur + 1);
		_cur = _end + 1;
		return v;
	}

	int realpos = pos;
	if (includetail == false)
	{

		if (realpos > 0)
			realpos--;
		else {
			_cur += s.size();
			return v;
		}
	}


	if (ignore) {
		while (true) {
			if (*_cur == ' ' || *_cur == '\r' || *_cur == '\n')
			{
				_cur++;
				if (_cur > _end)
					return v;
			}
			else
				break;
		}
		while (true) {
			if (*(_beg + realpos) == ' ' || *(_beg + realpos) == '\r' || *(_beg + realpos) == '\n')
			{
				realpos--;
				if (realpos + _beg < _cur)
					return v;
			}
			else
				break;
		}
	}

	v.resize(realpos - (_cur - _beg) + 1);
	memcpy(&v[0], _cur, v.size());

	_cur = _beg + pos + s.size();

	return v;
}

int CString::str_count(std::string term)
{
	int cnt = 0, pos = 0, p;
	while (true) {
		p = find(term.data(), term.size(), pos);
		//	DLOG(INFO) << "found pos:" << p<<",before:"<<std::string(_beg+p-10,10);
		if (p >= 0) {
			pos = p + term.size();
			cnt++;
		}
		else
			break;

	}
	return cnt;

}

std::list<std::string> CString::split(std::string by)
{
	std::list<std::string> v;

	while (true) {
		if (_cur > _end)
			break;
		//DLOG(INFO) << "before read pos:" << _cur - _beg;
		auto s = read_until(by);
		//DLOG(INFO) << "after read str size:"<< s.size() << ", content:" << s<<", cur pos:" << _cur - _beg;
		if (s.empty())
		{
			if (_cur - _beg == by.size())
				continue;
			else if (_cur > _end)
				break;
		}


		v.push_back(std::move(s));

	}
	return v;
}

std::unordered_map<std::string, std::string> CString::parse_kv(std::string term, std::string end, int maxpos)
{
	std::unordered_map<std::string, std::string> v;
	std::string k, va;
	if (maxpos < 0)
		maxpos = _end - _beg;
	while (true) {
		k = read_until(term);
		if (k.empty() || _cur - _beg > maxpos)
			break;
		_cur++;
		va = read_until(end);
		if (va.empty() || _cur - _beg > maxpos)
			break;
		v.insert(std::pair(std::move(k), std::move(va)));

	}

	return v;
}

std::string CString::to_string()
{
	return std::string(_beg, _end - _beg + 1);
}

bool CString::operator==(std::string& v)
{
	if (_end - _beg + 1 != v.size())
		return false;
	for (int i = 0; i < v.size(); i++)
		if (*(_beg + i) != v[i])
			return false;
	return true;
}

