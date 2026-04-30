#include <fstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "salticidae/event.h"
#include "salticidae/msg.h"
#include "salticidae/network.h"
#include "salticidae/stream.h"
#include "message.h"

const uint8_t MsgNewviewDamysus::opcode;
const uint8_t MsgLdrprepareDamysus::opcode;
const uint8_t MsgPrepareDamysus::opcode;
const uint8_t MsgPrecommitDamysus::opcode;
const uint8_t MsgNewviewHotstuff::opcode;
const uint8_t MsgPrepareHotstuff::opcode;
const uint8_t MsgLdrprepareHotstuff::opcode;
const uint8_t MsgPrecommitHotstuff::opcode;
const uint8_t MsgCommitHotstuff::opcode;
const uint8_t MsgNewviewHotsus::opcode;
const uint8_t MsgLdrprepareHotsus::opcode;
const uint8_t MsgPrepareHotsus::opcode;
const uint8_t MsgPrecommitHotsus::opcode;
const uint8_t MsgExnewviewHotsus::opcode;
const uint8_t MsgExldrprepareHotsus::opcode;
const uint8_t MsgExprepareHotsus::opcode;
const uint8_t MsgExprecommitHotsus::opcode;
const uint8_t MsgExcommitHotsus::opcode;
const uint8_t MsgTransaction::opcode;
const uint8_t MsgReply::opcode;
const uint8_t MsgStart::opcode;

using MessageNet = salticidae::MsgNetwork<uint8_t>;
using Clock = std::chrono::time_point<std::chrono::steady_clock>;
using TransactionInformation = std::tuple<unsigned int, Clock, Transaction>; // [int] represents the number of replies

salticidae::EventContext eventContext;
std::unique_ptr<MessageNet> messageNet;						  // Message network
std::map<ReplicaID, MessageNet::conn_t> connects;			  // Connections to the replicas
std::thread sendThread;										  // Thread to send messages
std::map<TransactionID, TransactionInformation> transactions; // Current transactions
std::map<TransactionID, double> executeTransactions;		  // Set of executed transaction ids

Clock startTime;
std::string statisticsClient;

ClientID clientId;
unsigned int numGeneralReplicas;
unsigned int numTrustedReplicas;
unsigned int numReplicas;
unsigned int numFaults;
unsigned int numClientTransactions;
unsigned int sleepTime;
unsigned int experimentIndex;
unsigned int quorumSize;

std::string printClientId()
{
	return ("[CLIENT" + std::to_string(clientId) + "]");
}

void sendMsgStart()
{
	Sign sign = Sign();
	MsgStart msgStart = MsgStart(clientId, sign);
	if (DEBUG_CLIENT)
	{
		std::cout << COLOUR_ORANGE << printClientId() << "Sending MsgStart" << COLOUR_NORMAL << std::endl;
	}
	for (std::map<ReplicaID, MessageNet::conn_t>::iterator it = connects.begin(); it != connects.end(); it++)
	{
		MessageNet::conn_t connect = it->second;
		messageNet->send_msg(msgStart, connect);
	}
}

unsigned int updateTransaction(TransactionID transactionId)
{
	std::map<TransactionID, TransactionInformation>::iterator it = transactions.find(transactionId);
	unsigned int numReplies = 0;
	if (it != transactions.end())
	{
		TransactionInformation transactionInformation = it->second;
		numReplies = std::get<0>(transactionInformation) + 1;
		transactions[transactionId] = std::make_tuple(numReplies, std::get<1>(transactionInformation), std::get<2>(transactionInformation));
	}
	return numReplies;
}

void executed(TransactionID transactionId)
{
	std::map<TransactionID, TransactionInformation>::iterator it = transactions.find(transactionId);
	if (it != transactions.end())
	{
		TransactionInformation transactionInformation = it->second;
		auto start = std::get<1>(transactionInformation);
		auto end = std::chrono::steady_clock::now();
		double time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
		executeTransactions[transactionId] = time;
	}
}

