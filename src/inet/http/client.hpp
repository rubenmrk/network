#pragma once

#include "../gconnection.hpp"
#include "types.hpp"

namespace inet::http2
{
    class client;
}

namespace inet::http
{
    class client
    {
        friend inet::http2::client;

    public:

        client() = delete;

        client(bool encryption = true);

        client(const client& rhs) = delete;

		client(client&& rhs) noexcept;

        client(std::string_view host, bool encryption = true);

        ~client();

		bool isconnected() const;

        // Changes the host.
        client& sethost(std::string_view host);

        // Enables/disables TLS, CHANGING THIS VALUE IS EXPENSIVE!
        client& setencryption(bool encryption);

        // Connects to the server.
        client& connect();

        // Disconnects from the server.
        client& disconnect();

        // Sends a HTTP command to the server, creates a suitable message in place, includes an optional body.
        client& send(method_e m, std::string_view r, const char *data = nullptr, unsigned int size = 0);

        // Sends a HTTP command to the server
        client& send(const message& m);

        // Retrieves the response from the HTTP server to the last message, will automatically close the connection
        // if the server sends "Connection: close".
        client& retrieve(response& r);

    private:

        std::string _host;

        bool _encryption;

        gconnection<char> *_con;

        std::vector<method_e> _rstack;
    };

	std::map<std::string, std::string> cookieParser(std::string_view str);
}