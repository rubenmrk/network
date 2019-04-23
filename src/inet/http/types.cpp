#include "types.hpp"

using namespace inet::http;

exception::exception(except_e ecode)
    : ecode(ecode)
{
}

const char * exception::what() const noexcept
{
    switch (ecode) {
    case except_e::OPEN_FAIL:
        return "Failed to connect to the server";
    case except_e::UNKOWN_RSP:
	case except_e::DECODE_ERR:
        return "Invalid HTTP response";
    default:
        return "Unkown error occurred";
    }
}

message::message()
	: _body(nullptr), _bsize(0)
{
	(*this)["User-Agent"] = "cppclient/0.10";
}

message::message(std::string_view host)
	: _body(nullptr), _bsize(0)
{
	(*this)["Host"] = host;
	(*this)["User-Agent"] = "cppclient/0.10";
}

message::message(method_e m, std::string_view host)
	: _body(nullptr), _bsize(0)
{
    _method = m;
    _resource = "/";
    (*this)["Host"] = host;
	(*this)["User-Agent"] = "cppclient/0.10";
    (*this)["Connection"] = "close";
}

std::string_view message::host() const
{
	for (const auto& pair : (*this)) {
		if (pair.first == "Host")
			return pair.second;
	}
	return nullptr;
}

message& message::host(std::string_view h)
{
	(*this)["Host"] = std::string(h);
	return *this;
}

method_e message::method() const
{
    return _method;
}

message& message::method(method_e m)
{
    _method = m;
    return *this;
}

std::string_view message::resource() const
{
    return _resource;
}

message& message::resource(std::string_view r)
{
    _resource = r;
    return *this;
}

const char * message::body() const
{
	return _body;
}

const char * message::body(unsigned int& size) const
{
	size = _bsize;
	return _body;
}

message& message::body(const char *body, unsigned int size)
{
	_bsize = size;
	_body = body;
	(*this)["Content-Length"] = std::to_string(size);
	return *this;
}

message& message::body(std::string_view body)
{
	_body = body.data();
	_bsize = body.size();
	(*this)["Content-Length"] = std::to_string(_bsize);
	return *this;
}

message& message::clear()
{
	std::map<std::string, std::string>::clear();
	return clearbody();
}

message& message::clearbody()
{
	_body = nullptr;
	_bsize = 0;
	erase("Content-Length");
	return *this;
}

namespace std
{
	string to_string(method_e method)
	{
		switch (method) {
		case method_e::GET:
			return "GET";
		case method_e::HEAD:
			return "HEAD";
		case method_e::POST:
			return "POST";
		default:
			return string();
		}
	}

    string to_string(status_e status)
    {
        return to_string(underlying_type_t<status_e>(status));
    }

    string to_string(version_e version)
    {
        switch (version) {
        case (version_e::HTTP09):
            return "HTTP/0.9";
        case (version_e::HTTP10):
            return "HTTP/1.0";
        case (version_e::HTTP11):
            return "HTTP/1.1";
        case (version_e::HTTP20):
            return "HTTP/2.0";
        default:
			return string();
        }
    }
}