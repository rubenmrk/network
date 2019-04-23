#pragma once

#include "gconnection.hpp"
#include <string_view>
#include <vector>

namespace inet::websocket
{
	enum class except_e { OPEN_FAIL, HANDSHAKE_FAIL, UNKOWN_RSP };

	class exception : public std::exception
	{
	public:

		exception(except_e ecode);

		const char* what() const noexcept override;

		const except_e ecode;
	};

	enum class response_e { TEXT, BIN, PONG, CLOSE };

	class client
	{
	public:

		client() = delete;

		client(bool encryption = true);

		client(const client& rhs) = delete;

		client(client&& rhs) noexcept;

		client(std::string_view host, bool encryption = true);

		~client();

		bool isconnected() const;

		client& sethost(std::string_view host);

		client& setencryption(bool encryption);

		// Connect without a handshake (you might try this after losing connection to the server)
		client& connect();

		// Connect with a handshake
		client& connect(std::string_view resource);

		client& send(const char *data, unsigned int size, bool text);

		client& retrieve(std::vector<char>& out, response_e& rtype);

		client& ping(const char *data, unsigned int size);

		client& disconnect();

	private:

		static const bool isbigendian()
		{
			uint16_t sval = 0xAABB;
			uint8_t aval[2] = { 0xAA, 0xBB };
			return (sval == *reinterpret_cast<uint16_t*>(aval));
		}

		template<typename T>
		T hton(T value)
		{
			if (sizeof(value) == 1 || isbigendian())
				return value;
			else {
				for (auto i = 0; i < sizeof(value) / 2; ++i)
					reinterpret_cast<uint8_t*>(&value)[i] = reinterpret_cast<uint8_t*>(&value)[sizeof(value)-i-1];
				return value;
			}
		}

		void pong(uint32_t mask, unsigned int size);

		std::string _host;

		bool _encryption;

		gconnection<char> *_con;
	};
}