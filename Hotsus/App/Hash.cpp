#include "Hash.h"

void Hash::serialize(salticidae::DataStream &data) const
{
	for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
	{
		data << this->hash[i];
	}
	data << this->set;
}

void Hash::unserialize(salticidae::DataStream &data)
{
	for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
	{
		data >> this->hash[i];
	}
	data >> this->set;
}

Hash::Hash()
{
	for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
	{
		this->hash[i] = '0';
	}
	this->set = true;
}

Hash::Hash(bool b)
{
	for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
	{
		this->hash[i] = '0';
	}
	this->set = b;
}

Hash::Hash(unsigned char hash[SHA256_DIGEST_LENGTH])
{
	memcpy(this->hash, hash, SHA256_DIGEST_LENGTH);
	this->set = true;
}

Hash::Hash(bool b, unsigned char hash[SHA256_DIGEST_LENGTH])
{
	memcpy(this->hash, hash, SHA256_DIGEST_LENGTH);
	this->set = b;
}

Hash::Hash(salticidae::DataStream &data)
{
	unserialize(data);
}

bool Hash::getSet()
{
	return this->set;
}

unsigned char *Hash::getHash()
{
	return this->hash;
}

bool Hash::isDummy()
{
	return !this->set;
}

bool Hash::isZero()
{
	for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
	{
		if (this->hash[i] != '0')
		{
			return false;
		}
	}
	return true;
}

std::string Hash::toPrint()
{
	std::string textHash = "";
	for (int i = 0; i < 6; i++)
	{
		textHash += std::to_string(int(this->hash[i]));
		if (i != 5)
		{
			textHash += " ";
		}
	}
	std::string text = "";
	text += "HASH[";
	text += std::to_string(this->set);
	text += "-";
	text += textHash;
	text += "]";
	return text;
}

std::string Hash::toString()
{
	std::string textHash = "";
	for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
	{
		textHash += this->hash[i];
	}
	std::string text = "";
	text += textHash;
	text += std::to_string(this->set);
	return text;
}

bool Hash::operator==(const Hash &data) const
{
	for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
	{
		if (hash[i] != data.hash[i])
		{
			return false;
		}
	}
	return true;
}
