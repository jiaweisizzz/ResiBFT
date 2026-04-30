#include "Hotstuff.h"

// Local variables
Time startTime = std::chrono::steady_clock::now();
Time startView = std::chrono::steady_clock::now();
Time currentTime;
std::string statisticsValues;
std::string statisticsDone;
Statistics statistics;
HotstuffBasic hotstuffBasic;

// Opcodes
const uint8_t MsgNewviewHotstuff::opcode;
const uint8_t MsgLdrprepareHotstuff::opcode;
const uint8_t MsgPrepareHotstuff::opcode;
const uint8_t MsgPrecommitHotstuff::opcode;
const uint8_t MsgCommitHotstuff::opcode;
const uint8_t MsgTransaction::opcode;
const uint8_t MsgReply::opcode;
const uint8_t MsgStart::opcode;

// Print functions
std::string Hotstuff::printReplicaId()
{
	return "[" + std::to_string(this->replicaId) + "-" + std::to_string(this->view) + "]";
}

void Hotstuff::printNowTime(std::string msg)
{
	auto now = std::chrono::steady_clock::now();
	double time = std::chrono::duration_cast<std::chrono::microseconds>(now - startView).count();
	double etime = (statistics.getTotalViewTime().total + time) / (1000 * 1000);
	std::cout << COLOUR_BLUE << this->printReplicaId() << msg << " @ " << etime << COLOUR_NORMAL << std::endl;
}

void Hotstuff::printClientInfo()
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

std::string Hotstuff::recipients2string(Peers recipients)
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
unsigned int Hotstuff::getLeaderOf(View view)
{
	unsigned int leader = view % this->numGeneralReplicas;
	return leader;
}

unsigned int Hotstuff::getCurrentLeader()
{
	unsigned int leader = this->getLeaderOf(this->view);
	return leader;
}

bool Hotstuff::amLeaderOf(View view)
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

bool Hotstuff::amCurrentLeader()
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

Peers Hotstuff::removeFromPeers(ReplicaID replicaId)
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

Peers Hotstuff::keepFromPeers(ReplicaID replicaId)
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

std::vector<salticidae::PeerId> Hotstuff::getPeerIds(Peers recipients)
{
	std::vector<salticidae::PeerId> returnPeerId;
	for (Peers::iterator it = recipients.begin(); it != recipients.end(); it++)
	{
		Peer peer = *it;
		returnPeerId.push_back(std::get<1>(peer));
	}
	return returnPeerId;
}

void Hotstuff::setTimer()
{
	this->timer.del();
	this->timer.add(this->leaderChangeTime);
	this->timerView = this->view;
}

// Reply to clients
void Hotstuff::replyTransactions(Transaction *transactions)
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

void Hotstuff::replyHash(Hash hash)
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

// Call module functions
bool Hotstuff::verifyJustificationHotstuff(Justification justification)
{
	bool b = hotstuffBasic.verifyJustification(this->nodes, justification);
	return b;
}

bool Hotstuff::verifyProposalHotstuff(Proposal<Justification> proposal, Signs signs)
{
	bool b = hotstuffBasic.verifyProposal(this->nodes, proposal, signs);
	return b;
}

Justification Hotstuff::initializeMsgNewviewHotstuff()
{
	Justification justification_MsgNewview = hotstuffBasic.initializeMsgNewview();
	return justification_MsgNewview;
}

Justification Hotstuff::respondMsgNewviewProposalHotstuff(Hash proposeHash, Justification justification_MsgNewview)
{
	Justification justification_MsgPrepare = hotstuffBasic.respondProposal(this->nodes, proposeHash, justification_MsgNewview);
	return justification_MsgPrepare;
}

Signs Hotstuff::initializeMsgLdrprepareHotstuff(Proposal<Justification> proposal_MsgLdrprepare)
{
	Signs signs_MsgLdrprepare = hotstuffBasic.initializeMsgLdrprepare(proposal_MsgLdrprepare);
	return signs_MsgLdrprepare;
}

Justification Hotstuff::respondMsgLdrprepareProposalHotstuff(Hash proposeHash, Justification justification_MsgNewview)
{
	Justification justification_MsgPrepare = hotstuffBasic.respondProposal(this->nodes, proposeHash, justification_MsgNewview);
	return justification_MsgPrepare;
}

