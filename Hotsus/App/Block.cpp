#include "Block.h"

std::string Block::transactions2string()
{
	std::string text = "";
	for (int i = 0; i < this->size; i++)
	{
		text += this->transactions[i].toString();
	}
	return text;
}

void Block::serialize(salticidae::DataStream &data) const
{
	data << this->set << this->previousHash << this->size;
	for (int i = 0; i < MAX_NUM_TRANSACTIONS; i++)
	{
		data << this->transactions[i];
	}
}

void Block::unserialize(salticidae::DataStream &data)
{
	data >> this->set >> this->previousHash >> this->size;
	for (int i = 0; i < MAX_NUM_TRANSACTIONS; i++)
	{
		data >> this->transactions[i];
	}
}

Block::Block()
{
	this->set = true;
	this->previousHash = Hash();
	this->size = 0;
}

Block::Block(bool b)
{
	this->set = b;
	this->previousHash = Hash();
	this->size = 0;
}

Block::Block(Hash previousHash, unsigned int size, Transaction transactions[MAX_NUM_TRANSACTIONS])
{
	this->set = true;
	this->previousHash = previousHash;
	this->size = size;
	for (int i = 0; i < MAX_NUM_TRANSACTIONS; i++)
	{
		this->transactions[i] = transactions[i];
	}
}

bool Block::isDummy()
{
	return !this->set;
}

bool Block::getSet()
{
	return this->set;
}

Hash Block::getPreviousHash()
{
	return this->previousHash;
}

unsigned int Block::getSize()
{
	return this->size;
}

Transaction *Block::getTransactions()
{
	return this->transactions;
}

Transaction Block::get(unsigned int n)
{
	return this->transactions[n];
}

bool Block::extends(Hash hash)
{
	return (this->previousHash == hash);
}

Hash Block::hash()
{
	unsigned char hash[SHA256_DIGEST_LENGTH];
	std::string text = this->toString();
	for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
	{
		hash[i] = '0';
	}
	if (!SHA256((const unsigned char *)text.c_str(), text.size(), hash))
	{
		std::cout << COLOUR_CYAN << "SHA256 failed" << COLOUR_NORMAL << std::endl;
	}
	return Hash(hash);
}

std::string Block::toPrint()
{
	std::string textBlock = "";
	for (int i = 0; i < MAX_NUM_TRANSACTIONS && i < this->size; i++)
	{
		textBlock += this->transactions[i].toPrint();
	}
	std::string text = "";
	text += "BLOCK[";
	text += std::to_string(this->set);
	text += ",";
	text += this->previousHash.toPrint();
	text += ",";
	text += std::to_string(this->size);
	text += ",{";
	text += textBlock;
	text += "}]";
	return text;
}

std::string Block::toString()
{
	std::string text = "";
	text += std::to_string(this->set);
	text += this->previousHash.toString();
	text += std::to_string(this->size);
	text += transactions2string();
	return text;
}

bool Block::operator==(const Block &data) const
{
	if (this->set != data.set || !(this->previousHash == data.previousHash) || this->size != data.size)
	{
		return false;
	}
	for (int i = 0; i < MAX_NUM_TRANSACTIONS && i < this->size; i++)
	{
		if (!(transactions[i] == data.transactions[i]))
		{
			return false;
		}
	}
	return true;
}
