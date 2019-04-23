#include "jsontypes.hpp"
#include <cassert>

using namespace json;

/*
 * Exceptions
*/
exception::exception(except_e ecode)
	: ecode(ecode)
{
}

const char * exception::what() const noexcept
{
	switch (ecode) {
	case except_e::IO_ERROR:
		return "Failed to read file";
	case except_e::MALFORMED:
		return "Malformed json";
	case except_e::CORRUPT:
		return "Illegal UTF-8 character in JSON";
	default:
		return "Unkown json error";
	}
}

/*
 * JSON types
*/
std::string array::toString() const
{
	std::string ret = "[";

	// Useless "optimization" for the hell of it
	auto size = this->size();
	if (size > 0) {
		if (size < 1) {
			ret += (*this)[0].toString();
		}
		else {
			int i;
			for (i = 0; i <size-1; ++i) {
				ret += (*this)[i].toString() + ",";
			}
			ret += (*this)[i].toString();
		}
	}
	ret += "]";
	return ret;
}

std::string array::toHumanString(unsigned int level) const
{
	std::string ret = "[";

	// Useless "optimization" for the hell of it
	auto size = this->size();
	if (size > 0) {
		ret += "\n";
		if (size < 1) {
			for (unsigned int j = 0; j < level+1; ++j)
				ret += "  ";
			ret += (*this)[0].toHumanString(level+1) + "\n";
		}
		else {
			int i;
			for (i = 0; i <size-1; ++i) {
				for (unsigned int j = 0; j < level+1; ++j)
					ret += "  ";
				ret += (*this)[i].toHumanString(level+1) + ",\n";
			}
			for (unsigned int j = 0; j < level+1; ++j)
				ret += "  ";
			ret += (*this)[i].toHumanString(level+1) + "\n";
		}
	}
	else {
		ret += " ]";
		return ret;
	}
	for (unsigned int j = 0; j < level; ++j)
		ret += "  ";
	ret += "]";
	return ret;
}

std::string object::toString() const
{
	std::string ret = "{";

	// Useless "optimization" for the hell of it
	auto size = this->size();
	if (size > 0) {
		auto it = this->begin();
		if (size < 1) {
			ret += it->first + ":" + it->second.toString();
			++it;
		}
		else {
			for (int i = 0; i < size-1; ++i, ++it) {
				ret += it->first + ":" + it->second.toString() + ",";
			}
			ret += it->first + ":" + it->second.toString();
		}
	}
	ret += "}";
	return ret;
}

std::string object::toHumanString(unsigned int level) const
{
	std::string ret = "{";

	// Useless "optimization" for the hell of it
	auto size = this->size();
	if (size > 0) {
		ret += "\n";
		auto it = this->begin();
		if (size < 1) {
			for (unsigned int j = 0; j < level+1; ++j)
				ret += "  ";
			ret += it->first + " : " + it->second.toHumanString(level+1) + "\n";
			++it;
		}
		else {
			for (int i = 0; i < size-1; ++i, ++it) {
				for (unsigned int j = 0; j < level+1; ++j)
					ret += "  ";
				ret += it->first + " : " + it->second.toHumanString(level+1) + ",\n";
			}
			for (unsigned int j = 0; j < level+1; ++j)
				ret += "  ";
			ret += it->first + " : " + it->second.toHumanString(level+1) + "\n";
		}
	}
	else {
		ret += " }";
		return ret;
	}
	for (unsigned int j = 0; j < level; ++j)
		ret += "  ";
	ret += "}";
	return ret;
}

/*
 * Raw types
*/
raw_type<type_e::INT>::raw_type(long long rhs) noexcept
	: _value(rhs)
{
}
raw_type<type_e::INT>& raw_type<type_e::INT>::operator=(const raw_type& rhs) noexcept
{
	_value = rhs._value;
	return *this;
}
raw_type<type_e::INT>& raw_type<type_e::INT>::operator=(long long rhs) noexcept
{
	_value = rhs;
	return *this;
}