Justification Hotstuff::saveMsgPrepareHotstuff(Justification justification_MsgPrepare)
{
	Justification justification_MsgPrecommit = hotstuffBasic.saveMsgPrepare(this->nodes, justification_MsgPrepare);
	return justification_MsgPrecommit;
}

Justification Hotstuff::lockMsgPrecommitHotstuff(Justification justification_MsgPrecommit)
{
	Justification justification_MsgCommit = hotstuffBasic.lockMsgPrecommit(this->nodes, justification_MsgPrecommit);
	return justification_MsgCommit;
}

// Receive messages
void Hotstuff::receiveMsgStartHotstuff(MsgStart msgStart, const ClientNet::conn_t &conn)
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

void Hotstuff::receiveMsgTransactionHotstuff(MsgTransaction msgTransaction, const ClientNet::conn_t &conn)
{
	this->handleMsgTransaction(msgTransaction);
}

void Hotstuff::receiveMsgNewviewHotstuff(MsgNewviewHotstuff msgNewview, const PeerNet::conn_t &conn)
{
	this->handleMsgNewviewHotstuff(msgNewview);
}

void Hotstuff::receiveMsgLdrprepareHotstuff(MsgLdrprepareHotstuff msgLdrprepare, const PeerNet::conn_t &conn)
{
	this->handleMsgLdrprepareHotstuff(msgLdrprepare);
}

void Hotstuff::receiveMsgPrepareHotstuff(MsgPrepareHotstuff msgPrepare, const PeerNet::conn_t &conn)
{
	this->handleMsgPrepareHotstuff(msgPrepare);
}

void Hotstuff::receiveMsgPrecommitHotstuff(MsgPrecommitHotstuff msgPrecommit, const PeerNet::conn_t &conn)
{
	this->handleMsgPrecommitHotstuff(msgPrecommit);
}

void Hotstuff::receiveMsgCommitHotstuff(MsgCommitHotstuff msgCommit, const PeerNet::conn_t &conn)
{
	this->handleMsgCommitHotstuff(msgCommit);
}

// Send messages
void Hotstuff::sendMsgNewviewHotstuff(MsgNewviewHotstuff msgNewview, Peers recipients)
{
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Sending: " << msgNewview.toPrint() << " -> " << this->recipients2string(recipients) << COLOUR_NORMAL << std::endl;
	}
	this->peerNet.multicast_msg(msgNewview, getPeerIds(recipients));
	if (DEBUG_TIME)
	{
		this->printNowTime("Sending MsgNewviewHotstuff");
	}
}

void Hotstuff::sendMsgLdrprepareHotstuff(MsgLdrprepareHotstuff msgLdrprepare, Peers recipients)
{
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Sending: " << msgLdrprepare.toPrint() << " -> " << this->recipients2string(recipients) << COLOUR_NORMAL << std::endl;
	}
	this->peerNet.multicast_msg(msgLdrprepare, getPeerIds(recipients));
	if (DEBUG_TIME)
	{
		this->printNowTime("Sending MsgLdrprepareHotstuff");
	}
}

void Hotstuff::sendMsgPrepareHotstuff(MsgPrepareHotstuff msgPrepare, Peers recipients)
{
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Sending: " << msgPrepare.toPrint() << " -> " << this->recipients2string(recipients) << COLOUR_NORMAL << std::endl;
	}
	this->peerNet.multicast_msg(msgPrepare, getPeerIds(recipients));
	if (DEBUG_TIME)
	{
		this->printNowTime("Sending MsgPrepareHotstuff");
	}
}

void Hotstuff::sendMsgPrecommitHotstuff(MsgPrecommitHotstuff msgPrecommit, Peers recipients)
{
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Sending: " << msgPrecommit.toPrint() << " -> " << this->recipients2string(recipients) << COLOUR_NORMAL << std::endl;
	}
	this->peerNet.multicast_msg(msgPrecommit, getPeerIds(recipients));
	if (DEBUG_TIME)
	{
		this->printNowTime("Sending MsgPrecommitHotstuff");
	}
}

