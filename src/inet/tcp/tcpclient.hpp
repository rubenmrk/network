#pragma once

#include <exception>
#include "../gconnection.hpp"

// If for some inane reason you don't want to use exception handling or want to use standard library exceptions
//#define INET_TCP_DISABLE_CUSTOM_EXCEPTION

namespace inet::tcp
{
#if (defined _WIN32 || defined WIN32)
	#define WINDOWS
	typedef uintptr_t socket_t;
#else
	typedef int socket_t;
#endif

#ifndef INET_TCP_DISABLE_CUSTOM_EXCEPTION
    class exception : public std::exception
    {
    public:

        exception();

        exception(int ecode, bool gai = true);

		~exception();

        const char * what() const noexcept override;

        const int ecode;

        const bool gai;

#ifdef WINDOWS
	private:

		void winerror();

		char *_msg;
#endif
    };
#endif

    class streambuf : public gstreambuf<char>
    {
    public:

        streambuf(int socket);

        streambuf();

        ~streambuf();

    protected:

		using typename gstreambuf<char>::milisecond;

		bool checksocket(socket_t s);

        void readfunc(const void * const t, size_t& res, char *begin, size_t len) override;

        void writefunc(const void * const t, size_t& res, char *begin, size_t len) override;
    };

    class client : public gconnection<char>
    {
    public:

		client();

        ~client();

        virtual bool is_open() const noexcept override;

        virtual void open(std::string_view node, std::string_view service) override;

        virtual void close() override;

		virtual const char * getprotocol() override;

    protected:

        virtual void _createsb();

        virtual void _resetsb();

        void _connect(std::string_view node, std::string_view service);

        void _disconnect();

		socket_t _socket;
    };
}