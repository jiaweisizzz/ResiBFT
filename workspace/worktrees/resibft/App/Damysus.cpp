#include "Damysus.h"

// Local variables
Time startTime = std::chrono::steady_clock::now();
Time startView = std::chrono::steady_clock::now();
Time currentTime;
std::string statisticsValues;
std::string statisticsDone;
Statistics statistics;
sgx_enclave_id_t global_eid = 0;

// Opcodes
const uint8_t MsgNewviewDamysus::opcode;
const uint8_t MsgLdrprepareDamysus::opcode;
const uint8_t MsgPrepareDamysus::opcode;
const uint8_t MsgPrecommitDamysus::opcode;
const uint8_t MsgTransaction::opcode;
const uint8_t MsgReply::opcode;
const uint8_t MsgStart::opcode;

// Converts between classes and simpler structures used in enclaves
// Store [transaction] in [transaction_t]
void setTransaction(Transaction transaction, Transaction_t *transaction_t)
{
	transaction_t->clientId = transaction.getClientId();
	transaction_t->transactionId = transaction.getTransactionId();
	memcpy(transaction_t->transactionData, transaction.getTransactionData(), PAYLOAD_SIZE);
}

// Store [hash] in [hash_t]
void setHash(Hash hash, Hash_t *hash_t)
{
	memcpy(hash_t->hash, hash.getHash(), SHA256_DIGEST_LENGTH);
	hash_t->set = hash.getSet();
}

// Load [hash] from [hash_t]
Hash getHash(Hash_t *hash_t)
{
	bool set = hash_t->set;
	unsigned char *hash = hash_t->hash;
	Hash hash_ = Hash(set, hash);
	return hash_;
}

// Store [block] in [block_t]
void setBlock(Block block, Block_t *block_t)
{
	block_t->set = block.getSet();
	setHash(block.getPreviousHash(), &(block_t->previousHash));
	block_t->size = block.getSize();
	for (int i = 0; i < block.getSize(); i++)
	{
		setTransaction(block.get(i), &(block_t->transactions[i]));
	}
}

// Store [roundData] in [roundData_t]
void setRoundData(RoundData roundData, RoundData_t *roundData_t)
{
	setHash(roundData.getProposeHash(), &(roundData_t->proposeHash));
	roundData_t->proposeView = roundData.getProposeView();
	setHash(roundData.getJustifyHash(), &(roundData_t->justifyHash));
	roundData_t->justifyView = roundData.getJustifyView();
	roundData_t->phase = roundData.getPhase();
}

// Load [roundData] from [roundData_t]
RoundData getRoundData(RoundData_t *roundData_t)
{
	Hash proposeHash = getHash(&(roundData_t->proposeHash));
	View proposeView = roundData_t->proposeView;
	Hash justifyHash = getHash(&(roundData_t->justifyHash));
	View justifyView = roundData_t->justifyView;
	Phase phase = roundData_t->phase;
	RoundData roundData = RoundData(proposeHash, proposeView, justifyHash, justifyView, phase);
	return roundData;
}

// Store [sign] in [sign_t]
void setSign(Sign sign, Sign_t *sign_t)
{
	sign_t->set = sign.isSet();
	sign_t->signer = sign.getSigner();
	memcpy(sign_t->signtext, sign.getSigntext(), SIGN_LEN);
}

// Load [sign] from [sign_t]
Sign getSign(Sign_t *sign_t)
{
	bool b = sign_t->set;
	ReplicaID signer = sign_t->signer;
	unsigned char *signtext = sign_t->signtext;
	Sign sign = Sign(b, signer, signtext);
	return sign;
}

// Store [signs] in [signs_t]
void setSigns(Signs signs, Signs_t *signs_t)
{
	signs_t->size = signs.getSize();
	for (int i = 0; i < signs.getSize(); i++)
	{
		setSign(signs.get(i), &(signs_t->signs[i]));
	}
}

// Load [signs] from [signs_t]
Signs getSigns(Signs_t *signs_t)
{
	unsigned int size = signs_t->size;
	Sign signs[MAX_NUM_SIGNATURES];
	for (int i = 0; i < size; i++)
	{
		signs[i] = getSign(&(signs_t->signs[i]));
	}
	Signs signs_ = Signs(size, signs);
	return signs_;
}

// Store [justification] in [justification_t]
void setJustification(Justification justification, Justification_t *justification_t)
{
	justification_t->set = justification.isSet();
	setRoundData(justification.getRoundData(), &(justification_t->roundData));
	setSigns(justification.getSigns(), &(justification_t->signs));
}

// Store [justifications] in [justifications_t]
void setJustifications(Justification justifications[MAX_NUM_SIGNATURES], Justifications_t *justifications_t)
{
	for (int i = 0; i < MAX_NUM_SIGNATURES; i++)
	{
		setJustification(justifications[i], &(justifications_t->justifications[i]));
	}
}

// Load [justification] from [justification_t]
Justification getJustification(Justification_t *justification_t)
{
	bool set = justification_t->set;
	RoundData roundData = getRoundData(&(justification_t->roundData));
	Sign sign[MAX_NUM_SIGNATURES];
	for (int i = 0; i < MAX_NUM_SIGNATURES; i++)
	{
		sign[i] = Sign(justification_t->signs.signs[i].set, justification_t->signs.signs[i].signer, justification_t->signs.signs[i].signtext);
	}
	Signs signs(justification_t->signs.size, sign);
	Justification justification = Justification(set, roundData, signs);
	return justification;
}

// Store [accumulator] in [accumulator_t]
void setAccumulator(Accumulator accumulator, Accumulator_t *accumulator_t)
{
	accumulator_t->set = accumulator.isSet();
	accumulator_t->proposeView = accumulator.getProposeView();
	accumulator_t->prepareHash.set = accumulator.getPrepareHash().getSet();
	memcpy(accumulator_t->prepareHash.hash, accumulator.getPrepareHash().getHash(), SHA256_DIGEST_LENGTH);
	accumulator_t->prepareView = accumulator.getPrepareView();
	accumulator_t->size = accumulator.getSize();
}

// Load [accumulator] from [accumulator_t]
Accumulator getAccumulator(Accumulator_t *accumulator_t)
{
	bool set = accumulator_t->set;
	View proposeView = accumulator_t->proposeView;
	Hash prepareHash = getHash(&(accumulator_t->prepareHash));
	View prepareView = accumulator_t->prepareView;
	unsigned int size = accumulator_t->size;
	Accumulator accumulator = Accumulator(set, proposeView, prepareHash, prepareView, size);
	return accumulator;
}

// Store [proposal] in [proposal_t]
void setProposal(Proposal<Accumulator> proposal, Proposal_t *proposal_t)
{
	setAccumulator(proposal.getCertification(), &(proposal_t->accumulator));
	setBlock(proposal.getBlock(), &(proposal_t->block));
}

void TEE_Print(const char *text)
{
	printf("%s\n", text);
}

// Print functions
std::string Damysus::printReplicaId()
{
	return "[" + std::to_string(this->replicaId) + "-" + std::to_string(this->view) + "]";
}

void Damysus::printNowTime(std::string msg)
{
	auto now = std::chrono::steady_clock::now();
	double time = std::chrono::duration_cast<std::chrono::microseconds>(now - startView).count();
	double etime = (statistics.getTotalViewTime().total + time) / (1000 * 1000);
	std::cout << COLOUR_BLUE << this->printReplicaId() << msg << " @ " << etime << COLOUR_NORMAL << std::endl;
}

