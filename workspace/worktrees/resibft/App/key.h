#ifndef KEY_H
#define KEY_H

#include <openssl/ec.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>

#define KK_EC256
#define EC_key EC_KEY *

#ifdef KK_EC256
#define SIGN_LEN 72
typedef EC_key Key;
#endif

const char priv_key256[] = {
	"-----BEGIN PRIVATE KEY-----\n"
	"MIGHAgEAMBMGByqGSM49AgEGCCqGSM49AwEHBG0wawIBAQQgnI0T6AoPs+ufh54e\n"
	"3tr6ywY7KkMBZhBs69NvMpvtXeehRANCAAS+G04ABpuwCvaS0v5fi9vuNOEitPon\n"
	"4nIDK/IJOsGXv85Jw5wayZI19lSB6ox05rLB+CxmEXrDyiOhX8Sz7c0L\n"
	"-----END PRIVATE KEY-----"};

const char pub_key256[] = {
	"-----BEGIN PUBLIC KEY-----\n"
	"MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEvhtOAAabsAr2ktL+X4vb7jThIrT6\n"
	"J+JyAyvyCTrBl7/OScOcGsmSNfZUgeqMdOaywfgsZhF6w8ojoV/Es+3NCw==\n"
	"-----END PUBLIC KEY-----"};

#endif
