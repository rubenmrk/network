#include "tcpclient.hpp"

#if (defined _WIN32 || defined WIN32)
#define WINDOWS
#include <WinSock2.h>
#include <WS2tcpip.h>
#include "../media/unicode.hpp"
#undef max
#else
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <poll.h>
#include <unistd.h>
#endif

#include <cassert>
#include <algorithm>

using namespace inet::tcp;

/*
 * Static functions that are used to intialize winsock
*/
#ifdef WINDOWS
static int winsock_stack = 0;
static WSADATA wsadata;

#ifndef INET_TCP_DISABLE_CUSTOM_EXCEPTION
static void init_winsock()
#else
static bool init_winsock()
#endif
{
	if (winsock_stack == 0) {
		auto ret = WSAStartup(MAKEWORD(2, 2), &wsadata);
		if (ret < 0) {
#ifndef INET_TCP_DISABLE_CUSTOM_EXCEPTION
			throw exception(ret, false);
#else
			return false;
#endif
		}
	}
	++winsock_stack;
#ifdef INET_TCP_DISABLE_CUSTOM_EXCEPTION
	return true;
#endif
}

static void close_winsock()
{
	if (winsock_stack == 1) {
		WSACleanup();
	}
	if (winsock_stack >= 1) {
		--winsock_stack;
	}
}
#endif

/*
 * Exception class
 */
#ifndef INET_TCP_DISABLE_CUSTOM_EXCEPTION
#ifndef WINDOWS
exception::exception()
    : ecode(errno), gai(false)
{
}

exception::exception(int ecode, bool gai)
    : ecode(ecode), gai(gai)
{
}

exception::~exception()
{
}

const char * exception::what() const noexcept
{
    if (!gai)
        return strerror(ecode);
    else
        return gai_strerror(ecode);
}
#else
exception::exception()
	: ecode(WSAGetLastError()), gai(false)
{
	winerror();
}

exception::exception(int ecode, bool gai)
	: ecode(ecode), gai(gai)
{
	if (!gai)
		winerror();
}

exception::~exception()
{
	delete[] _msg;
}

const char * exception::what() const noexcept
{
	if (!gai)
		return _msg;
	else
		return gai_strerror(ecode);
}

void exception::winerror()
{
	// Grab the UCS-2 string
	wchar_t *tmp = nullptr;
	auto len = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, ecode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&tmp, 0, NULL);
	
	// Allocate the ASCII string
	_msg = new char[len+1];
	
	// Convert UCS-2 to ASCII if only latin characters are used
	// (UTF-16 is compatible with UCS-2 for low values and unicode is compatible with ASCII)
	char *ptr = reinterpret_cast<char*>(tmp); int i;
	for (i = 0; i < len; ++i) {
		uint32_t point = 0;
		if (!unicode::utf16<false>::decode(point, ptr, reinterpret_cast<char*>(tmp)+len) || point > 127)
			break;
		ptr[i] = static_cast<char>(point);
	}
	ptr[i] = '\0';

	// Free the UCS-2 string
	LocalFree(tmp);
}
#endif
#endif

/*
 * Streambuf class
 */
streambuf::streambuf(int socket)
    : gstreambuf(socket)
{
}

streambuf::streambuf()
    : gstreambuf()
{
}

streambuf::~streambuf()
{
    try {
        this->pubsync();
    } catch (...) {}
}

bool streambuf::checksocket(socket_t s)
{
	if (_inlimit && _curread >= _maxread)
		return false;

	// Check for timeout
	pollfd pfd ={s, POLLIN, 0};
	int timeleft = std::max(_duration - std::chrono::duration_cast<milisecond>(std::chrono::high_resolution_clock::now()-_begin).count(), 0);
#ifndef WINDOWS
	auto ret = poll(&pfd, 1, _duration > 0 ? timeleft : -1);
#else
	auto ret = WSAPoll(&pfd, 1, _duration > 0 ? timeleft : -1);
#endif
	if (ret == 0)
		return false;
	if (ret < 0) {
#ifndef INET_TCP_DISABLE_CUSTOM_EXCEPTION
		throw exception();
#else
		return false;
#endif
	}
	
	return true;
}

void streambuf::readfunc(const void * const t, size_t& res, char *begin, size_t len)
{
	// Check for time out and size limit
	auto socket = *static_cast<const socket_t * const>(t);
	if (!checksocket(socket))
		return;

	// Read data
    auto ret = recv(socket, begin, len, 0);
	if (ret < 0) {
#ifndef INET_TCP_DISABLE_CUSTOM_EXCEPTION
		throw exception();
#else
		return;
#endif
	}
    res += ret;

	// Update limits
	_curread += ret;
	_begin = std::chrono::high_resolution_clock::now();
}

void streambuf::writefunc(const void * const t, size_t& res, char *begin, size_t len)
{
	assert(len <= INT32_MAX);
    auto ret = send(*static_cast<const socket_t * const>(t), begin, len, 0);
    if (ret < 0) {
#ifndef INET_TCP_DISABLE_CUSTOM_EXCEPTION
        throw exception();
#else
        return;
#endif
    }
    res += ret;
}

/*
 * Client class
 */
client::client()
{
#ifdef WINDOWS
#ifndef INET_TCP_DISABLE_CUSTOM_EXCEPTION
	init_winsock();
#else
	if (!init_winsock())
		this->setstate(std::ios_base::badbit);
#endif
#endif
}

client::~client()
{
    _disconnect();
#ifdef WINDOWS
	close_winsock();
#endif
}

bool client::is_open() const noexcept
{
    return _connected;
}

void client::open(std::string_view node, std::string_view service)
{
    _disconnect();   
    _connect(node, service);
}

void client::close()
{
    _disconnect();
}

const char * client::getprotocol()
{
	if (_connected)
		return "TCP";
	else
		return "Not connected";
}

void client::_createsb()
{
    // The base class deletes this value
    _sb = new streambuf();
    this->set_rdbuf(_sb);
    this->clear();
}

void client::_resetsb()
{
    if (_connected == false) {
        _sb->reset(_socket);
        this->clear();
    }
    else {
        _sb->reset();
        this->clear();
    }
}

void client::_connect(std::string_view node, std::string_view service)
{
    static bool sb_init = false;
    addrinfo hints = {}, *info, *p;

    // Initialize the stream buffer if that hasn't been done
    if (sb_init == false) {
        _createsb();
        sb_init = true;
    }

    // Find an appropriate socket
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    int ret = getaddrinfo(std::string(node).c_str(), std::string(service).c_str(), &hints, &info);
    if (ret < 0) {
#ifndef INET_TCP_DISABLE_CUSTOM_EXCEPTION
        throw exception(ret);
#else
        this->setstate(std::ios_base::badbit);
        return;
#endif
    }

    // Loop through possible settings and select the first one that works
    for (p = info; p != nullptr; p = p->ai_next) {
        _socket = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (_socket == -1) continue;

        ret = connect(_socket, p->ai_addr, p->ai_addrlen);
		if (ret < 0) {
#ifndef WINDOWS
			::close(_socket);
#else
			::closesocket(_socket);
#endif
		}
        else 
            break;
    }

    // Cleanup
    freeaddrinfo(info);
    if (p != nullptr) {
        _resetsb();
        _connected = true; // MUST COME AFTER _resetsb
    }
    else {
        this->setstate(std::ios_base::badbit);
    }
}

void client::_disconnect()
{
    if (_connected) {
#ifndef WINDOWS
		::close(_socket);
#else
		::closesocket(_socket);
#endif
        _resetsb();
        _connected = false; // MUST COME AFTER _resetsb
    }
}