void Hotstuff::sendMsgCommitHotstuff(MsgCommitHotstuff msgCommit, Peers recipients)
{
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Sending: " << msgCommit.toPrint() << " -> " << this->recipients2string(recipients) << COLOUR_NORMAL << std::endl;
	}
	this->peerNet.multicast_msg(msgCommit, getPeerIds(recipients));
	if (DEBUG_TIME)
	{
		this->printNowTime("Sending MsgCommitHotstuff");
	}
}

// Handle messages
void Hotstuff::handleMsgTransaction(MsgTransaction msgTransaction)
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

void Hotstuff::handleEarlierMessagesHotstuff()
{
	// Check if there are enough messages to start the next view
	if (this->amCurrentLeader())
	{
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Leader handling earlier messages" << COLOUR_NORMAL << std::endl;
		}
		Signs signs_MsgNewview = this->log.getMsgNewviewHotstuff(this->view, this->generalQuorumSize);
		if (signs_MsgNewview.getSize() == this->generalQuorumSize)
		{
			this->initiateMsgNewviewHotstuff();
		}
	}
	else
	{
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Replica handling earlier messages" << COLOUR_NORMAL << std::endl;
		}
		// Check if the view has already been locked
		Signs signs_MsgPrecommit = this->log.getMsgPrecommitHotstuff(this->view, this->generalQuorumSize);
		if (signs_MsgPrecommit.getSize() == this->generalQuorumSize)
		{
			if (DEBUG_HELP)
			{
				std::cout << COLOUR_BLUE << this->printReplicaId() << "Catching up using MsgPrecommit certificate" << COLOUR_NORMAL << std::endl;
			}
			Justification justification_MsgPrecommit = this->log.firstMsgPrecommitHotstuff(this->view);

			// Skip the prepare phase and pre-commit phase
			this->initializeMsgNewviewHotstuff();
			this->initializeMsgNewviewHotstuff();

			// Store [justification_MsgPrecommit]
			this->respondMsgPrecommitHotstuff(justification_MsgPrecommit);

			Signs signs_MsgCommit = this->log.getMsgCommitHotstuff(this->view, this->generalQuorumSize);
			if (signs_MsgCommit.getSize() == this->generalQuorumSize)
			{
				Justification justification_MsgCommit = this->log.firstMsgCommitHotstuff(this->view);
				this->executeBlockHotstuff(justification_MsgCommit.getRoundData());
			}
		}
		else
		{
			Signs signs_MsgPrepare = this->log.getMsgPrepareHotstuff(this->view, this->generalQuorumSize);
			if (signs_MsgPrepare.getSize() == this->generalQuorumSize)
			{
				if (DEBUG_HELP)
				{
					std::cout << COLOUR_BLUE << this->printReplicaId() << "Catching up using MsgPrepare certificate" << COLOUR_NORMAL << std::endl;
				}
				Justification justification_MsgPrepare = this->log.firstMsgPrepareHotstuff(this->view);

				// Skip the prepare phase
				this->initializeMsgNewviewHotstuff();

				// Store [justification_MsgPrepare]
				this->respondMsgPrepareHotstuff(justification_MsgPrepare);
			}
			else
			{
				MsgLdrprepareHotstuff msgLdrprepare = this->log.firstMsgLdrprepareHotstuff(this->view);

				// Check if the proposal has been stored
				if (msgLdrprepare.signs.getSize() == 1)
				{
					if (DEBUG_HELP)
					{
						std::cout << COLOUR_BLUE << this->printReplicaId() << "Catching up using MsgLdrprepare proposal" << COLOUR_NORMAL << std::endl;
					}
					Proposal<Justification> proposal = msgLdrprepare.proposal;
					Justification justification_MsgLdrprepare = proposal.getCertification();
					Block block = proposal.getBlock();
					this->respondMsgLdrprepareHotstuff(justification_MsgLdrprepare, block);
				}
			}
		}
	}
}

