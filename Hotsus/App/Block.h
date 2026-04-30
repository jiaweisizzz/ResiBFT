#ifndef BLOCK_H
#define BLOCK_H

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "salticidae/stream.h"
#include "config.h"
#include "types.h"
#include "Hash.h"
#include "Transaction.h"

class Block
{
private:
	bool set; // True if [Block] is not dummy block
	Hash previousHash;
	unsigned int size; // Number of non-dummy transactions
	Transaction transactions[MAX_NUM_TRANSACTIONS];

	std::string transactions2string();

public:
	void serialize(salticidae::DataStream &data) const;
	void unserialize(salticidae::DataStream &data);

	Block();																					 // Return the genesis block
	Block(bool b);																				 // Return the genesis block if [bool] is true or the dummy block if [bool] is false
	Block(Hash previousHash, unsigned int size, Transaction transactions[MAX_NUM_TRANSACTIONS]); // Create an extension of [Block]

	bool isDummy(); // True if the block is not set
	bool getSet();
	Hash getPreviousHash();
	unsigned int getSize();
	Transaction *getTransactions();
	Transaction get(unsigned int n);

	bool extends(Hash hash); // Check whether this block extends from [Hash]
	Hash hash();

	std::string toPrint();
	std::string toString();

	bool operator==(const Block &data) const;
};

#endif
