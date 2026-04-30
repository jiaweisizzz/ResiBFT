#ifndef DAMYSUS_H
#define DAMYSUS_H

#include <fstream>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <sys/socket.h>
#include "../Enclave/EnclaveUsertypes.h"
#include "salticidae/event.h"
#include "salticidae/msg.h"
#include "salticidae/network.h"
#include "salticidae/stream.h"
#include "sgx_urts.h"
#include "sgx_utils/sgx_utils.h"
#include "Enclave_u.h"
#include "Nodes.h"
#include "KeysFunctions.h"
#include "Log.h"
#include "Proposal.h"
#include "Statistics.h"
#include "Transaction.h"

using std::placeholders::_1;
using std::placeholders::_2;
using Peer = std::tuple<ReplicaID, salticidae::PeerId>;
using Peers = std::vector<Peer>;
using PeerNet = salticidae::PeerNetwork<uint8_t>;
using ClientNet = salticidae::ClientNetwork<uint8_t>;
using ClientInformation = std::tuple<bool, unsigned int, unsigned int, ClientNet::conn_t>; // [bool] is true if the client hasn't stopped, 1st [int] is the number of transactions received from clients, 2nd [int] is the number of transactions replied to
using Clients = std::map<ClientID, ClientInformation>;
using MessageNet = salticidae::MsgNetwork<uint8_t>;
using ExecutionQueue = salticidae::MPSCQueueEventDriven<std::pair<TransactionID, ClientID>>;
using Time = std::chrono::time_point<std::chrono::steady_clock>;

class Damysus
{
private:
	// Basic settings
	KeysFunctions keysFunction;
	ReplicaID replicaId;
	unsigned int numGeneralReplicas;
	unsigned int numTrustedReplicas;
	unsigned int numReplicas;
	unsigned int numViews;
	unsigned int numFaults;
	double leaderChangeTime;
	Nodes nodes;
	Key privateKey;
	unsigned int generalQuorumSize;
	unsigned int trustedQuorumSize;
	View view;

	// Message handlers
	salticidae::EventContext peerEventContext;	  // Peer event context
	salticidae::EventContext requestEventContext; // Request event context
	Peers peers;
	PeerNet peerNet;
	Clients clients;
	ClientNet clientNet;
	std::thread requestThread;
	salticidae::BoxObj<salticidae::ThreadCall> requestCall;
	salticidae::TimerEvent timer;
	ExecutionQueue executionQueue;

	// State variables
	std::list<Transaction> transactions; // Current transactions waiting to be processed
	std::map<View, Block> blocks;		 // Blocks received in each view
	std::mutex mutexTransaction;
	std::mutex mutexHandle;
	unsigned int viewsWithoutNewTrans = 0;
	bool started = false;
	bool stopped = false;
	View timerView; // View at which the timer was started
	Log log;

	// Print functions
	std::string printReplicaId();
	void printNowTime(std::string msg);
	void printClientInfo();
	std::string recipients2string(Peers recipients);

	// Setting functions
	unsigned int getLeaderOf(View view);
	unsigned int getCurrentLeader();
	bool amLeaderOf(View view);
	bool amCurrentLeader();
	Peers removeFromPeers(ReplicaID replicaId);
	Peers keepFromPeers(ReplicaID replicaId);
	std::vector<salticidae::PeerId> getPeerIds(Peers recipients);
	void setTimer();

	// Reply to clients
	void replyTransactions(Transaction *transactions);
	void replyHash(Hash hash);

	// Call TEE functions
	bool verifyJustificationDamysus(Justification justification);
	bool verifyProposalDamysus(Proposal<Accumulator> proposal, Signs signs);

	Justification initializeMsgNewviewDamysus();
	Accumulator initializeAccumulatorDamysus(Justification justifications_MsgNewview[MAX_NUM_SIGNATURES]);
	Signs initializeMsgLdrprepareDamysus(Proposal<Accumulator> proposal_MsgLdrprepare);
	Justification respondMsgLdrprepareProposalDamysus(Hash proposeHash, Accumulator accumulator_MsgLdrprepare);
	Justification saveMsgPrepareDamysus(Justification justification_MsgPrepare);

