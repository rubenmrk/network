#include "tlsclient.hpp"
#include "wincert.hpp"
#include <openssl/err.h>
#include <openssl/rand.h>

using namespace inet::tls;

/*
 * Functions that are used to intialize OpenSSL and managing an OpenSSL context
 */
static int openssl_stack = 0;

static SSL_CTX *ctx = nullptr;

#ifndef INET_TLS_DISABLE_CUSTOM_EXCEPTION
static void init_openssl()
#else
static bool init_openssl()
#endif
{
	auto error_handling = [](except_e except) {
#ifndef INET_TLS_DISABLE_CUSTOM_EXCEPTION
		throw exception(except);
#else
		return false;
#endif
	};

    static bool initialized = false;

    if (!initialized) {
        SSL_library_init();
        initialized = true;
    }
    if (openssl_stack == 0) {
        // Check OpenSSL PRNG
		if (RAND_status() != 1)
			return error_handling(except_e::PRNG);
        
        // Create OpenSSL context
        ctx = SSL_CTX_new(TLS_client_method());

        // Set the minimum TLS version
        if (SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION) != 1)
            return error_handling(except_e::TLS_VER);

        // Only enable strong ciphers
        const char *secure = "DHE-RSA-AES256-GCM-SHA384:DHE-RSA-AES128-GCM-SHA256:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-RSA-AES128-GCM-SHA256";
		if (SSL_CTX_set_cipher_list(ctx, secure) != 1)
			return error_handling(except_e::CIPHER);
		
		// Cipher suites for TLS 1.3
		if (SSL_CTX_set_ciphersuites(ctx, "TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256:TLS_AES_128_GCM_SHA256") != 1)
			return error_handling(except_e::CIPHER);

		// Load trusted root certificates
#ifndef WINDOWS
		// On linux the OS root certificates are usually linked inside the default_verify_path
		if (!SSL_CTX_set_default_verify_paths(ctx))
			return error_handling(except_e::CERT_LOAD);
#else
		// On windows the OS root certificates need to be retrieved through the windows crypto api
		auto store = X509_STORE_new(); // free'd by SSL_CTX_free
		if (!store || !winload_default_roots(store))
			return error_handling(except_e::CERT_LOAD);
		SSL_CTX_set_cert_store(ctx, store);
#endif
    }
    ++openssl_stack;
#ifdef INET_TLS_DISABLE_CUSTOM_EXCEPTION
    return true;
#endif
}

static void close_openssl()
{
    if (openssl_stack == 1) {
		SSL_CTX_free(ctx);
    }
	if (openssl_stack >= 1) {
		--openssl_stack;
	}
}

/*
 * Exception class
 */
#ifndef INET_TLS_DISABLE_CUSTOM_EXCEPTION
exception::exception(except_e ecode)
    : ecode(ecode)
{
}

const char * exception::what() const noexcept
{
    switch (ecode) {
    case (except_e::PRNG):
        return "PRNG not seeded with enough data";
    case (except_e::TLS_VER):
        return "Failed to set minimum TLS version";
    case (except_e::CIPHER):
        return "Failed to set cipher list";
    case (except_e::CERT_LOAD):
        return "Failed to load trusted root certificates";
    case (except_e::CONNECT):
        return "Failed to connect to server";
    case (except_e::SSL_STRUCT):
        return "Failed to create new SSL structure";
    case (except_e::SET_HOSTNAME):
        return "Failed to set the expected DNS host name";
    case (except_e::SSL_SOCK):
        return "Failed to link socket to SSL structure";
    case (except_e::HANDSHAKE):
        return "TLS handshake with server failed";
    case (except_e::VERIFY):
        return "Failed to verify server certificate";
    case (except_e::WRITE):
        return "Writing to over TLS failed";
    case (except_e::READ):
        return "Reading over TLS failed";
    default:
        return "An unkown OpenSSL error occured";    
    }
}

const char * exception::details() noexcept
{
	static std::string msg = "";

	msg.clear();
	unsigned long err;
	do {
		err = ERR_get_error();
		msg.append(ERR_error_string(err, nullptr)).append(", ");
	} while (err == SSL_ERROR_SYSCALL);
	msg.resize(msg.size()-2);
	return msg.c_str();
}
#endif

/*
 * Streambuf class
 */
streambuf::streambuf(SSL *ssl)
    : tcp::streambuf()
{
	reset(ssl);
}

streambuf::streambuf()
	: tcp::streambuf()
{
}