void Damysus::printClientInfo()
{
	for (Clients::iterator it = this->clients.begin(); it != this->clients.end(); it++)
	{
		ClientID clientId = it->first;
		ClientInformation clientInfo = it->second;
		bool running = std::get<0>(clientInfo);
		unsigned int received = std::get<1>(clientInfo);
		unsigned int replied = std::get<2>(clientInfo);
		ClientNet::conn_t conn = std::get<3>(clientInfo);
		if (DEBUG_BASIC)
		{
			std::cout << COLOUR_RED
					  << this->printReplicaId() << "CLIENT[id: "
					  << clientId << ", running: "
					  << running << ", numbers of received: "
					  << received << ", numbers of replied: "
					  << replied << "]" << COLOUR_NORMAL << std::endl;
		}
	}
}

std::string Damysus::recipients2string(Peers recipients)
{
	std::string text = "";
	for (Peers::iterator it = recipients.begin(); it != recipients.end(); it++)
	{
		Peer peer = *it;
		text += std::to_string(std::get<0>(peer)) + " ";
	}
	return text;
}

// Setting functions
unsigned int Damysus::getLeaderOf(View view)
{
	unsigned int leader = view % this->numTrustedReplicas;
	return leader;
}

unsigned int Damysus::getCurrentLeader()
{
	unsigned int leader = this->getLeaderOf(this->view);
	return leader;
}

bool Damysus::amLeaderOf(View view)
{
	if (this->replicaId == this->getLeaderOf(view))
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool Damysus::amCurrentLeader()
{
	if (this->replicaId == this->getCurrentLeader())
	{
		return true;
	}
	else
	{
		return false;
	}
}

Peers Damysus::removeFromPeers(ReplicaID replicaId)
{
	Peers peers;
	for (Peers::iterator itPeers = this->peers.begin(); itPeers != this->peers.end(); itPeers++)
	{
		Peer peer = *itPeers;
		if (std::get<0>(peer) != replicaId)
		{
			peers.push_back(peer);
		}
	}
	return peers;
}

Peers Damysus::keepFromPeers(ReplicaID replicaId)
{
	Peers peers;
	for (Peers::iterator itPeers = this->peers.begin(); itPeers != this->peers.end(); itPeers++)
	{
		Peer peer = *itPeers;
		if (std::get<0>(peer) == replicaId)
		{
			peers.push_back(peer);
		}
	}
	return peers;
}

std::vector<salticidae::PeerId> Damysus::getPeerIds(Peers recipients)
{
	std::vector<salticidae::PeerId> returnPeerId;
	for (Peers::iterator it = recipients.begin(); it != recipients.end(); it++)
	{
		Peer peer = *it;
		returnPeerId.push_back(std::get<1>(peer));
	}
	return returnPeerId;
}

void Damysus::setTimer()
{
	this->timer.del();
	this->timer.add(this->leaderChangeTime);
	this->timerView = this->view;
}

// Reply to clients
void Damysus::replyTransactions(Transaction *transactions)
{
	for (int i = 0; i < MAX_NUM_TRANSACTIONS; i++)
	{
		Transaction transaction = transactions[i];
		ClientID clientId = transaction.getClientId();
		TransactionID transactionId = transaction.getTransactionId(); // TransactionID 0 is for dummy transactions
		if (transactionId != 0)
		{
			Clients::iterator itClient = this->clients.find(clientId);
			if (itClient != this->clients.end())
			{
				this->executionQueue.enqueue(std::make_pair(transactionId, clientId));
				if (DEBUG_HELP)
				{
					std::cout << COLOUR_BLUE << this->printReplicaId() << "Sending reply to " << clientId << ": " << transactionId << COLOUR_NORMAL << std::endl;
				}
			}
			else
			{
				if (DEBUG_HELP)
				{
					std::cout << COLOUR_BLUE << this->printReplicaId() << "Unknown client: " << clientId << COLOUR_NORMAL << std::endl;
				}
			}
		}
	}
}

void Damysus::replyHash(Hash hash)
{
	std::map<View, Block>::iterator it = this->blocks.find(this->view);
	if (it != this->blocks.end())
	{
		Block block = it->second;
		if (hash == block.hash())
		{
			if (DEBUG_HELP)
			{
				std::cout << COLOUR_BLUE << this->printReplicaId() << "Found block for view " << this->view << ": " << block.toPrint() << COLOUR_NORMAL << std::endl;
			}
			this->replyTransactions(block.getTransactions());
		}
		else
		{
			if (DEBUG_HELP)
			{
				std::cout << COLOUR_BLUE << this->printReplicaId() << "Recorded block but incorrect hash for view " << this->view << COLOUR_NORMAL << std::endl;
			}
			if (DEBUG_HELP)
			{
				std::cout << COLOUR_BLUE << this->printReplicaId() << "Checking hash: " << hash.toString() << COLOUR_NORMAL << std::endl;
			}
		}
	}
	else
	{
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "No block recorded for view " << this->view << COLOUR_NORMAL << std::endl;
		}
	}
}

// Call TEE functions
bool Damysus::verifyJustificationDamysus(Justification justification)
{
	Justification_t justification_t;
	setJustification(justification, &justification_t);
	bool b;
	sgx_status_t extra_t;
	sgx_status_t status_t;
	status_t = TEE_verifyJustificationDamysus(global_eid, &extra_t, &justification_t, &b);
	return b;
}

bool Damysus::verifyProposalDamysus(Proposal<Accumulator> proposal, Signs signs)
{
	Proposal_t proposal_t;
	setProposal(proposal, &proposal_t);
	Signs_t signs_t;
	setSigns(signs, &signs_t);
	bool b;
	sgx_status_t extra_t;
	sgx_status_t status_t;
	status_t = TEE_verifyProposalDamysus(global_eid, &extra_t, &proposal_t, &signs_t, &b);
	return b;
}

Justification Damysus::initializeMsgNewviewDamysus()
{
	Justification_t justification_MsgNewview_t;
	sgx_status_t extra_t;
	sgx_status_t status_t;
	status_t = TEE_initializeMsgNewviewDamysus(global_eid, &extra_t, &justification_MsgNewview_t);
	Justification justification_MsgNewview = getJustification(&justification_MsgNewview_t);
	return justification_MsgNewview;
}

Accumulator Damysus::initializeAccumulatorDamysus(Justification justifications_MsgNewview[MAX_NUM_SIGNATURES])
{
	Justifications_t justifications_MsgNewview_t;
	setJustifications(justifications_MsgNewview, &justifications_MsgNewview_t);
	Accumulator_t accumulator_MsgLdrprepare_t;
	sgx_status_t extra_t;
	sgx_status_t status_t;
	status_t = TEE_initializeAccumulatorDamysus(global_eid, &extra_t, &justifications_MsgNewview_t, &accumulator_MsgLdrprepare_t);
	Accumulator accumulator_MsgLdrprepare = getAccumulator(&accumulator_MsgLdrprepare_t);
	return accumulator_MsgLdrprepare;
}

