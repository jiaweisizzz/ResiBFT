#ifndef SIGN_H
#define SIGN_H

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include "salticidae/stream.h"
#include "config.h"
#include "key.h"
#include "types.h"

class Sign
{
private:
	bool set;		  // True if [Sign] is not dummy sign
	ReplicaID signer; // The signer
	unsigned char signtext[SIGN_LEN];

public:
	void serialize(salticidae::DataStream &data) const;
	void unserialize(salticidae::DataStream &data);

	Sign();
	Sign(ReplicaID signer, char text);
	Sign(ReplicaID signer, unsigned char text[SIGN_LEN]);
	Sign(bool b, ReplicaID signer, unsigned char text[SIGN_LEN]);
	Sign(Key privateKey, ReplicaID signer, std::string text);

	unsigned char *getSigntext();
	ReplicaID getSigner();
	bool isSet();

	bool verify(Key publicKey, std::string text);

	std::string toPrint();
	std::string toString();

	bool operator==(const Sign &data) const;
	bool operator<(const Sign &data) const;
};

#endif