void printStatistics()
{
	auto end = std::chrono::steady_clock::now();
	double time = std::chrono::duration_cast<std::chrono::microseconds>(end - startTime).count();
	double secs = time / (1000 * 1000);
	double kops = (numClientTransactions * 1.0) / 1000;
	double throughput = kops / secs;
	if (DEBUG_CLIENT)
	{
		std::cout << COLOUR_ORANGE << printClientId() << "numClientTransactions = " << numClientTransactions << "; Kops = " << kops << "; secs = " << secs << COLOUR_NORMAL << std::endl;
	}
	// Gather all latencies in a list
	std::list<double> allLatencies;
	for (std::map<TransactionID, double>::iterator it = executeTransactions.begin(); it != executeTransactions.end(); ++it)
	{
		if (DEBUG_CLIENT)
		{
			std::cout << COLOUR_ORANGE << printClientId() << "transactionId = " << it->first << "; time = " << it->second << COLOUR_NORMAL << std::endl;
		}
		double ms = (it->second) / 1000; // Latency in milliseconds
		allLatencies.push_back(ms);
	}

	double avg = 0.0;
	for (std::list<double>::iterator it = allLatencies.begin(); it != allLatencies.end(); ++it)
	{
		avg += (double)*it;
	}
	double latency = avg / allLatencies.size(); // Average of milliseconds spent on a transaction
	if (DEBUG_CLIENT)
	{
		std::cout << COLOUR_ORANGE << printClientId() << "latency = " << latency << COLOUR_NORMAL << std::endl;
	}
	std::ofstream f(statisticsClient);
	f << std::to_string(throughput) << " " << std::to_string(latency);
	f.close();
	if (DEBUG_CLIENT)
	{
		std::cout << COLOUR_ORANGE << printClientId() << "before = " << executeTransactions.size() << "; after = " << allLatencies.size() << COLOUR_NORMAL << std::endl;
	}
}

void handleReply(MsgReply &&msgReply, const MessageNet::conn_t &conn)
{
	TransactionID transactionId = msgReply.reply;
	if (executeTransactions.find(transactionId) == executeTransactions.end())
	{
		// The transaction hasn't been executed yet
		unsigned int numReplies = updateTransaction(transactionId);
		if (DEBUG_CLIENT)
		{
			std::cout << COLOUR_ORANGE << printClientId() << "Received " << numReplies << "/" << quorumSize << " replies for transaction " << transactionId << COLOUR_NORMAL << std::endl;
		}
		if (numReplies == quorumSize)
		{
			if (DEBUG_CLIENT)
			{
				std::cout << COLOUR_ORANGE << printClientId() << "Received all " << numReplies << " replies for transaction " << transactionId << COLOUR_NORMAL << std::endl;
			}
			executed(transactionId);

			if (DEBUG_CLIENT)
			{
				std::cout << COLOUR_ORANGE << printClientId() << "Received:" << executeTransactions.size() << "/" << numClientTransactions << COLOUR_NORMAL << std::endl;
			}

			if (executeTransactions.size() == numClientTransactions)
			{
				if (DEBUG_CLIENT)
				{
					std::cout << COLOUR_ORANGE << printClientId() << "Received replies for all " << numClientTransactions << " transactions...stopping..." << COLOUR_NORMAL << std::endl;
				}
				printStatistics();
				// Once we have received all the replies we want, we stop by waiting for the sending thread to finish and stopping the eventContext
				sendThread.join();
				eventContext.stop();
			}
		}
	}
}

ReplicaID getFirstLeader()
{
#if defined(BASIC_HOTSTUFF)
	ReplicaID firstLeader = 0;
#elif defined(BASIC_DAMYSUS)
	ReplicaID firstLeader = 0;
#elif defined(BASIC_HOTSUS)
	ReplicaID firstLeader = numGeneralReplicas;
#endif
	return firstLeader;
}

