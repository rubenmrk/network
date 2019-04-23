#include "client.hpp"
#include "../tls/tlsclient.hpp"
#include <cassert>

using namespace inet::http;

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
	_host = std::move(rhs._host);
	_encryption = rhs._encryption;
	_con = rhs._con;
	_rstack = std::move(rhs._rstack);
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
    
    // Verify the connection and enable all exceptions
    if (!_con->is_open())
        throw exception(except_e::OPEN_FAIL);
    _con->exceptions(std::ios_base::eofbit | std::ios_base::failbit | std::ios_base::badbit);
	_con->enable_timeout(5000);
    return *this;
}

client& client::disconnect()
{
	if (_con->is_open()) {
		_rstack.clear();
		_con->close();
	}
    return *this;
}

client& client::send(method_e m, std::string_view r, const char *data, unsigned int size)
{
    message msg(m, _host);
    msg.resource(r);
	msg.body(data, size);
    return send(msg);
}

client& client::send(const message& m)
{
    assert(_con->is_open());

    // Create the intial command line
    std::string commandline = std::to_string(m.method()) + " " + std::string(m.resource()) + " " + "HTTP/1.1\r\n";
    (*_con) << commandline;

    // Add the headers
    for (auto pair : m) {
        (*_con) << pair.first << ": " << pair.second << "\r\n";
    }
    (*_con) << "\r\n";

    // Finally add the data and flush the buffer
	unsigned int size = 0;
	auto data = m.body(size);
    if (size > 0)
        _con->write(data, size);
    _con->flush();

    // Add the last send command to the stack
    _rstack.push_back(m.method());
    return *this;
}

client& client::retrieve(response& r)
{
    // Retrieve the corresponding request method
    assert(_rstack.size() > 0);
    auto m = _rstack.back();

	// Set an 8KB header soft limit
	_con->enable_read_limit(8*1024);

    // Grab and decode the status line (e.g. HTTP/1.1 200 OK)
    std::string str;
    _con->getCRLF(str);
    if (str[5] == '0' && str[7] == '9')
        r.version = version_e::HTTP09;
    else if (str[5] == '1' && str[7] == '0')
        r.version = version_e::HTTP10;
    else if (str[5] == '1' && str[7] == '1')
        r.version = version_e::HTTP11;
    else if (str[5] == '2' && str[7] == '0')
        r.version = version_e::HTTP20;
    else
        throw exception(except_e::UNKOWN_RSP);
    r.status = static_cast<status_e>(std::stoi(str.substr(9, 3)));
    r.reasonphrase = str.substr(13);

    // Store the headers
    while (true)
    {
        // Grab a line, an emtpy line indicates the end of the header
        str.clear();
        _con->getCRLF(str);
        if (str.size() == 0)
            break;
        
        // Store the header line (eg. "Connection: closed")
        r.header[str.substr(0, str.find(':'))] = str.substr(str.find(':')+2);
    }

	// Set a 50MB body soft limit (this client is not designed for large file transfer)
	_con->enable_read_limit(50*1024*1024);

    // Not an actual response to a request, so no body and the stack remains the same, also connection is guaranteed to remain open
	if (r.status == status_e::CONTINUE) {
		r.body.clear();
		return *this;
	}

    // HEAD's reply does not contain a body
    if (m != method_e::HEAD) {
		// The data is send in one piece
		if (r.header.count("Content-Length") == 1) {
			auto size = std::stoi(r.header["Content-Length"]);
			if (size > 0) {
				r.body.resize(size);
				_con->read(r.body.data(), size);
			}
			else {
				r.body.clear();
			}
		}
        // Chunked encoding is used
        else if (r.version == version_e::HTTP11 && r.header.count("Transfer-Encoding") == 1 && 
            r.header["Transfer-Encoding"].find("chunked") != std::string::npos) 
        {
            bool footer = false;
            while (true) {
                // Grab a line, if we are reading footers and the line is empty, the transmission is over
                str.clear();
                _con->getCRLF(str);
                if (footer && str.size() == 0)
                    break;

                // We are reading a single chunk (a chunk with length 0 indicates the beginning of the footers)
                if (!footer) {
                    auto chunksize = std::stoi(str, nullptr, 16);
                    if (chunksize == 0)
                        footer = true;
                    else {
                        auto oldsize = r.body.size();
                        r.body.resize(oldsize+chunksize);
                        _con->read(r.body.data()+oldsize, chunksize);
                        _con->ignore(2); // CRLF after the data
                    }
                }
                // We are reading a single footer
                else {
                    r.header[str.substr(0, str.find(':'))] = str.substr(str.find(':')+2);
                }
            }
        }
		else {	
			r.body.clear();
		}
    }
	else {
		r.body.clear();
	}

    // Pop the message stack and close the client->server connection if the server->client connection closes
    _rstack.pop_back();
    if (r.header.count("Connection") && r.header["Connection"].find("close") != std::string::npos)
        disconnect();
    return *this;
}

std::map<std::string, std::string> inet::http::cookieParser(std::string_view str)
{
	std::map<std::string, std::string> cookies;

	size_t begin = 0;
	while (true) {
		// Find the key
		size_t cur = str.find('=', begin);
		if (cur == str.npos)
			return cookies;
		auto key = str.substr(begin, (cur-begin));
		begin = ++cur;

		// Find the value
		cur = str.find(';', begin);
		cookies[std::string(key)] = str.substr(begin, (cur-begin));
		if (cur == str.npos)
			return cookies;
		else 
			begin = ++cur;
	}
}