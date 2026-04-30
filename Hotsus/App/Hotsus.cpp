#include "Hotsus.h"

// Local variables
Time startTime = std::chrono::steady_clock::now();
Time startView = std::chrono::steady_clock::now();
Time currentTime;
std::string statisticsValues;
std::string statisticsDone;
Statistics statistics;
HotsusBasic hotsusBasic;
sgx_enclave_id_t global_eid = 0;

// Opcodes
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

// Store [exproposal] in [exproposal_t]
void setExproposal(Proposal<Justification> exproposal, Exproposal_t *exproposal_t)
{
	setJustification(exproposal.getCertification(), &(exproposal_t->justification));
	setBlock(exproposal.getBlock(), &(exproposal_t->block));
}

void TEE_Print(const char *text)
{
	printf("%s\n", text);
}

// Print functions
std::string Hotsus::printReplicaId()
{
	return "[" + std::to_string(this->replicaId) + "-" + std::to_string(this->view) + "]";
}

void Hotsus::printNowTime(std::string msg)
{
	auto now = std::chrono::steady_clock::now();
	double time = std::chrono::duration_cast<std::chrono::microseconds>(now - startView).count();
	double etime = (statistics.getTotalViewTime().total + time) / (1000 * 1000);
	std::cout << COLOUR_BLUE << this->printReplicaId() << msg << " @ " << etime << COLOUR_NORMAL << std::endl;
}

void Hotsus::printClientInfo()
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

std::string Hotsus::recipients2string(Peers recipients)
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
unsigned int Hotsus::getLeaderOf(View view)
{
	unsigned int leader;
	if (this->protocol == PROTOCOL_HOTSTUFF)
	{
		leader = (this->view + this->numGeneralReplicas - 1) % (this->numTrustedReplicas + this->numGeneralReplicas);
	}
	else if (this->protocol == PROTOCOL_DAMYSUS)
	{
		leader = this->trustedGroup.getGroup()[this->view % this->trustedGroup.getSize()];
	}
	return leader;
}

unsigned int Hotsus::getCurrentLeader()
{
	unsigned int leader = this->getLeaderOf(this->view);
	return leader;
}

bool Hotsus::amLeaderOf(View view)
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

bool Hotsus::amCurrentLeader()
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

std::vector<ReplicaID> Hotsus::getGeneralReplicaIds()
{
	std::vector<ReplicaID> generalReplicaIds;
	for (unsigned int i = 0; i < this->numGeneralReplicas; i++)
	{
		generalReplicaIds.push_back(i);
	}
	return generalReplicaIds;
}

bool Hotsus::amGeneralReplicaIds()
{
	std::vector<ReplicaID> generalReplicaIds = this->getGeneralReplicaIds();
	for (std::vector<ReplicaID>::iterator itReplica = generalReplicaIds.begin(); itReplica != generalReplicaIds.end(); itReplica++)
	{
		ReplicaID generalReplicaId = *itReplica;
		if (this->replicaId == generalReplicaId)
		{
			return true;
		}
	}
	return false;
}

bool Hotsus::isGeneralReplicaIds(ReplicaID replicaId)
{
	std::vector<ReplicaID> generalReplicaIds = this->getGeneralReplicaIds();
	for (std::vector<ReplicaID>::iterator itReplica = generalReplicaIds.begin(); itReplica != generalReplicaIds.end(); itReplica++)
	{
		ReplicaID generalReplicaId = *itReplica;
		if (replicaId == generalReplicaId)
		{
			return true;
		}
	}
	return false;
}

bool Hotsus::amTrustedReplicaIds()
{
	Group trustedReplicaIds = this->trustedGroup;
	ReplicaID *trustedReplicGroup = trustedReplicaIds.getGroup();
	for (int i = 0; i < trustedReplicaIds.getSize(); i++)
	{
		ReplicaID trustedReplicaId = trustedReplicGroup[i];
		if (this->replicaId == trustedReplicaId)
		{
			return true;
		}
	}
	return false;
}

bool Hotsus::isTrustedReplicaIds(ReplicaID replicaId)
{
	Group trustedReplicaIds = this->trustedGroup;
	ReplicaID *trustedReplicGroup = trustedReplicaIds.getGroup();
	for (int i = 0; i < trustedReplicaIds.getSize(); i++)
	{
		ReplicaID trustedReplicaId = trustedReplicGroup[i];
		if (replicaId == trustedReplicaId)
		{
			return true;
		}
	}
	return false;
}

Peers Hotsus::removeFromPeers(ReplicaID replicaId)
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

Peers Hotsus::removeFromPeers(std::vector<ReplicaID> generalReplicaIds)
{
	Peers peers;
	for (Peers::iterator itPeers = this->peers.begin(); itPeers != this->peers.end(); itPeers++)
	{
		Peer peer = *itPeers;
		bool tag = true;
		for (std::vector<ReplicaID>::iterator itReplica = generalReplicaIds.begin(); itReplica != generalReplicaIds.end(); itReplica++)
		{
			ReplicaID replicaId = *itReplica;
			if (std::get<0>(peer) == replicaId)
			{
				tag = false;
			}
		}
		if (tag)
		{
			peers.push_back(peer);
		}
	}
	return peers;
}

Peers Hotsus::removeFromTrustedPeers(ReplicaID replicaId)
{
	Peers peers;
	for (Peers::iterator itPeers = this->peers.begin(); itPeers != this->peers.end(); itPeers++)
	{
		Peer peer = *itPeers;
		if (std::get<0>(peer) != replicaId && this->isTrustedReplicaIds(std::get<0>(peer)))
		{
			peers.push_back(peer);
		}
	}
	return peers;
}

Peers Hotsus::keepFromPeers(ReplicaID replicaId)
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

Peers Hotsus::keepFromTrustedPeers(ReplicaID replicaId)
{
	Peers peers;
	for (Peers::iterator itPeers = this->peers.begin(); itPeers != this->peers.end(); itPeers++)
	{
		Peer peer = *itPeers;
		if (std::get<0>(peer) == replicaId && this->isTrustedReplicaIds(std::get<0>(peer)))
		{
			peers.push_back(peer);
		}
	}
	return peers;
}

std::vector<salticidae::PeerId> Hotsus::getPeerIds(Peers recipients)
{
	std::vector<salticidae::PeerId> returnPeerId;
	for (Peers::iterator it = recipients.begin(); it != recipients.end(); it++)
	{
		Peer peer = *it;
		returnPeerId.push_back(std::get<1>(peer));
	}
	return returnPeerId;
}

void Hotsus::setTimer()
{
	this->timer.del();
	this->timer.add(this->leaderChangeTime);
	this->timerView = this->view;
}

void Hotsus::changeSwitcher()
{
	if (!this->amGeneralReplicaIds())
	{
		sgx_status_t extra_t;
		sgx_status_t status_t;
		status_t = TEE_changeSwitcher(global_eid, &extra_t);
	}
	else
	{
		hotsusBasic.changeSwitcher();
	}
}

void Hotsus::changeAuthenticator()
{
	if (!this->amGeneralReplicaIds())
	{
		sgx_status_t extra_t;
		sgx_status_t status_t;
		status_t = TEE_changeAuthenticator(global_eid, &extra_t);
	}
	else
	{
		hotsusBasic.changeAuthenticator();
	}
}

void Hotsus::setTrustedQuorumSize()
{
	if (!this->amGeneralReplicaIds())
	{
		sgx_status_t extra_t;
		sgx_status_t status_t;
		status_t = setTrustedQuorumSize_t(global_eid, &extra_t, &(this->trustedQuorumSize));
	}
	else
	{
		hotsusBasic.setTrustedQuorumSize(this->trustedQuorumSize);
	}
}

// Reply to clients
void Hotsus::replyTransactions(Transaction *transactions)
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

void Hotsus::replyHash(Hash hash)
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
bool Hotsus::verifyJustificationHotsus(Justification justification)
{
	bool b;
	if (!this->amGeneralReplicaIds())
	{
		Justification_t justification_t;
		setJustification(justification, &justification_t);
		sgx_status_t extra_t;
		sgx_status_t status_t;
		status_t = TEE_verifyJustificationHotsus(global_eid, &extra_t, &justification_t, &b);
	}
	else
	{
		b = hotsusBasic.verifyJustification(this->nodes, justification);
	}
	return b;
}

bool Hotsus::verifyProposalHotsus(Proposal<Accumulator> proposal, Signs signs)
{
	bool b;
	if (!this->amGeneralReplicaIds())
	{
		Proposal_t proposal_t;
		setProposal(proposal, &proposal_t);
		Signs_t signs_t;
		setSigns(signs, &signs_t);
		sgx_status_t extra_t;
		sgx_status_t status_t;
		status_t = TEE_verifyProposalHotsus(global_eid, &extra_t, &proposal_t, &signs_t, &b);
	}
	else
	{
		b = hotsusBasic.verifyProposal(this->nodes, proposal, signs);
	}
	return b;
}

bool Hotsus::verifyExproposalHotsus(Proposal<Justification> exproposal, Signs signs)
{
	bool b;
	if (!this->amGeneralReplicaIds())
	{
		Exproposal_t exproposal_t;
		setExproposal(exproposal, &exproposal_t);
		Signs_t signs_t;
		setSigns(signs, &signs_t);
		sgx_status_t extra_t;
		sgx_status_t status_t;
		status_t = TEE_verifyExproposalHotsus(global_eid, &extra_t, &exproposal_t, &signs_t, &b);
	}
	else
	{
		b = hotsusBasic.verifyExproposal(this->nodes, exproposal, signs);
	}
	return b;
}

Justification Hotsus::initializeMsgNewviewHotsus()
{
	Justification justification_MsgNewview = Justification();
	if (!this->amGeneralReplicaIds())
	{
		Justification_t justification_MsgNewview_t;
		sgx_status_t extra_t;
		sgx_status_t status_t;
		status_t = TEE_initializeMsgNewviewHotsus(global_eid, &extra_t, &justification_MsgNewview_t);
		justification_MsgNewview = getJustification(&justification_MsgNewview_t);
	}
	else
	{
		justification_MsgNewview = hotsusBasic.initializeMsgNewview();
	}
	return justification_MsgNewview;
}

