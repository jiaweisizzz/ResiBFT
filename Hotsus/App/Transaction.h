#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "salticidae/stream.h"
#include "parameters.h"
#include "types.h"

class Transaction
{
private:
	ClientID clientId;
	TransactionID transactionId; // Transaction id 0 is reserved for dummy transaction
	unsigned char transactionData[PAYLOAD_SIZE];

public:
	void serialize(salticidae::DataStream &data) const;
	void unserialize(salticidae::DataStream &data);

	Transaction();
	Transaction(ClientID clientid, TransactionID transactionId);
	Transaction(ClientID clientid, TransactionID transactionId, char text);
	Transaction(salticidae::DataStream &data);

	ClientID getClientId();
	TransactionID getTransactionId();
	unsigned char *getTransactionData();

	std::string toPrint();
	std::string toString();

	bool operator==(const Transaction &data) const;
	bool operator<(const Transaction &data) const;
};

#endif