Signs Damysus::initializeMsgLdrprepareDamysus(Proposal<Accumulator> proposal_MsgLdrprepare)
{
	Proposal_t proposal_MsgLdrprepare_t;
	setProposal(proposal_MsgLdrprepare, &proposal_MsgLdrprepare_t);
	Signs_t signs_MsgLdrprepare_t;
	sgx_status_t extra_t;
	sgx_status_t status_t;
	status_t = TEE_initializeMsgLdrprepareDamysus(global_eid, &extra_t, &proposal_MsgLdrprepare_t, &signs_MsgLdrprepare_t);
	Signs signs_MsgLdrprepare = getSigns(&signs_MsgLdrprepare_t);
	return signs_MsgLdrprepare;
}

Justification Damysus::respondMsgLdrprepareProposalDamysus(Hash proposeHash, Accumulator accumulator_MsgLdrprepare)
{
	Accumulator_t accumulator_MsgLdrprepare_t;
	setAccumulator(accumulator_MsgLdrprepare, &accumulator_MsgLdrprepare_t);
	Hash_t proposeHash_t;
	setHash(proposeHash, &proposeHash_t);
	Justification_t justification_MsgPrepare_t;
	sgx_status_t extra_t;
	sgx_status_t status_t;
	status_t = TEE_respondProposalDamysus(global_eid, &extra_t, &proposeHash_t, &accumulator_MsgLdrprepare_t, &justification_MsgPrepare_t);
	Justification justification_MsgPrepare = getJustification(&justification_MsgPrepare_t);
	return justification_MsgPrepare;
}

Justification Damysus::saveMsgPrepareDamysus(Justification justification_MsgPrepare)
{
	Justification_t justification_MsgPrepare_t;
	setJustification(justification_MsgPrepare, &justification_MsgPrepare_t);
	Justification_t justification_MsgPrecommit_t;
	sgx_status_t extra;
	sgx_status_t status_t;
	status_t = TEE_saveMsgPrepareDamysus(global_eid, &extra, &justification_MsgPrepare_t, &justification_MsgPrecommit_t);
	Justification justification_MsgPrecommit = getJustification(&justification_MsgPrecommit_t);
	return justification_MsgPrecommit;
}

Accumulator Damysus::initializeAccumulator(std::set<MsgNewviewDamysus> msgNewviews)
{
	Justification justifications_MsgNewview[MAX_NUM_SIGNATURES];
	unsigned int i = 0;
	for (std::set<MsgNewviewDamysus>::iterator it = msgNewviews.begin(); it != msgNewviews.end() && i < MAX_NUM_SIGNATURES; it++, i++)
	{
		MsgNewviewDamysus msgNewview = *it;
		RoundData roundData_MsgNewview = msgNewview.roundData;
		Signs signs_MsgNewview = msgNewview.signs;
		Justification justification_MsgNewview = Justification(roundData_MsgNewview, signs_MsgNewview);
		justifications_MsgNewview[i] = justification_MsgNewview;
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "MsgNewview[" << i << "]: " << msgNewview.toPrint() << COLOUR_NORMAL << std::endl;
		}
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Justification of MsgNewview[" << i << "]: " << justifications_MsgNewview[i].toPrint() << COLOUR_NORMAL << std::endl;
		}
	}

	Accumulator accumulator_MsgLdrprepare;
	accumulator_MsgLdrprepare = this->initializeAccumulatorDamysus(justifications_MsgNewview);
	return accumulator_MsgLdrprepare;
}

// Receive messages
void Damysus::receiveMsgStartDamysus(MsgStart msgStart, const ClientNet::conn_t &conn)
{
	ClientID clientId = msgStart.clientId;
	if (this->clients.find(clientId) == this->clients.end())
	{
		(this->clients)[clientId] = std::make_tuple(true, 0, 0, conn);
	}
	if (!this->started)
	{
		this->started = true;
		this->getStarted();
	}
}

void Damysus::receiveMsgTransactionDamysus(MsgTransaction msgTransaction, const ClientNet::conn_t &conn)
{
	this->handleMsgTransaction(msgTransaction);
}

void Damysus::receiveMsgNewviewDamysus(MsgNewviewDamysus msgNewview, const PeerNet::conn_t &conn)
{
	this->handleMsgNewviewDamysus(msgNewview);
}

void Damysus::receiveMsgLdrprepareDamysus(MsgLdrprepareDamysus msgLdrprepare, const PeerNet::conn_t &conn)
{
	this->handleMsgLdrprepareDamysus(msgLdrprepare);
}

void Damysus::receiveMsgPrepareDamysus(MsgPrepareDamysus msgPrepare, const PeerNet::conn_t &conn)
{
	this->handleMsgPrepareDamysus(msgPrepare);
}

void Damysus::receiveMsgPrecommitDamysus(MsgPrecommitDamysus msgPrecommit, const PeerNet::conn_t &conn)
{
	this->handleMsgPrecommitDamysus(msgPrecommit);
}

// Send messages
void Damysus::sendMsgNewviewDamysus(MsgNewviewDamysus msgNewview, Peers recipients)
{
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Sending: " << msgNewview.toPrint() << " -> " << this->recipients2string(recipients) << COLOUR_NORMAL << std::endl;
	}
	this->peerNet.multicast_msg(msgNewview, getPeerIds(recipients));
	if (DEBUG_TIME)
	{
		this->printNowTime("Sending MsgNewviewDamysus");
	}
}

void Damysus::sendMsgLdrprepareDamysus(MsgLdrprepareDamysus msgLdrprepare, Peers recipients)
{
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Sending: " << msgLdrprepare.toPrint() << " -> " << this->recipients2string(recipients) << COLOUR_NORMAL << std::endl;
	}
	this->peerNet.multicast_msg(msgLdrprepare, getPeerIds(recipients));
	if (DEBUG_TIME)
	{
		this->printNowTime("Sending MsgLdrprepareDamysus");
	}
}

void Damysus::sendMsgPrepareDamysus(MsgPrepareDamysus msg, Peers recipients)
{
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Sending: " << msg.toPrint() << " -> " << this->recipients2string(recipients) << COLOUR_NORMAL << std::endl;
	}
	this->peerNet.multicast_msg(msg, getPeerIds(recipients));
	if (DEBUG_TIME)
	{
		this->printNowTime("Sending MsgPrepareDamysus");
	}
}

void Damysus::sendMsgPrecommitDamysus(MsgPrecommitDamysus msg, Peers recipients)
{
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Sending: " << msg.toPrint() << " -> " << this->recipients2string(recipients) << COLOUR_NORMAL << std::endl;
	}
	this->peerNet.multicast_msg(msg, getPeerIds(recipients));
	if (DEBUG_TIME)
	{
		this->printNowTime("Sending MsgPrecommitDamysus");
	}
}

