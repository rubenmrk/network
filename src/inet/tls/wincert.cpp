#include "wincert.hpp"
#define WINDOWS_LEAN_AND_MEAN
#include <Windows.h>
#include <wincrypt.h>

bool winload_default_roots(X509_STORE *store)
{
	// Grab the windows trusted root certificate store
	auto winstore = CertOpenSystemStore(0, "ROOT");
	if (!winstore)
		return false;

	// Load the trusted root certificates into the OpenSSL store
	PCCERT_CONTEXT context = nullptr;
	while ((context = CertEnumCertificatesInStore(winstore, context))) {
		auto cert = d2i_X509(nullptr, (const unsigned char**)&context->pbCertEncoded, context->cbCertEncoded);
		if (cert) {
			X509_STORE_add_cert(store, cert);
			X509_free(cert);
		}
	}

	// Release the windows store and certificates context
	CertFreeCertificateContext(context);
	CertCloseStore(winstore, 0);
	return true;
}