raw_type<type_e::UINT>::raw_type(unsigned long long rhs) noexcept
	: _value(rhs)
{
}
raw_type<type_e::UINT>& raw_type<type_e::UINT>::operator=(const raw_type& rhs) noexcept
{
	_value = rhs._value;
	return *this;
}
raw_type<type_e::UINT>& raw_type<type_e::UINT>::operator=(unsigned long long rhs) noexcept
{
	_value = rhs;
	return *this;
}

raw_type<type_e::DOUBLE>::raw_type(double rhs) noexcept
	: _value(rhs)
{
}
raw_type<type_e::DOUBLE>& raw_type<type_e::DOUBLE>::operator=(const raw_type& rhs) noexcept
{
	_value = rhs._value;
	return *this;
}
raw_type<type_e::DOUBLE>& raw_type<type_e::DOUBLE>::operator=(double rhs) noexcept
{
	_value = rhs;
	return *this;
}

raw_type<type_e::BOOL>::raw_type(bool rhs) noexcept
	: _value(rhs)
{
}
raw_type<type_e::BOOL>& raw_type<type_e::BOOL>::operator=(const raw_type& rhs) noexcept
{
	_value = rhs._value;
	return *this;
}
raw_type<type_e::BOOL>& raw_type<type_e::BOOL>::operator=(bool rhs) noexcept
{
	_value = rhs;
	return *this;
}

raw_type<type_e::STR>::raw_type(const std::string& rhs)
	: _value(rhs)
{
}
raw_type<type_e::STR>::raw_type(std::string&& rhs) noexcept
	: _value(std::move(rhs))
{
}
raw_type<type_e::STR>& raw_type<type_e::STR>::operator=(const raw_type& rhs)
{
	_value = rhs._value;
	return *this;
}
raw_type<type_e::STR>& raw_type<type_e::STR>::operator=(raw_type&& rhs) noexcept
{
	_value = std::move(rhs._value);
	return *this;
}
raw_type<type_e::STR>& raw_type<type_e::STR>::operator=(const std::string& rhs)
{
	_value = rhs;
	return *this;
}
raw_type<type_e::STR>& raw_type<type_e::STR>::operator=(std::string&& rhs) noexcept
{
	_value = std::move(rhs);
	return *this;
}

raw_type<type_e::ARR>::raw_type(const array& rhs)
	: _value(rhs)
{
}
raw_type<type_e::ARR>::raw_type(array&& rhs) noexcept
	: _value(std::move(rhs))
{
}
raw_type<type_e::ARR>& raw_type<type_e::ARR>::operator=(const raw_type& rhs)
{
	_value = rhs._value;
	return *this;
}
raw_type<type_e::ARR>& raw_type<type_e::ARR>::operator=(raw_type&& rhs) noexcept
{
	_value = std::move(rhs._value);
	return *this;
}
raw_type<type_e::ARR>& raw_type<type_e::ARR>::operator=(const array& rhs)
{
	_value = rhs;
	return *this;
}
raw_type<type_e::ARR>& raw_type<type_e::ARR>::operator=(array&& rhs) noexcept
{
	_value = std::move(rhs);
	return *this;
}

raw_type<type_e::OBJ>::raw_type(const object& rhs)
	: _value(rhs)
{
}
raw_type<type_e::OBJ>::raw_type(object&& rhs) noexcept
	: _value(std::move(rhs))
{
}
raw_type<type_e::OBJ>& raw_type<type_e::OBJ>::operator=(const raw_type& rhs)
{
	_value = rhs._value;
	return *this;
}
raw_type<type_e::OBJ>& raw_type<type_e::OBJ>::operator=(raw_type&& rhs) noexcept
{
	_value = std::move(rhs._value);
	return *this;
}
raw_type<type_e::OBJ>& raw_type<type_e::OBJ>::operator=(const object& rhs)
{
	_value = rhs;
	return *this;
}
raw_type<type_e::OBJ>& raw_type<type_e::OBJ>::operator=(object&& rhs) noexcept
{
	_value = std::move(rhs);
	return *this;
}

/*
 * JSON type container
*/
type_container::type_container() noexcept
	: _data(nullptr)
{
}

type_container::type_container(const type_container& rhs)
	: _data(nullptr)
{
	(*this) = rhs;
}

