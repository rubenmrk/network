#pragma once

#include <exception>
#include <map>
#include <vector>
#include <string>

namespace inet::http
{
    enum class except_e { OPEN_FAIL, UNKOWN_RSP, DECODE_ERR };

    class exception : public std::exception
    {
    public:

        exception(except_e ecode);

        const char * what() const noexcept override;

        const except_e ecode;
    };

    enum class method_e { GET, HEAD, POST };

    class message : public std::map<std::string, std::string>
    {
    public:

		message();
        message(std::string_view host);
        message(method_e m, std::string_view host);

		std::string_view host() const;
		message& host(std::string_view h);

        method_e method() const;
        message& method(method_e m);

        std::string_view resource() const;
        message& resource(std::string_view r);

        const char * body() const;
		const char * body(unsigned int& size) const;
        message& body(const char *body, unsigned int size);
		message& body(std::string_view body);

		message& clear();
		message& clearbody();

    private:

        method_e _method;

        std::string _resource;

		const char *_body;

		unsigned int _bsize;
    };

    enum class version_e { HTTP09, HTTP10, HTTP11, HTTP20 };

    enum class status_e { 
		CONTINUE = 100, SWITCH_PROTOCOL = 101, PROCESSING = 102, EARLY_HINTS = 103, OK = 200, CREATED = 201, ACCEPTED = 202, NON_AUTHORATIVE_INFORMATION = 203, 
		NO_CONTENT = 204, RESET_CONTENT = 205, PARTIAL_CONTENT = 206, MULTI_STATUS = 207, ALREADY_REPORTED = 208, IM_USED = 226, MULTIPLE_CHOICES = 300, 
		MOVED_PERMANENTLY = 301, MOVED_TEMPORARILY = 302, SEE_OTHER = 303, NOT_MODIFIED = 304, USE_PROXY = 305, SWITCH_PROXY = 306, TEMPORARY_REDIRECT = 307, 
		PERMANENT_REDIRECT = 308, BAD_REQUEST = 400, UNAUTHORIZED = 401, PAYMENT_REQUIRED = 402, FORBIDDEN = 403, NOT_FOUND = 404, METHOD_NOT_ALLOWED = 405, 
		NOT_ACCEPTABLE = 406, PROXY_AUTHENTICATION_REQUIRED = 407, REQUEST_TIMEOUT = 408, CONFLICT = 409, GONE = 410, LENGTH_REQUIRED = 411, 
		PRECONDITION_FAILED = 412, PAYLOAD_TOO_LARGE = 413, URI_TO_LONG = 414, UNSUPPORTED_MEDIA_TYPE = 415, RANGE_NOT_SATISFIABLE = 416, EXPECTATION_FAILED = 417, 
		IM_A_TEAPOT = 418, MISDIRECT_REQUEST = 421, UNPROCESSABLE_ENTITY = 422, LOCKED = 423, FAILED_DEPENDENCY = 424, UPGRADE_REQUIRED = 426, 
		PRECONDITION_REQUIRED = 428, TOO_MANY_REQUESTS = 429, REQUEST_HEADER_FIELDS_TOO_LARGE = 431, UNAVAILABLE_FOR_LEGAL_REASONS = 451 
	};

    struct response
    {
        version_e version;

        status_e status;

        std::string reasonphrase;

        std::map<std::string, std::string> header;

        std::vector<char> body;
    };
}

namespace std
{
	string to_string(inet::http::method_e method);

    string to_string(inet::http::status_e status);

    string to_string(inet::http::version_e version);
}