Accumulator Hotsus::initializeAccumulatorHotsus(Justification justifications_MsgNewview[MAX_NUM_SIGNATURES])
{
	Justifications_t justifications_MsgNewview_t;
	setJustifications(justifications_MsgNewview, &justifications_MsgNewview_t);
	Accumulator_t accumulator_MsgLdrprepare_t;
	sgx_status_t extra_t;
	sgx_status_t status_t;
	status_t = TEE_initializeAccumulatorHotsus(global_eid, &extra_t, &justifications_MsgNewview_t, &accumulator_MsgLdrprepare_t);
	Accumulator accumulator_MsgLdrprepare = getAccumulator(&accumulator_MsgLdrprepare_t);
	return accumulator_MsgLdrprepare;
}

Signs Hotsus::initializeMsgLdrprepareHotsus(Proposal<Accumulator> proposal_MsgLdrprepare)
{
	Proposal_t proposal_MsgLdrprepare_t;
	setProposal(proposal_MsgLdrprepare, &proposal_MsgLdrprepare_t);
	Signs_t signs_MsgLdrprepare_t;
	sgx_status_t extra_t;
	sgx_status_t status_t;
	status_t = TEE_initializeMsgLdrprepareHotsus(global_eid, &extra_t, &proposal_MsgLdrprepare_t, &signs_MsgLdrprepare_t);
	Signs signs_MsgLdrprepare = getSigns(&signs_MsgLdrprepare_t);
	return signs_MsgLdrprepare;
}

Justification Hotsus::respondMsgLdrprepareProposalHotsus(Hash proposeHash, Accumulator accumulator_MsgLdrprepare)
{
	Justification justification_MsgPrepare = Justification();
	if (!this->amGeneralReplicaIds())
	{
		Accumulator_t accumulator_MsgLdrprepare_t;
		setAccumulator(accumulator_MsgLdrprepare, &accumulator_MsgLdrprepare_t);
		Hash_t proposeHash_t;
		setHash(proposeHash, &proposeHash_t);
		Justification_t justification_MsgPrepare_t;
		sgx_status_t extra_t;
		sgx_status_t status_t;
		status_t = TEE_respondProposalHotsus(global_eid, &extra_t, &proposeHash_t, &accumulator_MsgLdrprepare_t, &justification_MsgPrepare_t);
		justification_MsgPrepare = getJustification(&justification_MsgPrepare_t);
	}
	else
	{
		justification_MsgPrepare = hotsusBasic.respondProposal(this->nodes, proposeHash, accumulator_MsgLdrprepare);
	}
	return justification_MsgPrepare;
}

Justification Hotsus::saveMsgPrepareHotsus(Justification justification_MsgPrepare)
{
	Justification_t justification_MsgPrepare_t;
	setJustification(justification_MsgPrepare, &justification_MsgPrepare_t);
	Justification_t justification_MsgPrecommit_t;
	sgx_status_t extra;
	sgx_status_t status_t;
	status_t = TEE_saveMsgPrepareHotsus(global_eid, &extra, &justification_MsgPrepare_t, &justification_MsgPrecommit_t);
	Justification justification_MsgPrecommit = getJustification(&justification_MsgPrecommit_t);
	return justification_MsgPrecommit;
}

void Hotsus::skipRoundHotsus()
{
	hotsusBasic.skipRound();
}

Justification Hotsus::initializeMsgExnewviewHotsus()
{
	Justification justification_MsgExnewview = Justification();
	if (!this->amGeneralReplicaIds())
	{
		Justification_t justification_MsgExnewview_t;
		sgx_status_t extra_t;
		sgx_status_t status_t;
		status_t = TEE_initializeMsgExnewviewHotsus(global_eid, &extra_t, &justification_MsgExnewview_t);
		justification_MsgExnewview = getJustification(&justification_MsgExnewview_t);
	}
	else
	{
		justification_MsgExnewview = hotsusBasic.initializeMsgExnewview();
	}
	return justification_MsgExnewview;
}

Justification Hotsus::respondMsgExnewviewProposalHotsus(Hash proposeHash, Justification justification_MsgExnewview)
{
	Justification justification_MsgExprepare = Justification();
	if (!this->amGeneralReplicaIds())
	{
		Hash_t proposeHash_t;
		setHash(proposeHash, &proposeHash_t);
		Justification_t justification_MsgExnewview_t;
		setJustification(justification_MsgExnewview, &justification_MsgExnewview_t);
		Justification_t justification_MsgExprepare_t;
		sgx_status_t extra_t;
		sgx_status_t status_t;
		status_t = TEE_respondExproposalHotsus(global_eid, &extra_t, &proposeHash_t, &justification_MsgExnewview_t, &justification_MsgExprepare_t);
		justification_MsgExprepare = getJustification(&justification_MsgExprepare_t);
	}
	else
	{
		justification_MsgExprepare = hotsusBasic.respondExproposal(this->nodes, proposeHash, justification_MsgExnewview);
	}
	return justification_MsgExprepare;
}

Signs Hotsus::initializeMsgExldrprepareHotsus(Proposal<Justification> proposal_MsgExldrprepare)
{
	Signs signs_MsgExldrprepare = Signs();
	if (!this->amGeneralReplicaIds())
	{
		Exproposal_t proposal_MsgExldrprepare_t;
		setExproposal(proposal_MsgExldrprepare, &proposal_MsgExldrprepare_t);
		Signs_t signs_MsgExldrprepare_t;
		sgx_status_t extra_t;
		sgx_status_t status_t;
		status_t = TEE_initializeMsgExldrprepareHotsus(global_eid, &extra_t, &proposal_MsgExldrprepare_t, &signs_MsgExldrprepare_t);
		signs_MsgExldrprepare = getSigns(&signs_MsgExldrprepare_t);
	}
	else
	{
		signs_MsgExldrprepare = hotsusBasic.initializeMsgExldrprepare(proposal_MsgExldrprepare);
	}
	return signs_MsgExldrprepare;
}

Justification Hotsus::respondMsgExldrprepareProposalHotsus(Hash proposeHash, Justification justification_MsgExnewview)
{
	Justification justification_MsgExprepare = Justification();
	if (!this->amGeneralReplicaIds())
	{
		Hash_t proposeHash_t;
		setHash(proposeHash, &proposeHash_t);
		Justification_t justification_MsgExnewview_t;
		setJustification(justification_MsgExnewview, &justification_MsgExnewview_t);
		Justification_t justification_MsgExprepare_t;
		sgx_status_t extra_t;
		sgx_status_t status_t;
		status_t = TEE_respondExproposalHotsus(global_eid, &extra_t, &proposeHash_t, &justification_MsgExnewview_t, &justification_MsgExprepare_t);
		justification_MsgExprepare = getJustification(&justification_MsgExprepare_t);
	}
	else
	{
		justification_MsgExprepare = hotsusBasic.respondExproposal(this->nodes, proposeHash, justification_MsgExnewview);
	}
	return justification_MsgExprepare;
}

Justification Hotsus::saveMsgExprepareHotsus(Justification justification_MsgExprepare)
{
	Justification justification_MsgExprecommit = Justification();
	if (!this->amGeneralReplicaIds())
	{
		Justification_t justification_MsgExprepare_t;
		setJustification(justification_MsgExprepare, &justification_MsgExprepare_t);
		Justification_t justification_MsgExprecommit_t;
		sgx_status_t extra_t;
		sgx_status_t status_t;
		status_t = TEE_saveMsgExprepareHotsus(global_eid, &extra_t, &justification_MsgExprepare_t, &justification_MsgExprecommit_t);
		justification_MsgExprecommit = getJustification(&justification_MsgExprecommit_t);
	}
	else
	{
		justification_MsgExprecommit = hotsusBasic.saveMsgExprepare(this->nodes, justification_MsgExprepare);
	}
	return justification_MsgExprecommit;
}

Justification Hotsus::lockMsgExprecommitHotsus(Justification justification_MsgExprecommit)
{
	Justification justification_MsgExcommit = Justification();
	if (!this->amGeneralReplicaIds())
	{
		Justification_t justification_MsgExprecommit_t;
		setJustification(justification_MsgExprecommit, &justification_MsgExprecommit_t);
		Justification_t justification_MsgExcommit_t;
		sgx_status_t extra_t;
		sgx_status_t status_t;
		status_t = TEE_lockMsgExprecommitHotsus(global_eid, &extra_t, &justification_MsgExprecommit_t, &justification_MsgExcommit_t);
		justification_MsgExcommit = getJustification(&justification_MsgExcommit_t);
	}
	else
	{
		justification_MsgExcommit = hotsusBasic.lockMsgExprecommit(this->nodes, justification_MsgExprecommit);
	}
	return justification_MsgExcommit;
}

