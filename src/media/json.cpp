#include "json.hpp"
#include <iostream>
#include <fstream>
#include <cassert>

json::type_container json::parser::parseMemory(const char *begin, unsigned int size)
{
	assert(begin != nullptr && size != 0);
	
	// Check for UTF-8 bom
	if (size >= 3) {
		if (static_cast<unsigned char>(begin[0]) == 0xEF && static_cast<unsigned char>(begin[1]) == 0xBB
			&& static_cast<unsigned char>(begin[2]) == 0xBF) {
			begin += 3;
			size -= 3;
		}
	}
	
	// Initialize the class
	_cur = begin;
	_end = begin+size;

	// Yes all types are possible, see RFC 7159
	ignoreSpace();
	if (*_cur == '{')
		return extractObject();
	else if (*_cur == '[') 
		return extractArray();
	else if ((*_cur >= '0' && *_cur <= '9') || *_cur == '-')
		return extractNumber();
	else if (*_cur == 't' || *_cur == 'f')
		return extractBoolean();
	else if (*_cur == '\"')
		return extractString();
	else if (*_cur == 'n')
		return extractNull();
	else
		throw exception(except_e::MALFORMED);
}

json::type_container json::parser::parseFile(std::string_view path)
{
	// Open the json file
	std::ifstream file;
	file.open(std::string(path), std::ios_base::ate);
	if (!file.is_open())
		throw exception(except_e::IO_ERROR);
	auto length = file.tellg();
	file.seekg(0, std::ios_base::beg);

	// Read the json file into memory and parse it
	char *test = new char[length];
	file.read(test, length);
	auto obj = parseMemory(test, length);

	// Free memory and return
	delete[] test;
	return obj;
}

json::parser& json::parser::disableExpensiveChecks(bool disable)
{
	_fast = disable;
	return *this;
}

void json::parser::ignoreSpace()
{
	if (!_fast) {
		while (_cur != _end) {
			const char *next = unicode::utf8::next(_cur, _end);
			if (next == _cur+1) {
				if (*_cur == '\t' || *_cur == ' ' || *_cur == '\r' || *_cur == '\n') {
					_cur = next;
					continue;
				}
				else {
					return;
				}
			}
			else {
				throw exception(except_e::MALFORMED);
			}
		}
		throw exception(except_e::MALFORMED);
	}
	else {
		while (_cur != _end) {
			if (*_cur == '\t' || *_cur == ' ' || *_cur == '\r' || *_cur == '\n')
				++_cur;
			else
				return;
		}
		throw exception(except_e::MALFORMED);
	}
}

std::string json::parser::extractStdString()
{
	auto begin = ++_cur;

	if (!_fast) {
		bool checknext = false;
		int checkhex = 0;
		while (_cur != _end) {
			auto next = unicode::utf8::next(_cur, _end);

			// Check for \", \\, \/, \b, \f, \n, \r, \t, \u
			if (checknext) {
				if (*_cur == 'u')
					checkhex = 4;
				else if (*_cur != '\"' && *_cur != '\\' && *_cur != '/' && *_cur != 'n' && *_cur != 't' && *_cur != 'r' && *_cur != 'f' && *_cur != 'b')
					throw exception(except_e::MALFORMED);
				checknext = false;
			}
			// Check whether the characters are indeed hex
			else if (checkhex) {
				if ((*_cur >= '0' && *_cur <= '9') || (*_cur >= 'a' && *_cur <= 'f') || (*_cur >= 'A' && *_cur <= 'F'))
					--checkhex;
				else
					throw exception(except_e::MALFORMED);
			}
			// Special character coming up
			else if (*_cur == '\\') {
				checknext = true;
			}
			// Check whether the string is terminated
			else if (*_cur == '\"') {
				std::string ret;
				ret.reserve(static_cast<std::ptrdiff_t>(_cur-begin));
				return ret.assign(begin, _cur++);
			}
			_cur = next;
		}
		throw exception(except_e::MALFORMED);
	}
	else {
		while (_cur != _end) {
			if (*_cur == '"' && *(_cur-1) != '\\') {
				std::string ret;
				ret.reserve(static_cast<std::ptrdiff_t>(_cur-begin));
				return ret.assign(begin, _cur++);
			}
			else
				++_cur;
		}
		throw exception(except_e::MALFORMED);
	}
}

json::type_container json::parser::extractNumber()
{
	auto begin = _cur;
	auto isDouble = [&]() {
		bool found = false;
		while (_cur != _end) {
			auto next = unicode::utf8::next(_cur, _end);

			if (*_cur == '.' || *_cur == 'e' || *_cur == 'E')
				found = true;
			else if ((*_cur < '0' || *_cur > '9') && (*_cur != '-' || *_cur != '+'))
				return found;
			_cur = next;
		}
		throw exception(except_e::MALFORMED);
	};


	if (isDouble()) {
		auto num = std::stod(std::string(begin, _cur));
		return type_container().move<type_e::DOUBLE>(num);
	}
	else {
		// The default is a signed long long
		try {
			auto num = std::stoll(std::string(begin, _cur));
			return type_container().move<type_e::INT>(num);
		}
		catch (std::out_of_range) { 
		}
		catch (...) {
			throw exception(except_e::MALFORMED);
		}	
		// If the type is too large try unsigned long long
		try {
			auto num = std::stoull(std::string(begin, _cur));
			return type_container().move<type_e::UINT>(num);
		}
		catch (...) {
			throw exception(except_e::MALFORMED);
		}
	}
}

