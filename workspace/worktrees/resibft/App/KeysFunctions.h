#ifndef KEYSFUNCTIONS_H
#define KEYSFUNCTIONS_H

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <openssl/bio.h>
#include <openssl/ec.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include "config.h"
#include "key.h"
#include "types.h"

class KeysFunctions
{
public:
	int loadPrivateKey(ReplicaID id, EC_key *privateKey);
	int loadPublicKey(ReplicaID id, EC_key *publicKey);
	void generateEc256Keys(int id);
};

#endif