Accumulator Hotsus::initializeAccumulator(std::set<MsgNewviewHotsus> msgNewviews)
{
	Justification justifications_MsgNewview[MAX_NUM_SIGNATURES];
	unsigned int i = 0;
	for (std::set<MsgNewviewHotsus>::iterator it = msgNewviews.begin(); it != msgNewviews.end() && i < MAX_NUM_SIGNATURES; it++, i++)
	{
		MsgNewviewHotsus msgNewview = *it;
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
	accumulator_MsgLdrprepare = this->initializeAccumulatorHotsus(justifications_MsgNewview);
	return accumulator_MsgLdrprepare;
}

// Receive messages
void Hotsus::receiveMsgStartHotsus(MsgStart msgStart, const ClientNet::conn_t &conn)
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

void Hotsus::receiveMsgTransactionHotsus(MsgTransaction msgTransaction, const ClientNet::conn_t &conn)
{
	this->handleMsgTransaction(msgTransaction);
}

void Hotsus::receiveMsgNewviewHotsus(MsgNewviewHotsus msgNewview, const PeerNet::conn_t &conn)
{
	this->handleMsgNewviewHotsus(msgNewview);
}

void Hotsus::receiveMsgLdrprepareHotsus(MsgLdrprepareHotsus msgLdrprepare, const PeerNet::conn_t &conn)
{
	this->handleMsgLdrprepareHotsus(msgLdrprepare);
}

void Hotsus::receiveMsgPrepareHotsus(MsgPrepareHotsus msgPrepare, const PeerNet::conn_t &conn)
{
	this->handleMsgPrepareHotsus(msgPrepare);
}

void Hotsus::receiveMsgPrecommitHotsus(MsgPrecommitHotsus msgPrecommit, const PeerNet::conn_t &conn)
{
	this->handleMsgPrecommitHotsus(msgPrecommit);
}

void Hotsus::receiveMsgExnewviewHotsus(MsgExnewviewHotsus msgExnewview, const PeerNet::conn_t &conn)
{
	this->handleMsgExnewviewHotsus(msgExnewview);
}

void Hotsus::receiveMsgExldrprepareHotsus(MsgExldrprepareHotsus msgExldrprepare, const PeerNet::conn_t &conn)
{
	this->handleMsgExldrprepareHotsus(msgExldrprepare);
}

void Hotsus::receiveMsgExprepareHotsus(MsgExprepareHotsus msgExprepare, const PeerNet::conn_t &conn)
{
	this->handleMsgExprepareHotsus(msgExprepare);
}

void Hotsus::receiveMsgExprecommitHotsus(MsgExprecommitHotsus msgExprecommit, const PeerNet::conn_t &conn)
{
	this->handleMsgExprecommitHotsus(msgExprecommit);
}

void Hotsus::receiveMsgExcommitHotsus(MsgExcommitHotsus msgExcommit, const PeerNet::conn_t &conn)
{
	this->handleMsgExcommitHotsus(msgExcommit);
}

// Send messages
void Hotsus::sendMsgNewviewHotsus(MsgNewviewHotsus msgNewview, Peers recipients)
{
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Sending: " << msgNewview.toPrint() << " -> " << this->recipients2string(recipients) << COLOUR_NORMAL << std::endl;
	}
	this->peerNet.multicast_msg(msgNewview, getPeerIds(recipients));
	if (DEBUG_TIME)
	{
		this->printNowTime("Sending MsgNewviewHotsus");
	}
}

void Hotsus::sendMsgLdrprepareHotsus(MsgLdrprepareHotsus msgLdrprepare, Peers recipients)
{
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Sending: " << msgLdrprepare.toPrint() << " -> " << this->recipients2string(recipients) << COLOUR_NORMAL << std::endl;
	}
	this->peerNet.multicast_msg(msgLdrprepare, getPeerIds(recipients));
	if (DEBUG_TIME)
	{
		this->printNowTime("Sending MsgLdrprepareHotsus");
	}
}

void Hotsus::sendMsgPrepareHotsus(MsgPrepareHotsus msgPrepare, Peers recipients)
{
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Sending: " << msgPrepare.toPrint() << " -> " << this->recipients2string(recipients) << COLOUR_NORMAL << std::endl;
	}
	this->peerNet.multicast_msg(msgPrepare, getPeerIds(recipients));
	if (DEBUG_TIME)
	{
		this->printNowTime("Sending MsgPrepareHotsus");
	}
}

void Hotsus::sendMsgPrecommitHotsus(MsgPrecommitHotsus msgPrecommit, Peers recipients)
{
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Sending: " << msgPrecommit.toPrint() << " -> " << this->recipients2string(recipients) << COLOUR_NORMAL << std::endl;
	}
	this->peerNet.multicast_msg(msgPrecommit, getPeerIds(recipients));
	if (DEBUG_TIME)
	{
		this->printNowTime("Sending MsgPrecommitHotsus");
	}
}

void Hotsus::sendMsgExnewviewHotsus(MsgExnewviewHotsus msgExnewview, Peers recipients)
{
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Sending: " << msgExnewview.toPrint() << " -> " << this->recipients2string(recipients) << COLOUR_NORMAL << std::endl;
	}
	this->peerNet.multicast_msg(msgExnewview, getPeerIds(recipients));
	if (DEBUG_TIME)
	{
		this->printNowTime("Sending MsgExnewviewHotsus");
	}
}

void Hotsus::sendMsgExldrprepareHotsus(MsgExldrprepareHotsus msgExldrprepare, Peers recipients)
{
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Sending: " << msgExldrprepare.toPrint() << " -> " << this->recipients2string(recipients) << COLOUR_NORMAL << std::endl;
	}
	this->peerNet.multicast_msg(msgExldrprepare, getPeerIds(recipients));
	if (DEBUG_TIME)
	{
		this->printNowTime("Sending MsgExldrprepareHotsus");
	}
}

void Hotsus::sendMsgExprepareHotsus(MsgExprepareHotsus msgExprepare, Peers recipients)
{
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Sending: " << msgExprepare.toPrint() << " -> " << this->recipients2string(recipients) << COLOUR_NORMAL << std::endl;
	}
	this->peerNet.multicast_msg(msgExprepare, getPeerIds(recipients));
	if (DEBUG_TIME)
	{
		this->printNowTime("Sending MsgExprepareHotsus");
	}
}

void Hotsus::sendMsgExprecommitHotsus(MsgExprecommitHotsus msgExprecommit, Peers recipients)
{
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Sending: " << msgExprecommit.toPrint() << " -> " << this->recipients2string(recipients) << COLOUR_NORMAL << std::endl;
	}
	this->peerNet.multicast_msg(msgExprecommit, getPeerIds(recipients));
	if (DEBUG_TIME)
	{
		this->printNowTime("Sending MsgExprecommitHotsus");
	}
}

void Hotsus::sendMsgExcommitHotsus(MsgExcommitHotsus msgExcommit, Peers recipients)
{
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Sending: " << msgExcommit.toPrint() << " -> " << this->recipients2string(recipients) << COLOUR_NORMAL << std::endl;
	}
	this->peerNet.multicast_msg(msgExcommit, getPeerIds(recipients));
	if (DEBUG_TIME)
	{
		this->printNowTime("Sending MsgExcommitHotsus");
	}
}

// Handle messages
void Hotsus::handleMsgTransaction(MsgTransaction msgTransaction)
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

void Hotsus::handleEarlierMessagesHotsus()
{
	// Check if there are enough messages to start the next view
	if (this->amCurrentLeader())
	{
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Leader handling earlier messages" << COLOUR_NORMAL << std::endl;
		}
		std::set<MsgNewviewHotsus> msgNewviews = this->log.getMsgNewviewHotsus(this->view, this->trustedQuorumSize);
		if (msgNewviews.size() == this->trustedQuorumSize)
		{
			this->initiateMsgNewviewHotsus();
		}
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Leader handled earlier messages" << COLOUR_NORMAL << std::endl;
		}
	}
	else
	{
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Replica handling earlier messages" << COLOUR_NORMAL << std::endl;
		}
		// Check if the view has already been locked
		Signs signs_MsgPrecommit = this->log.getMsgPrecommitHotsus(this->view, this->trustedQuorumSize);
		if (signs_MsgPrecommit.getSize() == this->trustedQuorumSize)
		{
			if (DEBUG_HELP)
			{
				std::cout << COLOUR_BLUE << this->printReplicaId() << "Catching up using MsgPrecommit certificate" << COLOUR_NORMAL << std::endl;
			}

			// Skip the prepare phase and pre-commit phase
			this->initializeMsgNewviewHotsus();
			this->initializeMsgNewviewHotsus();

			// Fill the block
			if (DEBUG_HELP)
			{
				std::cout << COLOUR_BLUE << this->printReplicaId() << "Fill the block with MsgLdrprepare" << COLOUR_NORMAL << std::endl;
			}
			MsgLdrprepareHotsus msgLdrprepare = this->log.firstMsgLdrprepareHotsus(this->view);
			if (msgLdrprepare.signs.getSize() == 1)
			{
				Proposal<Accumulator> proposal = msgLdrprepare.proposal;
				Block block = proposal.getBlock();
				this->blocks[this->view] = block;
			}

			// Execute the block
			Justification justification_MsgPrecommit = this->log.firstMsgPrecommitHotsus(this->view);
			RoundData roundData_MsgPrecommit = justification_MsgPrecommit.getRoundData();
			Signs signs_MsgPrecommit = justification_MsgPrecommit.getSigns();
			if (signs_MsgPrecommit.getSize() == this->trustedQuorumSize && this->verifyJustificationHotsus(justification_MsgPrecommit))
			{
				this->executeBlockHotsus(roundData_MsgPrecommit);
			}
		}
		else
		{
			Signs signs_MsgPrepare = this->log.getMsgPrepareHotsus(this->view, this->trustedQuorumSize);
			if (signs_MsgPrepare.getSize() == this->trustedQuorumSize)
			{
				if (DEBUG_HELP)
				{
					std::cout << COLOUR_BLUE << this->printReplicaId() << "Catching up using MsgPrepare certificate" << COLOUR_NORMAL << std::endl;
				}
				Justification justification_MsgPrepare = this->log.firstMsgPrepareHotsus(this->view);

				// Skip the prepare phase
				this->initializeMsgNewviewHotsus();

				// Fill the block
				if (DEBUG_HELP)
				{
					std::cout << COLOUR_BLUE << this->printReplicaId() << "Fill the block with MsgLdrprepare" << COLOUR_NORMAL << std::endl;
				}
				MsgLdrprepareHotsus msgLdrprepare = this->log.firstMsgLdrprepareHotsus(this->view);
				if (msgLdrprepare.signs.getSize() == 1)
				{
					Proposal<Accumulator> proposal = msgLdrprepare.proposal;
					Block block = proposal.getBlock();
					this->blocks[this->view] = block;
				}

				// Store [justification_MsgPrepare]
				this->respondMsgPrepareHotsus(justification_MsgPrepare);
			}
			else
			{
				MsgLdrprepareHotsus msgLdrprepare = this->log.firstMsgLdrprepareHotsus(this->view);

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
					if (!this->amGeneralReplicaIds())
					{
						this->respondMsgLdrprepareHotsus(accumulator_MsgLdrprepare, block);
					}
					else
					{
						if (DEBUG_HELP)
						{
							std::cout << COLOUR_BLUE << this->printReplicaId() << "Fill the block with MsgLdrprepare" << COLOUR_NORMAL << std::endl;
						}
						this->blocks[this->view] = block;
					}
				}
			}
		}
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Replica handled earlier messages" << COLOUR_NORMAL << std::endl;
		}
	}
}