void Hotstuff::handleMsgNewviewHotstuff(MsgNewviewHotstuff msgNewview)
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
			if (this->log.storeMsgNewviewHotstuff(msgNewview) == this->generalQuorumSize)
			{
				this->initiateMsgNewviewHotstuff();
			}
		}
		else
		{
			if (DEBUG_HELP)
			{
				std::cout << COLOUR_BLUE << this->printReplicaId() << "Storing MsgNewview: " << msgNewview.toPrint() << COLOUR_NORMAL << std::endl;
			}
			this->log.storeMsgNewviewHotstuff(msgNewview);
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

void Hotstuff::handleMsgLdrprepareHotstuff(MsgLdrprepareHotstuff msgLdrprepare)
{
	auto start = std::chrono::steady_clock::now();

	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Handling MsgLdrprepare: " << msgLdrprepare.toPrint() << COLOUR_NORMAL << std::endl;
	}
	Proposal<Justification> proposal_MsgLdrprepare = msgLdrprepare.proposal;
	Signs signs_MsgLdrprepare = msgLdrprepare.signs;
	Justification justification_MsgNewview = proposal_MsgLdrprepare.getCertification();
	RoundData roundData_MsgNewview = justification_MsgNewview.getRoundData();
	View proposeView_MsgNewview = roundData_MsgNewview.getProposeView();
	Hash justifyHash_MsgNewview = roundData_MsgNewview.getJustifyHash();
	Block block = proposal_MsgLdrprepare.getBlock();

	// Verify the [signs_MsgLdrprepare] in [msgLdrprepare]
	if (this->verifyProposalHotstuff(proposal_MsgLdrprepare, signs_MsgLdrprepare) && block.extends(justifyHash_MsgNewview) && proposeView_MsgNewview >= this->view)
	{
		if (proposeView_MsgNewview == this->view)
		{
			this->respondMsgLdrprepareHotstuff(justification_MsgNewview, block);
		}
		else
		{
			if (DEBUG_HELP)
			{
				std::cout << COLOUR_BLUE << this->printReplicaId() << "Storing MsgLdrprepare: " << msgLdrprepare.toPrint() << COLOUR_NORMAL << std::endl;
			}
			this->log.storeMsgLdrprepareHotstuff(msgLdrprepare);
		}
	}

	auto end = std::chrono::steady_clock::now();
	double time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	statistics.addHandleTime(time);
}

void Hotstuff::handleMsgPrepareHotstuff(MsgPrepareHotstuff msgPrepare)
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
				if (this->log.storeMsgPrepareHotstuff(msgPrepare) == this->generalQuorumSize)
				{
					this->initiateMsgPrepareHotstuff(roundData_MsgPrepare);
				}
			}
			else
			{
				if (signs_MsgPrepare.getSize() == this->generalQuorumSize)
				{
					this->respondMsgPrepareHotstuff(justification_MsgPrepare);
				}
			}
		}
		else
		{
			if (DEBUG_HELP)
			{
				std::cout << COLOUR_BLUE << this->printReplicaId() << "Storing MsgPrepare:" << msgPrepare.toPrint() << COLOUR_NORMAL << std::endl;
			}
			this->log.storeMsgPrepareHotstuff(msgPrepare);
		}
	}

	auto end = std::chrono::steady_clock::now();
	double time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	statistics.addHandleTime(time);
}

void Hotstuff::handleMsgPrecommitHotstuff(MsgPrecommitHotstuff msgPrecommit)
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
				if (this->log.storeMsgPrecommitHotstuff(msgPrecommit) == this->generalQuorumSize)
				{
					this->initiateMsgPrecommitHotstuff(roundData_MsgPrecommit);
				}
			}
			else
			{
				if (signs_MsgPrecommit.getSize() == this->generalQuorumSize)
				{
					this->respondMsgPrecommitHotstuff(justification_MsgPrecommit);
				}
			}
		}
		else
		{
			if (DEBUG_HELP)
			{
				std::cout << COLOUR_BLUE << this->printReplicaId() << "Storing MsgPrecommit: " << msgPrecommit.toPrint() << COLOUR_NORMAL << std::endl;
			}
			this->log.storeMsgPrecommitHotstuff(msgPrecommit);
		}
	}

	auto end = std::chrono::steady_clock::now();
	double time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	statistics.addHandleTime(time);
}

