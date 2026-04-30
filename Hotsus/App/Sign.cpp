#include "Sign.h"

void Sign::serialize(salticidae::DataStream &data) const
{
	data << this->set << this->signer;
	for (int i = 0; i < SIGN_LEN; i++)
	{
		data << this->signtext[i];
	}
}

void Sign::unserialize(salticidae::DataStream &data)
{
	data >> this->set >> this->signer;
	for (int i = 0; i < SIGN_LEN; i++)
	{
		data >> this->signtext[i];
	}
}

Sign::Sign()
{
	this->set = false;
	for (int i = 0; i < SIGN_LEN; i++)
	{
		this->signtext[i] = '0';
	}
}

Sign::Sign(ReplicaID signer, char text)
{
	this->set = true;
	this->signer = signer;
	for (int i = 0; i < SIGN_LEN; i++)
	{
		this->signtext[i] = text;
	}
}

Sign::Sign(ReplicaID signer, unsigned char text[SIGN_LEN])
{
	this->set = true;
	this->signer = signer;
	for (int i = 0; i < SIGN_LEN; i++)
	{
		this->signtext[i] = text[i];
	}
}

Sign::Sign(bool b, ReplicaID signer, unsigned char text[SIGN_LEN])
{
	this->set = b;
	this->signer = signer;
	for (int i = 0; i < SIGN_LEN; i++)
	{
		this->signtext[i] = text[i];
	}
}

void signText(std::string text, EC_key privateKey, unsigned char sign[SIGN_LEN])
{
	unsigned char hash[SHA256_DIGEST_LENGTH];

	if (!SHA256((const unsigned char *)text.c_str(), text.size(), hash))
	{
		std::cout << COLOUR_CYAN << "SHA256 failed" << COLOUR_NORMAL << std::endl;
		exit(0);
	}
	unsigned int signLen = ECDSA_size(privateKey);
	unsigned char *buf = NULL;
	if ((buf = (unsigned char *)OPENSSL_malloc(signLen)) == NULL)
	{
		std::cout << COLOUR_CYAN << "Malloc failed" << COLOUR_NORMAL << std::endl;
		exit(0);
	}
	for (int k = 0; k < SHA256_DIGEST_LENGTH; k++)
	{
		buf[k] = '0';
	}

	if (!ECDSA_sign(NID_sha256, hash, SHA256_DIGEST_LENGTH, buf, &signLen, privateKey))
	{
		std::cout << COLOUR_CYAN << "ECDSA_sign failed" << COLOUR_NORMAL << std::endl;
		exit(0);
	}

	for (int k = 0; k < SHA256_DIGEST_LENGTH; k++)
	{
		sign[k] = buf[k];
	}
	OPENSSL_free(buf);
}

Sign::Sign(Key privateKey, ReplicaID signer, std::string text)
{
	this->set = true;
	this->signer = signer;
	signText(text, privateKey, this->signtext);
}

ReplicaID Sign::getSigner()
{
	return this->signer;
}

unsigned char *Sign::getSigntext()
{
	return this->signtext;
}

bool Sign::isSet()
{
	return this->set;
}

bool verifyText(std::string text, EC_key publicKey, unsigned char sign[SIGN_LEN])
{
	if (DEBUG_MODULES)
	{
		std::cout << COLOUR_CYAN << "Verifying text" << COLOUR_NORMAL << std::endl;
	}

	unsigned int signLen = SIGN_LEN;
	unsigned char hash[SHA256_DIGEST_LENGTH];
	if (!SHA256((const unsigned char *)text.c_str(), text.size(), hash))
	{
		std::cout << COLOUR_CYAN << "SHA256 failed" << COLOUR_NORMAL << std::endl;
		exit(0);
	}

	bool b = ECDSA_verify(NID_sha256, hash, SHA256_DIGEST_LENGTH, sign, signLen, publicKey);

	return b;
}

bool Sign::verify(Key publicKey, std::string text)
{
	bool b = verifyText(text, publicKey, this->signtext);
	return b;
}

std::string Sign::toPrint()
{
	std::string text = "";
	text += "SIGN[";
	text += std::to_string(this->set);
	text += ",";
	text += std::to_string(this->signer);
	text += "]";
	return text;
}

std::string Sign::toString()
{
	std::string text = "";
	text += std::to_string(this->set);
	text += std::to_string(this->signer);
	for (int i = 0; i < SIGN_LEN; i++)
	{
		text += this->signtext[i];
	}
	return text;
}

bool Sign::operator==(const Sign &data) const
{
	if (this->set != data.set || this->signer != data.signer)
	{
		return false;
	}
	for (int i = 0; i < SIGN_LEN; i++)
	{
		if (this->signtext[i] != data.signtext[i])
		{
			return false;
		}
	}
	return true;
}

bool Sign::operator<(const Sign &data) const
{
	if (signer < data.signer)
	{
		return true;
	}
	else
	{
		return false;
	}
}