void Hotsus::handleExtraEarlierMessagesHotsus()
{
	// Check if there are enough messages to start the next view
	if (this->amCurrentLeader())
	{
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Leader handling extra earlier messages" << COLOUR_NORMAL << std::endl;
		}
		Signs signs_MsgExnewview = this->log.getMsgExnewviewHotsus(this->view, this->generalQuorumSize);
		if (signs_MsgExnewview.getSize() == this->generalQuorumSize)
		{
			this->initiateMsgExnewviewHotsus();
		}
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Leader handled extra earlier messages" << COLOUR_NORMAL << std::endl;
		}
	}
	else
	{
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Replica handling extra earlier messages" << COLOUR_NORMAL << std::endl;
		}
		// Check if the view has already been locked
		Signs signs_MsgExprecommit = this->log.getMsgExprecommitHotsus(this->view, this->generalQuorumSize);
		if (signs_MsgExprecommit.getSize() == this->generalQuorumSize)
		{
			if (DEBUG_HELP)
			{
				std::cout << COLOUR_BLUE << this->printReplicaId() << "Catching up using MsgExprecommit certificate" << COLOUR_NORMAL << std::endl;
			}
			Justification justification_MsgExprecommit = this->log.firstMsgExprecommitHotsus(this->view);

			// Skip the prepare phase and pre-commit phase
			this->initializeMsgExnewviewHotsus();
			this->initializeMsgExnewviewHotsus();

			// Fill the block and check the trusted group
			MsgExldrprepareHotsus msgExldrprepare = this->log.firstMsgExldrprepareHotsus(this->view);
			if (msgExldrprepare.signs.getSize() == 1 && msgExldrprepare.group.getSize() > 0)
			{
				Proposal<Justification> proposal = msgExldrprepare.proposal;
				Block block = proposal.getBlock();
				this->blocks[this->view] = block;
				if (DEBUG_HELP)
				{
					std::cout << COLOUR_BLUE << this->printReplicaId() << "Fill the block with MsgExldrprepare" << COLOUR_NORMAL << std::endl;
				}

				this->trustedGroup = msgExldrprepare.group;
				this->changeAuthenticator();
				if (DEBUG_HELP)
				{
					std::cout << COLOUR_BLUE << this->printReplicaId() << "Update the trusted group" << COLOUR_NORMAL << std::endl;
				}
			}

			// Store [justification_MsgExprecommit]
			this->respondMsgExprecommitHotsus(justification_MsgExprecommit);

			Signs signs_MsgExcommit = this->log.getMsgExcommitHotsus(this->view, this->generalQuorumSize);
			if (signs_MsgExcommit.getSize() == this->generalQuorumSize)
			{
				Justification justification_MsgExcommit = this->log.firstMsgExcommitHotsus(this->view);
				this->executeExtraBlockHotsus(justification_MsgExcommit.getRoundData());
			}
		}
		else
		{
			Signs signs_MsgExprepare = this->log.getMsgExprepareHotsus(this->view, this->generalQuorumSize);
			if (signs_MsgExprepare.getSize() == this->generalQuorumSize)
			{
				if (DEBUG_HELP)
				{
					std::cout << COLOUR_BLUE << this->printReplicaId() << "Catching up using MsgExprepare certificate" << COLOUR_NORMAL << std::endl;
				}
				Justification justification_MsgExprepare = this->log.firstMsgExprepareHotsus(this->view);

				// Skip the prepare phase
				this->initializeMsgExnewviewHotsus();

				// Store [justification_MsgExprepare]
				this->respondMsgExprepareHotsus(justification_MsgExprepare);

				// Fill the block and check the trusted group
				MsgExldrprepareHotsus msgExldrprepare = this->log.firstMsgExldrprepareHotsus(this->view);
				if (msgExldrprepare.signs.getSize() == 1 && msgExldrprepare.group.getSize() > 0)
				{
					Proposal<Justification> proposal = msgExldrprepare.proposal;
					Block block = proposal.getBlock();
					this->blocks[this->view] = block;
					if (DEBUG_HELP)
					{
						std::cout << COLOUR_BLUE << this->printReplicaId() << "Fill the block with MsgExldrprepare" << COLOUR_NORMAL << std::endl;
					}

					this->trustedGroup = msgExldrprepare.group;
					this->changeAuthenticator();
					if (DEBUG_HELP)
					{
						std::cout << COLOUR_BLUE << this->printReplicaId() << "Update the trusted group" << COLOUR_NORMAL << std::endl;
					}
				}
			}
			else
			{
				MsgExldrprepareHotsus msgExldrprepare = this->log.firstMsgExldrprepareHotsus(this->view);

				// Check if the proposal has been stored
				if (msgExldrprepare.signs.getSize() == 1)
				{
					if (DEBUG_HELP)
					{
						std::cout << COLOUR_BLUE << this->printReplicaId() << "Catching up using MsgExldrprepare proposal" << COLOUR_NORMAL << std::endl;
					}
					Proposal<Justification> proposal = msgExldrprepare.proposal;
					Justification justification_MsgExldrprepare = proposal.getCertification();
					Block block = proposal.getBlock();
					if (msgExldrprepare.group.getSize() > 0)
					{
						this->trustedGroup = msgExldrprepare.group;
						this->changeAuthenticator();
					}
					this->respondMsgExldrprepareHotsus(justification_MsgExldrprepare, block);
				}
			}
		}
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Replica handled extra earlier messages" << COLOUR_NORMAL << std::endl;
		}
	}
}

void Hotsus::handleMsgNewviewHotsus(MsgNewviewHotsus msgNewview)
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
			if (this->log.storeMsgNewviewHotsus(msgNewview) == this->trustedQuorumSize)
			{
				this->initiateMsgNewviewHotsus();
			}
		}
		else
		{
			if (DEBUG_HELP)
			{
				std::cout << COLOUR_BLUE << this->printReplicaId() << "Storing MsgNewview: " << msgNewview.toPrint() << COLOUR_NORMAL << std::endl;
			}
			this->log.storeMsgNewviewHotsus(msgNewview);
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

void Hotsus::handleMsgLdrprepareHotsus(MsgLdrprepareHotsus msgLdrprepare)
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
	if (this->verifyProposalHotsus(proposal_MsgLdrprepare, signs_MsgLdrprepare) && block.extends(prepareHash_MsgLdrprepare) && proposeView_MsgLdrprepare >= this->view)
	{
		if (proposeView_MsgLdrprepare == this->view)
		{
			if (!this->amGeneralReplicaIds())
			{
				this->respondMsgLdrprepareHotsus(accumulator_MsgLdrprepare, block);
			}
			else
			{
				this->blocks[this->view] = block;
			}
		}
		else
		{
			if (DEBUG_HELP)
			{
				std::cout << COLOUR_BLUE << this->printReplicaId() << "Storing MsgLdrprepare: " << msgLdrprepare.toPrint() << COLOUR_NORMAL << std::endl;
			}
			this->log.storeMsgLdrprepareHotsus(msgLdrprepare);
		}
	}

	auto end = std::chrono::steady_clock::now();
	double time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	statistics.addHandleTime(time);
}

void Hotsus::handleMsgPrepareHotsus(MsgPrepareHotsus msgPrepare)
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
				if (this->log.storeMsgPrepareHotsus(msgPrepare) == this->trustedQuorumSize)
				{
					this->initiateMsgPrepareHotsus(roundData_MsgPrepare);
				}
			}
			else
			{
				if (signs_MsgPrepare.getSize() == this->trustedQuorumSize)
				{
					this->respondMsgPrepareHotsus(justification_MsgPrepare);
				}
			}
		}
		else
		{
			if (DEBUG_HELP)
			{
				std::cout << COLOUR_BLUE << this->printReplicaId() << "Storing MsgPrepare: " << msgPrepare.toPrint() << COLOUR_NORMAL << std::endl;
			}
			this->log.storeMsgPrepareHotsus(msgPrepare);
		}
	}

	auto end = std::chrono::steady_clock::now();
	double time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	statistics.addHandleTime(time);
}

void Hotsus::handleMsgPrecommitHotsus(MsgPrecommitHotsus msgPrecommit)
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
				if (this->log.storeMsgPrecommitHotsus(msgPrecommit) == this->trustedQuorumSize)
				{
					this->initiateMsgPrecommitHotsus(roundData_MsgPrecommit);
				}
			}
			else
			{
				if (signs_MsgPrecommit.getSize() == this->trustedQuorumSize && this->verifyJustificationHotsus(justification_MsgPrecommit))
				{
					if (this->amGeneralReplicaIds())
					{
						this->skipRoundHotsus();
					}
					this->executeBlockHotsus(roundData_MsgPrecommit);
				}
				else
				{
					this->changeSwitcher();
					this->getExtraStarted();
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
				this->log.storeMsgPrecommitHotsus(msgPrecommit);
			}
			else
			{
				if (this->verifyJustificationHotsus(justification_MsgPrecommit))
				{
					this->log.storeMsgPrecommitHotsus(msgPrecommit);
				}
			}
		}
	}

	auto end = std::chrono::steady_clock::now();
	double time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	statistics.addHandleTime(time);
}