type_container::type_container(type_container&& rhs) noexcept
{
	_data = rhs._data;
	rhs._data = nullptr;
}

type_container::~type_container()
{
	if (_data != nullptr)
		_del();
}

type_container& type_container::operator=(const type_container& rhs)
{
	assert(this != &rhs);
	if (rhs._data != nullptr) {
		switch (rhs._data->_type) {
		case type_e::INT:
			return copy<type_e::INT>(reinterpret_cast<raw_type<type_e::INT>*>(rhs._data)->_value);
		case type_e::UINT:
			return copy<type_e::UINT>(reinterpret_cast<raw_type<type_e::UINT>*>(rhs._data)->_value);
		case type_e::DOUBLE:
			return copy<type_e::DOUBLE>(reinterpret_cast<raw_type<type_e::DOUBLE>*>(rhs._data)->_value);
		case type_e::BOOL:
			return copy<type_e::BOOL>(reinterpret_cast<raw_type<type_e::BOOL>*>(rhs._data)->_value);
		case type_e::STR:
			return copy<type_e::STR>(reinterpret_cast<raw_type<type_e::STR>*>(rhs._data)->_value);
		case type_e::ARR:
			return copy<type_e::ARR>(reinterpret_cast<raw_type<type_e::ARR>*>(rhs._data)->_value);
		case type_e::OBJ:
			return copy<type_e::OBJ>(reinterpret_cast<raw_type<type_e::OBJ>*>(rhs._data)->_value);
		case type_e::NUL:
			return copy<type_e::NUL>(reinterpret_cast<raw_type<type_e::NUL>*>(rhs._data)->_value);
		}
	}
	else {
		if (_data != nullptr)
			_del();
		_data = nullptr;
	}
	return *this;
}

type_container& type_container::operator=(type_container&& rhs) noexcept
{
	assert(this != &rhs);
	if (rhs._data != nullptr) {
		switch (rhs._data->_type) {
		case type_e::INT:
			return move<type_e::INT>(reinterpret_cast<raw_type<type_e::INT>*>(rhs._data)->_value);
		case type_e::UINT:
			return move<type_e::UINT>(reinterpret_cast<raw_type<type_e::UINT>*>(rhs._data)->_value);
		case type_e::DOUBLE:
			return move<type_e::DOUBLE>(reinterpret_cast<raw_type<type_e::DOUBLE>*>(rhs._data)->_value);
		case type_e::BOOL:
			return move<type_e::BOOL>(reinterpret_cast<raw_type<type_e::BOOL>*>(rhs._data)->_value);
		case type_e::STR:
			return move<type_e::STR>(reinterpret_cast<raw_type<type_e::STR>*>(rhs._data)->_value);
		case type_e::ARR:
			return move<type_e::ARR>(reinterpret_cast<raw_type<type_e::ARR>*>(rhs._data)->_value);
		case type_e::OBJ:
			return move<type_e::OBJ>(reinterpret_cast<raw_type<type_e::OBJ>*>(rhs._data)->_value);
		case type_e::NUL:
			return move<type_e::NUL>(reinterpret_cast<raw_type<type_e::NUL>*>(rhs._data)->_value);
		}
	}
	else {
		if (_data != nullptr)
			_del();
		_data = nullptr;
	}
	return *this;
}

long long& type_container::getInt()
{
	assert(_data != nullptr && _data->_type == type_e::INT);
	return reinterpret_cast<raw_type<type_e::INT>*>(_data)->_value;
}
const long long& type_container::getInt() const
{
	assert(_data != nullptr && _data->_type == type_e::INT);
	return reinterpret_cast<raw_type<type_e::INT>*>(_data)->_value;
}

unsigned long long& type_container::getUInt()
{
	assert(_data != nullptr && _data->_type == type_e::UINT);
	return reinterpret_cast<raw_type<type_e::UINT>*>(_data)->_value;
}
const unsigned long long& type_container::getUInt() const
{
	assert(_data != nullptr && _data->_type == type_e::UINT);
	return reinterpret_cast<raw_type<type_e::UINT>*>(_data)->_value;
}