json::type_container json::parser::extractBoolean()
{
	if (_end >= _cur+5) {
		auto str = std::string(_cur, 5);
		if (str.compare(0, 4, "true") == 0) {
			auto container = type_container().move<type_e::BOOL>(true);
			_cur += 4;
			return container;
		}
		else if (str == "false") {
			auto container = type_container().move<type_e::BOOL>(false);
			_cur += 5;
			return container;
		}
	}
	throw exception(except_e::MALFORMED);
}

json::type_container json::parser::extractString()
{
	auto str = extractStdString();
	return type_container().move<type_e::STR>(str);
}

json::type_container json::parser::extractArray()
{
	array arr;
	++_cur; // Skip '[' char
	
	if (_cur != _end) {
		while (_cur != nullptr) {
			// Check the first non white space character
			ignoreSpace();
			if ((*_cur >= '0' && *_cur <= '9') || *_cur == '-') {
				auto container = extractNumber();
				arr.push_back(std::move(container));
			}
			else if (*_cur == 't' || *_cur == 'f') {
				auto container = extractBoolean();
				arr.push_back(std::move(container));
			}
			else if (*_cur == '\"') {
				auto container = extractString();
				arr.push_back(std::move(container));
			}
			else if (*_cur == '[') {
				auto container = extractArray();
				arr.push_back(std::move(container));
			}
			else if (*_cur == '{') {
				auto container = extractObject();
				arr.push_back(std::move(container));
			}
			else if (*_cur == 'n') {
				auto container = extractNull();
				arr.push_back(std::move(container));
			}
			else if (*_cur == ']') {
				++_cur;
				return type_container().move<type_e::ARR>(arr);
			}
			else
				throw exception(except_e::MALFORMED);

			// Find the comma or end
			ignoreSpace();
			if (*_cur != ',' && *_cur != ']')
				throw exception(except_e::MALFORMED);
			if (*_cur++ == ']') {
				return type_container().move<type_e::ARR>(arr);
			}
		}
	}
	throw exception(except_e::MALFORMED);
}

json::type_container json::parser::extractObject()
{
	object obj;
	++_cur; // Skip '{' char
	
	if (_cur != _end) {
		while (_cur != nullptr) {
			// The result
			std::pair<std::string, type_container> result;

			// Check if the object is empty
			ignoreSpace();
			if (*_cur == '}') {
				++_cur;
				return type_container().move<type_e::OBJ>(obj);
			}
			else if (*_cur != '\"')
				throw exception(except_e::MALFORMED);

			// Grab the key
			std::string key = extractStdString();
			result.first = std::move(key);

			// Find the next character after the ':' sign
			ignoreSpace();
			if (*_cur != ':')
				throw exception(except_e::MALFORMED);
			++_cur;
			ignoreSpace();

			// Check the retrieved character
			if ((*_cur >= '0' && *_cur <= '9') || *_cur == '-') {
				auto container = extractNumber();
				result.second = std::move(container);
				obj.insert(std::move(result));
			}
			else if (*_cur == 't' || *_cur == 'f') {
				auto container = extractBoolean();
				result.second = std::move(container);
				obj.insert(std::move(result));
			}
			else if (*_cur == '\"') {
				auto container = extractString();
				result.second = std::move(container);
				obj.insert(std::move(result));
			}
			else if (*_cur == '[') {
				auto container = extractArray();
				result.second = std::move(container);
				obj.insert(std::move(result));
			}
			else if (*_cur == '{') {
				auto container = extractObject();
				result.second = std::move(container);
				obj.insert(std::move(result));
			}
			else if (*_cur == 'n') {
				auto container = extractNull();
				result.second = std::move(container);
				obj.insert(std::move(result));
			}
			else
				throw exception(except_e::MALFORMED);

			// Find the comma or end
			ignoreSpace();
			if (*_cur != ',' && *_cur != '}')
				throw exception(except_e::MALFORMED);
			if (*_cur++ == '}') {
				return type_container().move<type_e::OBJ>(obj);
			}
		}
	}
	throw exception(except_e::MALFORMED);
}

json::type_container json::parser::extractNull()
{
	if (_end >= _cur+4) {
		auto str = std::string(_cur, 4);
		if (str == "null") {
			auto container = type_container().move<type_e::NUL>(nullptr);
			_cur += 4;
			return container;
		}
	}
	throw exception(except_e::MALFORMED);
}