void Hotstuff::handleMsgCommitHotstuff(MsgCommitHotstuff msgCommit)
{
	auto start = std::chrono::steady_clock::now();

	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Handling MsgCommit: " << msgCommit.toPrint() << COLOUR_NORMAL << std::endl;
	}
	RoundData roundData_MsgCommit = msgCommit.roundData;
	Signs signs_MsgCommit = msgCommit.signs;
	View proposeView_MsgCommit = roundData_MsgCommit.getProposeView();
	Phase phase_MsgCommit = roundData_MsgCommit.getPhase();
	Justification justification_MsgCommit = Justification(roundData_MsgCommit, signs_MsgCommit);

	if (proposeView_MsgCommit >= this->view && phase_MsgCommit == PHASE_COMMIT)
	{
		if (proposeView_MsgCommit == this->view)
		{
			if (this->amCurrentLeader())
			{
				if (this->log.storeMsgCommitHotstuff(msgCommit) == this->generalQuorumSize)
				{
					this->initiateMsgCommitHotstuff(roundData_MsgCommit);
				}
			}
			else
			{
				if (signs_MsgCommit.getSize() == this->generalQuorumSize && this->verifyJustificationHotstuff(justification_MsgCommit))
				{
					this->executeBlockHotstuff(roundData_MsgCommit);
				}
			}
		}
		else
		{
			if (DEBUG_HELP)
			{
				std::cout << COLOUR_BLUE << this->printReplicaId() << "Storing MsgCommit: " << msgCommit.toPrint() << COLOUR_NORMAL << std::endl;
			}
			if (this->amLeaderOf(proposeView_MsgCommit))
			{
				this->log.storeMsgCommitHotstuff(msgCommit);
			}
			else
			{
				if (this->verifyJustificationHotstuff(justification_MsgCommit))
				{
					this->log.storeMsgCommitHotstuff(msgCommit);
				}
			}
		}
	}

	auto end = std::chrono::steady_clock::now();
	double time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	statistics.addHandleTime(time);
}

// Initiate messages
void Hotstuff::initiateMsgNewviewHotstuff()
{
	// Create [block] extends the highest prepared block
	Justification justification_MsgNewview = this->log.findHighestMsgNewviewHotstuff(this->view);
	RoundData roundData_MsgNewview = justification_MsgNewview.getRoundData();
	Hash justifyHash_MsgNewview = roundData_MsgNewview.getJustifyHash();
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Highest Newview for view " << this->view << ": " << justification_MsgNewview.toPrint() << COLOUR_NORMAL << std::endl;
	}
	Block block = createNewBlockHotstuff(justifyHash_MsgNewview);

	// Create [justification_MsgPrepare] for that [block]
	Justification justification_MsgPrepare = this->respondMsgNewviewProposalHotstuff(block.hash(), justification_MsgNewview);
	if (justification_MsgPrepare.isSet())
	{
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Storing block for view " << this->view << ": " << block.toPrint() << COLOUR_NORMAL << std::endl;
		}
		this->blocks[this->view] = block;

		// Create [msgLdrprepare] out of [block]
		Proposal<Justification> proposal_MsgLdrprepare = Proposal<Justification>(justification_MsgNewview, block);
		Signs signs_MsgLdrprepare = this->initializeMsgLdrprepareHotstuff(proposal_MsgLdrprepare);
		MsgLdrprepareHotstuff msgLdrprepare = MsgLdrprepareHotstuff(proposal_MsgLdrprepare, signs_MsgLdrprepare);

		// Send [msgLdrprepare] to replicas
		Peers recipients = this->removeFromPeers(this->replicaId);
		this->sendMsgLdrprepareHotstuff(msgLdrprepare, recipients);
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Sent MsgLdrprepare to replicas: " << msgLdrprepare.toPrint() << COLOUR_NORMAL << std::endl;
		}

		// Create [msgPrepare]
		RoundData roundData_MsgPrepare = justification_MsgPrepare.getRoundData();
		Signs signs_MsgPrepare = justification_MsgPrepare.getSigns();
		MsgPrepareHotstuff msgPrepare = MsgPrepareHotstuff(roundData_MsgPrepare, signs_MsgPrepare);
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Hold on MsgPrepare to its own: " << msgPrepare.toPrint() << COLOUR_NORMAL << std::endl;
		}

		// Store own [msgPrepare] in the log
		if (this->log.storeMsgPrepareHotstuff(msgPrepare) >= this->generalQuorumSize)
		{
			this->initiateMsgPrepareHotstuff(justification_MsgPrepare.getRoundData());
		}
	}
	else
	{
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Bad justification of MsgPrepare" << justification_MsgPrepare.toPrint() << COLOUR_NORMAL << std::endl;
		}
	}
}

