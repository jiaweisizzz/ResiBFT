#ifndef HOTSUS_H
#define HOTSUS_H

#include <fstream>
#include <functional>
#include <iostream>
#include <list>
#include <math.h>
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
#include "Group.h"
#include "Nodes.h"
#include "KeysFunctions.h"
#include "Log.h"
#include "Proposal.h"
#include "HotsusBasic.h"
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

class Hotsus
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
	unsigned int lowTrustedSize;
	Group trustedGroup;
	View view;
	Protocol protocol;

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
	std::vector<ReplicaID> getGeneralReplicaIds();
	bool amGeneralReplicaIds();
	bool isGeneralReplicaIds(ReplicaID replicaId);
	bool amTrustedReplicaIds();
	bool isTrustedReplicaIds(ReplicaID replicaId);
	Peers removeFromPeers(ReplicaID replicaId);
	Peers removeFromPeers(std::vector<ReplicaID> generalReplicaIds);
	Peers removeFromTrustedPeers(ReplicaID replicaId);
	Peers keepFromPeers(ReplicaID replicaId);
	Peers keepFromTrustedPeers(ReplicaID replicaId);
	std::vector<salticidae::PeerId> getPeerIds(Peers recipients);
	void setTimer();
	void changeSwitcher();
	void changeAuthenticator();
	void setTrustedQuorumSize();

	// Reply to clients
	void replyTransactions(Transaction *transactions);
	void replyHash(Hash hash);

	// Call TEE functions
	bool verifyJustificationHotsus(Justification justification);
	bool verifyProposalHotsus(Proposal<Accumulator> proposal, Signs signs);
	bool verifyExproposalHotsus(Proposal<Justification> exproposal, Signs signs);

	Justification initializeMsgNewviewHotsus();
	Accumulator initializeAccumulatorHotsus(Justification justifications_MsgNewview[MAX_NUM_SIGNATURES]);
	Signs initializeMsgLdrprepareHotsus(Proposal<Accumulator> proposal_MsgLdrprepare);
	Justification respondMsgLdrprepareProposalHotsus(Hash proposeHash, Accumulator accumulator_MsgLdrprepare);
	Justification saveMsgPrepareHotsus(Justification justification_MsgPrepare);
	void skipRoundHotsus();
	Justification initializeMsgExnewviewHotsus();
	Justification respondMsgExnewviewProposalHotsus(Hash proposeHash, Justification justification_MsgExnewview);
	Signs initializeMsgExldrprepareHotsus(Proposal<Justification> proposal_MsgExldrprepare);
	Justification respondMsgExldrprepareProposalHotsus(Hash proposeHash, Justification justification_MsgExnewview);
	Justification saveMsgExprepareHotsus(Justification justification_MsgExprepare);
	Justification lockMsgExprecommitHotsus(Justification justification_MsgExprecommit);

	Accumulator initializeAccumulator(std::set<MsgNewviewHotsus> msgNewviews);

	// Receive messages
	void receiveMsgStartHotsus(MsgStart msgStart, const ClientNet::conn_t &conn);
	void receiveMsgTransactionHotsus(MsgTransaction msgTransaction, const ClientNet::conn_t &conn);
	void receiveMsgNewviewHotsus(MsgNewviewHotsus msgNewview, const PeerNet::conn_t &conn);
	void receiveMsgLdrprepareHotsus(MsgLdrprepareHotsus msgLdrprepare, const PeerNet::conn_t &conn);
	void receiveMsgPrepareHotsus(MsgPrepareHotsus msgPrepare, const PeerNet::conn_t &conn);
	void receiveMsgPrecommitHotsus(MsgPrecommitHotsus msgPrecommit, const PeerNet::conn_t &conn);
	void receiveMsgExnewviewHotsus(MsgExnewviewHotsus msgExnewview, const PeerNet::conn_t &conn);
	void receiveMsgExldrprepareHotsus(MsgExldrprepareHotsus msgExldrprepare, const PeerNet::conn_t &conn);
	void receiveMsgExprepareHotsus(MsgExprepareHotsus msgExprepare, const PeerNet::conn_t &conn);
	void receiveMsgExprecommitHotsus(MsgExprecommitHotsus msgExprecommit, const PeerNet::conn_t &conn);
	void receiveMsgExcommitHotsus(MsgExcommitHotsus msgExcommit, const PeerNet::conn_t &conn);

	// Send messages
	void sendMsgNewviewHotsus(MsgNewviewHotsus msgNewview, Peers recipients);
	void sendMsgLdrprepareHotsus(MsgLdrprepareHotsus msgLdrprepare, Peers recipients);
	void sendMsgPrepareHotsus(MsgPrepareHotsus msgPrepare, Peers recipients);
	void sendMsgPrecommitHotsus(MsgPrecommitHotsus msgPrecommit, Peers recipients);
	void sendMsgExnewviewHotsus(MsgExnewviewHotsus msgExnewview, Peers recipients);
	void sendMsgExldrprepareHotsus(MsgExldrprepareHotsus msgExldrprepare, Peers recipients);
	void sendMsgExprepareHotsus(MsgExprepareHotsus msgExprepare, Peers recipients);
	void sendMsgExprecommitHotsus(MsgExprecommitHotsus msgExprecommit, Peers recipients);
	void sendMsgExcommitHotsus(MsgExcommitHotsus msgExcommit, Peers recipients);

	// Handle messages
	void handleMsgTransaction(MsgTransaction msgTransaction);
	void handleEarlierMessagesHotsus();										 // For replicas to process messages they have already received for in new view
	void handleExtraEarlierMessagesHotsus();								 // For replicas to process messages they have already received for in new view
	void handleMsgNewviewHotsus(MsgNewviewHotsus msgNewview);				 // Once the leader has received [msgNewview], it creates [msgLdrprepare] out of the highest prepared block
	void handleMsgLdrprepareHotsus(MsgLdrprepareHotsus msgLdrprepare);		 // Once the replicas have received [msgLdrprepare], it creates [msgPrepare] out of the proposal
	void handleMsgPrepareHotsus(MsgPrepareHotsus msgPrepare);				 // For both the leader and replicas process [msgPrepare]
	void handleMsgPrecommitHotsus(MsgPrecommitHotsus msgPrecommit);			 // For both the leader and replicas process [msgPrecommit]
	void handleMsgExnewviewHotsus(MsgExnewviewHotsus msgExnewview);			 // Once the leader has received [msgExnewview], it creates [msgExldrprepare] out of the highest prepared block
	void handleMsgExldrprepareHotsus(MsgExldrprepareHotsus msgExldrprepare); // Once the replicas have received [msgExldrprepare], it creates [msgExprepare] out of the proposal
	void handleMsgExprepareHotsus(MsgExprepareHotsus msgExprepare);			 // For both the leader and replicas process [msgExprepare]
	void handleMsgExprecommitHotsus(MsgExprecommitHotsus msgExprecommit);	 // For both the leader and replicas process [msgExprecommit]
	void handleMsgExcommitHotsus(MsgExcommitHotsus msgExcommit);			 // For both the leader and replicas process [msgExcommit]

	// Initiate messages
	void initiateMsgNewviewHotsus();									   // Leader send [msgLdrprepare] to others and hold its own [msgPrepare]
	void initiateMsgPrepareHotsus(RoundData roundData_MsgPrepare);		   // Leader send [msgPrepare] to others and hold its own [msgPrecommit]
	void initiateMsgPrecommitHotsus(RoundData roundData_MsgPrecommit);	   // Leader send [msgPrecommit] to others and execute the block
	void initiateMsgExnewviewHotsus();									   // Leader send [msgExldrprepare] to others and hold its own [msgExprepare]
	void initiateMsgExprepareHotsus(RoundData roundData_MsgExprepare);	   // Leader send [msgExprepare] to others and hold its own [msgExprecommit]
	void initiateMsgExprecommitHotsus(RoundData roundData_MsgExprecommit); // Leader send [msgExprecommit] to others and hold its own [msgExcommit]
	void initiateMsgExcommitHotsus(RoundData roundData_MsgExcommit);	   // Leader send [msgExcommit] to others and execute the block

	// Respond messages
	void respondMsgLdrprepareHotsus(Accumulator accumulator_MsgLdrprepare, Block block);	  // Replicas respond to [msgLdrprepare] and send [msgPrepare] to the leader
	void respondMsgPrepareHotsus(Justification justification_MsgPrepare);					  // Replicas respond to [msgPrepare] and send [msgPrecommit] to the leader
	void respondMsgExldrprepareHotsus(Justification justification_MsgExnewview, Block block); // Replicas respond to [msgExldrprepare] and send [msgExprepare] to the leader
	void respondMsgExprepareHotsus(Justification justification_MsgExprepare);				  // Replicas respond to [msgExprepare] and send [msgExprecommit] to the leader
	void respondMsgExprecommitHotsus(Justification justification_MsgExprecommit);			  // Replicas respond to [msgExprecommit] and send [msgExcommit] to the leader

	// Main functions
	int initializeSGX();
	void getStarted();
	void getExtraStarted();
	void startNewViewHotsus();
	Block createNewBlockHotsus(Hash hash);
	void executeBlockHotsus(RoundData roundData_MsgPrecommit);
	void executeExtraBlockHotsus(RoundData roundData_MsgExcommit);
	bool timeToStop();
	void recordStatisticsHotsus();

public:
	Hotsus(KeysFunctions keysFunctions, ReplicaID replicaId, unsigned int numGeneralReplicas, unsigned int numTrustedReplicas, unsigned int numReplicas, unsigned int numViews, unsigned int numFaults, double leaderChangeTime, Nodes nodes, Key privateKey, PeerNet::Config peerNetConfig, ClientNet::Config clientNetConfig);
};

#endif