void Hotsus::handleMsgExnewviewHotsus(MsgExnewviewHotsus msgExnewview)
{
	auto start = std::chrono::steady_clock::now();

	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Handling MsgExnewview: " << msgExnewview.toPrint() << COLOUR_NORMAL << std::endl;
	}
	RoundData roundData_MsgExnewview = msgExnewview.roundData;
	Hash proposeHash_MsgExnewview = roundData_MsgExnewview.getProposeHash();
	View proposeView_MsgExnewview = roundData_MsgExnewview.getProposeView();
	Phase phase_MsgExnewview = roundData_MsgExnewview.getPhase();

	if (proposeHash_MsgExnewview.isDummy() && proposeView_MsgExnewview >= this->view && phase_MsgExnewview == PHASE_EXNEWVIEW)
	{
		if (proposeView_MsgExnewview == this->view)
		{
			if (this->log.storeMsgExnewviewHotsus(msgExnewview) == this->generalQuorumSize)
			{
				this->initiateMsgExnewviewHotsus();
			}
		}
		else
		{
			if (DEBUG_HELP)
			{
				std::cout << COLOUR_BLUE << this->printReplicaId() << "Storing MsgExnewview: " << msgExnewview.toPrint() << COLOUR_NORMAL << std::endl;
			}
			this->log.storeMsgExnewviewHotsus(msgExnewview);
		}
	}
	else
	{
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Discarded MsgExnewview: " << msgExnewview.toPrint() << COLOUR_NORMAL << std::endl;
		}
	}

	auto end = std::chrono::steady_clock::now();
	double time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	statistics.addHandleTime(time);
}

void Hotsus::handleMsgExldrprepareHotsus(MsgExldrprepareHotsus msgExldrprepare)
{
	auto start = std::chrono::steady_clock::now();

	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Handling MsgExldrprepare: " << msgExldrprepare.toPrint() << COLOUR_NORMAL << std::endl;
	}
	Proposal<Justification> proposal_MsgExldrprepare = msgExldrprepare.proposal;
	Group group_MsgExldrprepare = msgExldrprepare.group;
	Signs signs_MsgExldrprepare = msgExldrprepare.signs;
	Justification justification_MsgExnewview = proposal_MsgExldrprepare.getCertification();
	RoundData roundData_MsgExnewview = justification_MsgExnewview.getRoundData();
	View proposeView_MsgExnewview = roundData_MsgExnewview.getProposeView();
	Hash justifyHash_MsgExnewview = roundData_MsgExnewview.getJustifyHash();
	Block block = proposal_MsgExldrprepare.getBlock();

	// Verify the [signs_MsgExldrprepare] in [msgExldrprepare]
	if (this->verifyExproposalHotsus(proposal_MsgExldrprepare, signs_MsgExldrprepare) && block.extends(justifyHash_MsgExnewview) && proposeView_MsgExnewview >= this->view)
	{
		if (proposeView_MsgExnewview == this->view)
		{
			if (group_MsgExldrprepare.getSize() > 0)
			{
				this->trustedGroup = group_MsgExldrprepare;
				this->changeAuthenticator();
			}
			this->respondMsgExldrprepareHotsus(justification_MsgExnewview, block);
		}
		else
		{
			if (DEBUG_HELP)
			{
				std::cout << COLOUR_BLUE << this->printReplicaId() << "Storing MsgExldrprepare: " << msgExldrprepare.toPrint() << COLOUR_NORMAL << std::endl;
			}
			this->log.storeMsgExldrprepareHotsus(msgExldrprepare);
		}
	}

	auto end = std::chrono::steady_clock::now();
	double time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	statistics.addHandleTime(time);
}

void Hotsus::handleMsgExprepareHotsus(MsgExprepareHotsus msgExprepare)
{
	auto start = std::chrono::steady_clock::now();

	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Handling MsgExprepare: " << msgExprepare.toPrint() << COLOUR_NORMAL << std::endl;
	}
	RoundData roundData_MsgExprepare = msgExprepare.roundData;
	Signs signs_MsgExprepare = msgExprepare.signs;
	View proposeView_MsgExprepare = roundData_MsgExprepare.getProposeView();
	Phase phase_MsgExprepare = roundData_MsgExprepare.getPhase();
	Justification justification_MsgExprepare = Justification(roundData_MsgExprepare, signs_MsgExprepare);

	if (proposeView_MsgExprepare >= this->view && phase_MsgExprepare == PHASE_EXPREPARE)
	{
		if (proposeView_MsgExprepare == this->view)
		{
			if (this->amCurrentLeader())
			{
				if (this->log.storeMsgExprepareHotsus(msgExprepare) == this->generalQuorumSize)
				{
					this->initiateMsgExprepareHotsus(roundData_MsgExprepare);
				}
			}
			else
			{
				if (signs_MsgExprepare.getSize() == this->generalQuorumSize)
				{
					this->respondMsgExprepareHotsus(justification_MsgExprepare);
				}
			}
		}
		else
		{
			if (DEBUG_HELP)
			{
				std::cout << COLOUR_BLUE << this->printReplicaId() << "Storing MsgExprepare: " << msgExprepare.toPrint() << COLOUR_NORMAL << std::endl;
			}
			this->log.storeMsgExprepareHotsus(msgExprepare);
		}
	}

	auto end = std::chrono::steady_clock::now();
	double time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	statistics.addHandleTime(time);
}

void Hotsus::handleMsgExprecommitHotsus(MsgExprecommitHotsus msgExprecommit)
{
	auto start = std::chrono::steady_clock::now();

	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Handling MsgExprecommit: " << msgExprecommit.toPrint() << COLOUR_NORMAL << std::endl;
	}
	RoundData roundData_MsgExprecommit = msgExprecommit.roundData;
	Signs signs_MsgExprecommit = msgExprecommit.signs;
	View proposeView_MsgExprecommit = roundData_MsgExprecommit.getProposeView();
	Phase phase_MsgExprecommit = roundData_MsgExprecommit.getPhase();
	Justification justification_MsgExprecommit = Justification(roundData_MsgExprecommit, signs_MsgExprecommit);

	if (proposeView_MsgExprecommit >= this->view && phase_MsgExprecommit == PHASE_EXPRECOMMIT)
	{
		if (proposeView_MsgExprecommit == this->view)
		{
			if (this->amCurrentLeader())
			{
				if (this->log.storeMsgExprecommitHotsus(msgExprecommit) == this->generalQuorumSize)
				{
					this->initiateMsgExprecommitHotsus(roundData_MsgExprecommit);
				}
			}
			else
			{
				if (signs_MsgExprecommit.getSize() == this->generalQuorumSize)
				{
					this->respondMsgExprecommitHotsus(justification_MsgExprecommit);
				}
			}
		}
		else
		{
			if (DEBUG_HELP)
			{
				std::cout << COLOUR_BLUE << this->printReplicaId() << "Storing MsgExprecommit: " << msgExprecommit.toPrint() << COLOUR_NORMAL << std::endl;
			}
			this->log.storeMsgExprecommitHotsus(msgExprecommit);
		}
	}

	auto end = std::chrono::steady_clock::now();
	double time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	statistics.addHandleTime(time);
}

void Hotsus::handleMsgExcommitHotsus(MsgExcommitHotsus msgExcommit)
{
	auto start = std::chrono::steady_clock::now();

	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Handling MsgExcommit: " << msgExcommit.toPrint() << COLOUR_NORMAL << std::endl;
	}
	RoundData roundData_MsgExcommit = msgExcommit.roundData;
	Signs signs_MsgExcommit = msgExcommit.signs;
	View proposeView_MsgExcommit = roundData_MsgExcommit.getProposeView();
	Phase phase_MsgExcommit = roundData_MsgExcommit.getPhase();
	Justification justification_MsgExcommit = Justification(roundData_MsgExcommit, signs_MsgExcommit);

	if (proposeView_MsgExcommit >= this->view && phase_MsgExcommit == PHASE_EXCOMMIT)
	{
		if (proposeView_MsgExcommit == this->view)
		{
			if (this->amCurrentLeader())
			{
				if (this->log.storeMsgExcommitHotsus(msgExcommit) == this->generalQuorumSize)
				{
					this->initiateMsgExcommitHotsus(roundData_MsgExcommit);
				}
			}
			else
			{
				if (signs_MsgExcommit.getSize() == this->generalQuorumSize && this->verifyJustificationHotsus(justification_MsgExcommit))
				{
					this->executeExtraBlockHotsus(roundData_MsgExcommit);
				}
			}
		}
		else
		{
			if (DEBUG_HELP)
			{
				std::cout << COLOUR_BLUE << this->printReplicaId() << "Storing MsgExcommit: " << msgExcommit.toPrint() << COLOUR_NORMAL << std::endl;
			}
			if (this->amLeaderOf(proposeView_MsgExcommit))
			{
				this->log.storeMsgExcommitHotsus(msgExcommit);
			}
			else
			{
				if (this->verifyJustificationHotsus(justification_MsgExcommit))
				{
					this->log.storeMsgExcommitHotsus(msgExcommit);
				}
			}
		}
	}

	auto end = std::chrono::steady_clock::now();
	double time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	statistics.addHandleTime(time);
}