void Hotstuff::initiateMsgPrepareHotstuff(RoundData roundData_MsgPrepare)
{
	View proposeView_MsgPrepare = roundData_MsgPrepare.getProposeView();
	Signs signs_MsgPrepare = this->log.getMsgPrepareHotstuff(proposeView_MsgPrepare, this->generalQuorumSize);
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "MsgPrepare signatures: " << signs_MsgPrepare.toPrint() << COLOUR_NORMAL << std::endl;
	}

	Justification justification_MsgPrepare = Justification(roundData_MsgPrepare, signs_MsgPrepare);
	if (signs_MsgPrepare.getSize() == this->generalQuorumSize)
	{
		// Create [msgPrepare]
		MsgPrepareHotstuff msgPrepare = MsgPrepareHotstuff(roundData_MsgPrepare, signs_MsgPrepare);

		// Send [msgPrepare] to replicas
		Peers recipients = this->removeFromPeers(this->replicaId);
		this->sendMsgPrepareHotstuff(msgPrepare, recipients);
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Sent MsgPrepare to replicas: " << msgPrepare.toPrint() << COLOUR_NORMAL << std::endl;
		}

		// Create [msgPrecommit]
		Justification justification_MsgPrecommit = this->saveMsgPrepareHotstuff(justification_MsgPrepare);
		RoundData roundData_MsgPrecommit = justification_MsgPrecommit.getRoundData();
		Signs signs_MsgPrecommit = justification_MsgPrecommit.getSigns();
		MsgPrecommitHotstuff msgPrecommit = MsgPrecommitHotstuff(roundData_MsgPrecommit, signs_MsgPrecommit);
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Hold on MsgPrecommit to its own: " << msgPrecommit.toPrint() << COLOUR_NORMAL << std::endl;
		}

		// Store own [msgPrecommit] in the log
		if (this->log.storeMsgPrecommitHotstuff(msgPrecommit) >= this->generalQuorumSize)
		{
			this->initiateMsgPrecommitHotstuff(justification_MsgPrecommit.getRoundData());
		}
	}
}

void Hotstuff::initiateMsgPrecommitHotstuff(RoundData roundData_MsgPrecommit)
{
	View proposeView_MsgPrecommit = roundData_MsgPrecommit.getProposeView();
	Signs signs_MsgPrecommit = this->log.getMsgPrecommitHotstuff(proposeView_MsgPrecommit, this->generalQuorumSize);
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "MsgPrecommit signatures: " << signs_MsgPrecommit.toPrint() << COLOUR_NORMAL << std::endl;
	}

	Justification justification_MsgPrecommit = Justification(roundData_MsgPrecommit, signs_MsgPrecommit);
	if (signs_MsgPrecommit.getSize() == this->generalQuorumSize)
	{
		// Create [msgPrecommit]
		MsgPrecommitHotstuff msgPrecommit = MsgPrecommitHotstuff(roundData_MsgPrecommit, signs_MsgPrecommit);

		// Send [msgPrecommit] to replicas
		Peers recipients = this->removeFromPeers(this->replicaId);
		this->sendMsgPrecommitHotstuff(msgPrecommit, recipients);
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Sent MsgPrecommit to replicas: " << msgPrecommit.toPrint() << COLOUR_NORMAL << std::endl;
		}

		// Create [msgCommit]
		Justification justification_MsgCommit = this->lockMsgPrecommitHotstuff(justification_MsgPrecommit);
		RoundData roundData_MsgCommit = justification_MsgCommit.getRoundData();
		Signs signs_MsgCommit = justification_MsgCommit.getSigns();
		MsgCommitHotstuff msgCommit = MsgCommitHotstuff(roundData_MsgCommit, signs_MsgCommit);
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Hold on MsgCommit to its own: " << msgCommit.toPrint() << COLOUR_NORMAL << std::endl;
		}

		// Store own [msgCommit] in the log
		if (this->log.storeMsgCommitHotstuff(msgCommit) >= this->generalQuorumSize)
		{
			this->initiateMsgCommitHotstuff(justification_MsgCommit.getRoundData());
		}
	}
}

