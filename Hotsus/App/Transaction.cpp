#include "Transaction.h"

void Transaction::serialize(salticidae::DataStream &data) const
{
	data << this->clientId << this->transactionId;
	for (int i = 0; i < PAYLOAD_SIZE; i++)
	{
		data << this->transactionData[i];
	}
}

void Transaction::unserialize(salticidae::DataStream &data)
{
	data >> this->clientId >> this->transactionId;
	for (int i = 0; i < PAYLOAD_SIZE; i++)
	{
		data >> this->transactionData[i];
	}
}

Transaction::Transaction()
{
	this->clientId = 0;
	this->transactionId = 0;
	for (int i = 0; i < PAYLOAD_SIZE; i++)
	{
		this->transactionData[i] = '0';
	}
}

Transaction::Transaction(ClientID clientId, TransactionID transactionId)
{
	this->clientId = clientId;
	this->transactionId = transactionId;
	for (int i = 0; i < PAYLOAD_SIZE; i++)
	{
		this->transactionData[i] = '0';
	}
}

Transaction::Transaction(ClientID clientId, TransactionID transactionId, char text)
{
	this->clientId = clientId;
	this->transactionId = transactionId;
	for (int i = 0; i < PAYLOAD_SIZE; i++)
	{
		this->transactionData[i] = text;
	}
}

Transaction::Transaction(salticidae::DataStream &data)
{
	unserialize(data);
}

ClientID Transaction::getClientId()
{
	return this->clientId;
}

TransactionID Transaction::getTransactionId()
{
	return this->transactionId;
}

unsigned char *Transaction::getTransactionData()
{
	return this->transactionData;
}

std::string Transaction::toPrint()
{
	std::string text = "";
	text += "TRANSACTION[";
	text += std::to_string(this->clientId);
	text += ",";
	text += std::to_string(this->transactionId);
	text += "]";
	return text;
}

std::string Transaction::toString()
{
	std::string text = "";
	text += std::to_string(this->clientId);
	text += std::to_string(this->transactionId);
	for (int i = 0; i < PAYLOAD_SIZE; i++)
	{
		text += std::to_string((this->transactionData)[i]);
	}
	return text;
}

bool Transaction::operator==(const Transaction &data) const
{
	if (this->clientId != data.clientId || this->transactionId != data.transactionId)
	{
		return false;
	}
	for (int i = 0; i < PAYLOAD_SIZE; i++)
	{
		if (!(transactionData[i] == data.transactionData[i]))
		{
			return false;
		}
	}
	return true;
}

bool Transaction::operator<(const Transaction &data) const
{
	if (clientId < data.clientId || transactionId < data.transactionId || transactionData < data.transactionData)
	{
		return true;
	}
	else
	{
		return false;
	}
}
