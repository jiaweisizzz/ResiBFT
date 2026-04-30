#ifndef HASH_H
#define HASH_H

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <openssl/sha.h>
#include "salticidae/stream.h"

class Hash
{
private:
	bool set; // True if [Hash] is not dummy
	unsigned char hash[SHA256_DIGEST_LENGTH];

public:
	void serialize(salticidae::DataStream &data) const;
	void unserialize(salticidae::DataStream &data);

	Hash();
	Hash(bool b);
	Hash(unsigned char hash[SHA256_DIGEST_LENGTH]);
	Hash(bool b, unsigned char hash[SHA256_DIGEST_LENGTH]);
	Hash(salticidae::DataStream &data);

	bool getSet();
	unsigned char *getHash();

	bool isDummy(); // True if [Hash] is dummy hash
	bool isZero();

	std::string toPrint();
	std::string toString();

	bool operator==(const Hash &data) const;
};

#endif