void Hotstuff::initiateMsgCommitHotstuff(RoundData roundData_MsgCommit)
{
	View proposeView_MsgCommit = roundData_MsgCommit.getProposeView();
	Signs signs_MsgCommit = this->log.getMsgCommitHotstuff(proposeView_MsgCommit, this->generalQuorumSize);
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "MsgCommit signatures: " << signs_MsgCommit.toPrint() << COLOUR_NORMAL << std::endl;
	}

	if (signs_MsgCommit.getSize() == this->generalQuorumSize)
	{
		// Create [msgCommit]
		MsgCommitHotstuff msgCommit = MsgCommitHotstuff(roundData_MsgCommit, signs_MsgCommit);

		// Send [msgCommit] to replicas
		Peers recipients = this->removeFromPeers(this->replicaId);
		this->sendMsgCommitHotstuff(msgCommit, recipients);
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Sent MsgCommit to replicas: " << msgCommit.toPrint() << COLOUR_NORMAL << std::endl;
		}

		// Execute the block
		this->executeBlockHotstuff(roundData_MsgCommit);
	}
}

// Respond messages
void Hotstuff::respondMsgLdrprepareHotstuff(Justification justification_MsgNewview, Block block)
{
	// Create own [justification_MsgPrepare] for that [block]
	Justification justification_MsgPrepare = this->respondMsgLdrprepareProposalHotstuff(block.hash(), justification_MsgNewview);
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
		MsgPrepareHotstuff msgPrepare = MsgPrepareHotstuff(roundData_MsgPrepare, signs_MsgPrepare);

		// Send [msgPrepare] to leader
		Peers recipients = this->keepFromPeers(this->getCurrentLeader());
		this->sendMsgPrepareHotstuff(msgPrepare, recipients);
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Sent MsgPrepare to leader: " << msgPrepare.toPrint() << COLOUR_NORMAL << std::endl;
		}
	}
}

void Hotstuff::respondMsgPrepareHotstuff(Justification justification_MsgPrepare)
{
	// Create [justification_MsgPrecommit]
	Justification justification_MsgPrecommit = this->saveMsgPrepareHotstuff(justification_MsgPrepare);

	// Create [msgPrecommit]
	RoundData roundData_MsgPrecommit = justification_MsgPrecommit.getRoundData();
	Signs signs_MsgPrecommit = justification_MsgPrecommit.getSigns();
	MsgPrecommitHotstuff msgPrecommit = MsgPrecommitHotstuff(roundData_MsgPrecommit, signs_MsgPrecommit);

	// Send [msgPrecommit] to leader
	Peers recipients = this->keepFromPeers(this->getCurrentLeader());
	this->sendMsgPrecommitHotstuff(msgPrecommit, recipients);
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Sent MsgPrecommit to leader: " << msgPrecommit.toPrint() << COLOUR_NORMAL << std::endl;
	}
}

void Hotstuff::respondMsgPrecommitHotstuff(Justification justification_MsgPrecommit)
{
	// Create [justification_MsgCommit]
	Justification justification_MsgCommit = this->lockMsgPrecommitHotstuff(justification_MsgPrecommit);

	// Create [msgCommit]
	RoundData roundData_MsgCommit = justification_MsgCommit.getRoundData();
	Signs signs_MsgCommit = justification_MsgCommit.getSigns();
	MsgCommitHotstuff msgCommit = MsgCommitHotstuff(roundData_MsgCommit, signs_MsgCommit);

	// Send [msgCommit] to leader
	Peers recipients = this->keepFromPeers(this->getCurrentLeader());
	this->sendMsgCommitHotstuff(msgCommit, recipients);
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Sent MsgCommit to leader: " << msgCommit.toPrint() << COLOUR_NORMAL << std::endl;
	}
}

// Main functions
void Hotstuff::getStarted()
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

	Justification justification_MsgNewview = this->initializeMsgNewviewHotstuff();
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Initial justification: " << justification_MsgNewview.toPrint() << COLOUR_NORMAL << std::endl;
	}
	RoundData roundData_MsgNewview = justification_MsgNewview.getRoundData();
	Signs signs_MsgNewview = justification_MsgNewview.getSigns();
	MsgNewviewHotstuff msgNewview = MsgNewviewHotstuff(roundData_MsgNewview, signs_MsgNewview);
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Starting with: " << msgNewview.toPrint() << COLOUR_NORMAL << std::endl;
	}
	if (this->amCurrentLeader())
	{
		this->handleMsgNewviewHotstuff(msgNewview);
	}
	else
	{
		this->sendMsgNewviewHotstuff(msgNewview, recipients);
	}
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Sent MsgNewview to leader[" << leader << "]" << COLOUR_NORMAL << std::endl;
	}
}