// Initiate messages
void Hotsus::initiateMsgNewviewHotsus()
{
	std::set<MsgNewviewHotsus> msgNewviews = this->log.getMsgNewviewHotsus(this->view, this->trustedQuorumSize);
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
			Block block = this->createNewBlockHotsus(prepareHash_MsgLdrprepare);

			// Create [justification_MsgPrepare] for that [block]
			Justification justification_MsgPrepare = this->respondMsgLdrprepareProposalHotsus(block.hash(), accumulator_MsgLdrprepare);
			if (justification_MsgPrepare.isSet())
			{
				if (DEBUG_HELP)
				{
					std::cout << COLOUR_BLUE << this->printReplicaId() << "Storing block for view " << this->view << ": " << block.toPrint() << COLOUR_NORMAL << std::endl;
				}
				this->blocks[this->view] = block;

				// Create [msgLdrprepare] out of [block]
				Proposal<Accumulator> proposal_MsgLdrprepare = Proposal<Accumulator>(accumulator_MsgLdrprepare, block);
				Signs signs_MsgLdrprepare = this->initializeMsgLdrprepareHotsus(proposal_MsgLdrprepare);
				MsgLdrprepareHotsus msgLdrprepare = MsgLdrprepareHotsus(proposal_MsgLdrprepare, signs_MsgLdrprepare);

				// Send [msgLdrprepare] to replicas
				Peers recipients = this->removeFromPeers(this->replicaId);
				this->sendMsgLdrprepareHotsus(msgLdrprepare, recipients);
				if (DEBUG_HELP)
				{
					std::cout << COLOUR_BLUE << this->printReplicaId() << "Sent MsgLdrprepare to replicas: " << msgLdrprepare.toPrint() << COLOUR_NORMAL << std::endl;
				}

				// Create [msgPrepare]
				RoundData roundData_MsgPrepare = justification_MsgPrepare.getRoundData();
				Signs signs_MsgPrepare = justification_MsgPrepare.getSigns();
				MsgPrepareHotsus msgPrepare = MsgPrepareHotsus(roundData_MsgPrepare, signs_MsgPrepare);
				if (DEBUG_HELP)
				{
					std::cout << COLOUR_BLUE << this->printReplicaId() << "Hold on MsgPrepare to its own: " << msgPrepare.toPrint() << COLOUR_NORMAL << std::endl;
				}

				// Store own [msgPrepare] in the log
				if (this->log.storeMsgPrepareHotsus(msgPrepare) == this->trustedQuorumSize)
				{
					this->initiateMsgPrepareHotsus(msgPrepare.roundData);
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

void Hotsus::initiateMsgPrepareHotsus(RoundData roundData_MsgPrepare)
{
	View proposeView_MsgPrepare = roundData_MsgPrepare.getProposeView();
	Signs signs_MsgPrepare = this->log.getMsgPrepareHotsus(proposeView_MsgPrepare, this->trustedQuorumSize);
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "MsgPrepare signatures: " << signs_MsgPrepare.toPrint() << COLOUR_NORMAL << std::endl;
	}

	Justification justification_MsgPrepare = Justification(roundData_MsgPrepare, signs_MsgPrepare);
	if (signs_MsgPrepare.getSize() == this->trustedQuorumSize)
	{
		// Create [msgPrepare]
		MsgPrepareHotsus msgPrepare = MsgPrepareHotsus(roundData_MsgPrepare, signs_MsgPrepare);

		// Send [msgPrepare] to replicas
		Peers recipients = this->removeFromTrustedPeers(this->replicaId);
		this->sendMsgPrepareHotsus(msgPrepare, recipients);
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Sent MsgPrepare to replicas: " << msgPrepare.toPrint() << COLOUR_NORMAL << std::endl;
		}

		// Create [msgPrecommit]
		Justification justification_MsgPrecommit = this->saveMsgPrepareHotsus(justification_MsgPrepare);
		RoundData roundData_MsgPrecommit = justification_MsgPrecommit.getRoundData();
		Signs signs_MsgPrecommit = justification_MsgPrecommit.getSigns();
		MsgPrecommitHotsus msgPrecommit = MsgPrecommitHotsus(roundData_MsgPrecommit, signs_MsgPrecommit);
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Hold on MsgPrecommit to its own: " << msgPrecommit.toPrint() << COLOUR_NORMAL << std::endl;
		}

		// Store own [msgPrecommit] in the log
		if (this->log.storeMsgPrecommitHotsus(msgPrecommit) >= this->trustedQuorumSize)
		{
			this->initiateMsgPrecommitHotsus(justification_MsgPrecommit.getRoundData());
		}
	}
}

void Hotsus::initiateMsgPrecommitHotsus(RoundData roundData_MsgPrecommit)
{
	View proposeView_MsgPrecommit = roundData_MsgPrecommit.getProposeView();
	Signs signs_MsgPrecommit = this->log.getMsgPrecommitHotsus(proposeView_MsgPrecommit, this->trustedQuorumSize);
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "MsgPrecommit signatures: " << signs_MsgPrecommit.toPrint() << COLOUR_NORMAL << std::endl;
	}

	if (signs_MsgPrecommit.getSize() == this->trustedQuorumSize)
	{
		// Create [msgPrecommit]
		MsgPrecommitHotsus msgPrecommit = MsgPrecommitHotsus(roundData_MsgPrecommit, signs_MsgPrecommit);

		// Send [msgPrecommit] to replicas
		Peers recipients = this->removeFromPeers(this->replicaId);
		this->sendMsgPrecommitHotsus(msgPrecommit, recipients);
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Sent MsgPrecommit to replicas: " << msgPrecommit.toPrint() << COLOUR_NORMAL << std::endl;
		}

		// Execute the block
		this->executeBlockHotsus(roundData_MsgPrecommit);
	}
	else
	{
		// Create [msgPrecommit]
		MsgPrecommitHotsus msgPrecommit = MsgPrecommitHotsus(roundData_MsgPrecommit, signs_MsgPrecommit);

		// Send [msgPrecommit] to replicas
		Peers recipients = this->removeFromPeers(this->replicaId);
		this->sendMsgPrecommitHotsus(msgPrecommit, recipients);
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Sent MsgPrecommit to replicas: " << msgPrecommit.toPrint() << COLOUR_NORMAL << std::endl;
		}

		this->getExtraStarted();
	}
}

void Hotsus::initiateMsgExnewviewHotsus()
{
	// Get the trusted replicas
	Group group_MsgExldrprepare = Group();
	if (this->protocol == PROTOCOL_HOTSTUFF && !this->amGeneralReplicaIds())
	{
		std::vector<ReplicaID> senders = this->log.getTrustedMsgExnewviewHotsus(this->view, this->generalQuorumSize);
		std::vector<ReplicaID> trustedSenders;
		for (std::vector<ReplicaID>::iterator itSenders = senders.begin(); itSenders != senders.end(); itSenders++)
		{
			ReplicaID sender = *itSenders;
			if (!this->isGeneralReplicaIds(sender))
			{
				trustedSenders.push_back(sender);
			}
		}
		if (trustedSenders.size() > this->lowTrustedSize)
		{
			this->trustedGroup = Group(trustedSenders);
			this->changeAuthenticator();
			group_MsgExldrprepare = this->trustedGroup;
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Trusted group: ";
			for (int trustedSendersNum = 0; trustedSendersNum < trustedSenders.size(); trustedSendersNum++)
			{
				std::cout << trustedSenders[trustedSendersNum] << " ";
			}
			std::cout << COLOUR_NORMAL << std::endl;
		}
	}

	// Create [block] extends the highest prepared block
	Justification justification_MsgExnewview = this->log.findHighestMsgExnewviewHotsus(this->view);
	RoundData roundData_MsgExnewview = justification_MsgExnewview.getRoundData();
	Hash justifyHash_MsgExnewview = roundData_MsgExnewview.getJustifyHash();
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Highest Newview for view " << this->view << ": " << justification_MsgExnewview.toPrint() << COLOUR_NORMAL << std::endl;
	}
	Block block = createNewBlockHotsus(justifyHash_MsgExnewview);

	// Create [justification_MsgExprepare] for that [block]
	Justification justification_MsgExprepare = this->respondMsgExnewviewProposalHotsus(block.hash(), justification_MsgExnewview);
	if (justification_MsgExprepare.isSet())
	{
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Storing block for view " << this->view << ": " << block.toPrint() << COLOUR_NORMAL << std::endl;
		}
		this->blocks[this->view] = block;

		// Create [msgExldrprepare] out of [block]
		Proposal<Justification> proposal_MsgExldrprepare = Proposal<Justification>(justification_MsgExnewview, block);
		Signs signs_MsgExldrprepare = this->initializeMsgExldrprepareHotsus(proposal_MsgExldrprepare);
		MsgExldrprepareHotsus msgExldrprepare = MsgExldrprepareHotsus(proposal_MsgExldrprepare, group_MsgExldrprepare, signs_MsgExldrprepare);

		// Send [msgExldrprepare] to replicas
		Peers recipients = this->removeFromPeers(this->replicaId);
		this->sendMsgExldrprepareHotsus(msgExldrprepare, recipients);
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Sent MsgExldrprepare to replicas: " << msgExldrprepare.toPrint() << COLOUR_NORMAL << std::endl;
		}

		// Create [msgExprepare]
		RoundData roundData_MsgExprepare = justification_MsgExprepare.getRoundData();
		Signs signs_MsgExprepare = justification_MsgExprepare.getSigns();
		MsgExprepareHotsus msgExprepare = MsgExprepareHotsus(roundData_MsgExprepare, signs_MsgExprepare);
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Hold on MsgExprepare to its own: " << msgExprepare.toPrint() << COLOUR_NORMAL << std::endl;
		}
	}
	else
	{
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Bad justification of MsgExprepare" << justification_MsgExprepare.toPrint() << COLOUR_NORMAL << std::endl;
		}
	}
}

void Hotsus::initiateMsgExprepareHotsus(RoundData roundData_MsgExprepare)
{
	View proposeView_MsgExprepare = roundData_MsgExprepare.getProposeView();
	Signs signs_MsgExprepare = this->log.getMsgExprepareHotsus(proposeView_MsgExprepare, this->generalQuorumSize);
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "MsgExprepare signatures: " << signs_MsgExprepare.toPrint() << COLOUR_NORMAL << std::endl;
	}

	Justification justification_MsgExprepare = Justification(roundData_MsgExprepare, signs_MsgExprepare);
	if (signs_MsgExprepare.getSize() == this->generalQuorumSize)
	{
		// Create [msgExprepare]
		MsgExprepareHotsus msgExprepare = MsgExprepareHotsus(roundData_MsgExprepare, signs_MsgExprepare);

		// Send [msgExprepare] to replicas
		Peers recipients = this->removeFromPeers(this->replicaId);
		this->sendMsgExprepareHotsus(msgExprepare, recipients);
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Sent MsgExprepare to replicas: " << msgExprepare.toPrint() << COLOUR_NORMAL << std::endl;
		}

		// Create [msgExprecommit]
		Justification justification_MsgExprecommit = this->saveMsgExprepareHotsus(justification_MsgExprepare);
		RoundData roundData_MsgExprecommit = justification_MsgExprecommit.getRoundData();
		Signs signs_MsgExprecommit = justification_MsgExprecommit.getSigns();
		MsgExprecommitHotsus msgExprecommit = MsgExprecommitHotsus(roundData_MsgExprecommit, signs_MsgExprecommit);
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Hold on MsgExprecommit to its own: " << msgExprecommit.toPrint() << COLOUR_NORMAL << std::endl;
		}
	}
}

