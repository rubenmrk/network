#include "websocket.hpp"
#include "tlsclient.hpp"
#include <cassert>
#include <memory>
#include <algorithm>

using namespace inet::websocket;

exception::exception(except_e ecode)
	: ecode(ecode)
{
}

const char *exception::what() const noexcept
{
	switch (ecode) {
	case except_e::OPEN_FAIL:
		return "Failed to connect to the server";
	case except_e::HANDSHAKE_FAIL:
		return "Failed websocket handshake";
	case except_e::UNKOWN_RSP:
		return "Unkown response from websocket";
	default:
		return "Unkown error occurred";
	}
}

client::client(bool encryption)
	: _encryption(encryption), _con(nullptr)
{
	if (encryption)
		_con = new tls::client;
	else
		_con = new tcp::client;
}

client::client(client&& rhs) noexcept
{
	_host = rhs._host;
	_encryption = rhs._encryption;
	_con = rhs._con;
	rhs._con = nullptr;
}

client::client(std::string_view host, bool encryption)
	: _host(host), _encryption(encryption), _con(nullptr)
{
	if (encryption)
		_con = new tls::client;
	else
		_con = new tcp::client;
}

client::~client()
{
	if (_con) {
		_con->close();
		delete _con;
	}
}

bool client::isconnected() const
{
	return _con->is_open();
}

client& client::sethost(std::string_view host)
{
	assert(!_con->is_open());
	_host = host;
	return *this;
}

client& client::setencryption(bool encryption)
{
	assert(!_con->is_open() && encryption != _encryption);
	delete _con;
	_encryption = encryption;
	if (encryption)
		_con = new tls::client;
	else
		_con = new tcp::client;
	return *this;
}

client& client::connect()
{
	assert(!_con->is_open());
	if (_encryption)
		_con->open(_host, "https");
	else
		_con->open(_host, "http");

	if (!_con->is_open())
		throw exception(except_e::OPEN_FAIL);
	_con->exceptions(std::ios_base::eofbit | std::ios_base::failbit | std::ios_base::badbit);
	_con->enable_timeout(5000);

	return *this;
}

client& client::connect(std::string_view resource)
{
	assert(!_con->is_open());
	if (_encryption)
		_con->open(_host, "https");
	else
		_con->open(_host, "http");

	if (!_con->is_open())
		throw exception(except_e::OPEN_FAIL);
	_con->exceptions(std::ios_base::eofbit | std::ios_base::failbit | std::ios_base::badbit);
	_con->enable_timeout(5000);

	// Send the upgrade request
	std::string upgrade = std::string("GET ").append(resource).append(" HTTP/1.1\r\nHost: ").append(_host).
		append("\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nSec-WebSocket-Version: 13\r\n\r\n");
	(*_con) << upgrade;
	_con->flush();

	// Check whether the server responds correctly
	std::string line;
	_con->getCRLF(line);
	if (line.length() < 13 || line.substr(9, 3) != "101")
		throw exception(except_e::HANDSHAKE_FAIL);

	// Ignore the header (it only contains things we already know or extensions which are not supported)
	while (line.size() != 0) {
		line.clear();
		_con->getCRLF(line);
	}

	return *this;
}

client& client::disconnect()
{
	if (_con->is_open()) {
		// Send a close frame before closing the connection (ignore the close response)
		uint8_t head[2] = { 0b1000'1000, 0b1000'0000 };
		_con->write(reinterpret_cast<char*>(head), 2);
		uint32_t mask = 0xDEADBEAF;
		_con->write(reinterpret_cast<char*>(&mask), 4);

		_con->flush();
		_con->close();
	}
	return *this;
}