// Handle messages
void Damysus::handleMsgTransaction(MsgTransaction msgTransaction)
{
	std::lock_guard<std::mutex> guard(mutexTransaction);
	auto start = std::chrono::steady_clock::now();
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Handling MsgTransaction: " << msgTransaction.toPrint() << COLOUR_NORMAL << std::endl;
	}

	Transaction transaction = msgTransaction.transaction;
	ClientID clientId = transaction.getClientId();
	Clients::iterator it = this->clients.find(clientId);
	if (it != this->clients.end()) // Found an entry for [clientId]
	{
		ClientInformation clientInformation = it->second;
		bool running = std::get<0>(clientInformation);
		if (running)
		{
			// Got a new transaction from a live client
			if (this->transactions.size() < this->transactions.max_size())
			{
				if (DEBUG_HELP)
				{
					std::cout << COLOUR_BLUE << this->printReplicaId() << "Pushing transaction: " << transaction.toPrint() << COLOUR_NORMAL << std::endl;
				}
				(this->clients)[clientId] = std::make_tuple(true, std::get<1>(clientInformation) + 1, std::get<2>(clientInformation), std::get<3>(clientInformation));
				this->transactions.push_back(transaction);
			}
			else
			{
				if (DEBUG_HELP)
				{
					std::cout << COLOUR_BLUE << this->printReplicaId() << "Too many transactions (" << this->transactions.size() << "/" << this->transactions.max_size() << ")" << clientId << COLOUR_NORMAL << std::endl;
				}
			}
		}
		else
		{
			if (DEBUG_HELP)
			{
				std::cout << COLOUR_BLUE << this->printReplicaId() << "Transaction rejected from stopped client: " << clientId << COLOUR_NORMAL << std::endl;
			}
		}
	}
	else
	{
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Transaction rejected from unknown client: " << clientId << COLOUR_NORMAL << std::endl;
		}
	}

	auto end = std::chrono::steady_clock::now();
	double time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	statistics.addHandleTime(time);
}

void Damysus::handleEarlierMessagesDamysus()
{
	// Check if there are enough messages to start the next view
	if (this->amCurrentLeader())
	{
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Leader handling earlier messages" << COLOUR_NORMAL << std::endl;
		}
		std::set<MsgNewviewDamysus> msgNewviews = this->log.getMsgNewviewDamysus(this->view, this->trustedQuorumSize);
		if (msgNewviews.size() == this->trustedQuorumSize)
		{
			this->initiateMsgNewviewDamysus();
		}
	}
	else
	{
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Replica handling earlier messages" << COLOUR_NORMAL << std::endl;
		}
		// Check if the view has already been locked
		Signs signs_MsgPrecommit = this->log.getMsgPrecommitDamysus(this->view, this->trustedQuorumSize);
		if (signs_MsgPrecommit.getSize() == this->trustedQuorumSize)
		{
			if (DEBUG_HELP)
			{
				std::cout << COLOUR_BLUE << this->printReplicaId() << "Catching up using MsgPrecommit certificate" << COLOUR_NORMAL << std::endl;
			}

			// Skip the prepare phase and pre-commit phase
			this->initializeMsgNewviewDamysus();
			this->initializeMsgNewviewDamysus();

			// Execute the block
			Justification justification_MsgPrecommit = this->log.firstMsgPrecommitDamysus(this->view);
			RoundData roundData_MsgPrecommit = justification_MsgPrecommit.getRoundData();
			Signs signs_MsgPrecommit = justification_MsgPrecommit.getSigns();
			if (signs_MsgPrecommit.getSize() == this->trustedQuorumSize && this->verifyJustificationDamysus(justification_MsgPrecommit))
			{
				this->executeBlockDamysus(roundData_MsgPrecommit);
			}
		}
		else
		{
			Signs signs_MsgPrepare = this->log.getMsgPrepareDamysus(this->view, this->trustedQuorumSize);
			if (signs_MsgPrepare.getSize() == this->trustedQuorumSize)
			{
				if (DEBUG_HELP)
				{
					std::cout << COLOUR_BLUE << this->printReplicaId() << "Catching up using MsgPrepare certificate" << COLOUR_NORMAL << std::endl;
				}
				Justification justification_MsgPrepare = this->log.firstMsgPrepareDamysus(this->view);

				// Skip the prepare phase
				this->initializeMsgNewviewDamysus();

				// Store [justification_MsgPrepare]
				this->respondMsgPrepareDamysus(justification_MsgPrepare);
			}
			else
			{
				MsgLdrprepareDamysus msgLdrprepare = this->log.firstMsgLdrprepareDamysus(this->view);

				// Check if the proposal has been stored
				if (msgLdrprepare.signs.getSize() == 1)
				{
					if (DEBUG_HELP)
					{
						std::cout << COLOUR_BLUE << this->printReplicaId() << "Catching up using MsgLdrprepare proposal" << COLOUR_NORMAL << std::endl;
					}
					Proposal<Accumulator> proposal = msgLdrprepare.proposal;
					Accumulator accumulator_MsgLdrprepare = proposal.getCertification();
					Block block = proposal.getBlock();
					this->respondMsgLdrprepareDamysus(accumulator_MsgLdrprepare, block);
				}
			}
		}
	}
}

void Damysus::handleMsgNewviewDamysus(MsgNewviewDamysus msgNewview)
{
	auto start = std::chrono::steady_clock::now();

	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Handling MsgNewview: " << msgNewview.toPrint() << COLOUR_NORMAL << std::endl;
	}
	RoundData roundData_MsgNewview = msgNewview.roundData;
	Hash proposeHash_MsgNewview = roundData_MsgNewview.getProposeHash();
	View proposeView_MsgNewview = roundData_MsgNewview.getProposeView();
	Phase phase_MsgNewview = roundData_MsgNewview.getPhase();

	if (proposeHash_MsgNewview.isDummy() && proposeView_MsgNewview >= this->view && phase_MsgNewview == PHASE_NEWVIEW)
	{
		if (proposeView_MsgNewview == this->view)
		{
			if (this->log.storeMsgNewviewDamysus(msgNewview) == this->trustedQuorumSize)
			{
				this->initiateMsgNewviewDamysus();
			}
		}
		else
		{
			if (DEBUG_HELP)
			{
				std::cout << COLOUR_BLUE << this->printReplicaId() << "Storing MsgNewview: " << msgNewview.toPrint() << COLOUR_NORMAL << std::endl;
			}
			this->log.storeMsgNewviewDamysus(msgNewview);
		}
	}
	else
	{
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Discarded MsgNewview: " << msgNewview.toPrint() << COLOUR_NORMAL << std::endl;
		}
	}

	auto end = std::chrono::steady_clock::now();
	double time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	statistics.addHandleTime(time);
}

void Damysus::handleMsgLdrprepareDamysus(MsgLdrprepareDamysus msgLdrprepare)
{
	auto start = std::chrono::steady_clock::now();

	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Handling MsgLdrprepare: " << msgLdrprepare.toPrint() << COLOUR_NORMAL << std::endl;
	}
	Proposal<Accumulator> proposal_MsgLdrprepare = msgLdrprepare.proposal;
	Signs signs_MsgLdrprepare = msgLdrprepare.signs;
	Accumulator accumulator_MsgLdrprepare = proposal_MsgLdrprepare.getCertification();
	View proposeView_MsgLdrprepare = accumulator_MsgLdrprepare.getProposeView();
	Hash prepareHash_MsgLdrprepare = accumulator_MsgLdrprepare.getPrepareHash();
	Block block = proposal_MsgLdrprepare.getBlock();

	// Verify the [signs_MsgLdrprepare] in [msgLdrprepare]
	if (this->verifyProposalDamysus(proposal_MsgLdrprepare, signs_MsgLdrprepare) && block.extends(prepareHash_MsgLdrprepare) && proposeView_MsgLdrprepare >= this->view)
	{
		if (proposeView_MsgLdrprepare == this->view)
		{
			this->respondMsgLdrprepareDamysus(accumulator_MsgLdrprepare, block);
		}
		else
		{
			if (DEBUG_HELP)
			{
				std::cout << COLOUR_BLUE << this->printReplicaId() << "Storing MsgLdrprepare: " << msgLdrprepare.toPrint() << COLOUR_NORMAL << std::endl;
			}
			this->log.storeMsgLdrprepareDamysus(msgLdrprepare);
		}
	}

	auto end = std::chrono::steady_clock::now();
	double time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	statistics.addHandleTime(time);
}

