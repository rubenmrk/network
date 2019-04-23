#pragma once

#include <openssl/x509v3.h>

bool winload_default_roots(X509_STORE *store);