void Hotsus::initiateMsgExprecommitHotsus(RoundData roundData_MsgExprecommit)
{
	View proposeView_MsgExprecommit = roundData_MsgExprecommit.getProposeView();
	Signs signs_MsgExprecommit = this->log.getMsgExprecommitHotsus(proposeView_MsgExprecommit, this->generalQuorumSize);
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "MsgExprecommit signatures: " << signs_MsgExprecommit.toPrint() << COLOUR_NORMAL << std::endl;
	}

	Justification justification_MsgExprecommit = Justification(roundData_MsgExprecommit, signs_MsgExprecommit);
	if (signs_MsgExprecommit.getSize() == this->generalQuorumSize)
	{
		// Create [msgExprecommit]
		MsgExprecommitHotsus msgExprecommit = MsgExprecommitHotsus(roundData_MsgExprecommit, signs_MsgExprecommit);

		// Send [msgExprecommit] to replicas
		Peers recipients = this->removeFromPeers(this->replicaId);
		this->sendMsgExprecommitHotsus(msgExprecommit, recipients);
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Sent MsgExprecommit to replicas: " << msgExprecommit.toPrint() << COLOUR_NORMAL << std::endl;
		}

		// Create [msgExcommit]
		Justification justification_MsgExcommit = this->lockMsgExprecommitHotsus(justification_MsgExprecommit);
		RoundData roundData_MsgExcommit = justification_MsgExcommit.getRoundData();
		Signs signs_MsgExcommit = justification_MsgExcommit.getSigns();
		MsgExcommitHotsus msgExcommit = MsgExcommitHotsus(roundData_MsgExcommit, signs_MsgExcommit);
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Hold on MsgExcommit to its own: " << msgExcommit.toPrint() << COLOUR_NORMAL << std::endl;
		}
	}
}

void Hotsus::initiateMsgExcommitHotsus(RoundData roundData_MsgExcommit)
{
	View proposeView_MsgExcommit = roundData_MsgExcommit.getProposeView();
	Signs signs_MsgExcommit = this->log.getMsgExcommitHotsus(proposeView_MsgExcommit, this->generalQuorumSize);
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "MsgExcommit signatures: " << signs_MsgExcommit.toPrint() << COLOUR_NORMAL << std::endl;
	}

	if (signs_MsgExcommit.getSize() == this->generalQuorumSize)
	{
		// Create [msgExcommit]
		MsgExcommitHotsus msgExcommit = MsgExcommitHotsus(roundData_MsgExcommit, signs_MsgExcommit);

		// Send [msgExcommit] to replicas
		Peers recipients = this->removeFromPeers(this->replicaId);
		this->sendMsgExcommitHotsus(msgExcommit, recipients);
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Sent MsgExcommit to replicas: " << msgExcommit.toPrint() << COLOUR_NORMAL << std::endl;
		}

		// Execute the block
		this->executeExtraBlockHotsus(roundData_MsgExcommit);
	}
}

// Respond messages
void Hotsus::respondMsgLdrprepareHotsus(Accumulator accumulator_MsgLdrprepare, Block block)
{
	// Create own [justification_MsgPrepare] for that [block]
	Justification justification_MsgPrepare = this->respondMsgLdrprepareProposalHotsus(block.hash(), accumulator_MsgLdrprepare);
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "justification_MsgPrepare: " << justification_MsgPrepare.toPrint() << COLOUR_NORMAL << std::endl;
	}
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
		MsgPrepareHotsus msgPrepare = MsgPrepareHotsus(justification_MsgPrepare.getRoundData(), justification_MsgPrepare.getSigns());

		// Send [msgPrepare] to leader
		if (!this->amGeneralReplicaIds())
		{
			Peers recipients = this->keepFromTrustedPeers(this->getCurrentLeader());
			this->sendMsgPrepareHotsus(msgPrepare, recipients);
			if (DEBUG_HELP)
			{
				std::cout << COLOUR_BLUE << this->printReplicaId() << "Sent MsgPrepare to leader: " << msgPrepare.toPrint() << COLOUR_NORMAL << std::endl;
			}
		}
	}
}

void Hotsus::respondMsgPrepareHotsus(Justification justification_MsgPrepare)
{
	// Create [justification_MsgPrecommit]
	Justification justification_MsgPrecommit = this->saveMsgPrepareHotsus(justification_MsgPrepare);

	// Create [msgPrecommit]
	RoundData roundData_MsgPrecommit = justification_MsgPrecommit.getRoundData();
	Signs signs_MsgPrecommit = justification_MsgPrecommit.getSigns();
	MsgPrecommitHotsus msgPrecommit = MsgPrecommitHotsus(roundData_MsgPrecommit, signs_MsgPrecommit);

	// Send [msgPrecommit] to leader
	Peers recipients = this->keepFromTrustedPeers(this->getCurrentLeader());
	this->sendMsgPrecommitHotsus(msgPrecommit, recipients);
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Sent MsgPrecommit to leader: " << msgPrecommit.toPrint() << COLOUR_NORMAL << std::endl;
	}
}

void Hotsus::respondMsgExldrprepareHotsus(Justification justification_MsgExnewview, Block block)
{
	// Get the trusted replicas
	if (this->protocol == PROTOCOL_HOTSTUFF && !this->isGeneralReplicaIds(this->getCurrentLeader()))
	{
		std::vector<ReplicaID> senders = this->log.getTrustedMsgExnewviewHotsus(this->view, this->generalQuorumSize);
		std::vector<ReplicaID> trustedSenders;
		for (std::vector<ReplicaID>::iterator itSenders = senders.begin(); itSenders != senders.end(); itSenders++)
		{
			ReplicaID sender = *itSenders;
			if (!this->isGeneralReplicaIds(sender))
			{
				trustedSenders.push_back(sender);
			}
		}
		if (trustedSenders.size() > this->lowTrustedSize)
		{
			this->trustedGroup = Group(trustedSenders);
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Trusted group: ";
			for (int trustedSendersNum = 0; trustedSendersNum < trustedSenders.size(); trustedSendersNum++)
			{
				std::cout << trustedSenders[trustedSendersNum] << " ";
			}
			std::cout << COLOUR_NORMAL << std::endl;
		}
	}

	// Create own [justification_MsgExprepare] for that [block]
	Justification justification_MsgExprepare = this->respondMsgExldrprepareProposalHotsus(block.hash(), justification_MsgExnewview);
	if (justification_MsgExprepare.isSet())
	{
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Storing block for view " << this->view << ": " << block.toPrint() << COLOUR_NORMAL << std::endl;
		}
		this->blocks[this->view] = block;

		// Create [msgExprepare] out of [block]
		RoundData roundData_MsgExprepare = justification_MsgExprepare.getRoundData();
		Signs signs_MsgExprepare = justification_MsgExprepare.getSigns();
		MsgExprepareHotsus msgExprepare = MsgExprepareHotsus(roundData_MsgExprepare, signs_MsgExprepare);

		// Send [msgExprepare] to leader
		Peers recipients = this->keepFromPeers(this->getCurrentLeader());
		this->sendMsgExprepareHotsus(msgExprepare, recipients);
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Sent MsgExprepare to leader: " << msgExprepare.toPrint() << COLOUR_NORMAL << std::endl;
		}
	}
}

void Hotsus::respondMsgExprepareHotsus(Justification justification_MsgExprepare)
{
	// Create [justification_MsgExprecommit]
	Justification justification_MsgExprecommit = this->saveMsgExprepareHotsus(justification_MsgExprepare);

	// Create [msgExprecommit]
	RoundData roundData_MsgExprecommit = justification_MsgExprecommit.getRoundData();
	Signs signs_MsgExprecommit = justification_MsgExprecommit.getSigns();
	MsgExprecommitHotsus msgExprecommit = MsgExprecommitHotsus(roundData_MsgExprecommit, signs_MsgExprecommit);

	// Send [msgExprecommit] to leader
	Peers recipients = this->keepFromPeers(this->getCurrentLeader());
	this->sendMsgExprecommitHotsus(msgExprecommit, recipients);
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Sent MsgExprecommit to leader: " << msgExprecommit.toPrint() << COLOUR_NORMAL << std::endl;
	}
}

void Hotsus::respondMsgExprecommitHotsus(Justification justification_MsgExprecommit)
{
	// Create [justification_MsgExcommit]
	Justification justification_MsgExcommit = this->lockMsgExprecommitHotsus(justification_MsgExprecommit);

	// Create [msgExcommit]
	RoundData roundData_MsgExcommit = justification_MsgExcommit.getRoundData();
	Signs signs_MsgExcommit = justification_MsgExcommit.getSigns();
	MsgExcommitHotsus msgExcommit = MsgExcommitHotsus(roundData_MsgExcommit, signs_MsgExcommit);

	// Send [msgExcommit] to leader
	Peers recipients = this->keepFromPeers(this->getCurrentLeader());
	this->sendMsgExcommitHotsus(msgExcommit, recipients);
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Sent MsgExcommit to leader: " << msgExcommit.toPrint() << COLOUR_NORMAL << std::endl;
	}
}

// Main functions
int Hotsus::initializeSGX()
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

void Hotsus::getStarted()
{
	if (this->protocol == PROTOCOL_HOTSTUFF)
	{
		if (DEBUG_BASIC)
		{
			std::cout << COLOUR_RED << this->printReplicaId() << "Starting in Hotstuff" << COLOUR_NORMAL << std::endl;
		}
	}
	else if (this->protocol == PROTOCOL_DAMYSUS)
	{
		if (DEBUG_BASIC)
		{
			std::cout << COLOUR_RED << this->printReplicaId() << "Starting in Damysus" << COLOUR_NORMAL << std::endl;
		}
	}

	startTime = std::chrono::steady_clock::now();
	startView = std::chrono::steady_clock::now();

	// Send [msgExnewview] to the leader of the current view
	ReplicaID leader = this->getCurrentLeader();
	Peers recipients = this->keepFromPeers(leader);

	Justification justification_MsgExnewview = this->initializeMsgExnewviewHotsus();
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Initial justification: " << justification_MsgExnewview.toPrint() << COLOUR_NORMAL << std::endl;
	}
	RoundData roundData_MsgExnewview = justification_MsgExnewview.getRoundData();
	Signs signs_MsgExnewview = justification_MsgExnewview.getSigns();
	MsgExnewviewHotsus msgExnewview = MsgExnewviewHotsus(roundData_MsgExnewview, signs_MsgExnewview);
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Starting with: " << msgExnewview.toPrint() << COLOUR_NORMAL << std::endl;
	}
	if (this->amCurrentLeader())
	{
		this->handleMsgExnewviewHotsus(msgExnewview);
	}
	else
	{
		this->sendMsgExnewviewHotsus(msgExnewview, recipients);
	}
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Sent MsgExnewview to leader[" << leader << "]" << COLOUR_NORMAL << std::endl;
	}
}