double& type_container::getDouble()
{
	assert(_data != nullptr && _data->_type == type_e::DOUBLE);
	return reinterpret_cast<raw_type<type_e::DOUBLE>*>(_data)->_value;
}
const double& type_container::getDouble() const
{
	assert(_data != nullptr && _data->_type == type_e::DOUBLE);
	return reinterpret_cast<raw_type<type_e::DOUBLE>*>(_data)->_value;
}

std::string& type_container::getString()
{
	assert(_data != nullptr && _data->_type == type_e::STR);
	return reinterpret_cast<raw_type<type_e::STR>*>(_data)->_value;
}
const std::string& type_container::getString() const
{
	assert(_data != nullptr && _data->_type == type_e::STR);
	return reinterpret_cast<raw_type<type_e::STR>*>(_data)->_value;
}

array& type_container::getArray()
{
	assert(_data != nullptr && _data->_type == type_e::ARR);
	return reinterpret_cast<raw_type<type_e::ARR>*>(_data)->_value;
}
const array& type_container::getArray() const
{
	assert(_data != nullptr && _data->_type == type_e::ARR);
	return reinterpret_cast<raw_type<type_e::ARR>*>(_data)->_value;
}

object& type_container::getObject()
{
	assert(_data != nullptr && _data->_type == type_e::OBJ);
	return reinterpret_cast<raw_type<type_e::OBJ>*>(_data)->_value;
}
const object& type_container::getObject() const
{
	assert(_data != nullptr && _data->_type == type_e::OBJ);
	return reinterpret_cast<raw_type<type_e::OBJ>*>(_data)->_value;
}

std::nullptr_t& type_container::getNull()
{
	assert(_data != nullptr && _data->_type == type_e::NUL);
	return reinterpret_cast<raw_type<type_e::NUL>*>(_data)->_value;
}
const std::nullptr_t& type_container::getNull () const
{
	assert(_data != nullptr && _data->_type == type_e::NUL);
	return reinterpret_cast<raw_type<type_e::NUL>*>(_data)->_value;
}

type_e type_container::getType() const
{
	assert(_data != nullptr);
	return _data->_type;
}

std::string type_container::toString() const
{
	assert(_data != nullptr);
	switch (_data->_type) {
	case type_e::INT:
		return std::to_string(reinterpret_cast<raw_type<type_e::INT>*>(_data)->_value);
	case type_e::UINT:
		return std::to_string(reinterpret_cast<raw_type<type_e::UINT>*>(_data)->_value);
	case type_e::DOUBLE:
		return std::to_string(reinterpret_cast<raw_type<type_e::DOUBLE>*>(_data)->_value);
	case type_e::BOOL:
		return reinterpret_cast<raw_type<type_e::BOOL>*>(_data)->_value ? "true" : "false";
	case type_e::STR:
		return std::string("\"") + reinterpret_cast<raw_type<type_e::STR>*>(_data)->_value + "\"";
	case type_e::ARR:
		return reinterpret_cast<raw_type<type_e::ARR>*>(_data)->_value.toString();
	case type_e::OBJ:
		return reinterpret_cast<raw_type<type_e::OBJ>*>(_data)->_value.toString();
	case type_e::NUL:
		return "null";
	default:
		throw 1; // To keep the compiler happy, this path is impossible to reach
	}
}

std::string type_container::toHumanString(unsigned int level) const
{
	assert(_data != nullptr);
	if (_data->_type == type_e::ARR)
		return reinterpret_cast<raw_type<type_e::ARR>*>(_data)->_value.toHumanString(level);
	else if (_data->_type == type_e::OBJ)
		return reinterpret_cast<raw_type<type_e::OBJ>*>(_data)->_value.toHumanString(level);
	else
		return toString();
}

void type_container::_del()
{
	switch (_data->_type) {
	case type_e::STR:
		delete reinterpret_cast<raw_type<type_e::STR>*>(_data);
		break;
	case type_e::ARR:
		delete reinterpret_cast<raw_type<type_e::ARR>*>(_data);
		break;
	case type_e::OBJ:
		delete reinterpret_cast<raw_type<type_e::OBJ>*>(_data);
		break;
	default:
		delete _data; // Ony STR, ARR and OBJ have destructors
		break;
	};
}