MsgTransaction makeTransaction(TransactionID transactionId)
{
	Transaction transaction = Transaction(clientId, transactionId, 17);
	auto start = std::chrono::steady_clock::now();
	transactions[transaction.getTransactionId()] = std::make_tuple(0, start, transaction);
	Sign sign = Sign();
	MsgTransaction msgTransaction = MsgTransaction(transaction, sign);
	return msgTransaction;
}

void sendMsgTransaction()
{
	// The transaction id '0' is reserved for dummy transactions
	unsigned int transactionId = 1;
	while (transactionId <= numClientTransactions)
	{
		for (std::map<ReplicaID, MessageNet::conn_t>::iterator it = connects.begin(); it != connects.end(); it++)
		{
			ReplicaID replicaId = getFirstLeader();
			if (it->first == replicaId)
			{
				if (DEBUG_CLIENT)
				{
					std::cout << COLOUR_ORANGE << printClientId() << "Sending transaction[" << transactionId << "] to " << it->first << COLOUR_NORMAL << std::endl;
				}
				MsgTransaction msgTransaction = makeTransaction(transactionId);
				MessageNet::conn_t connect = it->second;
				messageNet->send_msg(msgTransaction, connect);
				usleep(sleepTime);
				if (DEBUG_CLIENT)
				{
					std::cout << COLOUR_ORANGE << printClientId() << "Slept for " << sleepTime << "; transactionId = " << transactionId << COLOUR_NORMAL << std::endl;
				}
				transactionId++;
				if (transactionId > numClientTransactions)
				{
					break;
				}
			}
		}
	}
}

void sendTransactionsThread()
{
	sendMsgTransaction();
}

void shutdownHandler(int a)
{
	eventContext.stop();
}

