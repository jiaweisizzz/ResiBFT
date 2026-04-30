#include <fstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <thread>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <sys/socket.h>
#include <sys/select.h>
#include "message.h"
#include "KeysFunctions.h"
#include "Nodes.h"
#include "RoundData.h"
#include "Signs.h"
#if defined(BASIC_HOTSTUFF)
#include "Hotstuff.h"
#elif defined(BASIC_DAMYSUS)
#include "Damysus.h"
#elif defined(BASIC_HOTSUS)
#include "Hotsus.h"
#endif

unsigned int replicaId;
unsigned int numGeneralReplicas;
unsigned int numTrustedReplicas;
unsigned int numReplicas;
unsigned int numViews;
unsigned int numFaults;
double leaderChangeTime;

std::string printReplicaId()
{
	return ("[SERVER" + std::to_string(replicaId) + "]");
}

int main(int argc, char const *argv[])
{
	KeysFunctions keysFunctions;

	// Get inputs
	if (DEBUG_SERVER)
	{
		std::cout << COLOUR_ORANGE << "Parsing server inputs" << COLOUR_NORMAL << std::endl;
	}

	if (argc > 1)
	{
		sscanf(argv[1], "%d", &replicaId);
	}
	std::cout << COLOUR_ORANGE << printReplicaId() << "replicaId = " << replicaId << COLOUR_NORMAL << std::endl;

	if (argc > 2)
	{
		sscanf(argv[2], "%d", &numGeneralReplicas);
	}
	std::cout << COLOUR_ORANGE << printReplicaId() << "numGeneralReplicas = " << numGeneralReplicas << COLOUR_NORMAL << std::endl;

	if (argc > 3)
	{
		sscanf(argv[3], "%d", &numTrustedReplicas);
	}
	std::cout << COLOUR_ORANGE << printReplicaId() << "numTrustedReplicas = " << numTrustedReplicas << COLOUR_NORMAL << std::endl;

	if (argc > 4)
	{
		sscanf(argv[4], "%d", &numReplicas);
	}
	std::cout << COLOUR_ORANGE << printReplicaId() << "numReplicas = " << numReplicas << COLOUR_NORMAL << std::endl;

	if (argc > 5)
	{
		sscanf(argv[5], "%d", &numViews);
	}
	std::cout << COLOUR_ORANGE << printReplicaId() << "numViews = " << numViews << COLOUR_NORMAL << std::endl;

	if (argc > 6)
	{
		sscanf(argv[6], "%d", &numFaults);
	}
	std::cout << COLOUR_ORANGE << printReplicaId() << "numFaults = " << numFaults << COLOUR_NORMAL << std::endl;

	if (argc > 7)
	{
		sscanf(argv[7], "%lf", &leaderChangeTime);
	}
	std::cout << COLOUR_ORANGE << printReplicaId() << "leaderChangeTime = " << leaderChangeTime << COLOUR_NORMAL << std::endl;

	std::string configFile = "config";
	Nodes nodes(configFile, numReplicas);

	// Private key
	Key privateKey;
	BIO *bio = BIO_new(BIO_s_mem());
	int w = BIO_write(bio, priv_key256, sizeof(priv_key256));
	privateKey = PEM_read_bio_ECPrivateKey(bio, NULL, NULL, NULL);

	if (DEBUG_SERVER)
	{
		std::cout << COLOUR_ORANGE << printReplicaId() << "Checking private key" << COLOUR_NORMAL << std::endl;
	}
	if (!EC_KEY_check_key(privateKey))
	{
		std::cout << COLOUR_ORANGE << printReplicaId() << "Invalid private key" << COLOUR_NORMAL << std::endl;
	}
	if (DEBUG_SERVER)
	{
		std::cout << COLOUR_ORANGE << printReplicaId() << "Checked private key" << COLOUR_NORMAL << std::endl;
	}

	// Public keys
	for (unsigned int i = 0; i < numReplicas; i++)
	{
		Key publicKey;
		BIO *bio = BIO_new(BIO_s_mem());
		int w = BIO_write(bio, pub_key256, sizeof(pub_key256));
		publicKey = PEM_read_bio_EC_PUBKEY(bio, NULL, NULL, NULL);
		if (DEBUG_SERVER)
		{
			std::cout << COLOUR_ORANGE << printReplicaId() << "Replica id: " << i << COLOUR_NORMAL << std::endl;
		}
		nodes.setPublicKey(i, publicKey);
	}

	// Initialize handler
	if (DEBUG_SERVER)
	{
		std::cout << COLOUR_ORANGE << printReplicaId() << "Initializing handler" << COLOUR_NORMAL << std::endl;
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

	if (DEBUG_SERVER)
	{
		std::cout << COLOUR_ORANGE << printReplicaId() << "Max msg size = " << size << COLOUR_NORMAL << std::endl;
	}

	PeerNet::Config peerNetConfig;
	peerNetConfig.max_msg_size(2 * size);
	ClientNet::Config clientNetConfig;
	clientNetConfig.max_msg_size(2 * size);
	if (DEBUG_SERVER)
	{
		std::cout << COLOUR_ORANGE << printReplicaId() << "Starting handler" << COLOUR_NORMAL << std::endl;
	}

#if defined(BASIC_HOTSTUFF)
	Hotstuff hotstuff(keysFunctions, replicaId, numGeneralReplicas, numTrustedReplicas, numReplicas, numViews, numFaults, leaderChangeTime, nodes, privateKey, peerNetConfig, clientNetConfig);
#elif defined(BASIC_DAMYSUS)
	Damysus damysus(keysFunctions, replicaId, numGeneralReplicas, numTrustedReplicas, numReplicas, numViews, numFaults, leaderChangeTime, nodes, privateKey, peerNetConfig, clientNetConfig);
#elif defined(BASIC_HOTSUS)
	Hotsus hotsus(keysFunctions, replicaId, numGeneralReplicas, numTrustedReplicas, numReplicas, numViews, numFaults, leaderChangeTime, nodes, privateKey, peerNetConfig, clientNetConfig);
#endif

	return 0;
};