void Damysus::handleMsgPrepareDamysus(MsgPrepareDamysus msgPrepare)
{
	auto start = std::chrono::steady_clock::now();

	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Handling MsgPrepare: " << msgPrepare.toPrint() << COLOUR_NORMAL << std::endl;
	}
	RoundData roundData_MsgPrepare = msgPrepare.roundData;
	Signs signs_MsgPrepare = msgPrepare.signs;
	View proposeView_MsgPrepare = roundData_MsgPrepare.getProposeView();
	Phase phase_MsgPrepare = roundData_MsgPrepare.getPhase();
	Justification justification_MsgPrepare = Justification(roundData_MsgPrepare, signs_MsgPrepare);

	if (proposeView_MsgPrepare >= this->view && phase_MsgPrepare == PHASE_PREPARE)
	{
		if (proposeView_MsgPrepare == this->view)
		{
			if (this->amCurrentLeader())
			{
				if (this->log.storeMsgPrepareDamysus(msgPrepare) == this->trustedQuorumSize)
				{
					this->initiateMsgPrepareDamysus(roundData_MsgPrepare);
				}
			}
			else
			{
				if (signs_MsgPrepare.getSize() == this->trustedQuorumSize)
				{
					this->respondMsgPrepareDamysus(justification_MsgPrepare);
				}
			}
		}
		else
		{
			if (DEBUG_HELP)
			{
				std::cout << COLOUR_BLUE << this->printReplicaId() << "Storing MsgPrepare: " << msgPrepare.toPrint() << COLOUR_NORMAL << std::endl;
			}
			this->log.storeMsgPrepareDamysus(msgPrepare);
		}
	}

	auto end = std::chrono::steady_clock::now();
	double time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	statistics.addHandleTime(time);
}

void Damysus::handleMsgPrecommitDamysus(MsgPrecommitDamysus msgPrecommit)
{
	auto start = std::chrono::steady_clock::now();

	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Handling MsgPrecommit: " << msgPrecommit.toPrint() << COLOUR_NORMAL << std::endl;
	}
	RoundData roundData_MsgPrecommit = msgPrecommit.roundData;
	Signs signs_MsgPrecommit = msgPrecommit.signs;
	View proposeView_MsgPrecommit = roundData_MsgPrecommit.getProposeView();
	Phase phase_MsgPrecommit = roundData_MsgPrecommit.getPhase();
	Justification justification_MsgPrecommit = Justification(roundData_MsgPrecommit, signs_MsgPrecommit);

	if (proposeView_MsgPrecommit >= this->view && phase_MsgPrecommit == PHASE_PRECOMMIT)
	{
		if (proposeView_MsgPrecommit == this->view)
		{
			if (this->amCurrentLeader())
			{
				if (this->log.storeMsgPrecommitDamysus(msgPrecommit) == this->trustedQuorumSize)
				{
					this->initiateMsgPrecommitDamysus(roundData_MsgPrecommit);
				}
			}
			else
			{
				if (signs_MsgPrecommit.getSize() == this->trustedQuorumSize && this->verifyJustificationDamysus(justification_MsgPrecommit))
				{
					this->executeBlockDamysus(roundData_MsgPrecommit);
				}
			}
		}
		else
		{
			if (DEBUG_HELP)
			{
				std::cout << COLOUR_BLUE << this->printReplicaId() << "Storing MsgPrecommit: " << msgPrecommit.toPrint() << COLOUR_NORMAL << std::endl;
			}
			if (this->amLeaderOf(proposeView_MsgPrecommit))
			{
				this->log.storeMsgPrecommitDamysus(msgPrecommit);
			}
			else
			{
				if (this->verifyJustificationDamysus(justification_MsgPrecommit))
				{
					this->log.storeMsgPrecommitDamysus(msgPrecommit);
				}
			}
		}
	}

	auto end = std::chrono::steady_clock::now();
	double time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	statistics.addHandleTime(time);
}

// Initiate messages
void Damysus::initiateMsgNewviewDamysus()
{
	std::set<MsgNewviewDamysus> msgNewviews = this->log.getMsgNewviewDamysus(this->view, this->trustedQuorumSize);
	if (msgNewviews.size() == this->trustedQuorumSize)
	{
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Handling MsgNewview to accumulator" << COLOUR_NORMAL << std::endl;
		}
		Accumulator accumulator_MsgLdrprepare = this->initializeAccumulator(msgNewviews);

		if (accumulator_MsgLdrprepare.isSet())
		{
			// Create [block] extends the highest prepared block
			Hash prepareHash_MsgLdrprepare = accumulator_MsgLdrprepare.getPrepareHash();
			Block block = this->createNewBlockDamysus(prepareHash_MsgLdrprepare);

			// Create [justification_MsgPrepare] for that [block]
			Justification justification_MsgPrepare = this->respondMsgLdrprepareProposalDamysus(block.hash(), accumulator_MsgLdrprepare);
			if (justification_MsgPrepare.isSet())
			{
				if (DEBUG_HELP)
				{
					std::cout << COLOUR_BLUE << this->printReplicaId() << "Storing block for view " << this->view << ": " << block.toPrint() << COLOUR_NORMAL << std::endl;
				}
				this->blocks[this->view] = block;

				// Create [msgLdrprepare] out of [block]
				Proposal<Accumulator> proposal_MsgLdrprepare = Proposal<Accumulator>(accumulator_MsgLdrprepare, block);
				Signs signs_MsgLdrprepare = this->initializeMsgLdrprepareDamysus(proposal_MsgLdrprepare);
				MsgLdrprepareDamysus msgLdrprepare = MsgLdrprepareDamysus(proposal_MsgLdrprepare, signs_MsgLdrprepare);

				// Send [msgLdrprepare] to replicas
				Peers recipients = this->removeFromPeers(this->replicaId);
				this->sendMsgLdrprepareDamysus(msgLdrprepare, recipients);
				if (DEBUG_HELP)
				{
					std::cout << COLOUR_BLUE << this->printReplicaId() << "Sent MsgLdrprepare to replicas: " << msgLdrprepare.toPrint() << COLOUR_NORMAL << std::endl;
				}

				// Create [msgPrepare]
				RoundData roundData_MsgPrepare = justification_MsgPrepare.getRoundData();
				Signs signs_MsgPrepare = justification_MsgPrepare.getSigns();
				MsgPrepareDamysus msgPrepare = MsgPrepareDamysus(roundData_MsgPrepare, signs_MsgPrepare);
				if (DEBUG_HELP)
				{
					std::cout << COLOUR_BLUE << this->printReplicaId() << "Hold on MsgPrepare to its own: " << msgPrepare.toPrint() << COLOUR_NORMAL << std::endl;
				}

				// Store own [msgPrepare] in the log
				if (this->log.storeMsgPrepareDamysus(msgPrepare) == this->trustedQuorumSize)
				{
					this->initiateMsgPrepareDamysus(msgPrepare.roundData);
				}
			}
		}
		else
		{
			if (DEBUG_HELP)
			{
				std::cout << COLOUR_BLUE << this->printReplicaId() << "Bad accumulator for MsgLdrprepare: " << accumulator_MsgLdrprepare.toPrint() << COLOUR_NORMAL << std::endl;
			}
		}
	}
}