void Hotsus::getExtraStarted()
{
	if (DEBUG_BASIC)
	{
		std::cout << COLOUR_RED << this->printReplicaId() << "Starting extra round" << COLOUR_NORMAL << std::endl;
	}

	// Send [msgExnewview] to the leader of the current view
	ReplicaID leader = this->getCurrentLeader();
	Peers recipients = this->keepFromPeers(leader);

	Justification justification_MsgExnewview = this->initializeMsgExnewviewHotsus();
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Initial justification: " << justification_MsgExnewview.toPrint() << COLOUR_NORMAL << std::endl;
	}
	RoundData roundData_MsgExnewview = justification_MsgExnewview.getRoundData();
	Signs signs_MsgExnewview = justification_MsgExnewview.getSigns();
	MsgExnewviewHotsus msgExnewview = MsgExnewviewHotsus(roundData_MsgExnewview, signs_MsgExnewview);
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Starting with: " << msgExnewview.toPrint() << COLOUR_NORMAL << std::endl;
	}
	if (this->amCurrentLeader())
	{
		this->handleMsgExnewviewHotsus(msgExnewview);
	}
	else
	{
		this->sendMsgExnewviewHotsus(msgExnewview, recipients);
	}
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Sent MsgExnewview to leader[" << leader << "]" << COLOUR_NORMAL << std::endl;
	}
}

void Hotsus::startNewViewHotsus()
{
	// Generate [justification_MsgNewview] or [justification_MsgExnewview] until one for the next view
	if (this->protocol == PROTOCOL_HOTSTUFF)
	{
		Justification justification_MsgExnewview = this->initializeMsgExnewviewHotsus();
		View proposeView_MsgExnewview = justification_MsgExnewview.getRoundData().getProposeView();
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Generating Hotstuff justification: " << justification_MsgExnewview.toPrint() << COLOUR_NORMAL << std::endl;
		}
		while (proposeView_MsgExnewview <= this->view)
		{
			justification_MsgExnewview = this->initializeMsgNewviewHotsus();
			proposeView_MsgExnewview = justification_MsgExnewview.getRoundData().getProposeView();
			if (DEBUG_HELP)
			{
				std::cout << COLOUR_BLUE << this->printReplicaId() << "Generating Hotstuff justification: " << justification_MsgExnewview.toPrint() << COLOUR_NORMAL << std::endl;
			}
		}

		// Increase the view
		this->view++;

		// Start the timer
		this->setTimer();

		RoundData roundData_MsgExnewview = justification_MsgExnewview.getRoundData();
		Phase phase_MsgExnewview = roundData_MsgExnewview.getPhase();
		Signs signs_MsgExnewview = justification_MsgExnewview.getSigns();
		if (proposeView_MsgExnewview == this->view && phase_MsgExnewview == PHASE_EXNEWVIEW)
		{
			MsgExnewviewHotsus msgExnewview = MsgExnewviewHotsus(roundData_MsgExnewview, signs_MsgExnewview);
			if (this->amCurrentLeader())
			{
				this->handleExtraEarlierMessagesHotsus();
				this->handleMsgExnewviewHotsus(msgExnewview);
			}
			else
			{
				ReplicaID leader = this->getCurrentLeader();
				Peers recipients = this->keepFromPeers(leader);
				this->sendMsgExnewviewHotsus(msgExnewview, recipients);
				this->handleExtraEarlierMessagesHotsus();
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
	else if (this->protocol == PROTOCOL_DAMYSUS)
	{
		Justification justification_MsgNewview = this->initializeMsgNewviewHotsus();
		View proposeView_MsgNewview = justification_MsgNewview.getRoundData().getProposeView();
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Generating Damysus justification: " << justification_MsgNewview.toPrint() << COLOUR_NORMAL << std::endl;
		}
		while (proposeView_MsgNewview <= this->view)
		{
			justification_MsgNewview = this->initializeMsgNewviewHotsus();
			proposeView_MsgNewview = justification_MsgNewview.getRoundData().getProposeView();
			if (DEBUG_HELP)
			{
				std::cout << COLOUR_BLUE << this->printReplicaId() << "Generating Damysus justification: " << justification_MsgNewview.toPrint() << COLOUR_NORMAL << std::endl;
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
			MsgNewviewHotsus msgNewview = MsgNewviewHotsus(roundData_MsgNewview, signs_MsgNewview);
			if (this->amCurrentLeader())
			{
				this->handleEarlierMessagesHotsus();
				this->handleMsgNewviewHotsus(msgNewview);
			}
			else
			{
				ReplicaID leader = this->getCurrentLeader();
				Peers recipients = this->keepFromPeers(leader);
				if (this->amTrustedReplicaIds())
				{
					this->sendMsgNewviewHotsus(msgNewview, recipients);
				}
				this->handleEarlierMessagesHotsus();
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
}

Block Hotsus::createNewBlockHotsus(Hash hash)
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

void Hotsus::executeBlockHotsus(RoundData roundData_MsgPrecommit)
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
				  << "HOTSUS-DAMYSUS-EXECUTE(" << this->view << "/" << std::to_string(this->numViews - 1) << ":" << time << ") "
				  << statistics.toString() << COLOUR_NORMAL << std::endl;
	}

	this->replyHash(roundData_MsgPrecommit.getProposeHash());
	if (this->timeToStop())
	{
		this->recordStatisticsHotsus();
	}
	else
	{
		this->startNewViewHotsus();
	}
}

void Hotsus::executeExtraBlockHotsus(RoundData roundData_MsgExcommit)
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
				  << "HOTSUS-HOTSTUFF-EXECUTE(" << this->view << "/" << std::to_string(this->numViews - 1) << ":" << time << ") "
				  << statistics.toString() << COLOUR_NORMAL << std::endl;
	}

	this->replyHash(roundData_MsgExcommit.getProposeHash());

	// Check if switch the protocol
	if (this->trustedGroup.getSize() > this->lowTrustedSize)
	{
		this->protocol = PROTOCOL_DAMYSUS;
		this->trustedQuorumSize = floor(this->trustedGroup.getSize() / 2) + 1;
		this->setTrustedQuorumSize();
	}

	if (this->timeToStop())
	{
		this->recordStatisticsHotsus();
	}
	else
	{
		this->startNewViewHotsus();
	}
}

bool Hotsus::timeToStop()
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

void Hotsus::recordStatisticsHotsus()
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
Hotsus::Hotsus(KeysFunctions keysFunctions, ReplicaID replicaId, unsigned int numGeneralReplicas, unsigned int numTrustedReplicas, unsigned int numReplicas, unsigned int numViews, unsigned int numFaults, double leaderChangeTime, Nodes nodes, Key privateKey, PeerNet::Config peerNetConfig, ClientNet::Config clientNetConfig) : peerNet(peerEventContext, peerNetConfig), clientNet(requestEventContext, clientNetConfig)
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
	this->generalQuorumSize = this->numReplicas - this->numFaults;
	this->trustedQuorumSize = floor(this->numTrustedReplicas / 2) + 1;
	this->lowTrustedSize = 3;
	this->trustedGroup = Group();
	this->view = 0;
	this->protocol = PROTOCOL_HOTSTUFF;

	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Starting handler" << COLOUR_NORMAL << std::endl;
	}
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "General quorum size: " << this->generalQuorumSize << COLOUR_NORMAL << std::endl;
	}
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Trusted quorum size: " << this->trustedQuorumSize << COLOUR_NORMAL << std::endl;
	}

	// Trusted Functions
	if (!this->amGeneralReplicaIds())
	{
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Initializing TEE" << COLOUR_NORMAL << std::endl;
		}
		this->initializeSGX();
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Initialized TEE" << COLOUR_NORMAL << std::endl;
		}
	}
	else
	{
		hotsusBasic = HotsusBasic(this->replicaId, this->privateKey, this->generalQuorumSize, this->trustedQuorumSize);
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
													this->startNewViewHotsus();
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

	this->clientNet.reg_handler(salticidae::generic_bind(&Hotsus::receiveMsgStartHotsus, this, _1, _2));
	this->clientNet.reg_handler(salticidae::generic_bind(&Hotsus::receiveMsgTransactionHotsus, this, _1, _2));

	this->peerNet.reg_handler(salticidae::generic_bind(&Hotsus::receiveMsgNewviewHotsus, this, _1, _2));
	this->peerNet.reg_handler(salticidae::generic_bind(&Hotsus::receiveMsgLdrprepareHotsus, this, _1, _2));
	this->peerNet.reg_handler(salticidae::generic_bind(&Hotsus::receiveMsgPrepareHotsus, this, _1, _2));
	this->peerNet.reg_handler(salticidae::generic_bind(&Hotsus::receiveMsgPrecommitHotsus, this, _1, _2));
	this->peerNet.reg_handler(salticidae::generic_bind(&Hotsus::receiveMsgExnewviewHotsus, this, _1, _2));
	this->peerNet.reg_handler(salticidae::generic_bind(&Hotsus::receiveMsgExldrprepareHotsus, this, _1, _2));
	this->peerNet.reg_handler(salticidae::generic_bind(&Hotsus::receiveMsgExprepareHotsus, this, _1, _2));
	this->peerNet.reg_handler(salticidae::generic_bind(&Hotsus::receiveMsgExprecommitHotsus, this, _1, _2));
	this->peerNet.reg_handler(salticidae::generic_bind(&Hotsus::receiveMsgExcommitHotsus, this, _1, _2));

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