int main(int argc, char const *argv[])
{
	KeysFunctions keysFunctions;

	// Get inputs
	if (DEBUG_CLIENT)
	{
		std::cout << COLOUR_ORANGE << "Parsing client inputs" << COLOUR_NORMAL << std::endl;
	}

	if (argc > 1)
	{
		sscanf(argv[1], "%d", &clientId);
	}
	std::cout << COLOUR_ORANGE << printClientId() << "clientId = " << clientId << COLOUR_NORMAL << std::endl;

	if (argc > 2)
	{
		sscanf(argv[2], "%d", &numGeneralReplicas);
	}
	std::cout << COLOUR_ORANGE << printClientId() << "numGeneralReplicas = " << numGeneralReplicas << COLOUR_NORMAL << std::endl;

	if (argc > 3)
	{
		sscanf(argv[3], "%d", &numTrustedReplicas);
	}
	std::cout << COLOUR_ORANGE << printClientId() << "numTrustedReplicas = " << numTrustedReplicas << COLOUR_NORMAL << std::endl;

	if (argc > 4)
	{
		sscanf(argv[4], "%d", &numReplicas);
	}
	std::cout << COLOUR_ORANGE << printClientId() << "numReplicas = " << numReplicas << COLOUR_NORMAL << std::endl;

	if (argc > 5)
	{
		sscanf(argv[5], "%d", &numFaults);
	}
	std::cout << COLOUR_ORANGE << printClientId() << "numFaults = " << numFaults << COLOUR_NORMAL << std::endl;

	if (argc > 6)
	{
		sscanf(argv[6], "%d", &numClientTransactions);
	}
	std::cout << COLOUR_ORANGE << printClientId() << "numClientTransactions = " << numClientTransactions << COLOUR_NORMAL << std::endl;

	if (argc > 7)
	{
		sscanf(argv[7], "%d", &sleepTime);
	}
	std::cout << COLOUR_ORANGE << printClientId() << "sleepTime = " << sleepTime << COLOUR_NORMAL << std::endl;

	if (argc > 8)
	{
		sscanf(argv[8], "%d", &experimentIndex);
	}
	std::cout << COLOUR_ORANGE << printClientId() << "experimentIndex = " << experimentIndex << COLOUR_NORMAL << std::endl;

	quorumSize = numReplicas - numFaults;
	std::string configFile = "config";
	Nodes nodes(configFile, numReplicas);

	// Statistics
	auto timeNow = std::chrono::system_clock::now();
	std::time_t time = std::chrono::system_clock::to_time_t(timeNow);
	struct tm y2k = {0};
	double seconds = difftime(time, mktime(&y2k));
	std::string stamp = std::to_string(experimentIndex) + "-" + std::to_string(clientId) + "-" + std::to_string(seconds);
	statisticsClient = "results/clientvalues-" + stamp;

	// Public keys
	for (unsigned int i = 0; i < numReplicas; i++)
	{
		Key publicKey;
		BIO *bio = BIO_new(BIO_s_mem());
		int w = BIO_write(bio, pub_key256, sizeof(pub_key256));
		publicKey = PEM_read_bio_EC_PUBKEY(bio, NULL, NULL, NULL);
		if (DEBUG_CLIENT)
		{
			std::cout << COLOUR_ORANGE << printClientId() << "Replica id: " << i << COLOUR_NORMAL << std::endl;
		}
		nodes.setPublicKey(i, publicKey);
	}

	long unsigned int sizeBasic = std::max({sizeof(MsgTransaction), sizeof(MsgReply), sizeof(MsgStart)});
#if defined(BASIC_HOTSTUFF)
	long unsigned int sizeMessage = std::max({sizeof(MsgNewviewHotstuff), sizeof(MsgLdrprepareHotstuff), sizeof(MsgPrepareHotstuff), sizeof(MsgPrecommitHotstuff), sizeof(MsgCommitHotstuff)});
#elif defined(BASIC_DAMYSUS)
	long unsigned int sizeMessage = std::max({sizeof(MsgNewviewDamysus), sizeof(MsgLdrprepareDamysus), sizeof(MsgPrepareDamysus), sizeof(MsgPrecommitDamysus)});
#elif defined(BASIC_HOTSUS)
	long unsigned int sizeMessage = std::max({sizeof(MsgNewviewHotsus), sizeof(MsgLdrprepareHotsus), sizeof(MsgPrepareHotsus), sizeof(MsgPrecommitHotsus), sizeof(MsgExnewviewHotsus), sizeof(MsgExldrprepareHotsus), sizeof(MsgExprepareHotsus), sizeof(MsgExprecommitHotsus), sizeof(MsgExcommitHotsus)});
#endif
	long unsigned int size = std::max({sizeBasic, sizeMessage});

	MessageNet::Config messageNetConfig;
	messageNetConfig.max_msg_size(size);
	messageNet = std::make_unique<MessageNet>(eventContext, messageNetConfig);
	messageNet->start();

	std::cout << COLOUR_ORANGE << printClientId() << "Connecting..." << COLOUR_NORMAL << std::endl;
	for (size_t j = 0; j < numReplicas; j++)
	{
		Node *othernode = nodes.find(j);
		if (othernode != NULL)
		{
			std::cout << COLOUR_ORANGE << printClientId() << "Connecting to " << j << COLOUR_NORMAL << std::endl;
			salticidae::NetAddr peerAddress = salticidae::NetAddr(othernode->getHost() + ":" + std::to_string(othernode->getClientPort()));
			connects.insert(std::make_pair(j, messageNet->connect_sync(peerAddress)));
		}
		else
		{
			std::cout << COLOUR_ORANGE << printClientId() << "Couldn't find " << j << "'s information among nodes" << COLOUR_NORMAL << std::endl;
		}
	}

	messageNet->reg_handler(handleReply);

	sendMsgStart();
	sendThread = std::thread(sendTransactionsThread);

	salticidae::SigEvent eventSigTerm = salticidae::SigEvent(eventContext, shutdownHandler);
	eventSigTerm.add(SIGTERM);

	std::cout << COLOUR_ORANGE << printClientId() << "Dispatching" << COLOUR_NORMAL << std::endl;
	eventContext.dispatch();

	return 0;
}