void Damysus::initiateMsgPrepareDamysus(RoundData roundData_MsgPrepare)
{
	View proposeView_MsgPrepare = roundData_MsgPrepare.getProposeView();
	Signs signs_MsgPrepare = this->log.getMsgPrepareDamysus(proposeView_MsgPrepare, this->trustedQuorumSize);
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "MsgPrepare signatures: " << signs_MsgPrepare.toPrint() << COLOUR_NORMAL << std::endl;
	}

	Justification justification_MsgPrepare = Justification(roundData_MsgPrepare, signs_MsgPrepare);
	if (signs_MsgPrepare.getSize() == this->trustedQuorumSize)
	{
		// Create [msgPrepare]
		MsgPrepareDamysus msgPrepare = MsgPrepareDamysus(roundData_MsgPrepare, signs_MsgPrepare);

		// Send [msgPrepare] to replicas
		Peers recipients = this->removeFromPeers(this->replicaId);
		this->sendMsgPrepareDamysus(msgPrepare, recipients);
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Sent MsgPrepare to replicas: " << msgPrepare.toPrint() << COLOUR_NORMAL << std::endl;
		}

		// Create [msgPrecommit]
		Justification justification_MsgPrecommit = this->saveMsgPrepareDamysus(justification_MsgPrepare);
		RoundData roundData_MsgPrecommit = justification_MsgPrecommit.getRoundData();
		Signs signs_MsgPrecommit = justification_MsgPrecommit.getSigns();
		MsgPrecommitDamysus msgPrecommit = MsgPrecommitDamysus(roundData_MsgPrecommit, signs_MsgPrecommit);
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Hold on MsgPrecommit to its own: " << msgPrecommit.toPrint() << COLOUR_NORMAL << std::endl;
		}

		// Store own [msgPrecommit] in the log
		if (this->log.storeMsgPrecommitDamysus(msgPrecommit) >= this->trustedQuorumSize)
		{
			this->initiateMsgPrecommitDamysus(justification_MsgPrecommit.getRoundData());
		}
	}
}

void Damysus::initiateMsgPrecommitDamysus(RoundData roundData_MsgPrecommit)
{
	View proposeView_MsgPrecommit = roundData_MsgPrecommit.getProposeView();
	Signs signs_MsgPrecommit = this->log.getMsgPrecommitDamysus(proposeView_MsgPrecommit, this->trustedQuorumSize);
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "MsgPrecommit signatures: " << signs_MsgPrecommit.toPrint() << COLOUR_NORMAL << std::endl;
	}

	if (signs_MsgPrecommit.getSize() == this->trustedQuorumSize)
	{
		// Create [msgPrecommit]
		MsgPrecommitDamysus msgPrecommit = MsgPrecommitDamysus(roundData_MsgPrecommit, signs_MsgPrecommit);

		// Send [msgPrecommit] to replicas
		Peers recipients = this->removeFromPeers(this->replicaId);
		this->sendMsgPrecommitDamysus(msgPrecommit, recipients);
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Sent MsgPrecommit to replicas: " << msgPrecommit.toPrint() << COLOUR_NORMAL << std::endl;
		}

		// Execute the block
		this->executeBlockDamysus(roundData_MsgPrecommit);
	}
}

// Respond messages
void Damysus::respondMsgLdrprepareDamysus(Accumulator accumulator_MsgLdrprepare, Block block)
{
	// Create own [justification_MsgPrepare] for that [block]
	Justification justification_MsgPrepare = this->respondMsgLdrprepareProposalDamysus(block.hash(), accumulator_MsgLdrprepare);
	if (justification_MsgPrepare.isSet())
	{
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Storing block for view " << this->view << ": " << block.toPrint() << COLOUR_NORMAL << std::endl;
		}
		this->blocks[this->view] = block;

		// Create [msgPrepare] out of [block]
		RoundData roundData_MsgPrepare = justification_MsgPrepare.getRoundData();
		Signs signs_MsgPrepare = justification_MsgPrepare.getSigns();
		MsgPrepareDamysus msgPrepare = MsgPrepareDamysus(justification_MsgPrepare.getRoundData(), justification_MsgPrepare.getSigns());

		// Send [msgPrepare] to leader
		Peers recipients = this->keepFromPeers(this->getCurrentLeader());
		this->sendMsgPrepareDamysus(msgPrepare, recipients);
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Sent MsgPrepare to leader: " << msgPrepare.toPrint() << COLOUR_NORMAL << std::endl;
		}
	}
}

void Damysus::respondMsgPrepareDamysus(Justification justification_MsgPrepare)
{
	// Create [justification_MsgPrecommit]
	Justification justification_MsgPrecommit = this->saveMsgPrepareDamysus(justification_MsgPrepare);

	// Create [msgPrecommit]
	RoundData roundData_MsgPrecommit = justification_MsgPrecommit.getRoundData();
	Signs signs_MsgPrecommit = justification_MsgPrecommit.getSigns();
	MsgPrecommitDamysus msgPrecommit = MsgPrecommitDamysus(roundData_MsgPrecommit, signs_MsgPrecommit);

	// Send [msgPrecommit] to leader
	Peers recipients = this->keepFromPeers(this->getCurrentLeader());
	this->sendMsgPrecommitDamysus(msgPrecommit, recipients);
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Sent MsgPrecommit to leader: " << msgPrecommit.toPrint() << COLOUR_NORMAL << std::endl;
	}
}

// Main functions
int Damysus::initializeSGX()
{
	// Initializing enclave
	if (initialize_enclave(&global_eid, "enclave.token", "enclave.signed.so") < 0)
	{
		std::cout << this->printReplicaId() << "Failed to initialize enclave" << std::endl;
		return 1;
	}
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Initialized enclave" << COLOUR_NORMAL << std::endl;
	}

	// Initializing variables
	std::set<ReplicaID> replicaIds = this->nodes.getReplicaIds();
	unsigned int num = replicaIds.size();
	Pids_t others;
	others.num_nodes = num;
	unsigned int i = 0;
	for (std::set<ReplicaID>::iterator it = replicaIds.begin(); it != replicaIds.end(); it++, i++)
	{
		others.pids[i] = *it;
	}

	sgx_status_t extra;
	sgx_status_t status_t;
	status_t = initializeVariables_t(global_eid, &extra, &(this->replicaId), &others, &(this->generalQuorumSize), &(this->trustedQuorumSize));
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Enclave variables are initialized." << COLOUR_NORMAL << std::endl;
	}
	return 0;
}

void Damysus::getStarted()
{
	if (DEBUG_BASIC)
	{
		std::cout << COLOUR_RED << this->printReplicaId() << "Starting" << COLOUR_NORMAL << std::endl;
	}
	startTime = std::chrono::steady_clock::now();
	startView = std::chrono::steady_clock::now();

	// Send [msgNewview] to the leader of the current view
	ReplicaID leader = this->getCurrentLeader();
	Peers recipients = this->keepFromPeers(leader);

	Justification justification_MsgNewview = this->initializeMsgNewviewDamysus();
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Initial justification: " << justification_MsgNewview.toPrint() << COLOUR_NORMAL << std::endl;
	}
	RoundData roundData_MsgNewview = justification_MsgNewview.getRoundData();
	Signs signs_MsgNewview = justification_MsgNewview.getSigns();
	MsgNewviewDamysus msgNewview = MsgNewviewDamysus(roundData_MsgNewview, signs_MsgNewview);
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Starting with: " << msgNewview.toPrint() << COLOUR_NORMAL << std::endl;
	}
	if (this->amCurrentLeader())
	{
		this->handleMsgNewviewDamysus(msgNewview);
	}
	else
	{
		this->sendMsgNewviewDamysus(msgNewview, recipients);
	}
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Sent MsgNewview to leader[" << leader << "]" << COLOUR_NORMAL << std::endl;
	}
}

