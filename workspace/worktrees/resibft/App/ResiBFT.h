#ifndef RESIBFT_H
#define RESIBFT_H

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
#include <cstring>
#include <thread>
#include <unistd.h>
#include <vector>
#include <algorithm>
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
#include "ResiBFTBasic.h"
#include "Statistics.h"
#include "Transaction.h"
#include "VerificationCertificate.h"
#include "Checkpoint.h"
#include "BlockCertificate.h"
#include "experiment_config.h"

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

class ResiBFT
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
	unsigned int originalTrustedQuorumSize;  // 保存原始值，用于从 GRACIOUS 恢复
	unsigned int lowTrustedSize;
	Group trustedGroup;
	View view;
	Protocol protocol;

	// ResiBFT-specific basic settings
	Epoch epoch;
	Stage stage;
	Group committee;
	bool isCommitteeMember;

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
	std::set<View> executedViews;		 // Views that have been executed (prevent duplicate execution)
	std::set<View> votedViews;			 // Views that we have already voted in (prevent duplicate votes)
	std::set<View> lockedViews;			 // Views where lockedQC has been updated (prevent duplicate lock updates)
	std::set<View> precommitVotedViews; // Views where we have already sent precommit vote
	std::set<View> ldrprepareInitiatedViews; // Views where Leader has already sent MsgLdrprepare (prevent duplicate)
	std::set<View> prepareProposalSentViews; // Views where Leader has already sent MsgPrepareProposal (prevent duplicate)
	std::set<View> prepareBroadcastViews;   // Views where Leader has already broadcast MsgPrepare quorum (prevent duplicate)
	std::set<View> decideSentViews;         // Views where Leader has already sent MsgDecide (prevent duplicate)
	std::mutex mutexTransaction;
	std::mutex mutexHandle;
	unsigned int viewsWithoutNewTrans = 0;
	bool started = false;
	bool stopped = false;
	View timerView; // View at which the timer was started
	Log log;

	// ResiBFT-specific state variables
	std::set<Checkpoint> checkpoints;									   // Checkpoints for epoch management
	std::map<View, BlockCertificate> blockCertificates;					   // Block certificates for committed blocks
	std::set<VerificationCertificate> acceptedVCs;						   // Accepted verification certificates
	std::set<VerificationCertificate> rejectedVCs;						   // Rejected verification certificates
	std::set<VerificationCertificate> timeoutVCs;						   // Timeout verification certificates
	std::map<View, std::set<VerificationCertificate>> pendingVCs;		   // Pending VCs per view

	// HotStuff safety mechanism state variables
	Justification prepareQC;    // Latest prepareQC (certified by quorum MsgPrepare)
	Justification lockedQC;     // Latest lockedQC (certified by quorum MsgPrecommit)
	Hash prepareHash;           // Hash of prepared block
	View prepareView;           // View of prepared block
	Hash lockedHash;            // Hash of locked block
	View lockedView;            // View of locked block

	// VRF committee selection state
	std::set<ReplicaID> currentCommittee;  // Current committee members (VRF-selected)
	View committeeView;                    // View for which committee was selected
	ReplicaID committeeLeader;             // Leader of the committee (the node that triggered GRACIOUS)
	View stageTransitionView;              // View when entering current stage (for recovery)

	// Proposed committee state (等待共识达成)
	// 论文设计：committee 在 MsgLdrprepare 中提议，对 committee+block 同时投票
	// committee 在执行区块时才更新，GRACIOUS 模式在执行后才进入
	Group proposedCommittee;               // 提议中的 committee（未达成共识）
	View proposedCommitteeView;            // proposedCommittee 对应的 view

	// Test simulation flags (for testing GRACIOUS and UNCIVIL modes)
	bool simulateMalicious = false;   // Set to true to simulate malicious leader (triggers UNCIVIL)
	bool simulateTimeout = false;     // Set to true to force timeout behavior (triggers GRACIOUS faster)

	// ============================================================
	// 实验参数配置（TC节点比例 λ 等）
	// ============================================================
	double tcRatio;                       // TC节点比例：λ = TC节点数 / TEE节点总数
	double kRatio;                        // 委员会大小比例：k = d * kRatio
	int experimentMode;                   // 实验模式：OPTIMISTIC/RANDOM/ATTACK
	int tcBehavior;                       // TC节点行为：BAD_VOTE/SILENT/INCONSISTENT

	std::set<ReplicaID> tcNodes;          // TC节点集合（拜占庭TEE节点中投坏票的）
	std::set<ReplicaID> byzantineNodes;   // 拜占庭节点集合
	std::set<ReplicaID> silentNodes;      // 沉默节点集合（每轮视图随机生成）

	// 实验参数初始化方法
	void initializeExperimentConfig();
	void selectTCNodes();
	void selectByzantineNodes();
	bool isTCNode(ReplicaID replicaId);
	bool isByzantineNode(ReplicaID replicaId);
	bool isSilentNode(ReplicaID replicaId);
	void generateSilentNodesForView(View view);

	// TC节点行为模拟
	void simulateTCBehavior(ReplicaID sender, View view);

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

	// ResiBFT-specific setting functions
	Peers getCommitteePeers();
	bool isInCommittee(ReplicaID replicaId);

	// Stage transition methods
	void transitionToNormal();
	void transitionToGracious();
	void transitionToUncivil();

	// Committee management methods
	void selectCommittee();
	void revokeCommittee();
	void updateCommittee();

	// Validation methods
	bool validateBlock(Block block);
	bool checkAcceptedVCs();
	bool checkRejectedVCs();
	bool checkTimeoutVCs();
	void sendVC(VCType type, View view, Hash blockHash);

	// HotStuff safety methods
	bool checkSafeNode(Justification qc);
	Justification findHighestQC(std::set<MsgNewviewResiBFT> newviews);
	void updateLockedQC(Justification newQC);

	// VRF committee selection
	Group selectCommitteeByVRF(View view, unsigned int k);

	// VRF seed computation (生产环境)
	unsigned int computeVRFSeedFromCommittedBlocks(View view);
	unsigned int convertHashToSeed(Hash hash, View view);
	unsigned int sampleCommitteeSize(unsigned int seed, unsigned int numTrustedReplicas);

	// Reply to clients
	void replyTransactions(Transaction *transactions);
	void replyHash(Hash hash);

	// Call TEE functions
	bool verifyJustificationResiBFT(Justification justification);
	bool verifyProposalResiBFT(Proposal<Accumulator> proposal, Signs signs);

	Justification initializeMsgNewviewResiBFT();
	Accumulator initializeAccumulatorResiBFT(Justification justifications_MsgNewview[MAX_NUM_SIGNATURES]);
	Signs initializeMsgLdrprepareResiBFT(Proposal<Accumulator> proposal_MsgLdrprepare);
	Justification respondMsgLdrprepareProposalResiBFT(Hash proposeHash, Accumulator accumulator_MsgLdrprepare);
	Justification saveMsgPrepareResiBFT(Justification justification_MsgPrepare);
	void skipRoundResiBFT();

	Accumulator initializeAccumulator(std::set<MsgNewviewResiBFT> msgNewviews);

	// Receive messages
	void receiveMsgStartResiBFT(MsgStart msgStart, const ClientNet::conn_t &conn);
	void receiveMsgTransactionResiBFT(MsgTransaction msgTransaction, const ClientNet::conn_t &conn);
	void receiveMsgNewviewResiBFT(MsgNewviewResiBFT msgNewview, const PeerNet::conn_t &conn);
	void receiveMsgLdrprepareResiBFT(MsgLdrprepareResiBFT msgLdrprepare, const PeerNet::conn_t &conn);
	void receiveMsgPrepareResiBFT(MsgPrepareResiBFT msgPrepare, const PeerNet::conn_t &conn);
	void receiveMsgPrecommitResiBFT(MsgPrecommitResiBFT msgPrecommit, const PeerNet::conn_t &conn);
	void receiveMsgDecideResiBFT(MsgDecideResiBFT msgDecide, const PeerNet::conn_t &conn);
	void receiveMsgBCResiBFT(MsgBCResiBFT msgBC, const PeerNet::conn_t &conn);
	void receiveMsgVCResiBFT(MsgVCResiBFT msgVC, const PeerNet::conn_t &conn);
	void receiveMsgCommitteeResiBFT(MsgCommitteeResiBFT msgCommittee, const PeerNet::conn_t &conn);
	void receiveMsgPrepareProposalResiBFT(MsgPrepareProposalResiBFT msgPrepareProposal, const PeerNet::conn_t &conn);
	void receiveMsgCommitResiBFT(MsgCommitResiBFT msgCommit, const PeerNet::conn_t &conn);

	// Send messages
	void sendMsgNewviewResiBFT(MsgNewviewResiBFT msgNewview, Peers recipients);
	void sendMsgLdrprepareResiBFT(MsgLdrprepareResiBFT msgLdrprepare, Peers recipients);
	void sendMsgPrepareResiBFT(MsgPrepareResiBFT msgPrepare, Peers recipients);
	void sendMsgPrecommitResiBFT(MsgPrecommitResiBFT msgPrecommit, Peers recipients);
	void sendMsgDecideResiBFT(MsgDecideResiBFT msgDecide, Peers recipients);
	void sendMsgBCResiBFT(MsgBCResiBFT msgBC, Peers recipients);
	void sendMsgVCResiBFT(MsgVCResiBFT msgVC, Peers recipients);
	void sendMsgCommitteeResiBFT(MsgCommitteeResiBFT msgCommittee, Peers recipients);
	void sendMsgPrepareProposalResiBFT(MsgPrepareProposalResiBFT msgPrepareProposal, Peers recipients);
	void sendMsgCommitResiBFT(MsgCommitResiBFT msgCommit, Peers recipients);

	// Handle messages
	void handleMsgTransaction(MsgTransaction msgTransaction);
	void handleEarlierMessagesResiBFT();										   // For replicas to process messages they have already received for in new view
	void handleMsgNewviewResiBFT(MsgNewviewResiBFT msgNewview);					   // Once the leader has received [msgNewview], it creates [msgLdrprepare] out of the highest prepared block
	void handleMsgLdrprepareResiBFT(MsgLdrprepareResiBFT msgLdrprepare);		   // Once the replicas have received [msgLdrprepare], it creates [msgPrepare] out of the proposal
	void handleMsgPrepareResiBFT(MsgPrepareResiBFT msgPrepare);					   // For both the leader and replicas process [msgPrepare]
	void handleMsgPrecommitResiBFT(MsgPrecommitResiBFT msgPrecommit);			   // For both the leader and replicas process [msgPrecommit]
	void handleMsgDecideResiBFT(MsgDecideResiBFT msgDecide);					   // For both the leader and replicas process [msgDecide]
	void handleMsgBCResiBFT(MsgBCResiBFT msgBC);								   // Handle block certificate messages
	void handleMsgVCResiBFT(MsgVCResiBFT msgVC);								   // Handle verification certificate messages
	void handleMsgCommitteeResiBFT(MsgCommitteeResiBFT msgCommittee);			   // Handle committee management messages
	void handleMsgPrepareProposalResiBFT(MsgPrepareProposalResiBFT msgPrepareProposal); // Handle HotStuff prepare proposal (NORMAL mode)
	void handleMsgCommitResiBFT(MsgCommitResiBFT msgCommit);					   // Handle HotStuff commit message

	// Initiate messages
	void initiateMsgNewviewResiBFT();									   // Leader send [msgLdrprepare] to others and hold its own [msgPrepare]
	void initiateMsgPrepareResiBFT(RoundData roundData_MsgPrepare);		   // Leader send [msgPrepare] to others and hold its own [msgPrecommit]
	void initiateMsgPrecommitResiBFT(RoundData roundData_MsgPrecommit);	   // Leader send [msgPrecommit] to others and execute the block
	void initiateMsgDecideResiBFT(RoundData roundData_MsgDecide);		   // Leader send [msgDecide] to others
	void initiateMsgPrepareProposalResiBFT();							   // HotStuff: Leader send MsgPrepareProposal in NORMAL mode
	void initiateMsgCommitResiBFT(RoundData roundData_MsgCommit);		   // HotStuff: Leader send MsgCommit in NORMAL mode

	// Respond messages
	void respondMsgLdrprepareResiBFT(Accumulator accumulator_MsgLdrprepare, Block block); // Replicas respond to [msgLdrprepare] and send [msgPrepare] to the leader
	void respondMsgPrepareResiBFT(Justification justification_MsgPrepare);				// Replicas respond to [msgPrepare] and send [msgPrecommit] to the leader
	void respondMsgPrepareProposalResiBFT(Proposal<Justification> proposal);				// HotStuff: All replicas vote on prepare proposal (NORMAL mode)

	// Main functions
	int initializeSGX();
	void getStarted();
	void startNewViewResiBFT();
	Block createNewBlockResiBFT(Hash hash);
	void executeBlockResiBFT(RoundData roundData_MsgPrecommit, bool isCatchUp = false);
	void createCheckpointResiBFT(RoundData roundData, View targetView);
	void commitPreviousBlock();  // 论文设计：ACCEPTED VC ≥ f+1 时 commit 前一个 block
	bool timeToStop();
	void recordStatisticsResiBFT();

public:
	ResiBFT(KeysFunctions keysFunctions, ReplicaID replicaId, unsigned int numGeneralReplicas, unsigned int numTrustedReplicas, unsigned int numReplicas, unsigned int numViews, unsigned int numFaults, double leaderChangeTime, Nodes nodes, Key privateKey, PeerNet::Config peerNetConfig, ClientNet::Config clientNetConfig);
};

#endif