void Hotstuff::startNewViewHotstuff()
{
	// Generate [justification_MsgNewview] until one for the next view
	Justification justification_MsgNewview = this->initializeMsgNewviewHotstuff();
	View proposeView_MsgNewview = justification_MsgNewview.getRoundData().getProposeView();
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Generating justification: " << justification_MsgNewview.toPrint() << COLOUR_NORMAL << std::endl;
	}
	while (proposeView_MsgNewview <= this->view)
	{
		justification_MsgNewview = this->initializeMsgNewviewHotstuff();
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
		MsgNewviewHotstuff msgNewview = MsgNewviewHotstuff(roundData_MsgNewview, signs_MsgNewview);
		if (this->amCurrentLeader())
		{
			this->handleEarlierMessagesHotstuff();
			this->handleMsgNewviewHotstuff(msgNewview);
		}
		else
		{
			ReplicaID leader = this->getCurrentLeader();
			Peers recipients = this->keepFromPeers(leader);
			this->sendMsgNewviewHotstuff(msgNewview, recipients);
			this->handleEarlierMessagesHotstuff();
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

Block Hotstuff::createNewBlockHotstuff(Hash previousHash)
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
	Block block = Block(previousHash, size, transaction);
	return block;
}

void Hotstuff::executeBlockHotstuff(RoundData roundData_MsgCommit)
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
				  << "HOTSTUFF-EXECUTE(" << this->view << "/" << std::to_string(this->numViews - 1) << ":" << time << ") "
				  << statistics.toString() << COLOUR_NORMAL << std::endl;
	}

	this->replyHash(roundData_MsgCommit.getProposeHash());
	if (this->timeToStop())
	{
		this->recordStatisticsHotstuff();
	}
	else
	{
		this->startNewViewHotstuff();
	}
}

bool Hotstuff::timeToStop()
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

void Hotstuff::recordStatisticsHotstuff()
{
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Printing statistics: " << COLOUR_NORMAL << std::endl;
	}

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
Hotstuff::Hotstuff(KeysFunctions keysFunctions, ReplicaID replicaId, unsigned int numGeneralReplicas, unsigned int numTrustedReplicas, unsigned int numReplicas, unsigned int numViews, unsigned int numFaults, double leaderChangeTime, Nodes nodes, Key privateKey, PeerNet::Config peerNetConfig, ClientNet::Config clientNetConfig) : peerNet(peerEventContext, peerNetConfig), clientNet(requestEventContext, clientNetConfig)
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
	this->trustedQuorumSize = 0;
	this->view = 0;

	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Starting handler" << COLOUR_NORMAL << std::endl;
	}
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "General quorum size: " << this->generalQuorumSize << COLOUR_NORMAL << std::endl;
	}

	hotstuffBasic = HotstuffBasic(this->replicaId, this->privateKey, this->generalQuorumSize);

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
													this->startNewViewHotstuff();
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

	this->clientNet.reg_handler(salticidae::generic_bind(&Hotstuff::receiveMsgStartHotstuff, this, _1, _2));
	this->clientNet.reg_handler(salticidae::generic_bind(&Hotstuff::receiveMsgTransactionHotstuff, this, _1, _2));

	this->peerNet.reg_handler(salticidae::generic_bind(&Hotstuff::receiveMsgNewviewHotstuff, this, _1, _2));
	this->peerNet.reg_handler(salticidae::generic_bind(&Hotstuff::receiveMsgPrepareHotstuff, this, _1, _2));
	this->peerNet.reg_handler(salticidae::generic_bind(&Hotstuff::receiveMsgLdrprepareHotstuff, this, _1, _2));
	this->peerNet.reg_handler(salticidae::generic_bind(&Hotstuff::receiveMsgPrecommitHotstuff, this, _1, _2));
	this->peerNet.reg_handler(salticidae::generic_bind(&Hotstuff::receiveMsgCommitHotstuff, this, _1, _2));

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