void Damysus::startNewViewDamysus()
{
	// Generate [justification_MsgNewview] until one for the next view
	Justification justification_MsgNewview = this->initializeMsgNewviewDamysus();
	View proposeView_MsgNewview = justification_MsgNewview.getRoundData().getProposeView();
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Generating justification: " << justification_MsgNewview.toPrint() << COLOUR_NORMAL << std::endl;
	}
	while (proposeView_MsgNewview <= this->view)
	{
		justification_MsgNewview = this->initializeMsgNewviewDamysus();
		proposeView_MsgNewview = justification_MsgNewview.getRoundData().getProposeView();
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Generating justification: " << justification_MsgNewview.toPrint() << COLOUR_NORMAL << std::endl;
		}
	}

	// Increase the view
	this->view++;

	// Start the timer
	this->setTimer();

	RoundData roundData_MsgNewview = justification_MsgNewview.getRoundData();
	Phase phase_MsgNewview = roundData_MsgNewview.getPhase();
	Signs signs_MsgNewview = justification_MsgNewview.getSigns();
	if (proposeView_MsgNewview == this->view && phase_MsgNewview == PHASE_NEWVIEW)
	{
		MsgNewviewDamysus msgNewview = MsgNewviewDamysus(roundData_MsgNewview, signs_MsgNewview);
		if (this->amCurrentLeader())
		{
			this->handleEarlierMessagesDamysus();
			this->handleMsgNewviewDamysus(msgNewview);
		}
		else
		{
			ReplicaID leader = this->getCurrentLeader();
			Peers recipients = this->keepFromPeers(leader);
			this->sendMsgNewviewDamysus(msgNewview, recipients);
			this->handleEarlierMessagesDamysus();
		}
	}
	else
	{
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Failed to start" << COLOUR_NORMAL << std::endl;
		}
	}
}

Block Damysus::createNewBlockDamysus(Hash hash)
{
	std::lock_guard<std::mutex> guard(mutexTransaction);
	Transaction transaction[MAX_NUM_TRANSACTIONS];
	unsigned int i = 0;

	// We fill the block we have with transactions we have received so far
	while (i < MAX_NUM_TRANSACTIONS && !this->transactions.empty())
	{
		transaction[i] = this->transactions.front();
		this->transactions.pop_front();
		i++;
	}

	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Filled block with " << i << " transactions" << COLOUR_NORMAL << std::endl;
	}

	unsigned int size = i;
	// We fill the rest with dummy transactions
	while (i < MAX_NUM_TRANSACTIONS)
	{
		transaction[i] = Transaction();
		i++;
	}
	Block block = Block(hash, size, transaction);
	return block;
}

void Damysus::executeBlockDamysus(RoundData roundData_MsgPrecommit)
{
	auto endView = std::chrono::steady_clock::now();
	double time = std::chrono::duration_cast<std::chrono::microseconds>(endView - startView).count();
	startView = endView;
	statistics.increaseExecuteViews();
	statistics.addViewTime(time);

	if (this->transactions.empty())
	{
		this->viewsWithoutNewTrans++;
	}
	else
	{
		this->viewsWithoutNewTrans = 0;
	}

	// Execute
	if (DEBUG_BASIC)
	{
		std::cout << COLOUR_RED << this->printReplicaId()
				  << "DAMYSUS-EXECUTE(" << this->view << "/" << std::to_string(this->numViews - 1) << ":" << time << ") "
				  << statistics.toString() << COLOUR_NORMAL << std::endl;
	}

	this->replyHash(roundData_MsgPrecommit.getProposeHash());
	if (this->timeToStop())
	{
		this->recordStatisticsDamysus();
	}
	else
	{
		this->startNewViewDamysus();
	}
}

bool Damysus::timeToStop()
{
	bool b = this->numViews > 0 && this->numViews <= this->view + 1;
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE
				  << this->printReplicaId()
				  << "timeToStop = " << b
				  << "; numViews = " << this->numViews
				  << "; viewsWithoutNewTrans = " << this->viewsWithoutNewTrans
				  << "; Transaction sizes = " << this->transactions.size()
				  << COLOUR_NORMAL << std::endl;
	}
	if (DEBUG_HELP)
	{
		if (b)
		{
			std::cout << COLOUR_BLUE
					  << this->printReplicaId()
					  << "numViews = " << this->numViews
					  << "; viewsWithoutNewTrans = " << this->viewsWithoutNewTrans
					  << "; Transaction sizes = " << this->transactions.size()
					  << COLOUR_NORMAL << std::endl;
		}
	}
	return b;
}

void Damysus::recordStatisticsDamysus()
{
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "DONE - Printing statistics" << COLOUR_NORMAL << std::endl;
	}

	unsigned int quant1 = 0;
	unsigned int quant2 = 10;

	// Throughput
	Times totalView = statistics.getTotalViewTime();
	double kopsView = ((totalView.num) * (MAX_NUM_TRANSACTIONS) * 1.0) / 1000;
	double secsView = totalView.total / (1000 * 1000);
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId()
				  << "VIEW| View = " << this->view
				  << "; Kops = " << kopsView
				  << "; Secs = " << secsView
				  << "; num = " << totalView.num
				  << COLOUR_NORMAL << std::endl;
	}
	double throughputView = kopsView / secsView;

	// Handle
	Times totalHandle = statistics.getTotalHandleTime();
	double kopsHandle = ((totalHandle.num) * (MAX_NUM_TRANSACTIONS) * 1.0) / 1000;
	double secsHandle = totalHandle.total / (1000 * 1000);
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId()
				  << "HANDLE| View = " << this->view
				  << "; Kops = " << kopsHandle
				  << "; Secs = " << secsHandle
				  << "; num = " << totalHandle.num
				  << COLOUR_NORMAL << std::endl;
	}

	// Latency
	double latencyView = (totalView.total / totalView.num / 1000); // milli-seconds spent on views

	// Handle
	double handle = (totalHandle.total / 1000); // milli-seconds spent on handling messages

	std::ofstream fileVals(statisticsValues);
	fileVals << std::to_string(throughputView)
			 << " " << std::to_string(latencyView)
			 << " " << std::to_string(handle);
	fileVals.close();

	// Done
	std::ofstream fileDone(statisticsDone);
	fileDone.close();
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Printing DONE file: " << statisticsDone << COLOUR_NORMAL << std::endl;
	}
}

