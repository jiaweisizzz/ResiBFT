#include "KeysFunctions.h"

std::string dir = "somekeys/";

int KeysFunctions::loadPrivateKey(ReplicaID id, EC_key *privateKey)
{
	if (DEBUG_MODULES)
	{
		std::cout << COLOUR_CYAN << "Loading private key" << COLOUR_NORMAL << std::endl;
	}
	std::string pr;
	pr = dir + "ec256_private" + std::to_string(id);
	FILE *fpr = fopen(pr.c_str(), "rb");
	if (fpr == NULL)
	{
		if (DEBUG_MODULES)
		{
			std::cout << COLOUR_CYAN << "Unable to open file " << pr << COLOUR_NORMAL << std::endl;
		}
		return 1;
	}
	*privateKey = PEM_read_ECPrivateKey(fpr, NULL, NULL, NULL);
	if (!EC_KEY_check_key(*privateKey))
	{
		std::cout << COLOUR_CYAN << "Invalid key" << COLOUR_NORMAL << std::endl;
	}
	fclose(fpr);
	if (DEBUG_MODULES)
	{
		std::cout << COLOUR_CYAN << "Loaded private key from " << pr << COLOUR_NORMAL << std::endl;
	}
	return 0;
}

int KeysFunctions::loadPublicKey(ReplicaID id, EC_key *publicKey)
{
	if (DEBUG_MODULES)
	{
		std::cout << COLOUR_CYAN << "Loading public key" << COLOUR_NORMAL << std::endl;
	}
	std::string pb;
	pb = dir + "ec256_public" + std::to_string(id);
	FILE *fpb = fopen(pb.c_str(), "rb");
	if (fpb == NULL)
	{
		if (DEBUG_MODULES)
		{
			std::cout << COLOUR_CYAN << "Unable to open file " << pb << COLOUR_NORMAL << std::endl;
		}
		return 1;
	}
	*publicKey = PEM_read_EC_PUBKEY(fpb, NULL, NULL, NULL);
	if (!EC_KEY_check_key(*publicKey))
	{
		std::cout << COLOUR_CYAN << "invalid key" << COLOUR_NORMAL << std::endl;
	}
	fclose(fpb);
	if (DEBUG_MODULES)
	{
		std::cout << COLOUR_CYAN << "loaded public key from " << pb << COLOUR_NORMAL << std::endl;
	}
	return 0;
}

// based on http://fm4dd.com/openssl/eckeycreate.htm
void KeysFunctions::generateEc256Keys(int id)
{
	BIO *outbio = NULL;
	EC_KEY *myecc = NULL;
	EVP_PKEY *pkey = NULL;
	int eccgrp;

	// These function calls initialize openssl for correct work
	OpenSSL_add_all_algorithms();
	ERR_load_BIO_strings();
	ERR_load_crypto_strings();

	// Create the Input/Output BIO's
	outbio = BIO_new(BIO_s_mem());

	// Create a EC key sructure, setting the group type from NID
	eccgrp = OBJ_txt2nid("prime256v1");
	myecc = EC_KEY_new_by_curve_name(eccgrp);

	// For cert signing, we use the [OPENSSL_EC_NAMED_CURVE] flag
	EC_KEY_set_asn1_flag(myecc, OPENSSL_EC_NAMED_CURVE);

	// Create the public/private EC key pair here
	if (!(EC_KEY_generate_key(myecc)))
	{
		std::cout << COLOUR_CYAN << "Error generating the ECC key" << COLOUR_NORMAL << std::endl;
	}

	// Convert the EC key into a PKEY structure let us handle the key just like any other key pair
	pkey = EVP_PKEY_new();
	if (!EVP_PKEY_assign_EC_KEY(pkey, myecc))
	{
		std::cout << COLOUR_CYAN << "Error assigning ECC key to EVP_PKEY structure" << COLOUR_NORMAL << std::endl;
	}

	// Now we show how to extract EC-specifics from the key
	myecc = EVP_PKEY_get1_EC_KEY(pkey);
	const EC_GROUP *ecgrp = EC_KEY_get0_group(myecc);

	// Here we print the key length, and extract the curve type
	std::cout << COLOUR_CYAN << "ECC Key size: " << EVP_PKEY_bits(pkey) << " bit" << COLOUR_NORMAL << std::endl;
	std::cout << COLOUR_CYAN << "ECC Key type: " << OBJ_nid2sn(EC_GROUP_get_curve_name(ecgrp)) << COLOUR_NORMAL << std::endl;

	// Here we print the private/public key data in PEM format
	// Private key
	if (!PEM_write_bio_PrivateKey(outbio, pkey, NULL, NULL, 0, 0, NULL))
	{
		std::cout << COLOUR_CYAN << "Error writing private key data in PEM format" << COLOUR_NORMAL << std::endl;
	}

	// BIO_pending function return number of byte read to bio buffer during previous step
	int private_len = BIO_pending(outbio);
	char *private_key = new char[private_len + 1];
	BIO_read(outbio, private_key, private_len);
	private_key[private_len] = 0;

	// Public key
	if (!PEM_write_bio_PUBKEY(outbio, pkey))
	{
		std::cout << COLOUR_CYAN << "Error writing public key data in PEM format" << COLOUR_NORMAL << std::endl;
	}

	// BIO_pending function return number of byte read to bio buffer during previous step
	int public_len = BIO_pending(outbio);
	char *public_key = new char[public_len + 1];
	BIO_read(outbio, public_key, public_len);
	public_key[public_len] = 0;

	// Print keys to files
	std::string pr = dir + "ec256_private" + std::to_string(id);
	std::cout << COLOUR_CYAN << "writing EC private key to " << pr << COLOUR_NORMAL << std::endl;
	std::ofstream privateKey;
	privateKey.open(pr);
	privateKey << private_key;
	privateKey.close();

	std::string pu = dir + "ec256_public" + std::to_string(id);
	std::cout << COLOUR_CYAN << "writing EC public key to " << pu << COLOUR_NORMAL << std::endl;
	std::ofstream publicKey;
	publicKey.open(pu);
	publicKey << public_key;
	publicKey.close();

	// Free up all structures
	EVP_PKEY_free(pkey);
	EC_KEY_free(myecc);
	BIO_free_all(outbio);
}