void streambuf::readfunc(const void * const t, size_t& res, char *begin, size_t len)
{
	// Check for time out and size limit
	auto ssl = *static_cast<SSL** const>(const_cast<void * const>(t));
	if (!checksocket(SSL_get_fd(ssl)))
		return;

    auto ret = SSL_read(ssl, begin, len);
    if (ret <= 0) {
#ifndef INET_TLS_DISABLE_CUSTOM_EXCEPTION
        throw exception(except_e::READ);
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
    auto ret = SSL_write(*static_cast<SSL** const>(const_cast<void * const>(t)), begin, len);
    if (ret < 0) {
#ifndef INET_TLS_DISABLE_CUSTOM_EXCEPTION
        throw exception(except_e::WRITE);
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
#ifndef INET_TLS_DISABLE_CUSTOM_EXCEPTION
    init_openssl();
#else
	if (!init_openssl())
		this->setstate(std::ios_base::badbit);
#endif
}

client::~client()
{
    _disconnect();
    close_openssl();
}

bool client::is_open() const noexcept
{
    return _connected;
}

void client::open(std::string_view node, std::string_view service)
{
    _disconnect();   
    _connect(node, service, nullptr, 0);
}

void client::open(std::string_view node, std::string_view service, const uint8_t *protocolList, unsigned int listSize)
{
    _disconnect();
    _connect(node, service, protocolList, listSize);
}

void client::close()
{
    _disconnect();
}

const char * client::getprotocol()
{
	if (_connected) {
		auto session = SSL_get_session(_ssl);
		if (session == nullptr)
			return "TLS";

		auto id = SSL_SESSION_get_protocol_version(session);
		if (id == TLS1_2_VERSION)
			return "TLS 1.2";
		else
			return "TLS 1.3";
	}
	else {
		return "Not connnected";
	}
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
    _sb->reset();
    this->clear();
}

void client::_connect(std::string_view node, std::string_view service, const uint8_t *protocolList, unsigned int listSize)
{
    auto cleanup = [this](except_e except) {
        SSL_free(_ssl);
        tcp::client::_disconnect();
#ifndef INET_TLS_DISABLE_CUSTOM_EXCEPTION
		throw (exception(except));
#else
		this->setstate(std::ios_base::badbit);
#endif
    };

    // Connect to the server
    tcp::client::_connect(node, service);
    if (!_connected) {
#ifndef INET_TLS_DISABLE_CUSTOM_EXCEPTION
        throw (exception(except_e::CONNECT));
#else
        this->setstate(std::ios_base::badbit);
        return;
#endif
    }

    // Create a new SSL structure
    _ssl = SSL_new(ctx);
    if (_ssl == nullptr)
        return cleanup(except_e::SSL_STRUCT);

	// Make sure the host doesn't contain any cheeky null characters and is not null
	if (node.length() != std::string(node).length() && std::string(node).length() > 0)
		return cleanup(except_e::SET_HOSTNAME);

    // Enable certificate validation
    SSL_set_hostflags(_ssl, X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);
	if (SSL_set1_host(_ssl, std::string(node).c_str()) != 1)
		return cleanup(except_e::SET_HOSTNAME);
    SSL_set_verify(_ssl, SSL_VERIFY_PEER, nullptr);

    // Make sure TLS stuff get automatically handled inside SSL_read, instead of having to call SSL_read twice
    SSL_set_mode(_ssl, SSL_MODE_AUTO_RETRY);

	// Enables SNI (not required by the standard, but some servers throw a fit if you don't enable it)
	if (SSL_set_tlsext_host_name(_ssl, std::string(node).c_str()) != 1)
		return cleanup(except_e::SET_HOSTNAME);

    // Enables ALPN (required for http/2.0)
    if (protocolList)
        SSL_set_alpn_protos(_ssl, protocolList, listSize);

    // Connect the SSL object with a file descriptor
	if (SSL_set_fd(_ssl, _socket) != 1)
		return cleanup(except_e::SSL_SOCK);

    // Do the handshake (this also verifies the certificate)
	if (SSL_connect(_ssl) != 1) 
		return cleanup(except_e::HANDSHAKE);

    // Update the streambuf
    _sb->reset(_ssl);
    this->clear();
}

void client::_disconnect()
{
    if (_connected) {
        int ret;
        do {
            ret = SSL_shutdown(_ssl);
        } while (ret == 0);
        SSL_free(_ssl);
        tcp::client::_disconnect();
    }
}