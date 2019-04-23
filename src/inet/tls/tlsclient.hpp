#pragma once

#include "../tcp/tcpclient.hpp"
#include <openssl/ssl.h>

// If for some inane reason you don't want to use exception handling or want to use standard library exceptions
//#define INET_TLS_DISABLE_CUSTOM_EXCEPTION

namespace inet::tls
{
    enum class except_e { PRNG, TLS_VER, CIPHER, CERT_LOAD, CONNECT, SSL_STRUCT, SET_HOSTNAME, SSL_SOCK, HANDSHAKE, VERIFY, WRITE, READ };

#ifndef INET_TLS_DISABLE_CUSTOM_EXCEPTION
    class exception : public std::exception
    {
    public:

        exception(except_e ecode);

        const char * what() const noexcept override;

		const char * details() noexcept;

        const except_e ecode;
    };
#endif

    class streambuf : public tcp::streambuf
    {
    public:

        streambuf(SSL *ssl);

		streambuf();

    protected:

        void readfunc(const void * const t, size_t& res, char *begin, size_t len) override;

        void writefunc(const void * const t, size_t& res, char *begin, size_t len) override;
    };

    class client : public tcp::client
    {
    public:
    
        client();

        ~client();

        bool is_open() const noexcept override;

        void open(std::string_view node, std::string_view service) override;

        void open(std::string_view node, std::string_view service, const uint8_t *protocolList, unsigned int listSize);

        void close() override;

		const char * getprotocol() override;

    private:

        void _createsb() override;

        void _resetsb() override;

        void _connect(std::string_view node, std::string_view service, const uint8_t *protocolList, unsigned int listSize);

        void _disconnect();

        SSL* _ssl;
    };
}