	Accumulator initializeAccumulator(std::set<MsgNewviewDamysus> msgNewviews);

	// Receive messages
	void receiveMsgStartDamysus(MsgStart msgStart, const ClientNet::conn_t &conn);
	void receiveMsgTransactionDamysus(MsgTransaction msgTransaction, const ClientNet::conn_t &conn);
	void receiveMsgNewviewDamysus(MsgNewviewDamysus msgNewview, const PeerNet::conn_t &conn);
	void receiveMsgLdrprepareDamysus(MsgLdrprepareDamysus msgLdrprepare, const PeerNet::conn_t &conn);
	void receiveMsgPrepareDamysus(MsgPrepareDamysus msgPrepare, const PeerNet::conn_t &conn);
	void receiveMsgPrecommitDamysus(MsgPrecommitDamysus msgPrecommit, const PeerNet::conn_t &conn);

	// Send messages
	void sendMsgNewviewDamysus(MsgNewviewDamysus msgNewview, Peers recipients);
	void sendMsgLdrprepareDamysus(MsgLdrprepareDamysus msgLdrprepare, Peers recipients);
	void sendMsgPrepareDamysus(MsgPrepareDamysus msg, Peers recipients);
	void sendMsgPrecommitDamysus(MsgPrecommitDamysus msg, Peers recipients);

	// Handle messages
	void handleMsgTransaction(MsgTransaction msgTransaction);
	void handleEarlierMessagesDamysus();								 // For replicas to process messages they have already received for in new view
	void handleMsgNewviewDamysus(MsgNewviewDamysus msgNewview);			 // Once the leader has received [msgNewview], it creates [msgLdrprepare] out of the highest prepared block
	void handleMsgLdrprepareDamysus(MsgLdrprepareDamysus msgLdrprepare); // Once the replicas have received [msgLdrprepare], it creates [msgPrepare] out of the proposal
	void handleMsgPrepareDamysus(MsgPrepareDamysus msgPrepare);			 // For both the leader and replicas process [msgPrepare]
	void handleMsgPrecommitDamysus(MsgPrecommitDamysus msgPrecommit);	 // For both the leader and replicas process [msgPrecommit]

	// Initiate messages
	void initiateMsgNewviewDamysus();									// Leader send [msgLdrprepare] to others and hold its own [msgPrepare]
	void initiateMsgPrepareDamysus(RoundData roundData_MsgPrepare);		// Leader send [msgPrepare] to others and hold its own [msgPrecommit]
	void initiateMsgPrecommitDamysus(RoundData roundData_MsgPrecommit); // Leader send [msgPrecommit] to others and execute the block

	// Respond messages
	void respondMsgLdrprepareDamysus(Accumulator accumulator_MsgLdrprepare, Block block); // Replicas respond to [msgLdrprepare] and send [msgPrepare] to the leader
	void respondMsgPrepareDamysus(Justification justification_MsgPrepare);				  // Replicas respond to [msgPrepare] and send [msgPrecommit] to the leader

	// Main functions
	int initializeSGX();
	void getStarted();
	void startNewViewDamysus();
	Block createNewBlockDamysus(Hash hash);
	void executeBlockDamysus(RoundData roundData_MsgPrecommit);
	bool timeToStop();
	void recordStatisticsDamysus();

public:
	Damysus(KeysFunctions keysFunctions, ReplicaID replicaId, unsigned int numGeneralReplicas, unsigned int numTrustedReplicas, unsigned int numReplicas, unsigned int numViews, unsigned int numFaults, double leaderChangeTime, Nodes nodes, Key privateKey, PeerNet::Config peerNetConfig, ClientNet::Config clientNetConfig);
};

#endif