client& client::send(const char *data, unsigned int size, bool text)
{
	// Write the header

	uint8_t head[2] = { static_cast<uint8_t>(text ? 0b1000'0001 : 0b1000'0010), 0b1000'0000 };
	uint16_t exlena = 0;
	uint64_t exlenb = 0;

	if (size < 126) {
		head[1] |= static_cast<uint8_t>(size);
	}
	else if (size < 65535) {
		head[1] |= static_cast<uint8_t>(126);
		exlena = hton(size);
	}
	else {
		head[1] |= static_cast<uint8_t>(127);
		exlenb = hton(size);
	}

	_con->write(reinterpret_cast<char*>(head), 2);
	if (exlena)
		_con->write(reinterpret_cast<char*>(&exlena), 4);
	else if (exlenb)
		_con->write(reinterpret_cast<char*>(&exlenb), 8);

	uint32_t mask = 0xDEADBEAF;
	_con->write(reinterpret_cast<char*>(&mask), 4);

	// Write the payload

	for (unsigned int i = 0; i < size; i += 4) {
		if (size - i < 4) {
			while (i < size) {
				_con->put(data[i] ^ reinterpret_cast<char*>(&mask)[i % 4]);
				++i;
			}
			break;
		}
		else {
			uint32_t buffer;
			std::copy(data+i, data+i+4, reinterpret_cast<char*>(&buffer));
			buffer ^= mask;
			_con->write(reinterpret_cast<char*>(&buffer), 4);
		}
	}

	// Make sure the data is actually send

	_con->flush();

	return *this;
}

client& client::retrieve(std::vector<char>& out, response_e& rtype)
{
	uint8_t head[2];
	unsigned int len;

	_con->read(reinterpret_cast<char*>(&head), 2);

	// Get the content type, opcode and mask flag
	bool cont = !(head[0] & 0b1000'0000);
	uint32_t mask = (head[1] & 0b1000'0000);
	if ((head[0] & 0b0000'1111) == 0x1)
		rtype = response_e::TEXT;
	else if ((head[0] & 0b0000'1111) == 0x2)
		rtype = response_e::BIN;
	else if ((head[0] & 0b0000'1111) == 0xA)
		rtype = response_e::PONG;
	else if ((head[0] & 0b0000'1111) == 0x8)
		rtype = response_e::CLOSE;
	else if ((head[0] & 0b0000'1111) != 0x0 || (head[0] & 0b0000'1111) != 0x9)
		throw exception(except_e::UNKOWN_RSP);

	// Get the content length
	if ((head[1] & 0b0111'1111) < 126)
		len = head[1] & 0b0111'1111;
	else if ((head[1] & 0b0111'1111) == 126) {
		uint16_t tmplen;
		_con->read(reinterpret_cast<char*>(&tmplen), 2);
		len = hton(tmplen);
	}
	else {
		uint64_t tmplen;
		_con->read(reinterpret_cast<char*>(&tmplen), 8);
		len = hton(tmplen);
	}

	// Grab the mask
	if (mask)
		_con->read(reinterpret_cast<char*>(&mask), 4);

	// Respond if the message is a ping, and process the next message
	if ((head[1] & 0b0000'1111) == 0x9) {
		pong(mask, len);
		retrieve(out, rtype);
		return *this;
	}

	// Read the data into the string

	auto offset = out.size();
	out.resize(offset + len);
	if (mask) {
		for (unsigned int i = 0; i < len; i += 4) {
			if (len-i < 4) {
				while (i < len) {
					out[offset + i] = static_cast<char>(_con->get()) ^ reinterpret_cast<char*>(&mask)[i % 4];
					++i;
				}
				break;
			}
			else {
				uint32_t buffer;
				_con->read(reinterpret_cast<char*>(&buffer), 4);
				buffer ^= mask;
				std::copy(reinterpret_cast<char*>(&buffer), reinterpret_cast<char*>(&buffer)+4, out.data()+offset+i);
			}
		}
	}
	else {
		_con->read(out.data()+offset, len);
	}

	// Repeat if more data is coming

	if (cont)
		retrieve(out, rtype);

	// Close the socket if a close message was sent

	if (rtype == response_e::CLOSE)
		_con->close();

	return *this;
}

client& client::ping(const char* data, unsigned int size)
{
	size = std::min(size, 125U);
	uint8_t head[2] = { 0b1000'1001, 0b1000'0000 };
	head[1] |= static_cast<uint8_t>(size);
	_con->write(reinterpret_cast<char*>(head), 2);

	uint32_t mask = 0xDEADBEAF;
	_con->write(reinterpret_cast<char*>(&mask), 4);

	for (unsigned int i = 0; i < size; i += 4) {
		if (size - i < 4) {
			while (i < size) {
				_con->put(data[i] ^ reinterpret_cast<char*>(&mask)[i % 4]);
				++i;
			}
			break;
		}
		else {
			uint32_t buffer;
			std::copy(data + i, data + i + 4, reinterpret_cast<char*>(&buffer));
			buffer ^= mask;
			_con->write(reinterpret_cast<char*>(&buffer), 4);
		}
	}

	_con->flush();
	
	return *this;
}

void client::pong(uint32_t mask, unsigned int size)
{
	std::unique_ptr<char> buffer(new char[size]);
	_con->read(buffer.get(), size);

	uint8_t head[2] = { 0b1000'1010, 0b1000'0000 };
	head[1] |= static_cast<uint8_t>(size);
	_con->write(reinterpret_cast<char*>(head), 2);

	if (mask == 0) {
		mask = 0xDEADBEAF;
		for (unsigned int i = 0; i < size; i += 4) {
			if (size - i < 4) {
				while (i < size) {
					_con->put(buffer.get()[i] ^ reinterpret_cast<char*>(&mask)[i % 4]);
					++i;
				}
				break;
			}
			else {
				uint32_t buffer2;
				std::copy(buffer.get() + i, buffer.get() + i + 4, reinterpret_cast<char*>(&buffer2));
				buffer2 ^= mask;
				_con->write(reinterpret_cast<char*>(&buffer2), 4);
			}
		}
	}
	else {
		_con->write(buffer.get(), size);
	}

	_con->flush();
}