// Constuctor
Damysus::Damysus(KeysFunctions keysFunctions, ReplicaID replicaId, unsigned int numGeneralReplicas, unsigned int numTrustedReplicas, unsigned int numReplicas, unsigned int numViews, unsigned int numFaults, double leaderChangeTime, Nodes nodes, Key privateKey, PeerNet::Config peerNetConfig, ClientNet::Config clientNetConfig) : peerNet(peerEventContext, peerNetConfig), clientNet(requestEventContext, clientNetConfig)
{
	this->keysFunction = keysFunctions;
	this->replicaId = replicaId;
	this->numGeneralReplicas = numGeneralReplicas;
	this->numTrustedReplicas = numTrustedReplicas;
	this->numReplicas = numReplicas;
	this->numViews = numViews;
	this->numFaults = numFaults;
	this->leaderChangeTime = leaderChangeTime;
	this->nodes = nodes;
	this->privateKey = privateKey;
	this->generalQuorumSize = 0;
	this->trustedQuorumSize = this->numReplicas - this->numFaults;
	this->view = 0;

	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Starting handler" << COLOUR_NORMAL << std::endl;
	}
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Trusted quorum size: " << this->trustedQuorumSize << COLOUR_NORMAL << std::endl;
	}

	// Trusted Functions
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Initializing TEE" << COLOUR_NORMAL << std::endl;
	}
	this->initializeSGX();
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Initialized TEE" << COLOUR_NORMAL << std::endl;
	}

	// Salticidae
	this->requestCall = new salticidae::ThreadCall(this->requestEventContext);

	// The client event context handles replies through [executionQueue]
	this->executionQueue.reg_handler(this->requestEventContext, [this](ExecutionQueue &executionQueue)
									 {
										std::pair<TransactionID,ClientID> transactionPair;
										while (executionQueue.try_dequeue(transactionPair))
										{
											TransactionID transactionId = transactionPair.first;
											ClientID clientId = transactionPair.second;
											Clients::iterator itClient = this->clients.find(clientId);
											if (itClient != this->clients.end())
											{
												ClientInformation clientInformation = itClient->second;
												MsgReply msgReply = MsgReply(transactionId);
												ClientNet::conn_t recipient = std::get<3>(clientInformation);
												if (DEBUG_HELP)
												{
													std::cout << COLOUR_BLUE << this->printReplicaId() << "Sending reply to " << clientId << ": " << msgReply.toPrint() << COLOUR_NORMAL << std::endl;
												}
												try
												{
													this->clientNet.send_msg(msgReply,recipient);
													(this->clients)[clientId]=std::make_tuple(std::get<0>(clientInformation),std::get<1>(clientInformation),std::get<2>(clientInformation)+1,std::get<3>(clientInformation));
												}
												catch(std::exception &error)
												{
													if (DEBUG_HELP)
													{
														std::cout << COLOUR_BLUE << this->printReplicaId() << "Couldn't send reply to " << clientId << ": " << msgReply.toPrint() << "; " << error.what() << COLOUR_NORMAL << std::endl;
													}
												}
											}
											else
											{
												if (DEBUG_HELP)
												{
													std::cout << COLOUR_BLUE << this->printReplicaId() << "Couldn't reply to unknown client: " << clientId << COLOUR_NORMAL << std::endl;
												}
											}
										}
										return false; });

	this->timer = salticidae::TimerEvent(peerEventContext, [this](salticidae::TimerEvent &)
										 {
                                                if (DEBUG_HELP)
												{
													std::cout << COLOUR_BLUE << this->printReplicaId() << "timer ran out" << COLOUR_NORMAL << std::endl;
													this->startNewViewDamysus();
													this->timer.del();
													this->timer.add(this->leaderChangeTime);
												} });

	Host host = "127.0.0.1";
	PortID replicaPort = 8760 + this->replicaId;
	PortID clientPort = 9760 + this->replicaId;

	Node *thisNode = nodes.find(this->replicaId);
	if (thisNode != NULL)
	{
		host = thisNode->getHost();
		replicaPort = thisNode->getReplicaPort();
		clientPort = thisNode->getClientPort();
	}
	else
	{
		std::cout << COLOUR_RED << this->printReplicaId() << "Couldn't find own information among nodes" << COLOUR_NORMAL << std::endl;
	}

	salticidae::NetAddr peerAddress = salticidae::NetAddr(host + ":" + std::to_string(replicaPort));
	this->peerNet.start();
	this->peerNet.listen(peerAddress);

	salticidae::NetAddr clientAddress = salticidae::NetAddr(host + ":" + std::to_string(clientPort));
	this->clientNet.start();
	this->clientNet.listen(clientAddress);

	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Connecting..." << COLOUR_NORMAL << std::endl;
	}

	for (size_t j = 0; j < this->numReplicas; j++)
	{
		if (this->replicaId != j)
		{
			Node *otherNode = nodes.find(j);
			if (otherNode != NULL)
			{
				salticidae::NetAddr otherNodeAddress = salticidae::NetAddr(otherNode->getHost() + ":" + std::to_string(otherNode->getReplicaPort()));
				salticidae::PeerId otherPeerId{otherNodeAddress};
				this->peerNet.add_peer(otherPeerId);
				this->peerNet.set_peer_addr(otherPeerId, otherNodeAddress);
				this->peerNet.conn_peer(otherPeerId);
				if (DEBUG_HELP)
				{
					std::cout << COLOUR_BLUE << this->printReplicaId() << "Added peer: " << j << COLOUR_NORMAL << std::endl;
				}
				this->peers.push_back(std::make_pair(j, otherPeerId));
			}
			else
			{
				std::cout << COLOUR_RED << this->printReplicaId() << "Couldn't find " << j << "'s information among nodes" << COLOUR_NORMAL << std::endl;
			}
		}
	}

	this->clientNet.reg_handler(salticidae::generic_bind(&Damysus::receiveMsgStartDamysus, this, _1, _2));
	this->clientNet.reg_handler(salticidae::generic_bind(&Damysus::receiveMsgTransactionDamysus, this, _1, _2));

	this->peerNet.reg_handler(salticidae::generic_bind(&Damysus::receiveMsgNewviewDamysus, this, _1, _2));
	this->peerNet.reg_handler(salticidae::generic_bind(&Damysus::receiveMsgLdrprepareDamysus, this, _1, _2));
	this->peerNet.reg_handler(salticidae::generic_bind(&Damysus::receiveMsgPrepareDamysus, this, _1, _2));
	this->peerNet.reg_handler(salticidae::generic_bind(&Damysus::receiveMsgPrecommitDamysus, this, _1, _2));

	// Statistics
	auto timeNow = std::chrono::system_clock::now();
	std::time_t time = std::chrono::system_clock::to_time_t(timeNow);
	struct tm y2k = {0};
	double seconds = difftime(time, mktime(&y2k));
	statisticsValues = "results/values-" + std::to_string(this->replicaId) + "-" + std::to_string(seconds);
	statisticsDone = "results/done-" + std::to_string(this->replicaId) + "-" + std::to_string(seconds);
	statistics.setReplicaId(this->replicaId);

	auto peerShutDown = [&](int)
	{ peerEventContext.stop(); };
	salticidae::SigEvent peerSigTerm = salticidae::SigEvent(peerEventContext, peerShutDown);
	peerSigTerm.add(SIGTERM);

	auto clientShutDown = [&](int)
	{ requestEventContext.stop(); };
	salticidae::SigEvent clientSigTerm = salticidae::SigEvent(requestEventContext, clientShutDown);
	clientSigTerm.add(SIGTERM);

	requestThread = std::thread([this]()
								{ requestEventContext.dispatch(); });

	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Dispatching" << COLOUR_NORMAL << std::endl;
	}
	peerEventContext.dispatch();
}