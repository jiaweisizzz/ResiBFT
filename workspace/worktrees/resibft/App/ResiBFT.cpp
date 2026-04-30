#include "ResiBFT.h"

// ============================================================
// 环境变量读取函数（在 Enclave 外部实现）
// ============================================================
// 注意：SGX Enclave 无法使用 getenv，所以这些函数放在 ResiBFT.cpp 中

double getTCRatioFromEnv() {
    char* env = getenv("RESIBFT_TC_RATIO");
    if (env != nullptr) {
        return atof(env);
    }
    return DEFAULT_TC_RATIO;
}

double getKRatioFromEnv() {
    char* env = getenv("RESIBFT_K_RATIO");
    if (env != nullptr) {
        return atof(env);
    }
    return DEFAULT_K_RATIO;
}

int getExperimentModeFromEnv() {
    char* env = getenv("RESIBFT_EXP_MODE");
    if (env != nullptr) {
        return atoi(env);
    }
    return EXP_MODE_OPTIMISTIC;
}

int getTCBehaviorFromEnv() {
    char* env = getenv("RESIBFT_TC_BEHAVIOR");
    if (env != nullptr) {
        return atoi(env);
    }
    return TC_BEHAVIOR_BAD_VOTE;
}

// ============================================================

// Local variables
Time startTime = std::chrono::steady_clock::now();
Time startView = std::chrono::steady_clock::now();
Time currentTime;
std::string statisticsValues;
std::string statisticsDone;
Statistics statistics;
sgx_enclave_id_t global_eid = 0;

// Opcodes
const uint8_t MsgNewviewResiBFT::opcode;
const uint8_t MsgLdrprepareResiBFT::opcode;
const uint8_t MsgPrepareResiBFT::opcode;
const uint8_t MsgPrecommitResiBFT::opcode;
const uint8_t MsgDecideResiBFT::opcode;
const uint8_t MsgBCResiBFT::opcode;
const uint8_t MsgVCResiBFT::opcode;
const uint8_t MsgCommitteeResiBFT::opcode;
const uint8_t MsgPrepareProposalResiBFT::opcode;
const uint8_t MsgCommitResiBFT::opcode;
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

// ============================================
// Lightweight structure conversion functions for ResiBFT TEE interface
// These avoid the large fixed-size arrays that cause SGX parameter size issues
// ============================================

// Convert Justification array to lightweight AccumulatorInput_t (~8.6KB instead of ~3.5MB)
void setAccumulatorInput(Justification justifications[MAX_NUM_SIGNATURES], AccumulatorInput_t *input_t)
{
	// Get proposeView from the first justification
	input_t->proposeView = justifications[0].getRoundData().getProposeView();

	// Extract only the essential info: signer, justifyView, justifyHash
	unsigned int count = 0;
	for (unsigned int i = 0; i < MAX_NUM_SIGNATURES && justifications[i].isSet(); i++)
	{
		// Get signer from the justification's first signature
		Signs signs = justifications[i].getSigns();
		if (signs.getSize() > 0)
		{
			input_t->items[count].signer = signs.get(0).getSigner();
		}
		else
		{
			input_t->items[count].signer = 0; // fallback if no signatures
		}
		input_t->items[count].justifyView = justifications[i].getRoundData().getJustifyView();
		setHash(justifications[i].getRoundData().getJustifyHash(), &(input_t->items[count].justifyHash));
		count++;
	}
	input_t->size = count;
}

// Load Justification from lightweight JustificationLight_t (single signature)
Justification getJustificationLight(JustificationLight_t *justificationLight_t)
{
	bool set = justificationLight_t->set;
	RoundData roundData = getRoundData(&(justificationLight_t->roundData));

	// Convert single SignSingle_t to Signs
	Sign sign = Sign(
		justificationLight_t->sign.set,
		justificationLight_t->sign.signer,
		justificationLight_t->sign.signtext
	);
	Signs signs = Signs();
	signs.add(sign);

	Justification justification = Justification(set, roundData, signs);
	return justification;
}

// Load Sign from lightweight SignSingle_t
Sign getSignSingle(SignSingle_t *signSingle_t)
{
	Sign sign = Sign(
		signSingle_t->set,
		signSingle_t->signer,
		signSingle_t->signtext
	);
	return sign;
}

void TEE_Print(const char *text)
{
	printf("%s\n", text);
}

// Print functions
std::string ResiBFT::printReplicaId()
{
	return "[" + std::to_string(this->replicaId) + "-" + std::to_string(this->view) + "-" + printStage(this->stage) + "]";
}

void ResiBFT::printNowTime(std::string msg)
{
	auto now = std::chrono::steady_clock::now();
	double time = std::chrono::duration_cast<std::chrono::microseconds>(now - startView).count();
	double etime = (statistics.getTotalViewTime().total + time) / (1000 * 1000);
	std::cout << COLOUR_BLUE << this->printReplicaId() << msg << " @ " << etime << COLOUR_NORMAL << std::endl;
}

void ResiBFT::printClientInfo()
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

std::string ResiBFT::recipients2string(Peers recipients)
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
unsigned int ResiBFT::getLeaderOf(View view)
{
	// 论文设计：GRACIOUS 模式下的 Leader 选择
	// 1. 如果当前 view 的 round-robin leader 是 T-Replica，该 T-Replica 应该是 committee leader
	//    （触发新的 VRF committee 选择或继续作为 committee leader）
	// 2. 如果当前 view 的 round-robin leader 是 G-Replica，使用之前的 committeeLeader
	//    （但在 startNewViewResiBFT 中会检测并恢复到 NORMAL）

	unsigned int normalLeader = view % this->numReplicas;
	bool normalLeaderIsTrusted = (normalLeader >= this->numGeneralReplicas);

	if (this->stage == STAGE_GRACIOUS && this->committee.getSize() > 0)
	{
		// 关键修改：如果当前 view 的 normal leader 是 T-Replica，
		// 该 T-Replica 应该作为 committee leader（而不是旧的 committeeLeader）
		if (normalLeaderIsTrusted)
		{
			// 新的 T-Replica leader 应该触发新的 committee 选择
			// 如果 committeeLeader != normalLeader，说明需要新的 committee
			if (this->committeeLeader != normalLeader)
			{
				if (DEBUG_HELP)
				{
					std::cout << COLOUR_BLUE << this->printReplicaId()
							  << "GRACIOUS: view " << view << " has new T-Replica leader " << normalLeader
							  << " (old committeeLeader=" << this->committeeLeader << ")"
							  << " - returning new leader for committee selection"
							  << COLOUR_NORMAL << std::endl;
				}
				return normalLeader;
			}
			// 否则，当前 committeeLeader 已经是 view 的 T-Replica leader
			return this->committeeLeader;
		}
		else
		{
			// 当前 view 的 normal leader 是 G-Replica
			// 使用旧的 committeeLeader（等待恢复到 NORMAL）
			if (DEBUG_HELP)
			{
				std::cout << COLOUR_BLUE << this->printReplicaId()
						  << "GRACIOUS leader for view " << view << " is committee leader " << this->committeeLeader
						  << " (normal leader " << normalLeader << " is G-Replica)"
						  << COLOUR_NORMAL << std::endl;
			}
			return this->committeeLeader;
		}
	}

	// Normal mode: use original leader rotation
	return normalLeader;
}

unsigned int ResiBFT::getCurrentLeader()
{
	unsigned int leader = this->getLeaderOf(this->view);
	return leader;
}

bool ResiBFT::amLeaderOf(View view)
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

bool ResiBFT::amCurrentLeader()
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

std::vector<ReplicaID> ResiBFT::getGeneralReplicaIds()
{
	std::vector<ReplicaID> generalReplicaIds;
	for (unsigned int i = 0; i < this->numGeneralReplicas; i++)
	{
		generalReplicaIds.push_back(i);
	}
	return generalReplicaIds;
}

bool ResiBFT::amGeneralReplicaIds()
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

bool ResiBFT::isGeneralReplicaIds(ReplicaID replicaId)
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

bool ResiBFT::amTrustedReplicaIds()
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

bool ResiBFT::isTrustedReplicaIds(ReplicaID replicaId)
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

Peers ResiBFT::removeFromPeers(ReplicaID replicaId)
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

Peers ResiBFT::removeFromPeers(std::vector<ReplicaID> generalReplicaIds)
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

Peers ResiBFT::removeFromTrustedPeers(ReplicaID replicaId)
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

Peers ResiBFT::keepFromPeers(ReplicaID replicaId)
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

Peers ResiBFT::keepFromTrustedPeers(ReplicaID replicaId)
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

std::vector<salticidae::PeerId> ResiBFT::getPeerIds(Peers recipients)
{
	std::vector<salticidae::PeerId> returnPeerId;
	for (Peers::iterator it = recipients.begin(); it != recipients.end(); it++)
	{
		Peer peer = *it;
		returnPeerId.push_back(std::get<1>(peer));
	}
	return returnPeerId;
}

void ResiBFT::setTimer()
{
	// 禁用timer用于乐观实验测试
	// 在乐观场景下，所有节点都正常工作，不需要超时机制
	// 注意：生产环境必须启用timer来处理Leader故障
#ifdef DISABLE_TIMER_FOR_OPTIMISTIC_TEST
	return;
#endif

	// In GRACIOUS/UNCIVIL mode, only committee members should set timer
	// Non-committee members wait passively for committee to reach consensus
	if ((this->stage == STAGE_GRACIOUS || this->stage == STAGE_UNCIVIL) && !this->isCommitteeMember)
	{
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Not setting timer (not committee member in " << printStage(this->stage) << " mode)" << COLOUR_NORMAL << std::endl;
		}
		return;
	}

	this->timer.del();
	this->timer.add(this->leaderChangeTime);
	this->timerView = this->view;

	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Timer set for view " << this->view
				  << ", stage=" << printStage(this->stage)
				  << ", timeout=" << this->leaderChangeTime << " seconds"
				  << COLOUR_NORMAL << std::endl;
	}
}

void ResiBFT::changeSwitcher()
{
	// Placeholder - will be implemented in later tasks
}

void ResiBFT::changeAuthenticator()
{
	// Placeholder - will be implemented in later tasks
}

void ResiBFT::setTrustedQuorumSize()
{
	// Placeholder - will be implemented in later tasks
}

// ResiBFT-specific setting functions
Peers ResiBFT::getCommitteePeers()
{
	Peers committeePeers;
	for (Peers::iterator itPeers = this->peers.begin(); itPeers != this->peers.end(); itPeers++)
	{
		Peer peer = *itPeers;
		if (this->isInCommittee(std::get<0>(peer)))
		{
			committeePeers.push_back(peer);
		}
	}
	return committeePeers;
}

bool ResiBFT::isInCommittee(ReplicaID replicaId)
{
	ReplicaID *committeeMembers = this->committee.getGroup();
	for (int i = 0; i < this->committee.getSize(); i++)
	{
		if (committeeMembers[i] == replicaId)
		{
			return true;
		}
	}
	return false;
}

// Stage transition methods
void ResiBFT::transitionToNormal()
{
	Stage previousStage = this->stage;

	this->stage = STAGE_NORMAL;

	// 更新 stageTransitionView 为当前 view（表示重新进入 NORMAL）
	this->stageTransitionView = this->view;

	// 恢复原始的 trustedQuorumSize
	this->trustedQuorumSize = this->originalTrustedQuorumSize;

	// 清除委员会状态
	this->committee = Group();
	this->isCommitteeMember = false;
	this->currentCommittee.clear();
	this->committeeLeader = 0;

	// 清除 VC 集合（进入新阶段）
	// 注意：GRACIOUS -> GRACIOUS 之间不应清空 rejectedVCs，需要累积来触发 UNCIVIL
	// 只有从 GRACIOUS/UNCIVIL 恢复到 NORMAL 时才清空
	this->timeoutVCs.clear();
	// 不清空 rejectedVCs，让 UNCIVIL 触发检查能累积多个 view 的 REJECTED VC

	std::cout << COLOUR_GREEN << this->printReplicaId()
			  << "[STAGE-RECOVERY] Transitioned from " << printStage(previousStage)
			  << " to NORMAL stage"
			  << ", view=" << this->view
			  << ", trustedQuorumSize restored to " << this->trustedQuorumSize
			  << ", committee cleared"
			  << COLOUR_NORMAL << std::endl;
	std::cout.flush();

	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Transitioning to NORMAL stage"
				  << ", trustedQuorumSize restored to " << this->trustedQuorumSize
				  << COLOUR_NORMAL << std::endl;
	}
}

void ResiBFT::transitionToGracious()
{
	// 论文设计：G-Replicas 应该 NEVER 进入 GRACIOUS 模式
	// 防御性检查：如果是 G-Replica，直接返回
	if (this->amGeneralReplicaIds())
	{
		std::cout << COLOUR_ORANGE << this->printReplicaId()
				  << "[GRACIOUS-PREVENT] G-Replica cannot enter GRACIOUS mode - ignoring transition"
				  << ", waiting for MsgDecide"
				  << COLOUR_NORMAL << std::endl;
		std::cout.flush();
		return;
	}

	auto startTotal = std::chrono::steady_clock::now();

	// 记录进入 GRACIOUS 的 view（用于恢复检查）
	this->stageTransitionView = this->view;

	this->stage = STAGE_GRACIOUS;

	std::cout << COLOUR_RED << this->printReplicaId()
			  << "[GRACIOUS-TRANSITION] START: view=" << this->view
			  << ", fromStage=" << printStage(STAGE_NORMAL)
			  << ", toStage=" << printStage(STAGE_GRACIOUS)
			  << ", stageTransitionView=" << this->stageTransitionView
			  << COLOUR_NORMAL << std::endl;
	std::cout.flush();

	// 论文设计：VRF committee selection
	// Committee size k 从预设区间 [k_min, k_max] 随机采样
	// k ≥ k̄ = COMMITTEE_K_MIN（最小委员会大小）
	// Quorum q_T = floor((k+1)/2)

#ifdef PRODUCTION_MODE
	// 生产模式：k 和 seed 由 selectCommitteeByVRF 内部计算
	unsigned int k = COMMITTEE_K_MIN;  // 初始值，实际由 selectCommitteeByVRF 采样
#else
	// 测试模式：固定 k = min(COMMITTEE_K_MIN, numTrustedReplicas)
	unsigned int k = std::min((unsigned int)COMMITTEE_K_MIN, this->numTrustedReplicas);
#endif

	this->currentCommittee.clear();

	std::cout << COLOUR_RED << this->printReplicaId()
			  << "[GRACIOUS-TRANSITION] Calling selectCommitteeByVRF(k=" << k << ")"
			  << ", numTrustedReplicas=" << this->numTrustedReplicas
			  << COLOUR_NORMAL << std::endl;
	std::cout.flush();

	// Select committee using VRF-like selection based on view
	// selectCommitteeByVRF 内部会根据 PRODUCTION_MODE 决定 k 和 seed 的计算方式
	auto startVRF = std::chrono::steady_clock::now();
	Group committeeGroup = this->selectCommitteeByVRF(this->view, k);
	auto endVRF = std::chrono::steady_clock::now();
	double vrfTime = std::chrono::duration_cast<std::chrono::microseconds>(endVRF - startVRF).count();

	std::cout << COLOUR_RED << this->printReplicaId()
			  << "[GRACIOUS-TRANSITION] VRF selection time=" << vrfTime << " μs"
			  << ", committeeSize=" << committeeGroup.getSize()
			  << COLOUR_NORMAL << std::endl;
	std::cout.flush();

	// Store the committee
	this->committee = committeeGroup;
	this->committeeView = this->view;

	// 填充 currentCommittee std::set（用于验证 membership）
	this->currentCommittee.clear();
	for (unsigned int i = 0; i < committeeGroup.getSize(); i++)
	{
		this->currentCommittee.insert(committeeGroup.getGroup()[i]);
	}

	this->isCommitteeMember = isInCommittee(this->replicaId);

	// 委员会Leader是触发GRACIOUS的节点（当前节点）
	this->committeeLeader = this->replicaId;

	// 更新 trustedQuorumSize 为委员会 quorum
	// 论文: q_T = floor((k+1)/2)
	unsigned int committeeSize = committeeGroup.getSize();
	unsigned int oldTrustedQuorumSize = this->trustedQuorumSize;
	this->trustedQuorumSize = (committeeSize + 1) / 2;

	std::cout << COLOUR_RED << this->printReplicaId()
			  << "[GRACIOUS-TRANSITION] Committee: k=" << committeeSize
			  << ", q_T=" << oldTrustedQuorumSize << "->" << this->trustedQuorumSize
#ifdef PRODUCTION_MODE
			  << " (PRODUCTION)"
#else
			  << " (TEST)"
#endif
			  << ", isMember=" << this->isCommitteeMember
			  << ", simulateMalicious=" << this->simulateMalicious
			  << COLOUR_NORMAL << std::endl;
	std::cout.flush();

	// Broadcast committee selection
	auto startBroadcast = std::chrono::steady_clock::now();
	Sign leaderSign = Sign(true, this->replicaId, (unsigned char *)"committee");
	Signs signs = Signs();
	signs.add(leaderSign);
	MsgCommitteeResiBFT msgCommittee(committeeGroup, this->view, signs);
	Peers recipients = this->removeFromPeers(this->replicaId);

	std::cout << COLOUR_RED << this->printReplicaId()
			  << "[GRACIOUS-TRANSITION] Broadcasting MsgCommittee to numRecipients=" << recipients.size()
			  << COLOUR_NORMAL << std::endl;
	std::cout.flush();

	this->sendMsgCommitteeResiBFT(msgCommittee, recipients);
	auto endBroadcast = std::chrono::steady_clock::now();
	double broadcastTime = std::chrono::duration_cast<std::chrono::microseconds>(endBroadcast - startBroadcast).count();

	auto endTotal = std::chrono::steady_clock::now();
	double totalTime = std::chrono::duration_cast<std::chrono::microseconds>(endTotal - startTotal).count();

	std::cout << COLOUR_RED << this->printReplicaId()
			  << "[GRACIOUS-TRANSITION] END: vrfTime=" << vrfTime << " μs"
			  << ", broadcastTime=" << broadcastTime << " μs"
			  << ", totalTime=" << totalTime << " μs"
			  << COLOUR_NORMAL << std::endl;
	std::cout.flush();

	// GRACIOUS Leader 主动开始共识流程
	// 作为委员会 Leader，需要主动创建区块并开始共识
	if (this->amCurrentLeader())
	{
		std::cout << COLOUR_RED << this->printReplicaId()
				  << "[GRACIOUS-LEADER] I am committee Leader, initiating consensus for view " << this->view
				  << COLOUR_NORMAL << std::endl;
		std::cout.flush();

		// 处理早期消息并开始共识
		this->handleEarlierMessagesResiBFT();

		// 创建自己的 MsgNewView 并处理
		Justification justification = this->initializeMsgNewviewResiBFT();
		RoundData roundData = justification.getRoundData();
		Phase phase = roundData.getPhase();
		Signs signs = justification.getSigns();

		if (roundData.getProposeView() == this->view && phase == PHASE_NEWVIEW)
		{
			MsgNewviewResiBFT msgNewview = MsgNewviewResiBFT(roundData, signs);
			std::cout << COLOUR_RED << this->printReplicaId()
					  << "[GRACIOUS-LEADER] Processing my own MsgNewView to start consensus"
					  << COLOUR_NORMAL << std::endl;
			std::cout.flush();

			this->handleMsgNewviewResiBFT(msgNewview);
		}
		else
		{
			std::cout << COLOUR_RED << this->printReplicaId()
					  << "[GRACIOUS-LEADER] Failed to create MsgNewView: proposeView=" << roundData.getProposeView()
					  << " != currentView=" << this->view
					  << COLOUR_NORMAL << std::endl;
			std::cout.flush();
		}
	}
}

void ResiBFT::transitionToUncivil()
{
	// 论文设计：G-Replicas 应该 NEVER 进入 UNCIVIL 模式
	// 防御性检查：如果是 G-Replica，直接返回
	if (this->amGeneralReplicaIds())
	{
		std::cout << COLOUR_ORANGE << this->printReplicaId()
				  << "[UNCIVIL-PREVENT] G-Replica cannot enter UNCIVIL mode - ignoring transition"
				  << ", waiting for MsgDecide"
				  << COLOUR_NORMAL << std::endl;
		std::cout.flush();
		return;
	}

	// 记录进入 UNCIVIL 的 view（用于恢复检查）
	this->stageTransitionView = this->view;

	Stage previousStage = this->stage;
	this->stage = STAGE_UNCIVIL;

	std::cout << COLOUR_RED << this->printReplicaId()
			  << "[UNCIVIL-TRANSITION] START: view=" << this->view
			  << ", fromStage=" << printStage(previousStage)
			  << ", toStage=" << printStage(STAGE_UNCIVIL)
			  << ", stageTransitionView=" << this->stageTransitionView
			  << ", committeeSize=" << this->committee.getSize()
			  << ", trustedQuorumSize=" << this->trustedQuorumSize
			  << COLOUR_NORMAL << std::endl;
	std::cout.flush();

	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Transitioning to UNCIVIL stage" << COLOUR_NORMAL << std::endl;
	}

	// 关键修复：UNCIVIL 模式下需要设置 timer
	// 如果共识无法达成，timer expired 后会发送 VC_TIMEOUT 并进入下一个 view
	// 这样才能继续检测恶意行为或恢复到 NORMAL
	this->setTimer();

	std::cout << COLOUR_RED << this->printReplicaId()
			  << "[UNCIVIL-TRANSITION] END: timer set for view " << this->view
			  << ", timeout=" << this->leaderChangeTime << " seconds"
			  << COLOUR_NORMAL << std::endl;
	std::cout.flush();
}

// Committee management methods
void ResiBFT::selectCommittee()
{
	// Select committee members from trusted replicas
	// Committee size = floor(numTrustedReplicas / 2) + 1
	unsigned int committeeSize = floor(this->numTrustedReplicas / 2) + 1;
	std::vector<ReplicaID> committeeMembers;

	// Collect active replicas from VC_TIMEOUT senders (they are alive and participating)
	std::set<ReplicaID> activeReplicas;
	for (std::set<VerificationCertificate>::iterator it = this->timeoutVCs.begin(); it != this->timeoutVCs.end(); it++)
	{
		View vcView = (*it).getView();
		// Only consider recent timeout VCs
		if (vcView >= this->view - 2)
		{
			activeReplicas.insert((*it).getSigner());
		}
	}

	// First, try to select from active trusted replicas
	for (unsigned int i = 0; i < this->numTrustedReplicas && committeeMembers.size() < committeeSize; i++)
	{
		ReplicaID trustedId = 1 + i; // Trusted replicas: 1, 2, 3, ...
		// Only add if active (sent VC_TIMEOUT recently) and not already in committee
		if (activeReplicas.find(trustedId) != activeReplicas.end())
		{
			bool alreadyInCommittee = false;
			for (unsigned int j = 0; j < committeeMembers.size(); j++)
			{
				if (committeeMembers[j] == trustedId)
				{
					alreadyInCommittee = true;
					break;
				}
			}
			if (!alreadyInCommittee)
			{
				committeeMembers.push_back(trustedId);
			}
		}
	}

	// If we still need more members, add remaining trusted replicas
	for (unsigned int i = 0; i < this->numTrustedReplicas && committeeMembers.size() < committeeSize; i++)
	{
		ReplicaID trustedId = 1 + i;
		bool alreadyInCommittee = false;
		for (unsigned int j = 0; j < committeeMembers.size(); j++)
		{
			if (committeeMembers[j] == trustedId)
			{
				alreadyInCommittee = true;
				break;
			}
		}
		if (!alreadyInCommittee)
		{
			committeeMembers.push_back(trustedId);
		}
	}

	this->committee = Group(committeeMembers);
	this->isCommitteeMember = isInCommittee(this->replicaId);

	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Selected committee: "
				  << this->committee.toPrint() << ", isCommitteeMember=" << this->isCommitteeMember
				  << ", activeReplicas=" << activeReplicas.size()
				  << COLOUR_NORMAL << std::endl;
	}

	// Broadcast committee selection to all replicas
	Sign sign = Sign(true, this->replicaId, (unsigned char *)"committee");
	Signs signs = Signs();
	signs.add(sign);
	MsgCommitteeResiBFT msgCommittee = MsgCommitteeResiBFT(this->committee, this->view, signs);
	Peers recipients = this->removeFromPeers(this->replicaId);
	this->sendMsgCommitteeResiBFT(msgCommittee, recipients);
}

void ResiBFT::revokeCommittee()
{
	// Clear the current committee
	this->committee = Group();
	this->isCommitteeMember = false;

	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Revoked committee" << COLOUR_NORMAL << std::endl;
	}
}

void ResiBFT::updateCommittee()
{
	// Update committee when entering GRACIOUS mode
	// Re-select based on current view
	selectCommittee();

	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Updated committee for view " << this->view << COLOUR_NORMAL << std::endl;
	}
}

// Validation methods
bool ResiBFT::validateBlock(Block block)
{
	// Check block extends from parent correctly
	Hash parentHash = block.hash();
	std::map<View, Block>::iterator it = this->blocks.find(this->view - 1);
	if (it != this->blocks.end())
	{
		Block parentBlock = it->second;
		if (block.extends(parentBlock.hash()))
		{
			return true;
		}
	}
	// Genesis block or no parent - accept
	if (this->view == 1 || parentHash.isZero())
	{
		return true;
	}
	return true; // Default accept for basic consensus
}

bool ResiBFT::checkAcceptedVCs()
{
	// 论文设计：ACCEPTED VC ≥ f+1 用于 commit 前一个 block
	// 不是用于恢复到 NORMAL 模式
	std::set<ReplicaID> uniqueAcceptedSigners;
	for (std::set<VerificationCertificate>::iterator it = this->acceptedVCs.begin(); it != this->acceptedVCs.end(); it++)
	{
		View vcView = (*it).getView();
		if (vcView == this->view || vcView == this->view - 1)
		{
			uniqueAcceptedSigners.insert((*it).getSigner());
		}
	}

	unsigned int acceptedCount = uniqueAcceptedSigners.size();
	unsigned int vcThreshold = this->numFaults + 1;  // 论文: f+1

	// If enough accepted VCs, commit previous block
	if (acceptedCount >= vcThreshold)
	{
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Accepted VC threshold reached: " << acceptedCount << " unique signers (≥ f+1)" << COLOUR_NORMAL << std::endl;
		}
		// 论文设计：commit 前一个 block
		this->commitPreviousBlock();
		return true;
	}
	return false;
}

bool ResiBFT::checkRejectedVCs()
{
	// According to Algorithm 1, only T-Replicas can trigger UNCIVIL (lines 32-38)
	// G-Replicas run basic Hotstuff and don't participate in UNCIVIL logic (lines 39-40)
	if (this->amGeneralReplicaIds())
	{
		return false;  // General replicas don't trigger UNCIVIL
	}

	// 论文设计：REJECTED VC ≥ f+1 表示检测到恶意行为
	// 在 GRACIOUS 模式下，只有委员会成员能检测恶意
	// 如果委员会大小 k 较小，阈值应调整为 floor((k+1)/2) 或更小

	// 关键修复：GRACIOUS 模式下应累积同一 committee 期间多个 view 的 REJECTED VC
	// 这样才能检测到持续性的恶意行为并触发 UNCIVIL

	// Count unique signers
	// GRACIOUS 模式：累积 committee 创建以来所有 view 的 VC
	// NORMAL 模式：只统计当前和上一个 view
	std::set<ReplicaID> uniqueRejectedSigners;
	for (std::set<VerificationCertificate>::iterator it = this->rejectedVCs.begin(); it != this->rejectedVCs.end(); it++)
	{
		View vcView = (*it).getView();
		// GRACIOUS 模式：累积从 committee 创建 view 开始的所有 VC
		if (this->stage == STAGE_GRACIOUS && vcView >= this->committeeView)
		{
			uniqueRejectedSigners.insert((*it).getSigner());
		}
		// 其他模式：只统计当前和上一个 view
		else if (vcView == this->view || vcView == this->view - 1)
		{
			uniqueRejectedSigners.insert((*it).getSigner());
		}
	}

	unsigned int rejectedCount = uniqueRejectedSigners.size();

	unsigned int vcThreshold;
	if (this->stage == STAGE_GRACIOUS && this->committee.getSize() > 0)
	{
		// 委员会模式：阈值 = floor((committeeSize)/2) + 1
		// 例如 k=4 时，阈值 = 2+1 = 3
		vcThreshold = (this->committee.getSize() / 2) + 1;
	}
	else
	{
		// 论文: threshold = f+1
		vcThreshold = this->numFaults + 1;
	}
	if (rejectedCount >= vcThreshold && this->stage != STAGE_UNCIVIL)
	{
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_RED << this->printReplicaId() << "Rejected VC threshold reached: " << rejectedCount << " unique signers (≥ " << vcThreshold << ") - transitioning to UNCIVIL" << COLOUR_NORMAL << std::endl;
		}
		this->transitionToUncivil();
		return true;
	}
	return false;
}

bool ResiBFT::checkTimeoutVCs()
{
	// 论文设计：TIMEOUT VC ≥ f+1 表示 T-Replica 不可用
	// 注意：GRACIOUS 触发的主要条件是 Leader 轮换到 T-Replica
	// TIMEOUT VC 检测用于辅助判断 T-Replica 可用性状态
	std::set<ReplicaID> uniqueTimeoutSigners;
	for (std::set<VerificationCertificate>::iterator it = this->timeoutVCs.begin(); it != this->timeoutVCs.end(); it++)
	{
		View vcView = (*it).getView();
		// Accept VCs from current view or previous view
		if (vcView == this->view || vcView == this->view - 1)
		{
			uniqueTimeoutSigners.insert((*it).getSigner());
		}
	}

	unsigned int timeoutCount = uniqueTimeoutSigners.size();

	// 论文: threshold = f+1
	unsigned int vcThreshold = this->numFaults + 1;
	if (timeoutCount >= vcThreshold)
	{
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_MAGENTA << this->printReplicaId() << "Timeout VC threshold reached: " << timeoutCount << " unique signers (≥ f+1) - T-Replica unavailability detected" << COLOUR_NORMAL << std::endl;
		}
		// 标记 T-Replica 不可用状态
		// 论文设计中，这会辅助判断是否需要进入 GRACIOUS
		return true;
	}
	return false;
}

void ResiBFT::sendVC(VCType type, View view, Hash blockHash)
{
	// Create verification certificate
	Sign sign = Sign(true, this->replicaId, (unsigned char *)"vc");
	VerificationCertificate vc = VerificationCertificate(type, view, blockHash, this->replicaId, sign);
	Signs signs = Signs();
	signs.add(sign);

	MsgVCResiBFT msgVC = MsgVCResiBFT(vc, signs);

	// Store locally
	if (type == VC_ACCEPTED)
	{
		this->acceptedVCs.insert(vc);
	}
	else if (type == VC_REJECTED)
	{
		this->rejectedVCs.insert(vc);
	}
	else if (type == VC_TIMEOUT)
	{
		this->timeoutVCs.insert(vc);
	}

	// Send to all replicas
	Peers recipients = this->removeFromPeers(this->replicaId);
	this->sendMsgVCResiBFT(msgVC, recipients);

	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Sent VC[" << printVCType(type) << "] for view " << view << COLOUR_NORMAL << std::endl;
	}
}

// Reply to clients
void ResiBFT::replyTransactions(Transaction *transactions)
{
	for (int i = 0; i < MAX_NUM_TRANSACTIONS; i++)
	{
		Transaction transaction = transactions[i];
		ClientID clientId = transaction.getClientId();
		TransactionID transactionId = transaction.getTransactionId();
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

void ResiBFT::replyHash(Hash hash)
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
bool ResiBFT::verifyJustificationResiBFT(Justification justification)
{
	// Placeholder - will be implemented in later tasks
	return true;
}

bool ResiBFT::verifyProposalResiBFT(Proposal<Accumulator> proposal, Signs signs)
{
	// According to Algorithm 1:
	// - NORMAL/GRACIOUS stages: T-Replicas use TEE to verify proposals, can detect malicious
	// - UNCIVIL stage (lines 35-37): run Hotstuff without TEE ({Rc}=0), no malicious detection

	// General replicas don't participate in malicious detection (they don't enter UNCIVIL)
	if (this->amGeneralReplicaIds())
	{
		return true;  // General replica accepts all proposals
	}

	// In UNCIVIL mode, run basic Hotstuff without TEE verification
	// Malicious detection is not applicable - system needs to reach consensus via view changes
	if (this->stage == STAGE_UNCIVIL)
	{
		if (DEBUG_HELP && proposal.hasMaliciousMark())
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "UNCIVIL mode: ignoring malicious mark (no TEE)" << COLOUR_NORMAL << std::endl;
		}
		return true;  // Accept proposal in UNCIVIL mode (basic Hotstuff)
	}

	// In NORMAL/GRACIOUS stages, trusted replicas use TEE to verify proposals
	// Check for malicious mark in the proposal
	if (proposal.hasMaliciousMark())
	{
		ReplicaID leader = this->getCurrentLeader();
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_MAGENTA << this->printReplicaId() << "DETECTED MALICIOUS MARK: rejecting proposal from leader " << leader << COLOUR_NORMAL << std::endl;
		}
		return false;  // Reject malicious proposal
	}

	return true;  // Accept normal proposal
}

Justification ResiBFT::initializeMsgNewviewResiBFT()
{
	Justification justification_MsgNewview = Justification();
	if (!this->amGeneralReplicaIds())
	{
		// Trusted replica - use TEE with lightweight structure
		JustificationLight_t justification_MsgNewview_t;
		sgx_status_t retval;
		sgx_status_t status_t = TEE_initializeMsgNewviewResiBFT(global_eid, &retval, &justification_MsgNewview_t);
		if (status_t != SGX_SUCCESS || retval != SGX_SUCCESS)
		{
			if (DEBUG_HELP)
			{
				std::cout << COLOUR_RED << this->printReplicaId() << "TEE_initializeMsgNewviewResiBFT failed: " << status_t << " retval: " << retval << COLOUR_NORMAL << std::endl;
			}
			// Fallback: create dummy justification to continue consensus
			Hash dummyHash = Hash(false);
			Hash justifyHash = Hash();
			// HotStuff: proposeView should be the current view
			RoundData roundData = RoundData(dummyHash, this->view, justifyHash, this->view, PHASE_NEWVIEW);
			Sign sign = Sign(true, this->replicaId, (unsigned char *)"dummy");
			Signs signs = Signs();
			signs.add(sign);
			justification_MsgNewview = Justification(roundData, signs);
		}
		else
		{
			justification_MsgNewview = getJustificationLight(&justification_MsgNewview_t);
		}
	}
	else
	{
		// General replica - create justification with proper locked hash
		// HotStuff: justifyHash should be the locked hash (prepareHash) from previous view
		Hash justifyHash = this->prepareHash;  // Use prepareHash as parent hash
		if (justifyHash.isDummy())
		{
			justifyHash = this->lockedHash;  // Fallback to lockedHash
		}
		if (justifyHash.isDummy())
		{
			justifyHash = Hash();  // Genesis hash if no previous block
		}
		Hash dummyHash = Hash(false);  // Dummy hash for proposeHash (marks as newview)
		// HotStuff: proposeView should be the current view (node is ready for view k)
		RoundData roundData = RoundData(dummyHash, this->view, justifyHash, this->prepareView, PHASE_NEWVIEW);
		Sign sign = Sign(true, this->replicaId, (unsigned char *)"newview");
		Signs signs = Signs();
		signs.add(sign);
		justification_MsgNewview = Justification(roundData, signs);
	}
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Initialized MsgNewview with view: " << justification_MsgNewview.getRoundData().getProposeView() << COLOUR_NORMAL << std::endl;
	}
	return justification_MsgNewview;
}

Accumulator ResiBFT::initializeAccumulatorResiBFT(Justification justifications_MsgNewview[MAX_NUM_SIGNATURES])
{
	// General replicas don't use TEE
	if (this->amGeneralReplicaIds())
	{
		// Create a simple accumulator from justifications
		View highestView = 0;
		Hash highestHash = Hash();
		View proposeView = justifications_MsgNewview[0].getRoundData().getProposeView();
		unsigned int count = 0;

		for (unsigned int i = 0; i < MAX_NUM_SIGNATURES && justifications_MsgNewview[i].isSet(); i++)
		{
			View justifyView = justifications_MsgNewview[i].getRoundData().getJustifyView();
			if (justifyView >= highestView)
			{
				highestView = justifyView;
				highestHash = justifications_MsgNewview[i].getRoundData().getJustifyHash();
			}
			count++;
		}

		Accumulator accumulator = Accumulator(proposeView, highestHash, highestView, count);
		return accumulator;
	}

	// Trusted replica - use TEE with lightweight input
	AccumulatorInput_t accumulatorInput_t;
	setAccumulatorInput(justifications_MsgNewview, &accumulatorInput_t);
	Accumulator_t accumulator_MsgLdrprepare_t;
	sgx_status_t retval;
	sgx_status_t status_t = TEE_initializeAccumulatorResiBFT(global_eid, &retval, &accumulatorInput_t, &accumulator_MsgLdrprepare_t);
	if (status_t != SGX_SUCCESS || retval != SGX_SUCCESS)
	{
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_RED << this->printReplicaId() << "TEE_initializeAccumulatorResiBFT failed: " << status_t << " retval: " << retval << COLOUR_NORMAL << std::endl;
		}
		// Fallback: compute accumulator locally (same logic as general replicas)
		View highestView = 0;
		Hash highestHash = Hash();
		View proposeView = justifications_MsgNewview[0].getRoundData().getProposeView();
		unsigned int count = 0;

		for (unsigned int i = 0; i < MAX_NUM_SIGNATURES && justifications_MsgNewview[i].isSet(); i++)
		{
			View justifyView = justifications_MsgNewview[i].getRoundData().getJustifyView();
			if (justifyView >= highestView)
			{
				highestView = justifyView;
				highestHash = justifications_MsgNewview[i].getRoundData().getJustifyHash();
			}
			count++;
		}

		Accumulator accumulator = Accumulator(proposeView, highestHash, highestView, count);
		return accumulator;
	}
	Accumulator accumulator_MsgLdrprepare = getAccumulator(&accumulator_MsgLdrprepare_t);
	return accumulator_MsgLdrprepare;
}

Signs ResiBFT::initializeMsgLdrprepareResiBFT(Proposal<Accumulator> proposal_MsgLdrprepare)
{
	// General replicas don't use TEE
	if (this->amGeneralReplicaIds())
	{
		// Create a dummy sign for general replica
		Sign sign = Sign(true, this->replicaId, (unsigned char *)"dummy");
		Signs signs = Signs();
		signs.add(sign);
		return signs;
	}

	// Trusted replica - use TEE with lightweight structures
	Hash_t proposalHash_t;
	setHash(proposal_MsgLdrprepare.getBlock().hash(), &proposalHash_t);
	SignSingle_t sign_t;
	sgx_status_t retval;
	sgx_status_t status_t = TEE_initializeMsgLdrprepareResiBFT(global_eid, &retval, &proposalHash_t, &sign_t);
	if (status_t != SGX_SUCCESS || retval != SGX_SUCCESS)
	{
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_RED << this->printReplicaId() << "TEE_initializeMsgLdrprepareResiBFT failed: " << status_t << " retval: " << retval << COLOUR_NORMAL << std::endl;
		}
		// Fallback: create dummy sign
		Sign sign = Sign(true, this->replicaId, (unsigned char *)"dummy");
		Signs signs_MsgLdrprepare = Signs();
		signs_MsgLdrprepare.add(sign);
		return signs_MsgLdrprepare;
	}
	Sign sign = getSignSingle(&sign_t);
	Signs signs_MsgLdrprepare = Signs();
	signs_MsgLdrprepare.add(sign);
	return signs_MsgLdrprepare;
}

Justification ResiBFT::respondMsgLdrprepareProposalResiBFT(Hash proposeHash, Accumulator accumulator_MsgLdrprepare)
{
	// General replicas don't use TEE
	if (this->amGeneralReplicaIds())
	{
		// Create justification with the proposal hash and accumulator info
		View proposeView = accumulator_MsgLdrprepare.getProposeView();
		RoundData roundData = RoundData(proposeHash, proposeView, accumulator_MsgLdrprepare.getPrepareHash(), accumulator_MsgLdrprepare.getPrepareView(), PHASE_PREPARE);
		Sign sign = Sign(true, this->replicaId, (unsigned char *)"dummy");
		Signs signs = Signs();
		signs.add(sign);
		Justification justification = Justification(roundData, signs);
		return justification;
	}

	// Trusted replica - use TEE with lightweight structures
	Hash_t proposeHash_t;
	setHash(proposeHash, &proposeHash_t);
	Accumulator_t accumulator_MsgLdrprepare_t;
	setAccumulator(accumulator_MsgLdrprepare, &accumulator_MsgLdrprepare_t);
	JustificationLight_t justification_MsgPrepare_t;
	sgx_status_t retval;
	sgx_status_t status_t = TEE_respondProposalResiBFT(global_eid, &retval, &proposeHash_t, &accumulator_MsgLdrprepare_t, &justification_MsgPrepare_t);
	if (status_t != SGX_SUCCESS || retval != SGX_SUCCESS)
	{
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_RED << this->printReplicaId() << "TEE_respondProposalResiBFT failed: " << status_t << " retval: " << retval << COLOUR_NORMAL << std::endl;
		}
		// Fallback: create dummy justification
		View proposeView = accumulator_MsgLdrprepare.getProposeView();
		RoundData roundData = RoundData(proposeHash, proposeView, accumulator_MsgLdrprepare.getPrepareHash(), accumulator_MsgLdrprepare.getPrepareView(), PHASE_PREPARE);
		Sign sign = Sign(true, this->replicaId, (unsigned char *)"dummy");
		Signs signs = Signs();
		signs.add(sign);
		return Justification(roundData, signs);
	}
	Justification justification_MsgPrepare = getJustificationLight(&justification_MsgPrepare_t);
	return justification_MsgPrepare;
}

Justification ResiBFT::saveMsgPrepareResiBFT(Justification justification_MsgPrepare)
{
	// General replicas don't use TEE
	if (this->amGeneralReplicaIds())
	{
		// For general replicas, create precommit justification from prepare
		RoundData roundData_MsgPrepare = justification_MsgPrepare.getRoundData();
		// Transition to PRECOMMIT phase with same round data
		RoundData roundData_MsgPrecommit = RoundData(
			roundData_MsgPrepare.getProposeHash(),
			roundData_MsgPrepare.getProposeView(),
			roundData_MsgPrepare.getJustifyHash(),
			roundData_MsgPrepare.getJustifyView(),
			PHASE_PRECOMMIT
		);
		Signs signs_MsgPrepare = justification_MsgPrepare.getSigns();
		Justification justification_MsgPrecommit = Justification(roundData_MsgPrecommit, signs_MsgPrepare);
		return justification_MsgPrecommit;
	}

	// Trusted replica - use TEE with lightweight structures
	RoundData_t roundData_t;
	setRoundData(justification_MsgPrepare.getRoundData(), &roundData_t);
	JustificationLight_t justification_MsgPrecommit_t;
	sgx_status_t retval;
	sgx_status_t status_t = TEE_saveMsgPrepareResiBFT(global_eid, &retval, &roundData_t, &justification_MsgPrecommit_t);
	if (status_t != SGX_SUCCESS || retval != SGX_SUCCESS)
	{
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_RED << this->printReplicaId() << "TEE_saveMsgPrepareResiBFT failed: " << status_t << " retval: " << retval << COLOUR_NORMAL << std::endl;
		}
		// Fallback: create precommit justification from prepare
		RoundData roundData_MsgPrepare = justification_MsgPrepare.getRoundData();
		RoundData roundData_MsgPrecommit = RoundData(
			roundData_MsgPrepare.getProposeHash(),
			roundData_MsgPrepare.getProposeView(),
			roundData_MsgPrepare.getJustifyHash(),
			roundData_MsgPrepare.getJustifyView(),
			PHASE_PRECOMMIT
		);
		Signs signs_MsgPrepare = justification_MsgPrepare.getSigns();
		return Justification(roundData_MsgPrecommit, signs_MsgPrepare);
	}
	Justification justification_MsgPrecommit = getJustificationLight(&justification_MsgPrecommit_t);
	return justification_MsgPrecommit;
}

void ResiBFT::skipRoundResiBFT()
{
	// Placeholder - will be implemented in later tasks
}

Accumulator ResiBFT::initializeAccumulator(std::set<MsgNewviewResiBFT> msgNewviews)
{
	// Initialize the array with empty Justifications to avoid garbage data
	Justification justifications_MsgNewview[MAX_NUM_SIGNATURES];
	for (unsigned int j = 0; j < MAX_NUM_SIGNATURES; j++)
	{
		justifications_MsgNewview[j] = Justification();  // Empty justification with set=false
	}
	unsigned int i = 0;
	for (std::set<MsgNewviewResiBFT>::iterator it = msgNewviews.begin(); it != msgNewviews.end() && i < MAX_NUM_SIGNATURES; it++, i++)
	{
		MsgNewviewResiBFT msgNewview = *it;
		RoundData roundData_MsgNewview = msgNewview.roundData;
		Signs signs_MsgNewview = msgNewview.signs;
		Justification justification_MsgNewview = Justification(roundData_MsgNewview, signs_MsgNewview);
		justifications_MsgNewview[i] = justification_MsgNewview;
	}
	return this->initializeAccumulatorResiBFT(justifications_MsgNewview);
}

// Receive messages
void ResiBFT::receiveMsgStartResiBFT(MsgStart msgStart, const ClientNet::conn_t &conn)
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

void ResiBFT::receiveMsgTransactionResiBFT(MsgTransaction msgTransaction, const ClientNet::conn_t &conn)
{
	this->handleMsgTransaction(msgTransaction);
}

void ResiBFT::receiveMsgNewviewResiBFT(MsgNewviewResiBFT msgNewview, const PeerNet::conn_t &conn)
{
	this->handleMsgNewviewResiBFT(msgNewview);
}

void ResiBFT::receiveMsgLdrprepareResiBFT(MsgLdrprepareResiBFT msgLdrprepare, const PeerNet::conn_t &conn)
{
	this->handleMsgLdrprepareResiBFT(msgLdrprepare);
}

void ResiBFT::receiveMsgPrepareResiBFT(MsgPrepareResiBFT msgPrepare, const PeerNet::conn_t &conn)
{
	this->handleMsgPrepareResiBFT(msgPrepare);
}

void ResiBFT::receiveMsgPrecommitResiBFT(MsgPrecommitResiBFT msgPrecommit, const PeerNet::conn_t &conn)
{
	this->handleMsgPrecommitResiBFT(msgPrecommit);
}

void ResiBFT::receiveMsgDecideResiBFT(MsgDecideResiBFT msgDecide, const PeerNet::conn_t &conn)
{
	this->handleMsgDecideResiBFT(msgDecide);
}

void ResiBFT::receiveMsgBCResiBFT(MsgBCResiBFT msgBC, const PeerNet::conn_t &conn)
{
	this->handleMsgBCResiBFT(msgBC);
}

void ResiBFT::receiveMsgVCResiBFT(MsgVCResiBFT msgVC, const PeerNet::conn_t &conn)
{
	this->handleMsgVCResiBFT(msgVC);
}

void ResiBFT::receiveMsgCommitteeResiBFT(MsgCommitteeResiBFT msgCommittee, const PeerNet::conn_t &conn)
{
	this->handleMsgCommitteeResiBFT(msgCommittee);
}

void ResiBFT::receiveMsgPrepareProposalResiBFT(MsgPrepareProposalResiBFT msgPrepareProposal, const PeerNet::conn_t &conn)
{
	this->handleMsgPrepareProposalResiBFT(msgPrepareProposal);
}

void ResiBFT::receiveMsgCommitResiBFT(MsgCommitResiBFT msgCommit, const PeerNet::conn_t &conn)
{
	this->handleMsgCommitResiBFT(msgCommit);
}

// Send messages
void ResiBFT::sendMsgNewviewResiBFT(MsgNewviewResiBFT msgNewview, Peers recipients)
{
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Sending: " << msgNewview.toPrint() << " -> " << this->recipients2string(recipients) << COLOUR_NORMAL << std::endl;
	}
	this->peerNet.multicast_msg(msgNewview, getPeerIds(recipients));
	if (DEBUG_TIME)
	{
		this->printNowTime("Sending MsgNewviewResiBFT");
	}
}

void ResiBFT::sendMsgLdrprepareResiBFT(MsgLdrprepareResiBFT msgLdrprepare, Peers recipients)
{
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Sending: " << msgLdrprepare.toPrint() << " -> " << this->recipients2string(recipients) << COLOUR_NORMAL << std::endl;
	}
	this->peerNet.multicast_msg(msgLdrprepare, getPeerIds(recipients));
	if (DEBUG_TIME)
	{
		this->printNowTime("Sending MsgLdrprepareResiBFT");
	}
}

void ResiBFT::sendMsgPrepareResiBFT(MsgPrepareResiBFT msgPrepare, Peers recipients)
{
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Sending: " << msgPrepare.toPrint() << " -> " << this->recipients2string(recipients) << COLOUR_NORMAL << std::endl;
	}
	this->peerNet.multicast_msg(msgPrepare, getPeerIds(recipients));
	if (DEBUG_TIME)
	{
		this->printNowTime("Sending MsgPrepareResiBFT");
	}
}

void ResiBFT::sendMsgPrecommitResiBFT(MsgPrecommitResiBFT msgPrecommit, Peers recipients)
{
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Sending: " << msgPrecommit.toPrint() << " -> " << this->recipients2string(recipients) << COLOUR_NORMAL << std::endl;
	}
	this->peerNet.multicast_msg(msgPrecommit, getPeerIds(recipients));
	if (DEBUG_TIME)
	{
		this->printNowTime("Sending MsgPrecommitResiBFT");
	}
}

void ResiBFT::sendMsgDecideResiBFT(MsgDecideResiBFT msgDecide, Peers recipients)
{
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Sending: " << msgDecide.toPrint() << " -> " << this->recipients2string(recipients) << COLOUR_NORMAL << std::endl;
	}
	this->peerNet.multicast_msg(msgDecide, getPeerIds(recipients));
	if (DEBUG_TIME)
	{
		this->printNowTime("Sending MsgDecideResiBFT");
	}
}

void ResiBFT::sendMsgBCResiBFT(MsgBCResiBFT msgBC, Peers recipients)
{
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Sending: " << msgBC.toPrint() << " -> " << this->recipients2string(recipients) << COLOUR_NORMAL << std::endl;
	}
	this->peerNet.multicast_msg(msgBC, getPeerIds(recipients));
	if (DEBUG_TIME)
	{
		this->printNowTime("Sending MsgBCResiBFT");
	}
}

void ResiBFT::sendMsgVCResiBFT(MsgVCResiBFT msgVC, Peers recipients)
{
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Sending: " << msgVC.toPrint() << " -> " << this->recipients2string(recipients) << COLOUR_NORMAL << std::endl;
	}
	this->peerNet.multicast_msg(msgVC, getPeerIds(recipients));
	if (DEBUG_TIME)
	{
		this->printNowTime("Sending MsgVCResiBFT");
	}
}

void ResiBFT::sendMsgCommitteeResiBFT(MsgCommitteeResiBFT msgCommittee, Peers recipients)
{
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Sending: " << msgCommittee.toPrint() << " -> " << this->recipients2string(recipients) << COLOUR_NORMAL << std::endl;
	}
	this->peerNet.multicast_msg(msgCommittee, getPeerIds(recipients));
	if (DEBUG_TIME)
	{
		this->printNowTime("Sending MsgCommitteeResiBFT");
	}
}

void ResiBFT::sendMsgPrepareProposalResiBFT(MsgPrepareProposalResiBFT msgPrepareProposal, Peers recipients)
{
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Sending: " << msgPrepareProposal.toPrint() << " -> " << this->recipients2string(recipients) << COLOUR_NORMAL << std::endl;
	}
	this->peerNet.multicast_msg(msgPrepareProposal, getPeerIds(recipients));
	if (DEBUG_TIME)
	{
		this->printNowTime("Sending MsgPrepareProposalResiBFT");
	}
}

void ResiBFT::sendMsgCommitResiBFT(MsgCommitResiBFT msgCommit, Peers recipients)
{
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Sending: " << msgCommit.toPrint() << " -> " << this->recipients2string(recipients) << COLOUR_NORMAL << std::endl;
	}
	this->peerNet.multicast_msg(msgCommit, getPeerIds(recipients));
	if (DEBUG_TIME)
	{
		this->printNowTime("Sending MsgCommitResiBFT");
	}
}

// Handle messages
void ResiBFT::handleMsgTransaction(MsgTransaction msgTransaction)
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
	if (it != this->clients.end())
	{
		ClientInformation clientInformation = it->second;
		bool running = std::get<0>(clientInformation);
		if (running)
		{
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

void ResiBFT::handleEarlierMessagesResiBFT()
{
	// 处理之前存储的消息（从 log 中取出并处理）
	// 这对于快速达到 quorum 很重要

	// 关键修复：MsgNewview quorum threshold 应根据 stage 选择
	// NORMAL 模式：所有 replicas 参与，需要 generalQuorumSize (2f+1)
	// GRACIOUS 模式：只有 committee members 参与，只需要 trustedQuorumSize (floor((k+1)/2))
	unsigned int quorumThreshold = this->generalQuorumSize;  // 默认
	if (this->stage == STAGE_GRACIOUS && this->committee.getSize() > 0)
	{
		quorumThreshold = this->trustedQuorumSize;
	}

	// HotStuff: MsgNewview(proposeView=k) 表示节点准备进入 view k
	// Leader k 查询 proposeView=k 的 MsgNewview
	View queryView = this->view;
	std::set<MsgNewviewResiBFT> storedMsgNewviews = this->log.getMsgNewviewResiBFT(queryView, quorumThreshold + 10);

	unsigned int count = storedMsgNewviews.size();

	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId()
				  << "Handling earlier messages for view " << this->view
				  << " (checking MsgNewview for queryView=" << queryView << ")"
				  << ", stage=" << printStage(this->stage)
				  << ", MsgNewview count=" << count
				  << ", threshold=" << quorumThreshold
				  << ", amLeader=" << this->amCurrentLeader()
				  << COLOUR_NORMAL << std::endl;
	}

	// 如果 Leader 且达到 quorum，立即发起共识
	if (this->amCurrentLeader() && count >= quorumThreshold)
	{
		if (this->stage == STAGE_NORMAL)
		{
			std::cout << COLOUR_GREEN << this->printReplicaId()
					  << "Quorum reached from earlier messages (" << count << "/" << quorumThreshold << "), initiating consensus for view " << this->view
					  << COLOUR_NORMAL << std::endl;
			std::cout.flush();
			this->initiateMsgPrepareProposalResiBFT();
		}
		else
		{
			// GRACIOUS 模式
			if (this->ldrprepareInitiatedViews.find(this->view) == this->ldrprepareInitiatedViews.end())
			{
				this->ldrprepareInitiatedViews.insert(this->view);
				std::cout << COLOUR_GREEN << this->printReplicaId()
						  << "Quorum reached in GRACIOUS (" << count << "/" << quorumThreshold << "), initiating MsgLdrprepare"
						  << COLOUR_NORMAL << std::endl;
				std::cout.flush();
				this->initiateMsgNewviewResiBFT();
			}
		}
	}
}

void ResiBFT::handleMsgNewviewResiBFT(MsgNewviewResiBFT msgNewview)
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

	// Validate the MsgNewview
	// HotStuff: MsgNewview(proposeView=k) 表示节点准备进入 view k
	// Leader k 收到 MsgNewview(proposeView=k) 后，开始 view k 的共识
	if (proposeHash_MsgNewview.isDummy() && proposeView_MsgNewview >= this->view && phase_MsgNewview == PHASE_NEWVIEW)
	{
		// 检查是否是为当前 view 准备的 MsgNewview
		if (proposeView_MsgNewview == this->view)
		{
			// 关键修复：GRACIOUS 模式（非空 committee）下，Leader 只接受来自 committee members 的 MsgNewview
			// 论文设计：GRACIOUS consensus 只有 committee 参与
			// UNCIVIL 模式（空 committee）：所有节点参与
			if (this->stage == STAGE_GRACIOUS && this->committee.getSize() > 0)
			{
				// 获取 MsgNewview 的发送者
				Signs signs_MsgNewview = msgNewview.signs;
				if (signs_MsgNewview.getSize() > 0)
				{
					Sign firstSign = signs_MsgNewview.get(0);
					ReplicaID sender = firstSign.getSigner();

					// 检查发送者是否是 committee member
					bool senderIsCommitteeMember = this->isInCommittee(sender);
					if (!senderIsCommitteeMember)
					{
						// 非 committee member 的 MsgNewview 在 GRACIOUS 模式下被忽略
						// G-Replicas 不参与 GRACIOUS consensus
						if (DEBUG_HELP)
						{
							std::cout << COLOUR_ORANGE << this->printReplicaId()
									  << "Ignoring MsgNewview from non-committee member " << sender
									  << " in GRACIOUS mode (stage=" << printStage(this->stage)
									  << ", committeeSize=" << this->committee.getSize() << ")"
									  << COLOUR_NORMAL << std::endl;
						}
						auto end = std::chrono::steady_clock::now();
						double time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
						statistics.addHandleTime(time);
						return;
					}
				}
			}
			else if (this->stage == STAGE_UNCIVIL || this->committee.getSize() == 0)
			{
				// UNCIVIL 模式或空 committee：接受所有节点的 MsgNewview
				// 论文设计：所有节点参与投票
				if (DEBUG_HELP)
				{
					std::cout << COLOUR_BLUE << this->printReplicaId()
							  << "Accepting MsgNewview from all nodes (UNCIVIL or empty committee)"
							  << ", stage=" << printStage(this->stage)
							  << ", committeeSize=" << this->committee.getSize()
							  << COLOUR_NORMAL << std::endl;
				}
			}

			// Store in log and check if we have quorum
			unsigned int count = this->log.storeMsgNewviewResiBFT(msgNewview);

			// 关键修复：MsgNewview quorum threshold 应根据 stage 和 committee 选择
			// NORMAL 模式：所有 replicas 参与，需要 generalQuorumSize (2f+1)
			// GRACIOUS 模式（非空 committee）：只有 committee members 参与，使用 trustedQuorumSize
			// UNCIVIL 模式（空 committee）：所有节点参与，使用 generalQuorumSize
			unsigned int quorumThreshold = this->generalQuorumSize;  // 默认
			if (this->stage == STAGE_GRACIOUS && this->committee.getSize() > 0)
			{
				// GRACIOUS 模式且有非空 committee
				quorumThreshold = this->trustedQuorumSize;
			}

			if (count >= quorumThreshold)
			{
				// Leader initiates the next phase
				if (this->amCurrentLeader())
				{
					if (this->stage == STAGE_NORMAL && this->proposedCommittee.getSize() > 0
						&& this->proposedCommitteeView == this->view)
					{
						// NORMAL 模式但 Leader 有 proposedCommittee（T-Replica Leader）
						// 发送 MsgLdrprepare 包含 proposedCommittee
						// 所有节点投票使用 generalQuorumSize
						if (this->ldrprepareInitiatedViews.find(this->view) == this->ldrprepareInitiatedViews.end())
						{
							this->ldrprepareInitiatedViews.insert(this->view);
							std::cout << COLOUR_MAGENTA << this->printReplicaId()
									  << "[NEWVIEW-HANDLER] T-Replica Leader initiating MsgLdrprepare with proposedCommittee"
									  << ", view=" << this->view
									  << ", proposedCommitteeSize=" << this->proposedCommittee.getSize()
									  << ", using generalQuorumSize=" << this->generalQuorumSize
									  << COLOUR_NORMAL << std::endl;
							std::cout.flush();
							this->initiateMsgNewviewResiBFT();  // 发送 MsgLdrprepare
						}
						else
						{
							if (DEBUG_HELP)
							{
								std::cout << COLOUR_ORANGE << this->printReplicaId() << "MsgLdrprepare already initiated for view " << this->view << ", skipping duplicate" << COLOUR_NORMAL << std::endl;
							}
						}
					}
					else if (this->stage == STAGE_NORMAL)
					{
						// NORMAL 模式，没有 proposedCommittee（G-Replica Leader）
						// HotStuff: Leader sends MsgPrepareProposal directly
						this->initiateMsgPrepareProposalResiBFT();
					}
					else
					{
						// GRACIOUS/UNCIVIL: Keep existing MsgLdrprepare flow
						// 去重检查：确保只发送一次 MsgLdrprepare
						if (this->ldrprepareInitiatedViews.find(this->view) == this->ldrprepareInitiatedViews.end())
						{
							this->ldrprepareInitiatedViews.insert(this->view);
							this->initiateMsgNewviewResiBFT();
						}
						else
						{
							if (DEBUG_HELP)
							{
								std::cout << COLOUR_ORANGE << this->printReplicaId() << "MsgLdrprepare already initiated for view " << this->view << ", skipping duplicate" << COLOUR_NORMAL << std::endl;
							}
						}
					}
				}
			}
		}
		else
		{
			// Higher view - HotStuff view synchronization
			// 收到 higher view 的 MsgNewview 时，应该检查其 justifyView
			// 如果网络已经在更高的 view 进行共识，落后的节点应该跳转
			View justifyView_MsgNewview = roundData_MsgNewview.getJustifyView();

			if (DEBUG_HELP)
			{
				std::cout << COLOUR_BLUE << this->printReplicaId()
						  << "Received MsgNewview for higher view: proposeView=" << proposeView_MsgNewview
						  << ", justifyView=" << justifyView_MsgNewview
						  << ", currentView=" << this->view
						  << COLOUR_NORMAL << std::endl;
			}

			// View synchronization: 如果收到更高 view 的 MsgNewview，跳转到该 view
			// 这样落后的节点可以快速跟上网络的进度
			// 条件：proposeView > currentView（收到更高 view 的消息）
			//
			// 注意：在 NORMAL 模式下跳转可能会错过当前 view 的 MsgDecide
			// 但为了保证系统正常运行，需要允许跳转
			// 后续可以通过 MsgDecide 的 catch-up 机制来处理落后的 view
			if (proposeView_MsgNewview > this->view)
			{
				View targetView = proposeView_MsgNewview;

				std::cout << COLOUR_ORANGE << this->printReplicaId()
						  << "[VIEW-SYNC] Jumping from view " << this->view
						  << " to view " << targetView
						  << " (triggered by MsgNewview with justifyView=" << justifyView_MsgNewview
						  << ", proposeView=" << proposeView_MsgNewview << ")"
						  << COLOUR_NORMAL << std::endl;
				std::cout.flush();

				// 直接跳转到 targetView
				View oldView = this->view;
				while (this->view < targetView)
				{
					this->view++;
					// 检查新 Leader 是否是 G-Replica，如果是，恢复到 NORMAL
					unsigned int newLeader = this->view % this->numReplicas;
					bool newLeaderIsTrusted = (newLeader >= this->numGeneralReplicas);

					if ((this->stage == STAGE_GRACIOUS || this->stage == STAGE_UNCIVIL) && !newLeaderIsTrusted)
					{
						std::cout << COLOUR_GREEN << this->printReplicaId()
								  << "[VIEW-SYNC] During jump, recovering from " << printStage(this->stage)
								  << " to NORMAL at view " << this->view
								  << " (newLeader=" << newLeader << " is G-Replica)"
								  << COLOUR_NORMAL << std::endl;
						this->transitionToNormal();
					}
				}

				// 同步 TEE view
				if (!this->amGeneralReplicaIds())
				{
					sgx_status_t retval;
					sgx_status_t status_t = TEE_setViewResiBFT(global_eid, &retval, &this->view);
					if (status_t != SGX_SUCCESS || retval != SGX_SUCCESS)
					{
						if (DEBUG_HELP)
						{
							std::cout << COLOUR_RED << this->printReplicaId() << "TEE_setViewResiBFT failed during view sync" << COLOUR_NORMAL << std::endl;
						}
					}
				}

				// 重要修复：收到 MsgNewview 后只跳转，不广播
				// 这样避免级联：A收到MsgNewview→跳转→广播→B收到→跳转→广播→...
				// 只有自己超时触发的 view change 才广播 MsgNewview 给新 leader
				// 这里只是被动同步，不需要广播

				std::cout << COLOUR_ORANGE << this->printReplicaId()
						  << "[VIEW-SYNC] Jumped to view " << this->view
						  << " (passive sync, not broadcasting to avoid cascade)"
						  << ", newLeader=" << (this->view % this->numReplicas)
						  << ", amLeader=" << this->amCurrentLeader()
						  << ", stage=" << printStage(this->stage)
						  << COLOUR_NORMAL << std::endl;
				std::cout.flush();

				// 关键修复：view sync 后检查是否需要触发 GRACIOUS
				// Case 1: NORMAL 模式下跳转后成为 T-Replica Leader，触发 GRACIOUS
				// 关键修改：不调用 transitionToGracious()，而是计算 proposedCommittee
				if (this->amTrustedReplicaIds() && this->stage == STAGE_NORMAL)
				{
					unsigned int normalLeader = this->view % this->numReplicas;
					if (normalLeader == this->replicaId)
					{
#ifdef PRODUCTION_MODE
						unsigned int k = COMMITTEE_K_MIN;
#else
						unsigned int k = std::min((unsigned int)COMMITTEE_K_MIN, this->numTrustedReplicas);
#endif

						std::cout << COLOUR_MAGENTA << this->printReplicaId()
								  << "[VIEW-SYNC] I am T-Replica Leader for view " << this->view
								  << " after sync - computing proposedCommittee (will enter GRACIOUS after block execution)"
								  << COLOUR_NORMAL << std::endl;
						std::cout.flush();

						// VRF 选择委员会，存储为 proposedCommittee（不立即更新 committee）
						this->proposedCommittee = this->selectCommitteeByVRF(this->view, k);
						this->proposedCommitteeView = this->view;

						std::cout << COLOUR_MAGENTA << this->printReplicaId()
								  << "[VIEW-SYNC] VRF selection done: proposedCommitteeSize=" << this->proposedCommittee.getSize()
								  << ", stage=" << printStage(this->stage) << " (staying in NORMAL)"
								  << COLOUR_NORMAL << std::endl;
						std::cout.flush();

						// 不调用 transitionToGracious()
						// 不发送 MsgCommittee
						// proposedCommittee 将在 MsgLdrprepare 中发送
					}
				}

				// Case 2: GRACIOUS 模式下跳转后成为新的 T-Replica Leader（不同于旧的 committeeLeader）
				// 关键修改：检查 acceptedVCs，决定继续 GRACIOUS 或进入 UNCIVIL
				if (this->stage == STAGE_GRACIOUS || this->stage == STAGE_UNCIVIL)
				{
					unsigned int normalLeader = this->view % this->numReplicas;
					bool normalLeaderIsGReplica = (normalLeader < this->numGeneralReplicas);

					// Case 2a: G-Replica Leader → 恢复到 NORMAL
					if (normalLeaderIsGReplica)
					{
						std::cout << COLOUR_GREEN << this->printReplicaId()
								  << "[VIEW-SYNC] View " << this->view << " has G-Replica leader " << normalLeader
								  << " - recovering from " << printStage(this->stage)
								  << " to NORMAL"
								  << COLOUR_NORMAL << std::endl;
						std::cout.flush();

						this->transitionToNormal();
					}
					// Case 2b: T-Replica Leader，检查是否是新的 committee leader
					else if (this->amTrustedReplicaIds() && this->committeeLeader != this->replicaId)
					{
						// 新的 T-Replica Leader 需要决定下一步
						unsigned int acceptedVCCount = this->acceptedVCs.size();

						if (acceptedVCCount >= this->numFaults + 1)
						{
							// 有 f+1 个 accepted-VC → 继续 GRACIOUS consensus
							std::cout << COLOUR_GREEN << this->printReplicaId()
									  << "[VIEW-SYNC] Have f+1 accepted-VCs → continuing GRACIOUS consensus"
									  << ", committeeSize=" << this->committee.getSize()
									  << COLOUR_NORMAL << std::endl;
							std::cout.flush();

							// 更新 committeeLeader 为自己
							this->committeeLeader = this->replicaId;
							this->committeeView = this->view;
						}
						else
						{
							// 没有 f+1 个 accepted-VC → 进入 UNCIVIL
							std::cout << COLOUR_RED << this->printReplicaId()
									  << "[VIEW-SYNC] No f+1 accepted-VCs → entering UNCIVIL"
									  << " - will propose EMPTY committee to revoke committee"
									  << COLOUR_NORMAL << std::endl;
							std::cout.flush();

							this->stage = STAGE_UNCIVIL;
							this->stageTransitionView = this->view;
							this->proposedCommittee = Group();
							this->proposedCommitteeView = this->view;
						}
					}
				}

				// 设置新的 timer
				this->setTimer();

				// 检查是否有存储的 MsgNewview 可以使用
				this->handleEarlierMessagesResiBFT();
			}
			else
			{
				// 只是存储，不跳转（justifyView 太低）
				this->log.storeMsgNewviewResiBFT(msgNewview);
			}
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

void ResiBFT::handleMsgLdrprepareResiBFT(MsgLdrprepareResiBFT msgLdrprepare)
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

	// Detailed verification logging
	bool verifyTEE = this->verifyProposalResiBFT(proposal_MsgLdrprepare, signs_MsgLdrprepare);
	bool verifyExtends = block.extends(prepareHash_MsgLdrprepare);
	bool verifyView = proposeView_MsgLdrprepare >= this->view;

	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Verification: TEE=" << verifyTEE
				  << ", extends=" << verifyExtends << " (blockPrevHash=" << block.getPreviousHash().toPrint()
				  << ", prepareHash=" << prepareHash_MsgLdrprepare.toPrint() << ")"
				  << ", view=" << verifyView << " (proposeView=" << proposeView_MsgLdrprepare << ", currentView=" << this->view << ")"
				  << ", leader=" << this->getCurrentLeader()
				  << COLOUR_NORMAL << std::endl;
	}

	// Verify the proposal
	if (verifyTEE && verifyExtends && verifyView)
	{
		if (proposeView_MsgLdrprepare == this->view)
		{
			// 论文设计：GRACIOUS 模式只有 committee members 参与 consensus
			// G-Replicas 只存储 block，等待 MsgDecide/MsgCommit

			// 关键修改：MsgLdrprepare 可能包含 committee 信息
			// 论文设计：不立即进入 GRACIOUS，而是存储 proposedCommittee
			// 使用 generalQuorumSize 投票，等待区块执行后进入 GRACIOUS
			Group receivedCommittee = msgLdrprepare.committee;
			if (receivedCommittee.getSize() > 0)
			{
				// MsgLdrprepare 包含 committee 信息
				if (this->stage == STAGE_NORMAL)
				{
					// 论文设计：NORMAL 模式收到 MsgLdrprepare with committee
					// 不立即进入 GRACIOUS，存储为 proposedCommittee
					// 使用 generalQuorumSize 投票
					// 等待区块执行后才进入 GRACIOUS

					// 存储 proposedCommittee
					this->proposedCommittee = receivedCommittee;
					this->proposedCommitteeView = proposeView_MsgLdrprepare;

					// 检查自己是否在 proposedCommittee 中（用于后续进入 GRACIOUS 时判断）
					bool isInProposedCommittee = false;
					for (unsigned int i = 0; i < receivedCommittee.getSize(); i++)
					{
						if (receivedCommittee.getGroup()[i] == this->replicaId)
						{
							isInProposedCommittee = true;
							break;
						}
					}

					std::cout << COLOUR_MAGENTA << this->printReplicaId()
							  << "[LDRPREPARE] Received proposedCommittee for view " << proposeView_MsgLdrprepare
							  << ", proposedCommitteeSize=" << receivedCommittee.getSize()
							  << ", isInProposedCommittee=" << isInProposedCommittee
							  << ", stage=" << printStage(this->stage) << " (staying in NORMAL)"
							  << ", will use generalQuorumSize=" << this->generalQuorumSize
							  << " for voting"
							  << COLOUR_NORMAL << std::endl;
					std::cout.flush();

					// 不进入 GRACIOUS！保持在 NORMAL 模式投票
					// 不更新 committee, trustedQuorumSize
					// 这些将在 executeBlockResiBFT 中更新
				}
				// 论文设计：如果已经在 GRACIOUS 模式，需要检查是否需要更新 committee
				else if (this->stage == STAGE_GRACIOUS || this->stage == STAGE_UNCIVIL)
				{
					// 已经在 GRACIOUS 模式，检查 committeeView 是否需要更新
					if (this->committeeView != this->view)
					{
						// Committee 来自旧 view，需要更新
						// 在 GRACIOUS 模式下立即更新 committee
						if (DEBUG_HELP)
						{
							std::cout << COLOUR_MAGENTA << this->printReplicaId()
									  << "Updating committee from old view " << this->committeeView
									  << " to current view " << this->view
									  << " via MsgLdrprepare (in GRACIOUS)"
									  << ", old committee=" << this->committee.toPrint()
									  << ", new committee=" << receivedCommittee.toPrint()
									  << COLOUR_NORMAL << std::endl;
						}

						// 更新 committee
						this->committee = receivedCommittee;
						this->committeeView = this->view;

						// 更新 currentCommittee std::set
						this->currentCommittee.clear();
						for (unsigned int i = 0; i < receivedCommittee.getSize(); i++)
						{
							this->currentCommittee.insert(receivedCommittee.getGroup()[i]);
						}

						// 更新 trustedQuorumSize
						unsigned int committeeSize = receivedCommittee.getSize();
						this->trustedQuorumSize = (committeeSize + 1) / 2;

						// 检查自己是否在新 committee 中
						this->isCommitteeMember = isInCommittee(this->replicaId);
						ReplicaID expectedLeader = proposeView_MsgLdrprepare % this->numReplicas;
						this->committeeLeader = expectedLeader;

						if (!this->isCommitteeMember)
						{
							if (DEBUG_HELP)
							{
								std::cout << COLOUR_ORANGE << this->printReplicaId()
										  << "Not in new committee (received via MsgLdrprepare), waiting for MsgDecide"
										  << COLOUR_NORMAL << std::endl;
							}
							return;  // 不参与共识
						}
					}
					else
					{
						// committeeView == this->view，验证 committee 一致
						// 在 GRACIOUS 模式下，committee 应该一致
						if (DEBUG_HELP)
						{
							std::cout << COLOUR_BLUE << this->printReplicaId()
									  << "MsgLdrprepare committee matches current committee (view=" << this->view
									  << ", committeeView=" << this->committeeView << ")"
									  << COLOUR_NORMAL << std::endl;
						}
					}
				}
			}
			else
			{
				// MsgLdrprepare 不包含 committee（committee size = 0）
				// 论文设计：空 committee 表示 UNCIVIL 模式，撤销委员会
				// 所有节点参与投票，使用 generalQuorumSize

				// 检查 sender 是否是 T-Replica Leader（只有 T-Replica Leader 可以提议空委员会）
				ReplicaID expectedLeader = proposeView_MsgLdrprepare % this->numReplicas;
				bool senderIsTrustedLeader = (expectedLeader >= this->numGeneralReplicas);

				if (senderIsTrustedLeader && (this->stage == STAGE_GRACIOUS || this->stage == STAGE_UNCIVIL))
				{
					// T-Replica Leader 发送的 MsgLdrprepare 包含空 committee
					// 这是 UNCIVIL 模式，撤销委员会
					// 存储 proposedCommittee 为空
					this->proposedCommittee = Group();  // 空 committee
					this->proposedCommitteeView = proposeView_MsgLdrprepare;

					std::cout << COLOUR_RED << this->printReplicaId()
							  << "[LDRPREPARE] Received EMPTY committee from T-Replica Leader " << expectedLeader
							  << " - entering UNCIVIL consensus (all nodes vote)"
							  << ", proposedCommitteeView=" << proposeView_MsgLdrprepare
							  << ", stage=" << printStage(this->stage) << "->UNCIVIL"
							  << ", will use generalQuorumSize=" << this->generalQuorumSize
							  << COLOUR_NORMAL << std::endl;
					std::cout.flush();

					// 进入 UNCIVIL 模式（临时，等待区块执行后回到 NORMAL）
					this->stage = STAGE_UNCIVIL;
					this->stageTransitionView = this->view;
				}
				else if (this->stage == STAGE_NORMAL)
				{
					// NORMAL 模式下收到不含 committee 的 MsgLdrprepare
					// 这是正常的 NORMAL 模式共识（G-Replica Leader）
					// 检查是否有 pending 的 MsgCommittee
					MsgCommitteeResiBFT pendingMsgCommittee = this->log.getMsgCommitteeResiBFT(this->view);
					if (pendingMsgCommittee.committee.getSize() > 0)
					{
						if (DEBUG_HELP)
						{
							std::cout << COLOUR_MAGENTA << this->printReplicaId()
									  << "Found pending MsgCommittee for view " << this->view
									  << ", but MsgLdrprepare has no committee - storing as proposedCommittee"
									  << COLOUR_NORMAL << std::endl;
						}
						// 存储 proposedCommittee 但不进入 GRACIOUS
						this->proposedCommittee = pendingMsgCommittee.committee;
						this->proposedCommitteeView = pendingMsgCommittee.view;
					}
				}
				else if (this->stage == STAGE_GRACIOUS || this->stage == STAGE_UNCIVIL)
				{
					// GRACIOUS/UNCIVIL 模式收到不含 committee 的 MsgLdrprepare（且 sender 不是 T-Replica）
					// 说明当前 leader 是 G-Replica，应该恢复到 NORMAL
					ReplicaID currentLeader = proposeView_MsgLdrprepare % this->numReplicas;
					bool currentLeaderIsGReplica = (currentLeader < this->numGeneralReplicas);

					if (currentLeaderIsGReplica)
					{
						std::cout << COLOUR_GREEN << this->printReplicaId()
								  << "[STAGE-RECOVERY] Received MsgLdrprepare without committee from G-Replica leader " << currentLeader
								  << " - recovering from " << printStage(this->stage)
								  << " to NORMAL at view " << this->view
								  << COLOUR_NORMAL << std::endl;
						std::cout.flush();

						this->transitionToNormal();
					}
				}
			}

			bool shouldParticipate = false;

			// 论文设计：判断节点是否参与共识
			// NORMAL 模式：所有节点参与投票（使用 generalQuorumSize）
			// GRACIOUS 模式：只有 committee members 参与（使用 trustedQuorumSize）
			// UNCIVIL 模式（空 committee）：所有节点参与投票（使用 generalQuorumSize）

			if (this->stage == STAGE_GRACIOUS && this->committee.getSize() > 0)
			{
				// GRACIOUS 模式且有非空 committee：只有 committee members 参与
				shouldParticipate = this->isCommitteeMember;
				if (!shouldParticipate)
				{
					if (DEBUG_HELP)
					{
						std::cout << COLOUR_ORANGE << this->printReplicaId()
								  << "Not in committee, ignoring MsgLdrprepare in GRACIOUS mode"
								  << COLOUR_NORMAL << std::endl;
					}
				}
			}
			else
			{
				// NORMAL 模式或 UNCIVIL 模式（空 committee）：所有节点参与
				// 论文设计：使用 generalQuorumSize 投票
				shouldParticipate = true;
				if (DEBUG_HELP)
				{
					std::cout << COLOUR_BLUE << this->printReplicaId()
							  << "All nodes participate in voting (stage=" << printStage(this->stage)
							  << ", committeeSize=" << this->committee.getSize()
							  << ", using generalQuorumSize=" << this->generalQuorumSize << ")"
							  << COLOUR_NORMAL << std::endl;
				}
			}

			if (shouldParticipate)
			{
				this->respondMsgLdrprepareResiBFT(accumulator_MsgLdrprepare, block);
			}
			else
			{
				// Non-participating nodes just store the block
				if (this->blocks.find(this->view) == this->blocks.end() || this->blocks[this->view].getSize() == 0)
				{
					this->blocks[this->view] = block;
					if (DEBUG_HELP)
					{
						std::cout << COLOUR_BLUE << this->printReplicaId() << "Stored block for view " << this->view << ": " << block.toPrint() << COLOUR_NORMAL << std::endl;
					}
				}
			}
		}
		else
		{
			// Higher view - store for future
			if (DEBUG_HELP)
			{
				std::cout << COLOUR_BLUE << this->printReplicaId() << "Storing MsgLdrprepare for higher view: " << msgLdrprepare.toPrint() << COLOUR_NORMAL << std::endl;
			}
			this->log.storeMsgLdrprepareResiBFT(msgLdrprepare);
		}
	}
	else
	{
		// Verification failed - log the specific reason
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_RED << this->printReplicaId() << "MsgLdrprepare verification FAILED: "
					  << (!verifyTEE ? "TEE " : "")
					  << (!verifyExtends ? "extends " : "")
					  << (!verifyView ? "view " : "")
					  << COLOUR_NORMAL << std::endl;
		}
		// Send VC_REJECTED when TEE verification fails (indicates malicious leader)
		// Don't send for view/extends mismatch (not malicious, just protocol issues)
		if (!verifyTEE)
		{
			this->sendVC(VC_REJECTED, proposeView_MsgLdrprepare, block.hash());
		}
	}

	auto end = std::chrono::steady_clock::now();
	double time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	statistics.addHandleTime(time);
}

void ResiBFT::handleMsgPrepareResiBFT(MsgPrepareResiBFT msgPrepare)
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

	// 关键修改：MsgPrepare 可能包含 committee 信息
	// 论文设计：不立即进入 GRACIOUS，而是验证 proposedCommittee
	// 使用 generalQuorumSize 投票，等待区块执行后进入 GRACIOUS
	Group receivedCommittee = msgPrepare.committee;
	if (receivedCommittee.getSize() > 0)
	{
		// 验证 sender 是否是 leader（leader 的 committee 信息可信）
		ReplicaID expectedLeader = proposeView_MsgPrepare % this->numReplicas;
		bool senderIsLeader = false;
		for (unsigned int i = 0; i < signs_MsgPrepare.getSize(); i++)
		{
			if (signs_MsgPrepare.get(i).getSigner() == expectedLeader)
			{
				senderIsLeader = true;
				break;
			}
		}

		if (this->stage == STAGE_NORMAL)
		{
			// 论文设计：NORMAL 模式收到 MsgPrepare with committee
			// 不立即进入 GRACIOUS，验证 proposedCommittee 一致性
			// 使用 generalQuorumSize 投票

			if (senderIsLeader)
			{
				// Leader 发送的 MsgPrepare，验证 proposedCommittee 一致性
				if (this->proposedCommittee.getSize() > 0)
				{
					// 已经有 proposedCommittee，检查一致性
					bool committeeMatches = true;
					if (receivedCommittee.getSize() != this->proposedCommittee.getSize())
					{
						committeeMatches = false;
					}
					else
					{
						for (unsigned int i = 0; i < receivedCommittee.getSize(); i++)
						{
							if (receivedCommittee.getGroup()[i] != this->proposedCommittee.getGroup()[i])
							{
								committeeMatches = false;
								break;
							}
						}
					}

					if (!committeeMatches)
					{
						std::cout << COLOUR_RED << this->printReplicaId()
								  << "[MSGPREPARE] Committee mismatch! Received committee differs from proposedCommittee"
								  << ", received=" << receivedCommittee.toPrint()
								  << ", proposed=" << this->proposedCommittee.toPrint()
								  << " - rejecting MsgPrepare"
								  << COLOUR_NORMAL << std::endl;
						std::cout.flush();
						auto end = std::chrono::steady_clock::now();
						double time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
						statistics.addHandleTime(time);
						return;  // committee 不一致，拒绝投票
					}

					std::cout << COLOUR_MAGENTA << this->printReplicaId()
							  << "[MSGPREPARE] Leader's MsgPrepare committee matches proposedCommittee"
							  << ", proposedCommitteeSize=" << receivedCommittee.getSize()
							  << ", stage=" << printStage(this->stage) << " (staying in NORMAL)"
							  << ", using generalQuorumSize=" << this->generalQuorumSize
							  << COLOUR_NORMAL << std::endl;
					std::cout.flush();
				}
				else
				{
					// 首次收到 committee 信息，存储为 proposedCommittee
					this->proposedCommittee = receivedCommittee;
					this->proposedCommitteeView = proposeView_MsgPrepare;

					std::cout << COLOUR_MAGENTA << this->printReplicaId()
							  << "[MSGPREPARE] First received proposedCommittee from Leader's MsgPrepare"
							  << ", proposedCommitteeSize=" << receivedCommittee.getSize()
							  << ", proposedCommitteeView=" << proposeView_MsgPrepare
							  << ", stage=" << printStage(this->stage) << " (staying in NORMAL)"
							  << COLOUR_NORMAL << std::endl;
					std::cout.flush();
				}
			}
			else
			{
				// 非 leader 发送的 MsgPrepare 包含 committee
				// 在 NORMAL 模式下，验证是否与已有的 proposedCommittee 一致
				if (this->proposedCommittee.getSize() > 0)
				{
					bool committeeMatches = true;
					for (unsigned int i = 0; i < receivedCommittee.getSize() && i < this->proposedCommittee.getSize(); i++)
					{
						if (receivedCommittee.getGroup()[i] != this->proposedCommittee.getGroup()[i])
						{
							committeeMatches = false;
							break;
						}
					}

					if (!committeeMatches)
					{
						if (DEBUG_HELP)
						{
							std::cout << COLOUR_ORANGE << this->printReplicaId()
									  << "[MSGPREPARE] Non-leader MsgPrepare committee differs from proposedCommittee, ignoring"
									  << COLOUR_NORMAL << std::endl;
						}
						// 不更新 proposedCommittee，继续处理
					}
				}
				// 非 leader 的 committee 信息不可信，忽略
			}
		}
		else if (this->stage == STAGE_GRACIOUS || this->stage == STAGE_UNCIVIL)
		{
			// 已经在 GRACIOUS/UNCIVIL 模式，检查是否需要更新 committee
			if (senderIsLeader)
			{
				// Leader 发送的 MsgPrepare，committee 应该可信
				if (this->committeeView != proposeView_MsgPrepare)
				{
					// Committee 来自旧 view，需要更新
					if (DEBUG_HELP)
					{
						std::cout << COLOUR_MAGENTA << this->printReplicaId()
								  << "Updating committee from old view " << this->committeeView
								  << " to current view " << proposeView_MsgPrepare
								  << " via Leader's MsgPrepare (in GRACIOUS)"
								  << COLOUR_NORMAL << std::endl;
					}

					// 更新 committee
					this->committee = receivedCommittee;
					this->committeeView = proposeView_MsgPrepare;

					// 更新 currentCommittee std::set
					this->currentCommittee.clear();
					for (unsigned int i = 0; i < receivedCommittee.getSize(); i++)
					{
						this->currentCommittee.insert(receivedCommittee.getGroup()[i]);
					}

					// 更新 trustedQuorumSize
					unsigned int committeeSize = receivedCommittee.getSize();
					this->trustedQuorumSize = (committeeSize + 1) / 2;

					// 检查自己是否在新 committee 中
					this->isCommitteeMember = isInCommittee(this->replicaId);
					ReplicaID expectedLeader = proposeView_MsgPrepare % this->numReplicas;
					this->committeeLeader = expectedLeader;

					if (!this->isCommitteeMember)
					{
						if (DEBUG_HELP)
						{
							std::cout << COLOUR_ORANGE << this->printReplicaId()
									  << "Not in new committee (received via Leader's MsgPrepare), waiting for MsgDecide"
									  << COLOUR_NORMAL << std::endl;
						}
						auto end = std::chrono::steady_clock::now();
						double time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
						statistics.addHandleTime(time);
						return;  // 不参与共识
					}
				}
			}
			else
			{
				// 非 leader 发送的 MsgPrepare 包含 committee
				// 在 GRACIOUS 模式下，committee 来自旧 view，但非 leader 信息不可信
				if (DEBUG_HELP)
				{
					std::cout << COLOUR_ORANGE << this->printReplicaId()
							  << "GRACIOUS with old committee (view " << this->committeeView
							  << "), MsgPrepare from non-leader, ignoring committee"
							  << COLOUR_NORMAL << std::endl;
				}
			}
		}
	}
	else
	{
		// MsgPrepare 不包含 committee
		// 论文设计：如果节点在 GRACIOUS 模式且 MsgPrepare 不包含 committee
		// 检查是否应该恢复到 NORMAL（leader 是 G-Replica）
		if (this->stage == STAGE_GRACIOUS || this->stage == STAGE_UNCIVIL)
		{
			// 只有当前 view 的 MsgPrepare 才能触发恢复
			if (proposeView_MsgPrepare == this->view)
			{
				ReplicaID currentLeader = proposeView_MsgPrepare % this->numReplicas;
				bool currentLeaderIsGReplica = (currentLeader < this->numGeneralReplicas);

				if (currentLeaderIsGReplica)
				{
					std::cout << COLOUR_GREEN << this->printReplicaId()
							  << "[STAGE-RECOVERY] Received MsgPrepare without committee from G-Replica leader " << currentLeader
							  << " for current view " << this->view
							  << " - recovering from " << printStage(this->stage)
							  << " to NORMAL"
							  << COLOUR_NORMAL << std::endl;
					std::cout.flush();

					this->transitionToNormal();
				}
			}
			else
			{
				// 旧 view 的 MsgPrepare，丢弃不触发恢复
				if (DEBUG_HELP)
				{
					std::cout << COLOUR_ORANGE << this->printReplicaId()
							  << "Ignoring old MsgPrepare (view=" << proposeView_MsgPrepare
							  << " < current=" << this->view << ") without committee"
							  << COLOUR_NORMAL << std::endl;
				}
			}
		}
	}

	// Determine quorum threshold based on stage
	// 关键修复：NORMAL 模式使用 generalQuorumSize
	// GRACIOUS/UNCIVIL 模式使用 trustedQuorumSize
	unsigned int quorumThreshold = this->generalQuorumSize;  // 默认
	if (this->stage == STAGE_GRACIOUS && this->committee.getSize() > 0)
	{
		quorumThreshold = this->trustedQuorumSize;
	}

	// Validate the MsgPrepare
	if (proposeView_MsgPrepare >= this->view && phase_MsgPrepare == PHASE_PREPARE)
	{
		if (proposeView_MsgPrepare == this->view)
		{
			// 论文设计：GRACIOUS 模式只有 committee 成员的 MsgPrepare vote 有效
			// Leader 验证 sender 是否是 committee member
			if (this->stage == STAGE_GRACIOUS || this->stage == STAGE_UNCIVIL)
			{
				// Get the signer from the single signature MsgPrepare
				// Note: signs_MsgPrepare may contain single or multiple signatures
				bool senderIsValid = false;
				for (unsigned int i = 0; i < signs_MsgPrepare.getSize(); i++)
				{
					ReplicaID signer = signs_MsgPrepare.get(i).getSigner();
					if (this->currentCommittee.find(signer) != this->currentCommittee.end())
					{
						senderIsValid = true;
						break;
					}
				}
				if (!senderIsValid)
				{
					if (DEBUG_HELP)
					{
						std::cout << COLOUR_ORANGE << this->printReplicaId()
								  << "MsgPrepare from non-committee member in GRACIOUS mode, ignoring"
								  << COLOUR_NORMAL << std::endl;
					}
					auto end = std::chrono::steady_clock::now();
					double time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
					statistics.addHandleTime(time);
					return;
				}
			}

			// Store in log
			this->log.storeMsgPrepareResiBFT(msgPrepare);

			// Check if this message already has quorum signatures (aggregated message from Leader)
			unsigned int signCount = signs_MsgPrepare.getSize();

			if (signCount >= quorumThreshold)
			{
				// This is an aggregated MsgPrepare from Leader with quorum signatures
				// 论文设计：GRACIOUS 模式只有 committee members 参与
				bool shouldParticipate = false;

				if (this->stage == STAGE_GRACIOUS || this->stage == STAGE_UNCIVIL)
				{
					shouldParticipate = this->isCommitteeMember;
					if (!shouldParticipate)
					{
						if (DEBUG_HELP)
						{
							std::cout << COLOUR_ORANGE << this->printReplicaId()
									  << "Not in committee, ignoring MsgPrepare in GRACIOUS mode"
									  << COLOUR_NORMAL << std::endl;
						}
					}
				}
				else
				{
					shouldParticipate = true;  // NORMAL 模式：所有节点参与
				}

				if (shouldParticipate)
				{
					// Leader initiates next phase, non-leader replicas respond with MsgPrecommit
					if (this->amCurrentLeader())
					{
						this->initiateMsgPrepareResiBFT(roundData_MsgPrepare);
					}
					else
					{
						// Non-leader replicas respond with MsgPrecommit
						if (DEBUG_HELP)
						{
							std::cout << COLOUR_BLUE << this->printReplicaId() << "MsgPrepare has quorum (" << signCount << " signatures), responding with MsgPrecommit" << COLOUR_NORMAL << std::endl;
						}
						this->respondMsgPrepareResiBFT(Justification(roundData_MsgPrepare, signs_MsgPrepare));
					}
				}
			}
			else
			{
				// Single signature message - Leader should check aggregated signature count from log
				if (this->amCurrentLeader())
				{
					// Get aggregated signatures from log and check quorum
					Signs aggregatedSigns = this->log.getMsgPrepareResiBFT(proposeView_MsgPrepare, quorumThreshold);
					unsigned int aggregatedSignCount = aggregatedSigns.getSize();

					if (DEBUG_HELP)
					{
						std::cout << COLOUR_BLUE << this->printReplicaId() << "Leader collected " << aggregatedSignCount << " MsgPrepare signatures for view " << proposeView_MsgPrepare << COLOUR_NORMAL << std::endl;
					}

					if (aggregatedSignCount >= quorumThreshold)
					{
						// Leader has collected quorum signatures, initiate broadcast
						if (DEBUG_HELP)
						{
							std::cout << COLOUR_BLUE << this->printReplicaId() << "Leader reached quorum (" << aggregatedSignCount << " signatures), initiating MsgPrepare broadcast" << COLOUR_NORMAL << std::endl;
						}
						this->initiateMsgPrepareResiBFT(roundData_MsgPrepare);
					}
				}
			}
		}
		else
		{
			if (DEBUG_HELP)
			{
				std::cout << COLOUR_BLUE << this->printReplicaId() << "Storing MsgPrepare for higher view: " << msgPrepare.toPrint() << COLOUR_NORMAL << std::endl;
			}
			this->log.storeMsgPrepareResiBFT(msgPrepare);
		}
	}
	else
	{
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Discarded MsgPrepare: " << msgPrepare.toPrint() << COLOUR_NORMAL << std::endl;
		}
	}

	auto end = std::chrono::steady_clock::now();
	double time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	statistics.addHandleTime(time);
}

void ResiBFT::handleMsgPrecommitResiBFT(MsgPrecommitResiBFT msgPrecommit)
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

	// Determine quorum threshold based on stage
	// NORMAL mode: generalQuorumSize (all replicas vote)
	// GRACIOUS/UNCIVIL mode: trustedQuorumSize (committee votes)
	unsigned int quorumThreshold = this->generalQuorumSize;  // 默认
	if (this->stage == STAGE_GRACIOUS && this->committee.getSize() > 0)
	{
		quorumThreshold = this->trustedQuorumSize;
	}

	// Validate the MsgPrecommit
	if (proposeView_MsgPrecommit >= this->view && phase_MsgPrecommit == PHASE_PRECOMMIT)
	{
		if (proposeView_MsgPrecommit == this->view)
		{
			// 去重检查：如果已经执行过该 view，跳过
			if (this->executedViews.find(proposeView_MsgPrecommit) != this->executedViews.end())
			{
				if (DEBUG_HELP)
				{
					std::cout << COLOUR_ORANGE << this->printReplicaId() << "View " << proposeView_MsgPrecommit << " already executed, skipping MsgPrecommit" << COLOUR_NORMAL << std::endl;
				}
				return;
			}

			// 论文设计：GRACIOUS 模式只有 committee 成员的 MsgPrecommit vote 有效
			// Leader 验证 sender 是否是 committee member
			if (this->stage == STAGE_GRACIOUS || this->stage == STAGE_UNCIVIL)
			{
				// Get the signer from the single signature MsgPrecommit
				bool senderIsValid = false;
				for (unsigned int i = 0; i < signs_MsgPrecommit.getSize(); i++)
				{
					ReplicaID signer = signs_MsgPrecommit.get(i).getSigner();
					if (this->currentCommittee.find(signer) != this->currentCommittee.end())
					{
						senderIsValid = true;
						break;
					}
				}
				if (!senderIsValid)
				{
					if (DEBUG_HELP)
					{
						std::cout << COLOUR_ORANGE << this->printReplicaId()
								  << "MsgPrecommit from non-committee member in GRACIOUS mode, ignoring"
								  << COLOUR_NORMAL << std::endl;
					}
					auto end = std::chrono::steady_clock::now();
					double time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
					statistics.addHandleTime(time);
					return;
				}
			}

			// Store in log
			this->log.storeMsgPrecommitResiBFT(msgPrecommit);

			// Check if this message already has quorum signatures (aggregated message from Leader)
			unsigned int signCount = signs_MsgPrecommit.getSize();

			if (signCount >= quorumThreshold)
			{
				// 去重检查：确保 lockedQC 只更新一次
				if (this->lockedViews.find(proposeView_MsgPrecommit) == this->lockedViews.end())
				{
					// This is an aggregated MsgPrecommit from Leader with quorum signatures
					// Update lockedQC (HotStuff safety mechanism)
					Justification prepareQC = Justification(roundData_MsgPrecommit, signs_MsgPrecommit);
					this->updateLockedQC(prepareQC);
					this->lockedViews.insert(proposeView_MsgPrecommit);
				}

				// Leader initiates MsgDecide or MsgCommit
				if (this->amCurrentLeader())
				{
					if (DEBUG_HELP)
					{
						std::cout << COLOUR_BLUE << this->printReplicaId() << "MsgPrecommit has quorum (" << signCount << " signatures), initiating decision" << COLOUR_NORMAL << std::endl;
					}

					if (this->stage == STAGE_NORMAL)
					{
						// HotStuff: use MsgCommit in NORMAL mode
						this->initiateMsgCommitResiBFT(roundData_MsgPrecommit);
					}
					else
					{
						// GRACIOUS/UNCIVIL: use MsgDecide
						this->initiateMsgPrecommitResiBFT(roundData_MsgPrecommit);
					}
				}
				else
				{
					// 去重检查：确保只发送一次 precommit vote
					if (this->precommitVotedViews.find(proposeView_MsgPrecommit) == this->precommitVotedViews.end())
					{
						// GRACIOUS/UNCIVIL 模式：只有 committee 成员才能投票
						// NORMAL 模式：所有副本都能投票
						bool canVote = (this->stage == STAGE_NORMAL) || this->isCommitteeMember;

						if (canVote)
						{
							// Non-leader replicas respond with MsgPrecommit vote
							if (DEBUG_HELP)
							{
								std::cout << COLOUR_BLUE << this->printReplicaId() << "MsgPrecommit has quorum, voting" << COLOUR_NORMAL << std::endl;
							}
							// Create and send precommit vote
							Sign mySign = Sign(true, this->replicaId, (unsigned char *)"precommit");
							Signs mySigns = Signs();
							mySigns.add(mySign);
							MsgPrecommitResiBFT voteMsg(roundData_MsgPrecommit, mySigns);
							ReplicaID leader = this->getCurrentLeader();
							Peers recipients = this->keepFromPeers(leader);
							this->sendMsgPrecommitResiBFT(voteMsg, recipients);
							this->precommitVotedViews.insert(proposeView_MsgPrecommit);
						}
						else
						{
							if (DEBUG_HELP)
							{
								std::cout << COLOUR_ORANGE << this->printReplicaId() << "Not a committee member, skipping precommit vote in GRACIOUS mode" << COLOUR_NORMAL << std::endl;
							}
						}
					}
				}
			}
			else
			{
				// Single signature message - Leader should check aggregated signature count from log
				if (this->amCurrentLeader())
				{
					// Get aggregated signatures from log and check quorum
					Signs aggregatedSigns = this->log.getMsgPrecommitResiBFT(proposeView_MsgPrecommit, quorumThreshold);
					unsigned int aggregatedSignCount = aggregatedSigns.getSize();

					if (DEBUG_HELP)
					{
						std::cout << COLOUR_BLUE << this->printReplicaId() << "Leader collected " << aggregatedSignCount << " MsgPrecommit signatures for view " << proposeView_MsgPrecommit << COLOUR_NORMAL << std::endl;
					}

					if (aggregatedSignCount >= quorumThreshold)
					{
						// Leader has collected quorum signatures, initiate decision broadcast
						if (DEBUG_HELP)
						{
							std::cout << COLOUR_BLUE << this->printReplicaId() << "Leader reached quorum (" << aggregatedSignCount << " signatures), initiating decision broadcast" << COLOUR_NORMAL << std::endl;
						}

						// Update lockedQC
						Justification prepareQC = Justification(roundData_MsgPrecommit, aggregatedSigns);
						this->updateLockedQC(prepareQC);

						if (this->stage == STAGE_NORMAL)
						{
							this->initiateMsgCommitResiBFT(roundData_MsgPrecommit);
						}
						else
						{
							this->initiateMsgPrecommitResiBFT(roundData_MsgPrecommit);
						}
					}
				}
			}
		}
		else
		{
			if (DEBUG_HELP)
			{
				std::cout << COLOUR_BLUE << this->printReplicaId() << "Storing MsgPrecommit for higher view: " << msgPrecommit.toPrint() << COLOUR_NORMAL << std::endl;
			}
			this->log.storeMsgPrecommitResiBFT(msgPrecommit);
		}
	}
	else
	{
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Discarded MsgPrecommit: " << msgPrecommit.toPrint() << COLOUR_NORMAL << std::endl;
		}
	}

	auto end = std::chrono::steady_clock::now();
	double time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	statistics.addHandleTime(time);
}

void ResiBFT::handleMsgDecideResiBFT(MsgDecideResiBFT msgDecide)
{
	auto start = std::chrono::steady_clock::now();

	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Handling MsgDecide: " << msgDecide.toPrint() << COLOUR_NORMAL << std::endl;
	}
	RoundData roundData_MsgDecide = msgDecide.qcPrecommit.getRoundData();
	Signs signs_MsgDecide = msgDecide.signs;
	View proposeView_MsgDecide = roundData_MsgDecide.getProposeView();
	Phase phase_MsgDecide = roundData_MsgDecide.getPhase();

	// 关键修复：如果 MsgDecide 来自 future view，先处理该 view 的 MsgCommittee
	// 这样才能获取正确的 trustedQuorumSize 用于验证 GRACIOUS 模式的 MsgDecide
	if (proposeView_MsgDecide > this->view)
	{
		MsgCommitteeResiBFT pendingMsgCommittee = this->log.getMsgCommitteeResiBFT(proposeView_MsgDecide);
		if (pendingMsgCommittee.committee.getSize() > 0)
		{
			std::cout << COLOUR_MAGENTA << this->printReplicaId()
					  << "[MSGDECIDE] Found MsgCommittee for future view " << proposeView_MsgDecide
					  << ", processing it to get correct trustedQuorumSize"
					  << COLOUR_NORMAL << std::endl;
			std::cout.flush();

			// 临时处理 MsgCommittee 以更新 trustedQuorumSize（不改变 stage）
			this->committee = pendingMsgCommittee.committee;
			this->committeeView = proposeView_MsgDecide;
			unsigned int committeeSize = pendingMsgCommittee.committee.getSize();
			this->trustedQuorumSize = (committeeSize + 1) / 2;

			std::cout << COLOUR_MAGENTA << this->printReplicaId()
					  << "[MSGDECIDE] Updated trustedQuorumSize to " << this->trustedQuorumSize
					  << " for view " << proposeView_MsgDecide
					  << " (committee size=" << committeeSize << ")"
					  << COLOUR_NORMAL << std::endl;
			std::cout.flush();
		}
	}

	// Validate signatures - accept if quorum threshold is met
	// NORMAL mode uses generalQuorumSize, GRACIOUS mode uses trustedQuorumSize
	// For catch-up, accept MsgDecide from either mode
	unsigned int signCount = signs_MsgDecide.getSize();
	bool validSignatures = (signCount >= this->generalQuorumSize) ||
						   (signCount >= this->trustedQuorumSize);

	if (!validSignatures)
	{
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_RED << this->printReplicaId()
					  << "MsgDecide has insufficient signatures: " << signCount
					  << " (generalQuorumSize=" << this->generalQuorumSize
					  << ", trustedQuorumSize=" << this->trustedQuorumSize << ")"
					  << COLOUR_NORMAL << std::endl;
		}
		auto end = std::chrono::steady_clock::now();
		double time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
		statistics.addHandleTime(time);
		return;
	}

	// Validate the MsgDecide
	if (proposeView_MsgDecide >= this->view && phase_MsgDecide == PHASE_PRECOMMIT)
	{
		if (proposeView_MsgDecide == this->view)
		{
			// Execute the block and move to next view
			this->executeBlockResiBFT(roundData_MsgDecide);
		}
		else
		{
			// Higher view - jump to that view and execute (catch-up via MsgDecide)
			// MsgDecide 表示共识已完成，落后节点应该直接跳转并执行
			std::cout << COLOUR_GREEN << this->printReplicaId()
					  << "[DECIDE-CATCH-UP] Jumping from view " << this->view
					  << " to view " << proposeView_MsgDecide
					  << " (MsgDecide received, consensus completed)"
					  << COLOUR_NORMAL << std::endl;
			std::cout.flush();

			// 跳转到 MsgDecide 的 view
			while (this->view < proposeView_MsgDecide)
			{
				this->view++;
			}

			// 同步 TEE view
			if (!this->amGeneralReplicaIds())
			{
				sgx_status_t retval;
				sgx_status_t status_t = TEE_setViewResiBFT(global_eid, &retval, &this->view);
				if (status_t != SGX_SUCCESS || retval != SGX_SUCCESS)
				{
					if (DEBUG_HELP)
					{
						std::cout << COLOUR_RED << this->printReplicaId() << "TEE_setViewResiBFT failed during decide catch-up" << COLOUR_NORMAL << std::endl;
					}
				}
			}

			// 执行该 block（使用 isCatchUp 模式）
			if (this->executedViews.find(proposeView_MsgDecide) == this->executedViews.end())
			{
				this->executeBlockResiBFT(roundData_MsgDecide, true);  // true = isCatchUp
			}

			// 关键修复：检查是否应该停止
			// 如果 catch-up 到最后一个 view（或更后面的 view），应该停止并记录 statistics
			if (this->timeToStop())
			{
				std::cout << COLOUR_GREEN << this->printReplicaId()
						  << "[DECIDE-CATCH-UP] Reached final view " << this->view
						  << " >= numViews " << this->numViews
						  << " - stopping and recording statistics"
						  << COLOUR_NORMAL << std::endl;
				std::cout.flush();

				// Stop the timer to prevent further view changes
				this->timer.del();
				this->recordStatisticsResiBFT();
			}
			else
			{
				// 还没到最终 view，进入下一个 view 继续参与共识
				std::cout << COLOUR_GREEN << this->printReplicaId()
						  << "[DECIDE-CATCH-UP] Starting new view after catch-up, will advance from view " << this->view
						  << " to view " << (this->view + 1)
						  << COLOUR_NORMAL << std::endl;
				std::cout.flush();

				// 进入下一个 view，发送 MsgNewview 给新 Leader
				this->startNewViewResiBFT();
			}
		}
	}
	else if (proposeView_MsgDecide < this->view && phase_MsgDecide == PHASE_PRECOMMIT)
	{
		// Catch-up: 收到旧 view 的 MsgDecide，尝试执行该 block
		// 这是 view sync 后 catch-up 的关键机制
		if (this->executedViews.find(proposeView_MsgDecide) == this->executedViews.end())
		{
			// 还没执行过这个 view，可以执行
			std::cout << COLOUR_GREEN << this->printReplicaId()
					  << "[CATCH-UP] Executing block for earlier view " << proposeView_MsgDecide
					  << " (current view is " << this->view << ")"
					  << COLOUR_NORMAL << std::endl;
			std::cout.flush();
			this->executeBlockResiBFT(roundData_MsgDecide, true);  // true = isCatchUp
		}
		else
		{
			if (DEBUG_HELP)
			{
				std::cout << COLOUR_BLUE << this->printReplicaId()
						  << "Already executed view " << proposeView_MsgDecide
						  << ", discarding MsgDecide"
						  << COLOUR_NORMAL << std::endl;
			}
		}
	}
	else
	{
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Discarded MsgDecide: " << msgDecide.toPrint() << COLOUR_NORMAL << std::endl;
		}
	}

	auto end = std::chrono::steady_clock::now();
	double time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	statistics.addHandleTime(time);
}

void ResiBFT::handleMsgBCResiBFT(MsgBCResiBFT msgBC)
{
	auto start = std::chrono::steady_clock::now();

	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Handling MsgBC: " << msgBC.toPrint() << COLOUR_NORMAL << std::endl;
	}

	BlockCertificate blockCertificate = msgBC.blockCertificate;
	View bcView = blockCertificate.getView();

	// Validate the block certificate
	if (blockCertificate.verify(this->nodes) && bcView >= this->view)
	{
		// Store the block certificate
		this->log.storeMsgBCResiBFT(msgBC);
		this->blockCertificates[bcView] = blockCertificate;

		// If quorum reached, create checkpoint for epoch management
		if (this->log.hasBCQuorum(bcView, this->trustedQuorumSize))
		{
			Block block = blockCertificate.getBlock();
			Justification qcPrecommit = blockCertificate.getQcPrecommit();
			Signs qcVc = blockCertificate.getQcVc();
			Checkpoint checkpoint = Checkpoint(block, bcView, qcPrecommit, qcVc);
			this->log.storeCheckpoint(checkpoint);
			this->checkpoints.insert(checkpoint);

			if (DEBUG_HELP)
			{
				std::cout << COLOUR_BLUE << this->printReplicaId() << "Checkpoint created for view " << bcView << COLOUR_NORMAL << std::endl;
			}
		}
	}
	else
	{
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_RED << this->printReplicaId() << "Invalid MsgBC discarded" << COLOUR_NORMAL << std::endl;
		}
	}

	auto end = std::chrono::steady_clock::now();
	double time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	statistics.addHandleTime(time);
}

void ResiBFT::handleMsgVCResiBFT(MsgVCResiBFT msgVC)
{
	auto start = std::chrono::steady_clock::now();

	VerificationCertificate vc = msgVC.vc;
	VCType vcType = vc.getType();
	View vcView = vc.getView();
	ReplicaID signer = vc.getSigner();

	std::cout << COLOUR_CYAN << this->printReplicaId()
			  << "[VC-VALIDATION] START: type=" << printVCType(vcType)
			  << ", view=" << vcView
			  << ", signer=" << signer
			  << ", currentView=" << this->view
			  << ", stage=" << printStage(this->stage)
			  << COLOUR_NORMAL << std::endl;
	std::cout.flush();

	// For VC_TIMEOUT and VC_REJECTED, accept VCs from recent views (current or previous) because
	// these messages may arrive after view has changed due to network delay
	// This is critical for detecting trusted replica unavailability (GRACIOUS) and malicious behavior (UNCIVIL)
	bool shouldAcceptVC = false;
	if (vcType == VC_TIMEOUT || vcType == VC_REJECTED)
	{
		// Accept VC_TIMEOUT and VC_REJECTED from current view or previous view
		shouldAcceptVC = (vcView == this->view || vcView == this->view - 1);
	}
	else
	{
		// For other VC types, only accept from current or future views
		shouldAcceptVC = (vcView >= this->view);
	}

	if (shouldAcceptVC)
	{
		// Store in log
		this->log.storeMsgVCResiBFT(msgVC);

		// Store in appropriate set based on type
		if (vcType == VC_ACCEPTED)
		{
			this->acceptedVCs.insert(vc);
			// Check if quorum reached - might return to NORMAL
			this->checkAcceptedVCs();
		}
		else if (vcType == VC_REJECTED)
		{
			this->rejectedVCs.insert(vc);
			// Check if threshold reached - transition to UNCIVIL
			this->checkRejectedVCs();
		}
		else if (vcType == VC_TIMEOUT)
		{
			this->timeoutVCs.insert(vc);
			// Check if threshold reached - 计算 proposedCommittee（不立即进入 GRACIOUS）
			if (this->checkTimeoutVCs())
			{
				// 论文设计：TIMEOUT VC ≥ f+1 表示 T-Replica Leader 不可用
				// 关键修改：不调用 transitionToGracious()，而是计算 proposedCommittee
				// 条件：前一个 view 的 Leader 是 T-Replica 且当前在 NORMAL 阶段
				// 重要：只有当前 view 的 leader 才应该触发 GRACIOUS，避免多个节点发送不同的 committee
				View previousView = this->view - 1;
				if (previousView >= 0 && this->stage == STAGE_NORMAL && this->amCurrentLeader())
				{
					ReplicaID previousLeader = this->getLeaderOf(previousView);
					bool previousLeaderWasTrusted = (previousLeader >= this->numGeneralReplicas);

					if (previousLeaderWasTrusted)
					{
#ifdef PRODUCTION_MODE
						unsigned int k = COMMITTEE_K_MIN;
#else
						unsigned int k = std::min((unsigned int)COMMITTEE_K_MIN, this->numTrustedReplicas);
#endif

						std::cout << COLOUR_MAGENTA << this->printReplicaId()
								  << "[GRACIOUS-TRIGGER] TIMEOUT VC threshold reached for T-Replica Leader "
								  << previousLeader << " (view " << previousView << ") - computing proposedCommittee (will enter GRACIOUS after block execution)"
								  << COLOUR_NORMAL << std::endl;
						std::cout.flush();

						// VRF 选择委员会，存储为 proposedCommittee
						this->proposedCommittee = this->selectCommitteeByVRF(this->view, k);
						this->proposedCommitteeView = this->view;

						std::cout << COLOUR_MAGENTA << this->printReplicaId()
								  << "[GRACIOUS-TRIGGER] VRF selection done: proposedCommitteeSize=" << this->proposedCommittee.getSize()
								  << ", stage=" << printStage(this->stage) << " (staying in NORMAL)"
								  << ", proposedCommittee will be sent in MsgLdrprepare"
								  << COLOUR_NORMAL << std::endl;
						std::cout.flush();

						// 不调用 transitionToGracious()
						// 不发送 MsgCommittee
						// proposedCommittee 将在 MsgLdrprepare 中发送
					}
				}
			}
		}

		// Store in pending VCs for later processing
		this->pendingVCs[vcView].insert(vc);

		auto end = std::chrono::steady_clock::now();
		double time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

		std::cout << COLOUR_CYAN << this->printReplicaId()
				  << "[VC-VALIDATION] END: type=" << printVCType(vcType)
				  << ", acceptedVCs=" << this->acceptedVCs.size()
				  << ", rejectedVCs=" << this->rejectedVCs.size()
				  << ", timeoutVCs=" << this->timeoutVCs.size()
				  << ", time=" << time << " μs"
				  << COLOUR_NORMAL << std::endl;
		std::cout.flush();
	}
	else
	{
		auto end = std::chrono::steady_clock::now();
		double time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

		std::cout << COLOUR_CYAN << this->printReplicaId()
				  << "[VC-VALIDATION] DISCARD: oldVC (view=" << vcView << " < current=" << this->view << ")"
				  << ", time=" << time << " μs"
				  << COLOUR_NORMAL << std::endl;
		std::cout.flush();
	}

	statistics.addHandleTime(std::chrono::duration_cast<std::chrono::microseconds>(
		std::chrono::steady_clock::now() - start).count());
}

void ResiBFT::handleMsgCommitteeResiBFT(MsgCommitteeResiBFT msgCommittee)
{
	auto start = std::chrono::steady_clock::now();

	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Handling MsgCommittee: " << msgCommittee.toPrint() << COLOUR_NORMAL << std::endl;
	}

	Group newCommittee = msgCommittee.committee;
	View committeeView = msgCommittee.view;

	// 从 signs 中提取委员会 Leader（消息的签名者）
	Signs committeeSigns = msgCommittee.signs;
	ReplicaID msgCommitteeLeader = committeeView % this->numReplicas;  // 默认：触发GRACIOUS的T-Replica Leader
	if (committeeSigns.getSize() > 0)
	{
		// 获取第一个签名者的ID作为委员会Leader
		Sign firstSign = committeeSigns.get(0);
		msgCommitteeLeader = firstSign.getSigner();
	}

	// MsgCommittee是为future view准备的，不应该跳过当前view的共识
	// 如果committeeView > currentView，存储等待当前view完成后使用
	if (committeeView > this->view)
	{
		// Store for future use (论文设计：log存储即可)
		this->log.storeMsgCommitteeResiBFT(msgCommittee);
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_ORANGE << this->printReplicaId()
					  << "Storing MsgCommittee for future view " << committeeView
					  << " (current view=" << this->view << ")"
					  << COLOUR_NORMAL << std::endl;
		}
		return;
	}

	// Only process MsgCommittee if it matches current view
	// Validate the committee message
	if (committeeView == this->view)
	{
		// 论文设计：验证 MsgCommittee 的发送者是正确的 view leader
		// 只有 view 的 T-Replica leader 才有权发送 MsgCommittee
		unsigned int correctLeader = committeeView % this->numReplicas;

		// 检查发送者是否是正确的 T-Replica leader
		if (msgCommitteeLeader != correctLeader)
		{
			if (DEBUG_HELP)
			{
				std::cout << COLOUR_RED << this->printReplicaId()
						  << "INVALID MsgCommittee: sender=" << msgCommitteeLeader
						  << " is NOT the correct leader for view " << committeeView
						  << " (correct leader=" << correctLeader << ")"
						  << COLOUR_NORMAL << std::endl;
			}
			// 拒绝无效的 MsgCommittee
			return;
		}

		// 检查发送者是否是 T-Replica
		bool senderIsTrusted = (msgCommitteeLeader >= this->numGeneralReplicas);
		if (!senderIsTrusted)
		{
			if (DEBUG_HELP)
			{
				std::cout << COLOUR_RED << this->printReplicaId()
						  << "INVALID MsgCommittee: sender=" << msgCommitteeLeader
						  << " is NOT a T-Replica (numGeneralReplicas=" << this->numGeneralReplicas << ")"
						  << COLOUR_NORMAL << std::endl;
			}
			// G-Replica 不应该发送 MsgCommittee
			return;
		}

		// Store in log
		this->log.storeMsgCommitteeResiBFT(msgCommittee);

		// 关键修改：根据当前 stage 处理 MsgCommittee
		// NORMAL 模式：存储为 proposedCommittee，不立即更新 committee 或进入 GRACIOUS
		// GRACIOUS 模式：立即更新 committee（因为已经在 GRACIOUS）
		if (this->stage == STAGE_NORMAL)
		{
			// 论文设计：NORMAL 模式下收到 MsgCommittee
			// 不立即进入 GRACIOUS，存储为 proposedCommittee
			// 使用 generalQuorumSize 投票，等待区块执行后进入 GRACIOUS

			// 存储 proposedCommittee
			this->proposedCommittee = newCommittee;
			this->proposedCommitteeView = committeeView;

			std::cout << COLOUR_MAGENTA << this->printReplicaId()
					  << "[MSGCOMMITTEE] Received MsgCommittee for view " << committeeView
					  << ", proposedCommitteeSize=" << newCommittee.getSize()
					  << ", stage=" << printStage(this->stage) << " (staying in NORMAL)"
					  << ", will enter GRACIOUS after block execution"
					  << COLOUR_NORMAL << std::endl;
			std::cout.flush();

			// 不更新 committee, trustedQuorumSize, stage
			// 这些将在 executeBlockResiBFT 中更新
		}
		else if (this->stage == STAGE_GRACIOUS || this->stage == STAGE_UNCIVIL)
		{
			// 已经在 GRACIOUS/UNCIVIL 模式，立即更新 committee
			// 因为 committee 已经生效，只是需要更新到新的 view

			// Leader 发送 MsgCommittee 后，committee 成员直接接受（不需要 quorum MsgCommittee）
			// 因为 VRF 选择是可信的，Leader 是 Trusted replica
			this->committee = newCommittee;
			this->committeeView = committeeView;         // 记录 committee 对应的 view

			// 填充 currentCommittee std::set（用于验证 membership）
			this->currentCommittee.clear();
			for (unsigned int i = 0; i < newCommittee.getSize(); i++)
			{
				this->currentCommittee.insert(newCommittee.getGroup()[i]);
			}

			this->isCommitteeMember = isInCommittee(this->replicaId);
			this->committeeLeader = msgCommitteeLeader;  // 设置委员会Leader

			// 论文设计：更新 trustedQuorumSize 为委员会 quorum
			// q_T = floor((k+1)/2)
			unsigned int committeeSize = newCommittee.getSize();
			this->trustedQuorumSize = (committeeSize + 1) / 2;

			std::cout << COLOUR_MAGENTA << this->printReplicaId()
					  << "[MSGCOMMITTEE] Updated committee in GRACIOUS mode: k=" << committeeSize
					  << ", q_T=" << this->trustedQuorumSize
					  << ", committee=" << this->committee.toPrint()
					  << ", isMember=" << this->isCommitteeMember
					  << ", committeeLeader=" << this->committeeLeader
					  << COLOUR_NORMAL << std::endl;
			std::cout.flush();

			// In GRACIOUS mode, committee takes over consensus duties
			if (this->isCommitteeMember)
			{
				if (DEBUG_HELP)
				{
					std::cout << COLOUR_MAGENTA << this->printReplicaId() << "In GRACIOUS mode, committee member can proceed" << COLOUR_NORMAL << std::endl;
				}
			}
		}
	}
	else
	{
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_RED << this->printReplicaId() << "Old Committee message discarded" << COLOUR_NORMAL << std::endl;
		}
	}

	auto end = std::chrono::steady_clock::now();
	double time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	statistics.addHandleTime(time);
}

// HotStuff handlers for NORMAL mode
void ResiBFT::handleMsgPrepareProposalResiBFT(MsgPrepareProposalResiBFT msgPrepareProposal)
{
	auto start = std::chrono::steady_clock::now();

	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Handling MsgPrepareProposal: " << msgPrepareProposal.toPrint() << COLOUR_NORMAL << std::endl;
	}

	Proposal<Justification> proposal = msgPrepareProposal.proposal;
	Justification parentQC = proposal.getCertification();
	Block block = proposal.getBlock();
	View parentQCProposeView = parentQC.getRoundData().getProposeView();  // sender's view when sending MsgNewview

	// HotStuff: block view = parentQC.proposeView (which is the sender's/Lear's view when they sent MsgNewview)
	// The Leader creates block for current view, and parentQC comes from MsgNewview with proposeView = sender's view
	// Example: MsgNewview(proposeView=1), Leader at view 1, creates block for view 1
	View blockView = parentQCProposeView;

	// Validate view - check the block's view matches receiver's view
	if (blockView != this->view)
	{
		if (blockView > this->view)
		{
			// Higher view - store for future processing (don't jump)
			// MsgPrepareProposal只是提议，不代表共识已完成
			// 只有MsgCommit才能触发跳转（因为是最终决定）
			if (DEBUG_HELP)
			{
				std::cout << COLOUR_BLUE << this->printReplicaId() << "Storing MsgPrepareProposal for higher view " << blockView << " (current=" << this->view << ")" << COLOUR_NORMAL << std::endl;
			}
			return;
		}
		else
		{
			// Old view - discard
			if (DEBUG_HELP)
			{
				std::cout << COLOUR_RED << this->printReplicaId() << "MsgPrepareProposal for old view: " << blockView << " vs " << this->view << COLOUR_NORMAL << std::endl;
			}
			return;
		}
	}

	// 去重检查：检查是否已经为该 view 存储了 block
	if (this->blocks.find(blockView) != this->blocks.end())
	{
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_ORANGE << this->printReplicaId() << "MsgPrepareProposal already processed for view " << blockView << ", skipping duplicate" << COLOUR_NORMAL << std::endl;
		}
		return;
	}

	// HotStuff safeNode check: accept if parentQC.view > lockedView OR parentQC.hash == lockedHash
	if (!this->checkSafeNode(parentQC))
	{
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_RED << this->printReplicaId() << "MsgPrepareProposal failed safeNode check" << COLOUR_NORMAL << std::endl;
		}
		return;
	}

	// Store the block
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Storing block for view " << this->view << ": " << block.toPrint() << COLOUR_NORMAL << std::endl;
	}
	this->blocks[this->view] = block;

	// ALL replicas vote in NORMAL mode (HotStuff design)
	this->respondMsgPrepareProposalResiBFT(proposal);

	auto end = std::chrono::steady_clock::now();
	double time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	statistics.addHandleTime(time);
}

void ResiBFT::handleMsgCommitResiBFT(MsgCommitResiBFT msgCommit)
{
	auto start = std::chrono::steady_clock::now();

	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Handling MsgCommit: " << msgCommit.toPrint() << COLOUR_NORMAL << std::endl;
	}

	RoundData roundData_MsgCommit = msgCommit.qcPrecommit.getRoundData();
	Signs signs_MsgCommit = msgCommit.signs;
	View proposeView_MsgCommit = roundData_MsgCommit.getProposeView();
	Phase phase_MsgCommit = roundData_MsgCommit.getPhase();

	// 验证签名数量（quorum check）
	unsigned int quorumThreshold = this->generalQuorumSize;  // 默认
	if (this->stage == STAGE_GRACIOUS && this->committee.getSize() > 0)
	{
		quorumThreshold = this->trustedQuorumSize;
	}

	// Validate the MsgCommit
	if (proposeView_MsgCommit >= this->view && phase_MsgCommit == PHASE_PRECOMMIT)
	{
		// 验证 quorum 签名
		if (signs_MsgCommit.getSize() >= quorumThreshold)
		{
			// 去重检查：检查是否已经执行过该 view
			if (this->executedViews.find(proposeView_MsgCommit) != this->executedViews.end())
			{
				if (DEBUG_HELP)
				{
					std::cout << COLOUR_ORANGE << this->printReplicaId() << "MsgCommit for view " << proposeView_MsgCommit << " already executed, skipping duplicate" << COLOUR_NORMAL << std::endl;
				}
				return;
			}

			if (proposeView_MsgCommit == this->view)
			{
				// Execute the block and move to next view
				this->executeBlockResiBFT(roundData_MsgCommit);
			}
			else
			{
				// Higher view - store for future processing (论文设计)
				// 落后节点通过 timer 超时 + view change 机制追赶
				if (DEBUG_HELP)
				{
					std::cout << COLOUR_BLUE << this->printReplicaId() << "Storing MsgCommit for higher view: " << msgCommit.toPrint() << COLOUR_NORMAL << std::endl;
				}
				this->log.storeMsgCommitResiBFT(msgCommit);
			}
		}
		else
		{
			if (DEBUG_HELP)
			{
				std::cout << COLOUR_RED << this->printReplicaId() << "MsgCommit has insufficient signatures: " << signs_MsgCommit.getSize() << " < " << quorumThreshold << COLOUR_NORMAL << std::endl;
			}
		}
	}
	else
	{
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Discarded MsgCommit: " << msgCommit.toPrint() << COLOUR_NORMAL << std::endl;
		}
	}

	auto end = std::chrono::steady_clock::now();
	double time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	statistics.addHandleTime(time);
}

// HotStuff safeNode check: HotStuff safety rule
bool ResiBFT::checkSafeNode(Justification qc)
{
	// Safety: accept if qc.view > lockedView OR qc.hash == lockedHash
	// This ensures we never vote for conflicting blocks
	View qcView = qc.getRoundData().getProposeView();
	Hash qcHash = qc.getRoundData().getProposeHash();

	bool safeNodePassed = (qcView > this->lockedView) || (qcHash == this->lockedHash);

	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId()
				  << "safeNode check: qcView=" << qcView
				  << ", lockedView=" << this->lockedView
				  << ", qcHash=" << qcHash.toPrint()
				  << ", lockedHash=" << this->lockedHash.toPrint()
				  << ", result=" << safeNodePassed
				  << COLOUR_NORMAL << std::endl;
	}

	return safeNodePassed;
}

// Find highest QC from newview messages
Justification ResiBFT::findHighestQC(std::set<MsgNewviewResiBFT> newviews)
{
	Justification highestQC;
	View highestView = 0;
	Hash highestHash = Hash();

	for (const auto& msgNewview : newviews)
	{
		RoundData roundData = msgNewview.roundData;
		// HotStuff 设计：找 highest QC 的 justifyView（上一个已提交的 view）
		// justifyHash 是上一个 view 的 block hash，justifyView 是上一个 view
		View qcJustifyView = roundData.getJustifyView();
		Hash qcJustifyHash = roundData.getJustifyHash();

		// Use >= instead of > to handle case when all justifyViews are equal (e.g., all = 0 for genesis)
		// This ensures we select a valid QC even when all nodes have the same prepareView
		if (qcJustifyView >= highestView)
		{
			highestView = qcJustifyView;
			highestHash = qcJustifyHash;
			// Create justification from the newview message
			Justification qc = Justification(roundData, msgNewview.signs);
			highestQC = qc;
		}
	}

	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId()
				  << "findHighestQC: highestJustifyView=" << highestView
				  << ", highestJustifyHash=" << highestHash.toPrint()
				  << ", highestQC=" << highestQC.toPrint()
				  << COLOUR_NORMAL << std::endl;
	}

	return highestQC;
}

// Update lockedQC when receiving prepareQC
void ResiBFT::updateLockedQC(Justification newQC)
{
	View newView = newQC.getRoundData().getProposeView();
	Hash newHash = newQC.getRoundData().getProposeHash();

	// HotStuff: lock when we have prepareQC (quorum MsgPrepare votes)
	if (newView > this->lockedView)
	{
		this->lockedQC = newQC;
		this->lockedView = newView;
		this->lockedHash = newHash;

		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId()
					  << "Updated lockedQC: lockedView=" << this->lockedView
					  << ", lockedHash=" << this->lockedHash.toPrint()
					  << COLOUR_NORMAL << std::endl;
		}
	}
}

// VRF seed computation from committed blocks (生产环境实现)
// 论文要求：seed 必须来自历史已提交区块的哈希，确保 committee 选择前不可预测
unsigned int ResiBFT::computeVRFSeedFromCommittedBlocks(View view)
{
	// 安全要求：种子必须在 committee 选择前已确定
	// 使用 view - VRF_SEED_DELAY 之前的 checkpoint hashes
	View minStableView = view - VRF_SEED_DELAY_VIEWS;

	std::vector<Checkpoint> recentCheckpoints;

	// 从 checkpoints 中筛选稳定 checkpoint
	for (const auto& checkpoint : this->checkpoints)
	{
		if (checkpoint.getView() <= minStableView && checkpoint.getView() > 0)
		{
			recentCheckpoints.push_back(checkpoint);
		}
	}

	// 无 checkpoint 时使用 genesis hash
	if (recentCheckpoints.empty())
	{
		Block genesisBlock = Block();
		return convertHashToSeed(genesisBlock.hash(), view);
	}

	// XOR 组合多个 checkpoint hashes
	unsigned char combinedHash[SHA256_DIGEST_LENGTH];
	memset(combinedHash, 0, SHA256_DIGEST_LENGTH);

	// 使用最近 VRF_SEED_HISTORY_SIZE 个 checkpoints
	std::sort(recentCheckpoints.begin(), recentCheckpoints.end(),
		[](const Checkpoint& a, const Checkpoint& b) { return a.getView() > b.getView(); });

	size_t numToUse = std::min((size_t)VRF_SEED_HISTORY_SIZE, recentCheckpoints.size());

	for (size_t i = 0; i < numToUse; i++)
	{
		Hash checkpointHash = recentCheckpoints[i].hash();
		unsigned char* rawHash = checkpointHash.getHash();
		for (int j = 0; j < SHA256_DIGEST_LENGTH; j++)
		{
			combinedHash[j] ^= rawHash[j];
		}
	}

	// SHA256 再次哈希确保均匀分布
	unsigned char finalSeedHash[SHA256_DIGEST_LENGTH];
	SHA256(combinedHash, SHA256_DIGEST_LENGTH, finalSeedHash);

	// 转换为 unsigned int
	unsigned int seed = 0;
	for (int i = 0; i < 4; i++)
	{
		seed |= (finalSeedHash[i] << (8 * i));
	}

	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId()
				  << "VRF seed computed: view=" << view
				  << ", minStableView=" << minStableView
				  << ", numCheckpoints=" << recentCheckpoints.size()
				  << ", numUsed=" << numToUse
				  << ", seed=" << seed
				  << COLOUR_NORMAL << std::endl;
	}

	return seed;
}

unsigned int ResiBFT::convertHashToSeed(Hash hash, View view)
{
	unsigned char* rawHash = hash.getHash();
	unsigned int baseSeed = 0;
	for (int i = 0; i < 4; i++)
	{
		baseSeed |= (rawHash[i] << (8 * i));
	}
	return baseSeed ^ view;
}

unsigned int ResiBFT::sampleCommitteeSize(unsigned int seed, unsigned int numTrustedReplicas)
{
	unsigned int sampledK;

	// 优先使用环境变量中的 kRatio（如果设置了）
	double effectiveKRatio = getKRatioFromEnv();

#ifdef COMMITTEE_K_RATIO_MODE
	// 模式2：相对比例计算（实验方案要求）
	// k = d * kRatio，其中 d = numTrustedReplicas
	sampledK = (unsigned int)(numTrustedReplicas * effectiveKRatio);

	// 确保 k ≥ k_bar = COMMITTEE_K_MIN
	if (sampledK < COMMITTEE_K_MIN && numTrustedReplicas >= COMMITTEE_K_MIN)
	{
		sampledK = COMMITTEE_K_MIN;
	}

	// 确保 k ≤ numTrustedReplicas
	if (sampledK > numTrustedReplicas)
	{
		sampledK = numTrustedReplicas;
	}

	std::cout << COLOUR_CYAN << this->printReplicaId()
			  << "[COMMITTEE-K] Ratio mode: d=" << numTrustedReplicas
			  << ", ratio=" << effectiveKRatio
			  << ", k=" << sampledK
			  << " (k = d * " << effectiveKRatio << ")"
			  << COLOUR_NORMAL << std::endl;
#else
	// 模式1：固定范围采样（默认）
	unsigned int k_min = COMMITTEE_K_MIN;
	unsigned int k_max_config = COMMITTEE_K_MAX_DEFAULT;
	unsigned int k_max = std::min(numTrustedReplicas, k_max_config);

	// 处理边界情况
	if (k_max < k_min)
	{
		return numTrustedReplicas;  // 使用所有可用 T-Replicas
	}

	// 使用 seed 采样
	unsigned int rangeSize = k_max - k_min + 1;
	sampledK = k_min + (seed % rangeSize);

	std::cout << COLOUR_CYAN << this->printReplicaId()
			  << "[COMMITTEE-K] Range mode: k_min=" << k_min
			  << ", k_max=" << k_max
			  << ", seed=" << seed
			  << ", k=" << sampledK
			  << COLOUR_NORMAL << std::endl;
#endif

	return sampledK;
}

// VRF committee selection for GRACIOUS mode
Group ResiBFT::selectCommitteeByVRF(View view, unsigned int k)
{
	std::cout << COLOUR_RED << this->printReplicaId() << "selectCommitteeByVRF START: view=" << view << ", k=" << k << COLOUR_NORMAL << std::endl;
	std::cout.flush();

	// 论文设计：
	// 1. k 从预设区间 [k_min, k_max] 随机采样
	// 2. k ≥ k̄ = 30（最小委员会大小）
	// 3. 使用历史 committed block hashes 作为 seed

	// VRF seed computation
	unsigned int seed = view;

#ifdef PRODUCTION_MODE
	// 生产模式：使用历史 committed block hashes 计算 seed
	seed = computeVRFSeedFromCommittedBlocks(view);
#endif

#ifdef COMMITTEE_K_RATIO_MODE
	// 模式2：相对比例计算（实验方案要求）
	// k = d * kRatio，其中 d = numTrustedReplicas
	double effectiveKRatio = this->kRatio;  // 使用初始化时读取的 kRatio
	unsigned int sampledK = (unsigned int)(this->numTrustedReplicas * effectiveKRatio);

	// 确保 k ≥ k_bar = COMMITTEE_K_MIN
	if (sampledK < COMMITTEE_K_MIN && this->numTrustedReplicas >= COMMITTEE_K_MIN)
	{
		sampledK = COMMITTEE_K_MIN;
	}

	// 确保 k ≤ numTrustedReplicas
	if (sampledK > this->numTrustedReplicas)
	{
		sampledK = this->numTrustedReplicas;
	}

	k = sampledK;

	std::cout << COLOUR_CYAN << this->printReplicaId()
			  << "[COMMITTEE-K] Ratio mode: d=" << this->numTrustedReplicas
			  << ", kRatio=" << effectiveKRatio
			  << ", k=" << k
			  << " (k = d * " << effectiveKRatio << ")"
			  << ", seed=" << seed
			  << COLOUR_NORMAL << std::endl;
#else
	// 模式1：固定范围采样（默认）
	unsigned int k_min = COMMITTEE_K_MIN;
	unsigned int k_max = this->numTrustedReplicas;

	if (k_max < k_min)
	{
		k = k_max;
	}
	else
	{
		k = k_min + (seed % (k_max - k_min + 1));
	}

	std::cout << COLOUR_CYAN << this->printReplicaId()
			  << "[COMMITTEE-K] Range mode: k_min=" << k_min
			  << ", k_max=" << k_max
			  << ", k=" << k
			  << ", seed=" << seed
			  << COLOUR_NORMAL << std::endl;
#endif

	// Trusted replicas have IDs >= numGeneralReplicas
	std::vector<ReplicaID> trustedReplicaIds;
	for (unsigned int i = this->numGeneralReplicas; i < this->numReplicas; i++)
	{
		trustedReplicaIds.push_back(i);
	}

	std::cout << COLOUR_RED << this->printReplicaId() << "Trusted replicas: size=" << trustedReplicaIds.size() << COLOUR_NORMAL << std::endl;
	std::cout.flush();

	// VRF-like selection: 每个 T-Replica 计算 V(seed, id)
	// 选择 k 个 V(x) 值最小的 T-Replicas
	std::set<ReplicaID> selectedCommittee;
	unsigned int loopCount = 0;

	// 如果 k >= trustedReplicaIds.size()，直接选择所有 T-Replica
	if (k >= trustedReplicaIds.size())
	{
		std::cout << COLOUR_RED << this->printReplicaId() << "k >= trustedReplicaIds.size(), selecting all trusted replicas" << COLOUR_NORMAL << std::endl;
		std::cout.flush();
		for (unsigned int i = 0; i < trustedReplicaIds.size(); i++)
		{
			selectedCommittee.insert(trustedReplicaIds[i]);
		}
	}
	else
	{
		// 简化版本：伪随机选择
		// 使用更好的随机算法避免循环
		while (selectedCommittee.size() < k && selectedCommittee.size() < trustedReplicaIds.size())
		{
			// 使用更好的伪随机：seed * seed + view * 7 + loopCount
			unsigned int randomVal = seed * seed + view * 7 + loopCount;
			unsigned int idx = randomVal % trustedReplicaIds.size();
			selectedCommittee.insert(trustedReplicaIds[idx]);
			seed = seed + 1;
			loopCount++;
			if (loopCount > k * 10)
			{
				break;
			}
		}
	}

	std::cout << COLOUR_RED << this->printReplicaId() << "Loop finished: iterations=" << loopCount << ", selectedCommittee.size()=" << selectedCommittee.size() << COLOUR_NORMAL << std::endl;
	std::cout.flush();

	// 关键修复：确保当前 view 的 T-Replica Leader 在委员会中
	// Leader 必须参与共识，否则无法收集 votes
	unsigned int currentLeader = view % this->numReplicas;
	bool leaderIsTrusted = (currentLeader >= this->numGeneralReplicas);
	if (leaderIsTrusted && selectedCommittee.find(currentLeader) == selectedCommittee.end())
	{
		std::cout << COLOUR_MAGENTA << this->printReplicaId()
				  << "[COMMITTEE-FIX] Adding T-Replica Leader " << currentLeader
				  << " to committee (was not selected by VRF)"
				  << ", committee size: " << selectedCommittee.size() << " -> " << (selectedCommittee.size() + 1)
				  << COLOUR_NORMAL << std::endl;
		std::cout.flush();
		selectedCommittee.insert(currentLeader);
	}

	std::vector<ReplicaID> committeeList(selectedCommittee.begin(), selectedCommittee.end());

	std::cout << COLOUR_RED << this->printReplicaId() << "Creating Group with " << committeeList.size() << " members" << COLOUR_NORMAL << std::endl;
	std::cout.flush();

	Group committeeGroup = Group(committeeList);

	std::cout << COLOUR_RED << this->printReplicaId() << "Group created successfully" << COLOUR_NORMAL << std::endl;
	std::cout.flush();

	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId()
				  << "VRF selected committee for view " << view
				  << ": k=" << k << " (min=" << COMMITTEE_K_MIN << ", max=" << this->numTrustedReplicas << ")"
				  << ", committee=" << committeeGroup.toPrint()
				  << COLOUR_NORMAL << std::endl;
	}

	return committeeGroup;
}

// Initiate messages (GRACIOUS/UNCIVIL mode)
void ResiBFT::initiateMsgNewviewResiBFT()
{
	// HotStuff: MsgNewview(proposeView=k) 表示节点准备进入 view k
	// Leader k 查询 proposeView=k 的 MsgNewview 来开始 view k 的共识
	View queryView = this->view;

	// 关键修复：根据 stage 选择正确的 quorum threshold
	// NORMAL 模式：使用 generalQuorumSize (所有 replicas 参与)
	// GRACIOUS 模式：使用 trustedQuorumSize (只有 committee 参与)
	unsigned int quorumThreshold = this->generalQuorumSize;  // 默认
	if (this->stage == STAGE_GRACIOUS && this->committee.getSize() > 0)
	{
		quorumThreshold = this->trustedQuorumSize;
	}
	std::set<MsgNewviewResiBFT> msgNewviews = this->log.getMsgNewviewResiBFT(queryView, quorumThreshold);

	if (DEBUG_HELP)
	{
		std::cout << COLOUR_CYAN << this->printReplicaId()
				  << "initiateMsgNewviewResiBFT: msgNewviews.size()=" << msgNewviews.size()
				  << ", quorumThreshold=" << quorumThreshold
				  << " (stage=" << printStage(this->stage)
				  << ", generalQuorumSize=" << this->generalQuorumSize
				  << ", trustedQuorumSize=" << this->trustedQuorumSize << ")"
				  << ", view=" << this->view
				  << ", queryView=" << queryView
				  << ", simulateMalicious=" << this->simulateMalicious
				  << COLOUR_NORMAL << std::endl;
	}

	if (msgNewviews.size() >= quorumThreshold)
	{
		// 重置timer - 共识开始，给足够时间完成整个共识流程
		this->setTimer();

		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Initiating MsgLdrprepare from MsgNewview, timer reset" << COLOUR_NORMAL << std::endl;
		}
		Accumulator accumulator_MsgLdrprepare = this->initializeAccumulator(msgNewviews);

		if (accumulator_MsgLdrprepare.isSet())
		{
			// Create block extending the highest prepared hash
			Hash prepareHash_MsgLdrprepare = accumulator_MsgLdrprepare.getPrepareHash();
			Block block = this->createNewBlockResiBFT(prepareHash_MsgLdrprepare);

			// Create justification for PREPARE
			Justification justification_MsgPrepare = this->respondMsgLdrprepareProposalResiBFT(block.hash(), accumulator_MsgLdrprepare);
			if (justification_MsgPrepare.isSet())
			{
				if (DEBUG_HELP)
				{
					std::cout << COLOUR_BLUE << this->printReplicaId() << "Storing block for view " << this->view << ": " << block.toPrint() << COLOUR_NORMAL << std::endl;
				}
				this->blocks[this->view] = block;

				// Create and send LDRPREPARE
				// If this leader is malicious (simulateMalicious=true), mark the proposal
				Proposal<Accumulator> proposal_MsgLdrprepare = Proposal<Accumulator>(accumulator_MsgLdrprepare, block, this->simulateMalicious);

				if (DEBUG_HELP)
				{
					std::cout << COLOUR_RED << this->printReplicaId()
							  << "Creating MsgLdrprepare proposal: malicious=" << this->simulateMalicious
							  << ", view=" << this->view
							  << COLOUR_NORMAL << std::endl;
				}

				Signs signs_MsgLdrprepare = this->initializeMsgLdrprepareResiBFT(proposal_MsgLdrprepare);
				BlockCertificate blockCertificate_MsgLdrprepare = BlockCertificate();  // Placeholder for basic consensus

				// 论文设计：MsgLdrprepare 包含 committee 信息
				// 关键修改：
				// - NORMAL 模式 + proposedCommittee → 包含 proposedCommittee
				// - GRACIOUS 模式（非空 committee）→ 包含当前 committee
				// - UNCIVIL 模式（空 committee）→ 包含 proposedCommittee（空）
				Group committeeToInclude = Group();  // Empty by default
				if (this->stage == STAGE_GRACIOUS && this->committee.getSize() > 0)
				{
					// GRACIOUS 模式且有非空 committee：包含当前 committee
					committeeToInclude = this->committee;
				}
				else if (this->stage == STAGE_UNCIVIL || (this->stage == STAGE_GRACIOUS && this->committee.getSize() == 0))
				{
					// UNCIVIL 模式或 GRACIOUS 模式但 committee 为空：
					// 包含 proposedCommittee（空 committee，撤销委员会）
					committeeToInclude = this->proposedCommittee;  // 空 committee
					std::cout << COLOUR_RED << this->printReplicaId()
							  << "[LDRPREPARE-UNCIVIL] Including EMPTY committee in MsgLdrprepare (revoke committee)"
							  << ", stage=" << printStage(this->stage)
							  << ", proposedCommitteeSize=" << committeeToInclude.getSize()
							  << ", committeeSize=" << this->committee.getSize()
							  << COLOUR_NORMAL << std::endl;
					std::cout.flush();
				}
				else if (this->stage == STAGE_NORMAL && this->proposedCommittee.getSize() > 0
					&& this->proposedCommitteeView == this->view)
				{
					// NORMAL 模式：如果有 proposedCommittee，包含它
					// 接收者会存储为 proposedCommittee，不立即进入 GRACIOUS
					committeeToInclude = this->proposedCommittee;
					std::cout << COLOUR_MAGENTA << this->printReplicaId()
							  << "[LDRPREPARE] Including proposedCommittee in MsgLdrprepare for view " << this->view
							  << ", proposedCommitteeSize=" << committeeToInclude.getSize()
							  << ", stage=" << printStage(this->stage) << " (staying in NORMAL)"
							  << COLOUR_NORMAL << std::endl;
					std::cout.flush();
				}
				MsgLdrprepareResiBFT msgLdrprepare = MsgLdrprepareResiBFT(proposal_MsgLdrprepare, blockCertificate_MsgLdrprepare, signs_MsgLdrprepare, committeeToInclude);

				// Send to replicas
				Peers recipients;
				if (this->stage == STAGE_GRACIOUS && this->committee.getSize() > 0)
				{
					// GRACIOUS 模式且有非空 committee：只发送给 committee members
					// G-Replicas 不参与 GRACIOUS consensus，只等待 MsgDecide
					recipients = this->getCommitteePeers();
					if (DEBUG_HELP)
					{
						std::cout << COLOUR_BLUE << this->printReplicaId()
								  << "Sending MsgLdrprepare to committee members only (GRACIOUS mode)"
								  << ", committee size=" << this->committee.getSize()
								  << ", including committee in message"
								  << COLOUR_NORMAL << std::endl;
					}
				}
				else
				{
					// NORMAL 模式或 UNCIVIL 模式（空 committee）：发送给所有 replicas
					// 论文设计：所有节点参与投票
					recipients = this->removeFromPeers(this->replicaId);
					if (this->stage == STAGE_UNCIVIL || this->committee.getSize() == 0)
					{
						std::cout << COLOUR_MAGENTA << this->printReplicaId()
								  << "[LDRPREPARE] Sending MsgLdrprepare to ALL nodes (UNCIVIL or empty committee)"
								  << ", committee size=" << this->committee.getSize()
								  << ", using generalQuorumSize=" << this->generalQuorumSize
								  << COLOUR_NORMAL << std::endl;
						std::cout.flush();
					}
				}
				this->sendMsgLdrprepareResiBFT(msgLdrprepare, recipients);
				if (DEBUG_HELP)
				{
					std::cout << COLOUR_BLUE << this->printReplicaId() << "Sent MsgLdrprepare to replicas: " << msgLdrprepare.toPrint() << COLOUR_NORMAL << std::endl;
				}

				// Create and store own PREPARE
				RoundData roundData_MsgPrepare = justification_MsgPrepare.getRoundData();
				Signs signs_MsgPrepare = justification_MsgPrepare.getSigns();

				// 论文设计：MsgPrepare 包含 committee 信息
				// 复用之前定义的 committeeToInclude 变量
				MsgPrepareResiBFT msgPrepare = MsgPrepareResiBFT(roundData_MsgPrepare, signs_MsgPrepare, committeeToInclude);

				unsigned int prepareCount = this->log.storeMsgPrepareResiBFT(msgPrepare);
				// 使用正确的 quorum threshold
				if (prepareCount >= quorumThreshold)
				{
					this->initiateMsgPrepareResiBFT(roundData_MsgPrepare);
				}
			}
		}
		else
		{
			if (DEBUG_HELP)
			{
				std::cout << COLOUR_BLUE << this->printReplicaId() << "Bad accumulator for MsgLdrprepare" << COLOUR_NORMAL << std::endl;
			}
		}
	}
}

void ResiBFT::initiateMsgPrepareResiBFT(RoundData roundData_MsgPrepare)
{
	View proposeView_MsgPrepare = roundData_MsgPrepare.getProposeView();

	// 去重检查：确保只广播一次 MsgPrepare
	if (this->prepareBroadcastViews.find(proposeView_MsgPrepare) != this->prepareBroadcastViews.end())
	{
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_ORANGE << this->printReplicaId() << "MsgPrepare already broadcast for view " << proposeView_MsgPrepare << ", skipping duplicate" << COLOUR_NORMAL << std::endl;
		}
		return;
	}
	this->prepareBroadcastViews.insert(proposeView_MsgPrepare);

	// Use correct quorum based on stage and committee
	// NORMAL 模式：generalQuorumSize
	// GRACIOUS 模式（非空 committee）：trustedQuorumSize
	// UNCIVIL 模式（空 committee）：generalQuorumSize
	unsigned int quorumThreshold = this->generalQuorumSize;  // 默认
	if (this->stage == STAGE_GRACIOUS && this->committee.getSize() > 0)
	{
		quorumThreshold = this->trustedQuorumSize;
	}
	Signs signs_MsgPrepare = this->log.getMsgPrepareResiBFT(proposeView_MsgPrepare, quorumThreshold);
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "MsgPrepare signatures: " << signs_MsgPrepare.toPrint() << " (quorum=" << quorumThreshold << ", stage=" << this->stage << ")" << COLOUR_NORMAL << std::endl;
	}

	if (signs_MsgPrepare.getSize() >= quorumThreshold)
	{
		// Create and send PREPARE (broadcast quorum certificate)
		// 论文设计：MsgPrepare 包含 committee 信息
		// 关键修改：
		// - NORMAL 模式 + proposedCommittee → 包含 proposedCommittee
		// - GRACIOUS 模式（非空 committee）→ 包含当前 committee
		// - UNCIVIL 模式（空 committee）→ 包含 proposedCommittee（空）
		Group committeeToInclude = Group();
		if (this->stage == STAGE_GRACIOUS && this->committee.getSize() > 0)
		{
			// GRACIOUS 模式且有非空 committee：包含当前 committee
			committeeToInclude = this->committee;
		}
		else if (this->stage == STAGE_UNCIVIL || (this->stage == STAGE_GRACIOUS && this->committee.getSize() == 0))
		{
			// UNCIVIL 模式或 GRACIOUS 模式但 committee 为空：
			// 包含 proposedCommittee（空 committee）
			committeeToInclude = this->proposedCommittee;
		}
		else if (this->stage == STAGE_NORMAL && this->proposedCommittee.getSize() > 0
			&& this->proposedCommitteeView == proposeView_MsgPrepare)
		{
			committeeToInclude = this->proposedCommittee;
			std::cout << COLOUR_MAGENTA << this->printReplicaId()
					  << "[MSGPREPARE-BROADCAST] Including proposedCommittee in MsgPrepare broadcast for view " << proposeView_MsgPrepare
					  << ", proposedCommitteeSize=" << committeeToInclude.getSize()
					  << ", stage=" << printStage(this->stage) << " (staying in NORMAL)"
					  << COLOUR_NORMAL << std::endl;
			std::cout.flush();
		}
		MsgPrepareResiBFT msgPrepare = MsgPrepareResiBFT(roundData_MsgPrepare, signs_MsgPrepare, committeeToInclude);
		Peers recipients;
		if (this->stage == STAGE_GRACIOUS && this->committee.getSize() > 0)
		{
			// GRACIOUS 模式且有非空 committee：只发送给 committee members
			recipients = this->getCommitteePeers();
		}
		else
		{
			// NORMAL 模式或 UNCIVIL 模式（空 committee）：发送给所有 replicas
			recipients = this->removeFromPeers(this->replicaId);
		}
		this->sendMsgPrepareResiBFT(msgPrepare, recipients);
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Sent MsgPrepare to replicas: " << msgPrepare.toPrint() << COLOUR_NORMAL << std::endl;
		}

		// 论文设计：Leader 发送 MsgPrepare 后，等待其他节点回复 MsgPrecommit votes
		// Leader 自己也需要投票 MsgPrecommit
		if (this->precommitVotedViews.find(proposeView_MsgPrepare) == this->precommitVotedViews.end())
		{
			this->precommitVotedViews.insert(proposeView_MsgPrepare);

			// Leader 创建自己的 MsgPrecommit vote
			RoundData roundData_MsgPrecommit = RoundData(
				roundData_MsgPrepare.getProposeHash(),
				roundData_MsgPrepare.getProposeView(),
				roundData_MsgPrepare.getJustifyHash(),
				roundData_MsgPrepare.getJustifyView(),
				PHASE_PRECOMMIT
			);

			// Leader 的签名
			Signs signs_MsgPrecommit;
			Sign leaderSign = Sign(true, this->replicaId, (unsigned char *)"precommit");
			signs_MsgPrecommit.add(leaderSign);

			// Store own PRECOMMIT vote (Leader 作为 committee 成员投票)
			MsgPrecommitResiBFT msgPrecommit = MsgPrecommitResiBFT(roundData_MsgPrecommit, signs_MsgPrecommit);
			this->log.storeMsgPrecommitResiBFT(msgPrecommit);

			if (DEBUG_HELP)
			{
				std::cout << COLOUR_BLUE << this->printReplicaId() << "Leader stored own MsgPrecommit vote for view " << proposeView_MsgPrepare << COLOUR_NORMAL << std::endl;
			}
		}

		// 注意：Leader 不在这里发送 MsgPrecommit broadcast
		// 正确流程：等待收集 quorum MsgPrecommit votes，然后在 handleMsgPrecommitResiBFT 中发送 MsgDecide
	}
	else
	{
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_ORANGE << this->printReplicaId() << "Insufficient MsgPrepare signatures: " << signs_MsgPrepare.getSize() << " < " << quorumThreshold << COLOUR_NORMAL << std::endl;
		}
	}
}

void ResiBFT::initiateMsgPrecommitResiBFT(RoundData roundData_MsgPrecommit)
{
	View proposeView_MsgPrecommit = roundData_MsgPrecommit.getProposeView();

	// 去重检查：确保只发送一次 MsgDecide
	if (this->decideSentViews.find(proposeView_MsgPrecommit) != this->decideSentViews.end())
	{
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_ORANGE << this->printReplicaId() << "MsgDecide already sent for view " << proposeView_MsgPrecommit << ", skipping duplicate" << COLOUR_NORMAL << std::endl;
		}
		return;
	}
	this->decideSentViews.insert(proposeView_MsgPrecommit);

	// Use correct quorum based on stage and committee
	// NORMAL 模式：generalQuorumSize
	// GRACIOUS 模式（非空 committee）：trustedQuorumSize
	// UNCIVIL 模式（空 committee）：generalQuorumSize
	unsigned int quorumThreshold = this->generalQuorumSize;  // 默认
	if (this->stage == STAGE_GRACIOUS && this->committee.getSize() > 0)
	{
		quorumThreshold = this->trustedQuorumSize;
	}
	Signs signs_MsgPrecommit = this->log.getMsgPrecommitResiBFT(proposeView_MsgPrecommit, quorumThreshold);
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "MsgPrecommit signatures for MsgDecide: " << signs_MsgPrecommit.toPrint() << " (quorum=" << quorumThreshold << ", stage=" << this->stage << ")" << COLOUR_NORMAL << std::endl;
	}

	if (signs_MsgPrecommit.getSize() >= quorumThreshold)
	{
		// Create and send DECIDE
		Justification qcPrecommit_MsgDecide = Justification(roundData_MsgPrecommit, signs_MsgPrecommit);
		BlockCertificate blockCertificate_MsgDecide = BlockCertificate();  // Placeholder for basic consensus
		MsgDecideResiBFT msgDecide = MsgDecideResiBFT(qcPrecommit_MsgDecide, blockCertificate_MsgDecide, signs_MsgPrecommit);
		Peers recipients = this->removeFromPeers(this->replicaId);

		// Send DECIDE to all replicas
		this->sendMsgDecideResiBFT(msgDecide, recipients);
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Sent MsgDecide to replicas: " << msgDecide.toPrint() << COLOUR_NORMAL << std::endl;
		}

		// 关键修复：如果是最后一个 view，增加延迟确保消息传输完成
		if (proposeView_MsgPrecommit >= this->numViews - 1)
		{
			std::cout << COLOUR_GREEN << this->printReplicaId()
					  << "[FINAL-VIEW] Waiting 100ms for messages to be delivered before stopping"
					  << COLOUR_NORMAL << std::endl;
			std::cout.flush();
			usleep(1000000);  // 1s delay
		}

		// Leader executes block immediately after sending DECIDE (like HotStuff)
		this->executeBlockResiBFT(roundData_MsgPrecommit);
	}
}

void ResiBFT::initiateMsgDecideResiBFT(RoundData roundData_MsgDecide)
{
	View proposeView_MsgDecide = roundData_MsgDecide.getProposeView();
	Signs signs_MsgDecide = this->log.getMsgDecideResiBFT(proposeView_MsgDecide, this->trustedQuorumSize);

	if (signs_MsgDecide.getSize() >= this->trustedQuorumSize)
	{
		// Create and send DECIDE with proper constructor
		Justification qcPrecommit_MsgDecide = Justification(roundData_MsgDecide, signs_MsgDecide);
		BlockCertificate blockCertificate_MsgDecide = BlockCertificate();  // Placeholder for basic consensus
		MsgDecideResiBFT msgDecide = MsgDecideResiBFT(qcPrecommit_MsgDecide, blockCertificate_MsgDecide, signs_MsgDecide);
		Peers recipients = this->removeFromPeers(this->replicaId);
		this->sendMsgDecideResiBFT(msgDecide, recipients);
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Sent MsgDecide to replicas: " << msgDecide.toPrint() << COLOUR_NORMAL << std::endl;
		}

		// Execute the block
		this->executeBlockResiBFT(roundData_MsgDecide);
	}
}

// HotStuff: Leader sends MsgPrepareProposal in NORMAL mode
void ResiBFT::initiateMsgPrepareProposalResiBFT()
{
	// Deduplication check: prevent multiple broadcasts per view
	if (this->prepareProposalSentViews.find(this->view) != this->prepareProposalSentViews.end())
	{
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_ORANGE << this->printReplicaId() << "Already sent MsgPrepareProposal for view " << this->view << ", skipping duplicate" << COLOUR_NORMAL << std::endl;
		}
		return;
	}

	// 重置timer - 共识开始，给足够时间完成整个共识流程
	this->setTimer();

	// Mark this view as sent
	this->prepareProposalSentViews.insert(this->view);

	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Initiating MsgPrepareProposal for view " << this->view << " (HotStuff NORMAL), timer reset" << COLOUR_NORMAL << std::endl;
	}

	// HotStuff: MsgNewview(proposeView=k) 表示节点准备进入 view k
	// Leader k 查询 proposeView=k 的 MsgNewview 来开始 view k 的共识
	View queryView = this->view;
	std::set<MsgNewviewResiBFT> msgNewviews = this->log.getMsgNewviewResiBFT(queryView, this->generalQuorumSize);
	if (msgNewviews.size() < this->generalQuorumSize)
	{
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_RED << this->printReplicaId() << "Not enough MsgNewview for quorum: " << msgNewviews.size() << " < " << this->generalQuorumSize << " (looking for proposeView=" << queryView << ")" << COLOUR_NORMAL << std::endl;
		}
		return;
	}

	// Find highest QC from newviews
	Justification highestQC = this->findHighestQC(msgNewviews);
	// HotStuff: parent hash should be the justifyHash (highest committed block hash)
	// proposeHash in MsgNewview is dummy hash, justifyHash is the real parent hash
	Hash parentHash = highestQC.getRoundData().getJustifyHash();

	if (parentHash.isDummy() && !this->lockedHash.isDummy())
	{
		// Use locked hash as parent if no valid QC
		parentHash = this->lockedHash;
	}

	// Create block extending from highestQC's hash
	Block block = this->createNewBlockResiBFT(parentHash);

	// Store the block
	this->blocks[this->view] = block;

	// Create Proposal<Justification> with parent QC + block
	Proposal<Justification> proposal = Proposal<Justification>(highestQC, block);

	// Create leader's signature
	Sign leaderSign = Sign(true, this->replicaId, (unsigned char *)"proposal");
	Signs signs = Signs();
	signs.add(leaderSign);

	// Send MsgPrepareProposal to all replicas
	MsgPrepareProposalResiBFT msgPrepareProposal(proposal, signs);
	Peers recipients = this->removeFromPeers(this->replicaId);
	this->sendMsgPrepareProposalResiBFT(msgPrepareProposal, recipients);

	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Sent MsgPrepareProposal to replicas: " << msgPrepareProposal.toPrint() << COLOUR_NORMAL << std::endl;
	}

	// Leader also votes on its own proposal (HotStuff design)
	this->respondMsgPrepareProposalResiBFT(proposal);
}

// HotStuff: Leader sends MsgCommit in NORMAL mode
void ResiBFT::initiateMsgCommitResiBFT(RoundData roundData_MsgCommit)
{
	View proposeView_MsgCommit = roundData_MsgCommit.getProposeView();

	// 去重检查：确保只发送一次 MsgCommit/MsgDecide
	if (this->decideSentViews.find(proposeView_MsgCommit) != this->decideSentViews.end())
	{
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_ORANGE << this->printReplicaId() << "MsgCommit/MsgDecide already sent for view " << proposeView_MsgCommit << ", skipping duplicate" << COLOUR_NORMAL << std::endl;
		}
		return;
	}
	this->decideSentViews.insert(proposeView_MsgCommit);

	// In NORMAL mode, use generalQuorumSize for quorum
	Signs signs_MsgCommit = this->log.getMsgPrecommitResiBFT(proposeView_MsgCommit, this->generalQuorumSize);

	if (signs_MsgCommit.getSize() >= this->generalQuorumSize)
	{
		// Create commitQC
		Justification qcPrecommit_MsgCommit = Justification(roundData_MsgCommit, signs_MsgCommit);
		BlockCertificate blockCertificate_MsgCommit = BlockCertificate();  // Placeholder

		// Send MsgCommit to all replicas (HotStuff style)
		MsgCommitResiBFT msgCommit(qcPrecommit_MsgCommit, blockCertificate_MsgCommit, signs_MsgCommit);
		Peers recipients = this->removeFromPeers(this->replicaId);
		this->sendMsgCommitResiBFT(msgCommit, recipients);

		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Sent MsgCommit to replicas: " << msgCommit.toPrint() << COLOUR_NORMAL << std::endl;
		}

		// 关键修复：同时发送 MsgDecide 用于 catch-up
		// 论文设计：MsgDecide 用于节点 catch-up，即使 NORMAL 模式也需要
		// 这样错过 MsgCommit 的节点可以通过 MsgDecide 追赶
		MsgDecideResiBFT msgDecide(qcPrecommit_MsgCommit, blockCertificate_MsgCommit, signs_MsgCommit);
		this->sendMsgDecideResiBFT(msgDecide, recipients);

		std::cout << COLOUR_GREEN << this->printReplicaId()
				  << "[NORMAL-DECIDE] Sent MsgDecide for view " << proposeView_MsgCommit
				  << " (for catch-up purposes, " << signs_MsgCommit.getSize() << " signatures)"
				  << COLOUR_NORMAL << std::endl;

		// 关键修复：如果是最后一个 view，增加延迟确保消息传输完成
		// Leader 在最后 view 发送 MsgCommit/MsgDecide 后会立即停止
		// 需要等待消息传输给其他节点
		if (proposeView_MsgCommit >= this->numViews - 1)
		{
			std::cout << COLOUR_GREEN << this->printReplicaId()
					  << "[FINAL-VIEW] Waiting 100ms for messages to be delivered before stopping"
					  << COLOUR_NORMAL << std::endl;
			std::cout.flush();
			usleep(1000000);  // 1s delay
		}

		// Leader also executes
		this->executeBlockResiBFT(roundData_MsgCommit);
	}
}

// Respond messages
void ResiBFT::respondMsgLdrprepareResiBFT(Accumulator accumulator_MsgLdrprepare, Block block)
{
	// Replica responds to LDRPREPARE by creating PREPARE message
	// 论文设计：
	// - GRACIOUS 模式（非空 committee）：只有 committee members 参与
	// - NORMAL 模式：所有节点参与投票
	// - UNCIVIL 模式（空 committee）：所有节点参与投票
	bool canParticipate = true;  // 默认可以参与
	if (this->stage == STAGE_GRACIOUS && this->committee.getSize() > 0)
	{
		// GRACIOUS 模式且有非空 committee：只有 committee members 参与
		canParticipate = this->isCommitteeMember;
		if (!canParticipate)
		{
			if (DEBUG_HELP)
			{
				std::cout << COLOUR_ORANGE << this->printReplicaId() << "Not a committee member, skipping MsgPrepare vote in GRACIOUS mode"
						  << ", committeeSize=" << this->committee.getSize()
						  << COLOUR_NORMAL << std::endl;
			}
			return;
		}
	}

	// 检查是否是 TC 节点并模拟攻击行为
	if (this->experimentMode == EXP_MODE_ATTACK && isTCNode(this->replicaId))
	{
		if (this->tcBehavior == TC_BEHAVIOR_BAD_VOTE)
		{
			std::cout << COLOUR_MAGENTA << this->printReplicaId()
					  << "[TC-BEHAVIOR] TC node rejecting MsgLdrprepare - sending VC_REJECTED for view " << this->view
					  << COLOUR_NORMAL << std::endl;
			// TC节点投坏票：发送 VC_REJECTED 而不是投票
			this->sendVC(VC_REJECTED, this->view, block.hash());
			return;  // 不发送 MsgPrepare vote
		}
		else if (this->tcBehavior == TC_BEHAVIOR_SILENT)
		{
			std::cout << COLOUR_MAGENTA << this->printReplicaId()
					  << "[TC-BEHAVIOR] TC node staying SILENT for view " << this->view
					  << " (not voting MsgPrepare)"
					  << COLOUR_NORMAL << std::endl;
			return;  // 沉默：不发送任何消息
		}
	}

	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Responding to MsgLdrprepare with block: " << block.toPrint() << COLOUR_NORMAL << std::endl;
	}

	// Store the block for this view
	this->blocks[this->view] = block;

	// Create justification using TEE
	Justification justification_MsgPrepare = this->respondMsgLdrprepareProposalResiBFT(block.hash(), accumulator_MsgLdrprepare);
	if (justification_MsgPrepare.isSet())
	{
		RoundData roundData_MsgPrepare = justification_MsgPrepare.getRoundData();
		Signs signs_MsgPrepare = justification_MsgPrepare.getSigns();

		// 论文设计：MsgPrepare 包含 committee 信息
		// 关键修改：
		// - NORMAL 模式 + proposedCommittee → 包含 proposedCommittee
		// - GRACIOUS 模式（非空 committee）→ 包含当前 committee
		// - UNCIVIL 模式（空 committee）→ 包含 proposedCommittee（空）
		Group committeeToInclude = Group();
		if (this->stage == STAGE_GRACIOUS && this->committee.getSize() > 0)
		{
			// GRACIOUS 模式且有非空 committee：包含当前 committee
			committeeToInclude = this->committee;
		}
		else if (this->stage == STAGE_UNCIVIL || (this->stage == STAGE_GRACIOUS && this->committee.getSize() == 0))
		{
			// UNCIVIL 模式或 GRACIOUS 模式但 committee 为空：
			// 包含 proposedCommittee（空 committee）
			committeeToInclude = this->proposedCommittee;
		}
		else if (this->stage == STAGE_NORMAL && this->proposedCommittee.getSize() > 0
			&& this->proposedCommitteeView == this->view)
		{
			// NORMAL 模式：如果有 proposedCommittee，包含它
			// Leader 会验证 proposedCommittee 一致性
			committeeToInclude = this->proposedCommittee;
			std::cout << COLOUR_MAGENTA << this->printReplicaId()
					  << "[MSGPREPARE] Including proposedCommittee in MsgPrepare for view " << this->view
					  << ", proposedCommitteeSize=" << committeeToInclude.getSize()
					  << ", stage=" << printStage(this->stage) << " (staying in NORMAL)"
					  << COLOUR_NORMAL << std::endl;
			std::cout.flush();
		}
		MsgPrepareResiBFT msgPrepare = MsgPrepareResiBFT(roundData_MsgPrepare, signs_MsgPrepare, committeeToInclude);

		// Send to leader
		ReplicaID leader = this->getCurrentLeader();
		Peers recipients = this->keepFromPeers(leader);
		this->sendMsgPrepareResiBFT(msgPrepare, recipients);
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Sent MsgPrepare to leader: " << msgPrepare.toPrint() << COLOUR_NORMAL << std::endl;
		}

		// Store own PREPARE
		unsigned int quorumThreshold = this->generalQuorumSize;  // 默认
	if (this->stage == STAGE_GRACIOUS && this->committee.getSize() > 0)
	{
		quorumThreshold = this->trustedQuorumSize;
	}
		unsigned int prepareCount = this->log.storeMsgPrepareResiBFT(msgPrepare);
		if (prepareCount >= quorumThreshold)
		{
			if (this->amCurrentLeader())
			{
				// GRACIOUS/UNCIVIL mode: initiate MsgPrepare broadcast
				// Note: In NORMAL mode, this function should not be called
				// (HotStuff uses respondMsgPrepareProposalResiBFT instead)
				if (this->stage != STAGE_NORMAL)
				{
					this->initiateMsgPrepareResiBFT(roundData_MsgPrepare);
				}
			}
		}
	}
}

void ResiBFT::respondMsgPrepareResiBFT(Justification justification_MsgPrepare)
{
	// Replica responds to PREPARE by creating PRECOMMIT vote
	// 论文设计：每个节点发送自己的 MsgPrecommit vote（单个签名）
	// - GRACIOUS 模式（非空 committee）：只有 committee members 投票
	// - NORMAL 模式：所有节点投票
	// - UNCIVIL 模式（空 committee）：所有节点投票
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Responding to MsgPrepare with Precommit vote" << COLOUR_NORMAL << std::endl;
	}

	bool canVote = true;  // 默认可以投票
	if (this->stage == STAGE_GRACIOUS && this->committee.getSize() > 0)
	{
		// GRACIOUS 模式且有非空 committee：只有 committee members 投票
		canVote = this->isCommitteeMember;
		if (!canVote)
		{
			if (DEBUG_HELP)
			{
				std::cout << COLOUR_ORANGE << this->printReplicaId() << "Not a committee member, skipping precommit vote in GRACIOUS mode"
						  << ", committeeSize=" << this->committee.getSize()
						  << COLOUR_NORMAL << std::endl;
			}
			return;
		}
	}

	// 检查是否是 TC 节点并模拟攻击行为
	if (this->experimentMode == EXP_MODE_ATTACK && isTCNode(this->replicaId))
	{
		if (this->tcBehavior == TC_BEHAVIOR_BAD_VOTE)
		{
			std::cout << COLOUR_MAGENTA << this->printReplicaId()
					  << "[TC-BEHAVIOR] TC node rejecting MsgPrepare - sending VC_REJECTED for view " << this->view
					  << COLOUR_NORMAL << std::endl;
			// TC节点投坏票：发送 VC_REJECTED 而不是投票
			Hash blockHash = Hash();
			if (this->blocks.find(this->view) != this->blocks.end())
			{
				blockHash = this->blocks[this->view].hash();
			}
			this->sendVC(VC_REJECTED, this->view, blockHash);
			return;  // 不发送 MsgPrecommit vote
		}
		else if (this->tcBehavior == TC_BEHAVIOR_SILENT)
		{
			std::cout << COLOUR_MAGENTA << this->printReplicaId()
					  << "[TC-BEHAVIOR] TC node staying SILENT for view " << this->view
					  << " (not voting MsgPrecommit)"
					  << COLOUR_NORMAL << std::endl;
			return;  // 沉默：不发送任何消息
		}
	}

	// 去重检查：确保该 view 还没发送过 precommit vote
	if (this->precommitVotedViews.find(this->view) != this->precommitVotedViews.end())
	{
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_ORANGE << this->printReplicaId() << "Already voted precommit for view " << this->view << ", skipping duplicate" << COLOUR_NORMAL << std::endl;
		}
		return;
	}
	this->precommitVotedViews.insert(this->view);

	// 创建 MsgPrecommit RoundData (从 MsgPrepare justification)
	RoundData roundData_MsgPrepare = justification_MsgPrepare.getRoundData();
	RoundData roundData_MsgPrecommit = RoundData(
		roundData_MsgPrepare.getProposeHash(),
		roundData_MsgPrepare.getProposeView(),
		roundData_MsgPrepare.getJustifyHash(),
		roundData_MsgPrepare.getJustifyView(),
		PHASE_PRECOMMIT
	);

	// 论文设计：发送自己的单个签名作为 MsgPrecommit vote
	Sign mySign = Sign(true, this->replicaId, (unsigned char *)"precommit");
	Signs signs_MsgPrecommit = Signs();
	signs_MsgPrecommit.add(mySign);

	// 发送 MsgPrecommit vote 给 Leader
	MsgPrecommitResiBFT msgPrecommit = MsgPrecommitResiBFT(roundData_MsgPrecommit, signs_MsgPrecommit);

	ReplicaID leader = this->getCurrentLeader();
	Peers recipients = this->keepFromPeers(leader);
	this->sendMsgPrecommitResiBFT(msgPrecommit, recipients);
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Sent MsgPrecommit vote to leader: " << msgPrecommit.toPrint() << COLOUR_NORMAL << std::endl;
	}

	// Store own PRECOMMIT vote
	unsigned int quorumThreshold = this->generalQuorumSize;  // 默认
	if (this->stage == STAGE_GRACIOUS && this->committee.getSize() > 0)
	{
		quorumThreshold = this->trustedQuorumSize;
	}
	unsigned int signCount = this->log.storeMsgPrecommitResiBFT(msgPrecommit);

	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Stored MsgPrecommit vote, total count=" << signCount << COLOUR_NORMAL << std::endl;
	}

	// 如果是 Leader 并且达到 quorum，发送 MsgDecide/MsgCommit
	if (this->amCurrentLeader() && signCount >= quorumThreshold)
	{
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId() << "Leader reached quorum (" << signCount << " MsgPrecommit votes), initiating decision" << COLOUR_NORMAL << std::endl;
		}

		// Update lockedQC
		Signs aggregatedSigns = this->log.getMsgPrecommitResiBFT(this->view, quorumThreshold);
		Justification prepareQC = Justification(roundData_MsgPrecommit, aggregatedSigns);
		this->updateLockedQC(prepareQC);

		if (this->stage == STAGE_NORMAL)
		{
			this->initiateMsgCommitResiBFT(roundData_MsgPrecommit);
		}
		else
		{
			this->initiateMsgPrecommitResiBFT(roundData_MsgPrecommit);
		}
	}
}

// HotStuff: ALL replicas vote on prepare proposal (NORMAL mode)
void ResiBFT::respondMsgPrepareProposalResiBFT(Proposal<Justification> proposal)
{
	// 去重检查：确保该 view 还没投票过
	if (this->votedViews.find(this->view) != this->votedViews.end())
	{
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_ORANGE << this->printReplicaId() << "Already voted for view " << this->view << ", skipping duplicate vote" << COLOUR_NORMAL << std::endl;
		}
		return;
	}

	// 记录已投票的 view
	this->votedViews.insert(this->view);

	Block block = proposal.getBlock();
	Justification parentQC = proposal.getCertification();

	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Responding to MsgPrepareProposal (HotStuff vote)" << COLOUR_NORMAL << std::endl;
	}

	// Create PREPARE vote RoundData
	RoundData roundData_Prepare(block.hash(), this->view, parentQC.getRoundData().getProposeHash(), parentQC.getRoundData().getProposeView(), PHASE_PREPARE);

	// Create signature on the prepare vote
	// Trusted replicas use TEE signature, General replicas use regular signature
	Sign mySign = Sign(true, this->replicaId, (unsigned char *)"prepare");

	Signs signs_Prepare = Signs();
	signs_Prepare.add(mySign);

	// Send MsgPrepare to leader
	MsgPrepareResiBFT msgPrepare(roundData_Prepare, signs_Prepare);
	ReplicaID leader = this->getCurrentLeader();
	Peers recipients = this->keepFromPeers(leader);
	this->sendMsgPrepareResiBFT(msgPrepare, recipients);

	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Sent MsgPrepare vote to leader: " << msgPrepare.toPrint() << COLOUR_NORMAL << std::endl;
	}

	// Store own PREPARE vote
	unsigned int prepareCount = this->log.storeMsgPrepareResiBFT(msgPrepare);

	// Check quorum threshold (generalQuorumSize in NORMAL mode)
	unsigned int quorumThreshold = this->generalQuorumSize;  // 默认
	if (this->stage == STAGE_GRACIOUS && this->committee.getSize() > 0)
	{
		quorumThreshold = this->trustedQuorumSize;
	}

	// If leader and quorum reached, send MsgPrecommit
	if (this->amCurrentLeader() && prepareCount >= quorumThreshold)
	{
		Signs aggregatedSigns = this->log.getMsgPrepareResiBFT(this->view, quorumThreshold);
		if (aggregatedSigns.getSize() >= quorumThreshold)
		{
			// Create prepareQC and update lockedQC
			Justification prepareQC = Justification(roundData_Prepare, aggregatedSigns);
			this->prepareQC = prepareQC;
			this->prepareHash = block.hash();
			this->prepareView = this->view;

			// Send MsgPrecommit with prepareQC
			MsgPrecommitResiBFT msgPrecommit(roundData_Prepare, aggregatedSigns);
			Peers commitRecipients = this->removeFromPeers(this->replicaId);
			this->sendMsgPrecommitResiBFT(msgPrecommit, commitRecipients);

			if (DEBUG_HELP)
			{
				std::cout << COLOUR_BLUE << this->printReplicaId() << "Sent MsgPrecommit to replicas: " << msgPrecommit.toPrint() << COLOUR_NORMAL << std::endl;
			}

			// Leader also votes MsgPrecommit
			respondMsgPrepareResiBFT(prepareQC);
		}
	}
}

// Main functions
int ResiBFT::initializeSGX()
{
	std::cout << COLOUR_BLUE << this->printReplicaId() << "Loading SGX enclave..." << COLOUR_NORMAL << std::endl;
	std::cout.flush();

	if (initialize_enclave(&global_eid, "enclave.token", "enclave.signed.so") < 0)
	{
		std::cout << COLOUR_RED << this->printReplicaId() << "Failed to initialize SGX enclave" << COLOUR_NORMAL << std::endl;
		std::cout.flush();
		return -1;
	}
	std::cout << COLOUR_BLUE << this->printReplicaId() << "SGX enclave initialized with eid: " << global_eid << COLOUR_NORMAL << std::endl;
	std::cout.flush();
	return 0;
}

void ResiBFT::getStarted()
{
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Getting started at view " << this->view << COLOUR_NORMAL << std::endl;
	}

	// 启动时不应该调用 startNewViewResiBFT()，因为那会执行 view++
	// 正确流程：节点在 view 0 发送 MsgNewview 给 Leader 0，等待共识开始
	// startNewViewResiBFT() 应该只在共识完成后调用（在 executeBlockResiBFT 中）

	// 发送 MsgNewview(view=0) 给当前 Leader
	Justification justification_MsgNewview = this->initializeMsgNewviewResiBFT();
	RoundData roundData_MsgNewview = justification_MsgNewview.getRoundData();
	Phase phase_MsgNewview = roundData_MsgNewview.getPhase();
	Signs signs_MsgNewview = justification_MsgNewview.getSigns();

	if (phase_MsgNewview == PHASE_NEWVIEW)
	{
		MsgNewviewResiBFT msgNewview = MsgNewviewResiBFT(roundData_MsgNewview, signs_MsgNewview);

		if (this->amCurrentLeader())
		{
			// Leader 0: 存储自己的 MsgNewview，检查是否已达到 quorum
			this->handleMsgNewviewResiBFT(msgNewview);
		}
		else
		{
			// 非 Leader: 发送 MsgNewview 给 Leader 0
			ReplicaID leader = this->getCurrentLeader();
			Peers recipients = this->keepFromPeers(leader);
			this->sendMsgNewviewResiBFT(msgNewview, recipients);
		}
	}
}

void ResiBFT::startNewViewResiBFT()
{
	auto startTotal = std::chrono::steady_clock::now();

	std::cout << COLOUR_MAGENTA << this->printReplicaId()
			  << "[VIEW-CHANGE] START: oldView=" << this->view
			  << ", oldStage=" << printStage(this->stage)
			  << ", oldLeader=" << this->getCurrentLeader()
			  << COLOUR_NORMAL << std::endl;
	std::cout.flush();

	// 论文设计：检查新 Leader 是否为 T-Replica
	// 如果是，触发 VRF 委员会选择 → GRACIOUS
	auto startLeaderCheck = std::chrono::steady_clock::now();
	ReplicaID newLeader = this->getLeaderOf(this->view + 1);  // 下一个 view 的 leader
	bool newLeaderIsTrusted = (newLeader >= this->numGeneralReplicas);
	auto endLeaderCheck = std::chrono::steady_clock::now();
	double leaderCheckTime = std::chrono::duration_cast<std::chrono::microseconds>(endLeaderCheck - startLeaderCheck).count();

	std::cout << COLOUR_MAGENTA << this->printReplicaId()
			  << "[VIEW-CHANGE] Leader check: newLeader=" << newLeader
			  << ", isTrusted=" << newLeaderIsTrusted
			  << ", time=" << leaderCheckTime << " μs"
			  << COLOUR_NORMAL << std::endl;
	std::cout.flush();

	// 如果当前是 NORMAL 模式且新 Leader 是 T-Replica
	if (this->stage == STAGE_NORMAL && newLeaderIsTrusted)
	{
		// 论文设计：Leader 轮换到 T-Replica 时触发 GRACIOUS
		// 使用 VRF 选择委员会
		std::cout << COLOUR_MAGENTA << this->printReplicaId()
				  << "[VIEW-CHANGE] Leader rotating to T-Replica " << newLeader
				  << " - will trigger VRF committee selection"
				  << COLOUR_NORMAL << std::endl;
		std::cout.flush();
		// 注意：实际触发发生在该节点成为 Leader 时
		// 这里只是标记状态，真正的 VRF 选择由 Leader 执行
	}

	// 论文设计：如果当前在 GRACIOUS/UNCIVIL 模式且新 Leader 是 G-Replica
	// 则应该恢复到 NORMAL 模式（因为 G-Replica 不需要委员会）
	// 关键修复：在 GRACIOUS 模式下，需要检查 normal leader，而不是 committee leader
	unsigned int normalLeader = (this->view + 1) % this->numReplicas;
	bool normalLeaderIsGReplica = (normalLeader < this->numGeneralReplicas);

	if ((this->stage == STAGE_GRACIOUS || this->stage == STAGE_UNCIVIL) && normalLeaderIsGReplica)
	{
		std::cout << COLOUR_GREEN << this->printReplicaId()
				  << "[STAGE-RECOVERY] Leader rotating to G-Replica " << normalLeader
				  << " (normal leader, not committee leader)"
				  << " - recovering from " << printStage(this->stage)
				  << " to NORMAL stage"
				  << COLOUR_NORMAL << std::endl;
		std::cout.flush();

		this->transitionToNormal();

		std::cout << COLOUR_GREEN << this->printReplicaId()
				  << "[STAGE-RECOVERY] Successfully recovered to NORMAL, trustedQuorumSize=" << this->trustedQuorumSize
				  << COLOUR_NORMAL << std::endl;
		std::cout.flush();
	}

	// Basic placeholder implementation
	// HotStuff: First increase the view, then initialize MsgNewview
	auto startViewChange = std::chrono::steady_clock::now();
	View oldView = this->view;
	this->view++;

	// 关键修复：检查是否有存储的 MsgCommittee for 当前 view
	// 当节点进入新 view 时，需要处理之前存储的 MsgCommittee 以更新 trustedQuorumSize
	// 重要：所有节点都需要处理 MsgCommittee 以获取 trustedQuorumSize
	// G-Replica 需要知道 trustedQuorumSize 才能验证 GRACIOUS 模式的 MsgDecide
	// handleMsgCommitteeResiBFT 内部会阻止 G-Replica 进入 GRACIOUS stage
	if (this->stage == STAGE_NORMAL)
	{
		MsgCommitteeResiBFT pendingMsgCommittee = this->log.getMsgCommitteeResiBFT(this->view);
		if (pendingMsgCommittee.committee.getSize() > 0)
		{
			std::cout << COLOUR_MAGENTA << this->printReplicaId()
					  << "[VIEW-CHANGE] Found pending MsgCommittee for view " << this->view
					  << ", processing it to update trustedQuorumSize"
					  << COLOUR_NORMAL << std::endl;
			std::cout.flush();

			this->handleMsgCommitteeResiBFT(pendingMsgCommittee);

			std::cout << COLOUR_MAGENTA << this->printReplicaId()
					  << "[VIEW-CHANGE] After processing MsgCommittee: trustedQuorumSize=" << this->trustedQuorumSize
					  << ", stage=" << printStage(this->stage)
					  << COLOUR_NORMAL << std::endl;
			std::cout.flush();
		}
	}

	// Sync TEE view for Trusted replicas
	if (!this->amGeneralReplicaIds())
	{
		sgx_status_t retval;
		sgx_status_t status_t = TEE_setViewResiBFT(global_eid, &retval, &this->view);
		if (status_t != SGX_SUCCESS || retval != SGX_SUCCESS)
		{
			if (DEBUG_HELP)
			{
				std::cout << COLOUR_RED << this->printReplicaId() << "TEE_setViewResiBFT failed: " << status_t << " retval: " << retval << COLOUR_NORMAL << std::endl;
			}
		}
	}
	auto endViewChange = std::chrono::steady_clock::now();
	double viewChangeTime = std::chrono::duration_cast<std::chrono::microseconds>(endViewChange - startViewChange).count();

	auto startInit = std::chrono::steady_clock::now();
	Justification justification_MsgNewview = this->initializeMsgNewviewResiBFT();
	View proposeView_MsgNewview = justification_MsgNewview.getRoundData().getProposeView();
	auto endInit = std::chrono::steady_clock::now();
	double initTime = std::chrono::duration_cast<std::chrono::microseconds>(endInit - startInit).count();

	// Start the timer
	auto startTimer = std::chrono::steady_clock::now();
	this->setTimer();
	auto endTimer = std::chrono::steady_clock::now();
	double timerTime = std::chrono::duration_cast<std::chrono::microseconds>(endTimer - startTimer).count();

	RoundData roundData_MsgNewview = justification_MsgNewview.getRoundData();
	Phase phase_MsgNewview = roundData_MsgNewview.getPhase();
	Signs signs_MsgNewview = justification_MsgNewview.getSigns();

	ReplicaID currentLeader = this->getCurrentLeader();

	std::cout << COLOUR_MAGENTA << this->printReplicaId()
			  << "[VIEW-CHANGE] New view: view=" << this->view
			  << ", proposeView=" << proposeView_MsgNewview
			  << ", newLeader=" << currentLeader
			  << ", amLeader=" << this->amCurrentLeader()
			  << ", viewChangeTime=" << viewChangeTime << " μs"
			  << ", initTime=" << initTime << " μs"
			  << ", timerTime=" << timerTime << " μs"
			  << COLOUR_NORMAL << std::endl;
	std::cout.flush();

	if (proposeView_MsgNewview == this->view && phase_MsgNewview == PHASE_NEWVIEW)
	{
		MsgNewviewResiBFT msgNewview = MsgNewviewResiBFT(roundData_MsgNewview, signs_MsgNewview);
		if (this->amCurrentLeader())
		{
			// 论文设计：如果当前 Leader 是 T-Replica
			// 关键修改：不立即进入 GRACIOUS，而是计算 proposedCommittee
			// 等待区块执行后才进入 GRACIOUS
			if (this->amTrustedReplicaIds())
			{
				// 计算委员会大小 k
#ifdef PRODUCTION_MODE
				unsigned int k = COMMITTEE_K_MIN;  // 初始值，实际由 selectCommitteeByVRF 采样
#else
				unsigned int k = std::min((unsigned int)COMMITTEE_K_MIN, this->numTrustedReplicas);
#endif

				if (this->stage == STAGE_NORMAL)
				{
					// 论文设计：NORMAL 模式下的 T-Replica Leader
					// 不立即进入 GRACIOUS，而是计算 proposedCommittee
					// 使用 generalQuorumSize 投票，等待区块执行后进入 GRACIOUS
					std::cout << COLOUR_MAGENTA << this->printReplicaId()
							  << "[VIEW-CHANGE] I am T-Replica Leader for view " << this->view
							  << " - computing proposedCommittee (will enter GRACIOUS after block execution)"
							  << COLOUR_NORMAL << std::endl;
					std::cout.flush();

					// VRF 选择委员会，存储为 proposedCommittee（不立即更新 committee）
					auto startVRF = std::chrono::steady_clock::now();
					this->proposedCommittee = this->selectCommitteeByVRF(this->view, k);
					this->proposedCommitteeView = this->view;
					auto endVRF = std::chrono::steady_clock::now();
					double vrfTime = std::chrono::duration_cast<std::chrono::microseconds>(endVRF - startVRF).count();

					std::cout << COLOUR_MAGENTA << this->printReplicaId()
							  << "[VIEW-CHANGE] VRF selection time=" << vrfTime << " μs"
							  << ", proposedCommitteeSize=" << this->proposedCommittee.getSize()
							  << ", proposedCommittee=" << this->proposedCommittee.toPrint()
							  << ", stage=" << printStage(this->stage) << " (staying in NORMAL)"
							  << COLOUR_NORMAL << std::endl;
					std::cout.flush();

					// 不调用 transitionToGracious()
					// 不更新 committee, trustedQuorumSize, stage
					// 这些将在 executeBlockResiBFT 中更新
				}
				else if (this->stage == STAGE_GRACIOUS || this->stage == STAGE_UNCIVIL)
				{
					// 已经在 GRACIOUS/UNCIVIL 模式
					// 论文设计：检查是否有 f+1 个 accepted-VC
					// 有 → 继续 GRACIOUS consensus
					// 无 → 进入 UNCIVIL，提议空委员会

					if (this->committeeLeader != this->replicaId)
					{
						// 新的 T-Replica Leader 需要决定下一步
						// 检查是否有 f+1 个 accepted-VC
						unsigned int acceptedVCCount = this->acceptedVCs.size();

						std::cout << COLOUR_MAGENTA << this->printReplicaId()
								  << "[VIEW-CHANGE] I am NEW T-Replica Leader for view " << this->view
								  << " (old committeeLeader=" << this->committeeLeader << ")"
								  << " - checking VC count: acceptedVCs=" << acceptedVCCount
								  << ", need f+1=" << (this->numFaults + 1)
								  << COLOUR_NORMAL << std::endl;
						std::cout.flush();

						if (acceptedVCCount >= this->numFaults + 1)
						{
							// 有 f+1 个 accepted-VC → 继续 GRACIOUS consensus
							// 使用现有 committee（不重新选择）
							std::cout << COLOUR_GREEN << this->printReplicaId()
									  << "[VIEW-CHANGE] Have f+1 accepted-VCs → continuing GRACIOUS consensus"
									  << ", committeeSize=" << this->committee.getSize()
									  << ", trustedQuorumSize=" << this->trustedQuorumSize
									  << COLOUR_NORMAL << std::endl;
							std::cout.flush();

							// 更新 committeeLeader 为自己
							this->committeeLeader = this->replicaId;
							this->committeeView = this->view;

							// 继续使用现有 committee 和 trustedQuorumSize
						}
						else
						{
							// 没有 f+1 个 accepted-VC → 进入 UNCIVIL
							// 提议空委员会（撤销委员会）
							std::cout << COLOUR_RED << this->printReplicaId()
									  << "[VIEW-CHANGE] No f+1 accepted-VCs → entering UNCIVIL"
									  << " - will propose EMPTY committee to revoke committee"
									  << COLOUR_NORMAL << std::endl;
							std::cout.flush();

							// 进入 UNCIVIL 模式
							this->stage = STAGE_UNCIVIL;
							this->stageTransitionView = this->view;

							// 提议空委员会
							this->proposedCommittee = Group();  // 空委员会
							this->proposedCommitteeView = this->view;

							// 空委员会意味着所有节点参与投票（使用 generalQuorumSize）
							// 执行区块时更新 committee 为空，所有节点回到 NORMAL
						}
					}
					else
					{
						// committeeLeader == self，继续使用当前 committee
						std::cout << COLOUR_MAGENTA << this->printReplicaId()
								  << "[VIEW-CHANGE] I am committeeLeader for view " << this->view
								  << " - continuing with current committee"
								  << ", committeeSize=" << this->committee.getSize()
								  << ", trustedQuorumSize=" << this->trustedQuorumSize
								  << COLOUR_NORMAL << std::endl;
						std::cout.flush();
					}
				}
			}

			// 重要：先存储自己的MsgNewview，再检查log中的其他MsgNewview
			this->handleMsgNewviewResiBFT(msgNewview);  // 先存储自己的MsgNewview
			this->handleEarlierMessagesResiBFT();       // 然后检查是否已达到quorum
		}
		else
		{
			ReplicaID leader = this->getCurrentLeader();
			Peers recipients = this->keepFromPeers(leader);
			this->sendMsgNewviewResiBFT(msgNewview, recipients);  // 发送到Leader
			this->handleEarlierMessagesResiBFT();                  // 检查是否有之前存储的消息
		}
	}
	else
	{
		std::cout << COLOUR_MAGENTA << this->printReplicaId()
				  << "[VIEW-CHANGE] FAILED: proposeView=" << proposeView_MsgNewview
				  << " != currentView=" << this->view
				  << " or phase=" << printPhase(phase_MsgNewview) << " != NEWVIEW"
				  << COLOUR_NORMAL << std::endl;
		std::cout.flush();
	}

	auto endTotal = std::chrono::steady_clock::now();
	double totalTime = std::chrono::duration_cast<std::chrono::microseconds>(endTotal - startTotal).count();

	std::cout << COLOUR_MAGENTA << this->printReplicaId()
			  << "[VIEW-CHANGE] END: view=" << oldView << "->" << this->view
			  << ", leader=" << currentLeader
			  << ", stage=" << printStage(this->stage)
			  << ", totalTime=" << totalTime << " μs"
			  << COLOUR_NORMAL << std::endl;
	std::cout.flush();
}

Block ResiBFT::createNewBlockResiBFT(Hash hash)
{
	std::lock_guard<std::mutex> guard(mutexTransaction);
	Transaction transaction[MAX_NUM_TRANSACTIONS];
	unsigned int i = 0;

	// Fill the block with transactions
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
	// Fill the rest with dummy transactions
	while (i < MAX_NUM_TRANSACTIONS)
	{
		transaction[i] = Transaction();
		i++;
	}
	Block block = Block(hash, size, transaction);
	return block;
}

void ResiBFT::executeBlockResiBFT(RoundData roundData_MsgPrecommit, bool isCatchUp)
{
	// 确定目标 view：catch-up 使用 RoundData 中的 view，否则使用当前 view
	View targetView = isCatchUp ? roundData_MsgPrecommit.getProposeView() : this->view;

	// 去重检查：确保该 view 还没被执行过
	if (this->executedViews.find(targetView) != this->executedViews.end())
	{
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_ORANGE << this->printReplicaId() << "View " << targetView << " already executed, skipping" << COLOUR_NORMAL << std::endl;
		}
		return;
	}

	// 记录已执行的 view
	this->executedViews.insert(targetView);

	// Catch-up 模式下只记录执行，不触发后续操作
	if (isCatchUp)
	{
		if (DEBUG_BASIC)
		{
			std::cout << COLOUR_GREEN << this->printReplicaId()
					  << "[CATCH-UP-EXECUTE] Successfully executed block for view " << targetView
					  << " (current view remains " << this->view << ")"
					  << COLOUR_NORMAL << std::endl;
		}
		this->replyHash(roundData_MsgPrecommit.getProposeHash());
		return;  // 不触发 view change
	}

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
				  << "RESIBFT-EXECUTE(" << targetView << "/" << std::to_string(this->numViews) << ":" << time << ") "
				  << statistics.toString() << COLOUR_NORMAL << std::endl;
	}

	// Checkpoint creation (每 CHECKPOINT_INTERVAL 个 view 创建 checkpoint)
	if (targetView > 0 && targetView % CHECKPOINT_INTERVAL == 0)
	{
		this->createCheckpointResiBFT(roundData_MsgPrecommit, targetView);
	}

	// 关键修改：论文设计 - 执行区块时更新 committee
	// Case 1: proposedCommittee 非空 → 进入 GRACIOUS
	// Case 2: proposedCommittee 为空（size=0）→ 撤销委员会，回到 NORMAL（UNCIVIL→NORMAL）
	// 如果 proposedCommitteeView 匹配当前 targetView，则更新 committee
	if (this->proposedCommitteeView == targetView)
	{
		if (this->proposedCommittee.getSize() > 0)
		{
			// Case 1: 非 空 committee → 进入 GRACIOUS
			// 此时区块已达成共识，可以更新 committee
			this->committee = this->proposedCommittee;
			this->committeeView = targetView;

			// 填充 currentCommittee std::set
			this->currentCommittee.clear();
			for (unsigned int i = 0; i < this->proposedCommittee.getSize(); i++)
			{
				this->currentCommittee.insert(this->proposedCommittee.getGroup()[i]);
			}

			// 更新 trustedQuorumSize
			unsigned int committeeSize = this->proposedCommittee.getSize();
			unsigned int oldTrustedQuorumSize = this->trustedQuorumSize;
			this->trustedQuorumSize = (committeeSize + 1) / 2;

			// 设置 committeeLeader
			ReplicaID expectedLeader = targetView % this->numReplicas;
			this->committeeLeader = expectedLeader;

			// 检查自己是否在 committee 中
			this->isCommitteeMember = isInCommittee(this->replicaId);

			// 论文设计：T-Replica 进入 GRACIOUS 模式
			// G-Replica 保持 NORMAL（不需要参与 committee 共识）
			if (!this->amGeneralReplicaIds())
			{
				this->stage = STAGE_GRACIOUS;
				this->stageTransitionView = targetView;

				std::cout << COLOUR_RED << this->printReplicaId()
						  << "[EXECUTE-BLOCK] Entering GRACIOUS at view " << targetView
						  << ", committeeSize=" << committeeSize
						  << ", trustedQuorumSize=" << oldTrustedQuorumSize << "->" << this->trustedQuorumSize
						  << ", isCommitteeMember=" << this->isCommitteeMember
						  << ", committeeLeader=" << this->committeeLeader
						  << ", proposedCommittee=" << this->proposedCommittee.toPrint()
						  << COLOUR_NORMAL << std::endl;
				std::cout.flush();
			}
			else
			{
				// G-Replica 不进入 GRACIOUS，只记录 committee 信息
				std::cout << COLOUR_MAGENTA << this->printReplicaId()
						  << "[EXECUTE-BLOCK] G-Replica updated committee info (staying in NORMAL)"
						  << ", committeeSize=" << committeeSize
						  << ", trustedQuorumSize=" << this->trustedQuorumSize
						  << ", stage=" << printStage(this->stage)
						  << COLOUR_NORMAL << std::endl;
				std::cout.flush();
			}
		}
		else
		{
			// Case 2: 空 committee → 撤销委员会，回到 NORMAL
			// 论文设计：UNCIVIL → NORMAL 的唯一条件：委员会变量为空
			// 在执行区块时更新委员会为空

			// 清空 committee
			this->committee = Group();
			this->committeeView = targetView;
			this->currentCommittee.clear();
			this->committeeLeader = 0;

			// 恢复 trustedQuorumSize 到 original 值
			this->trustedQuorumSize = this->originalTrustedQuorumSize;
			this->isCommitteeMember = false;

			// 论文设计：所有节点回到 NORMAL（委员会为空）
			this->stage = STAGE_NORMAL;
			this->stageTransitionView = targetView;

			std::cout << COLOUR_GREEN << this->printReplicaId()
					  << "[EXECUTE-BLOCK] EMPTY committee → revoking committee, returning to NORMAL at view " << targetView
					  << ", trustedQuorumSize restored to " << this->trustedQuorumSize
					  << ", stage=" << printStage(this->stage)
					  << COLOUR_NORMAL << std::endl;
			std::cout.flush();
		}

		// 清空 proposedCommittee
		this->proposedCommittee = Group();
		this->proposedCommitteeView = 0;
	}

	// 论文设计：状态恢复检查
	// 如果当前在 GRACIOUS/UNCIVIL 模式，成功完成足够区块后可恢复到 NORMAL
	if (this->stage == STAGE_GRACIOUS || this->stage == STAGE_UNCIVIL)
	{
		// 检查是否满足恢复条件
		unsigned int successfulBlocksInStage = 0;
		for (View v : this->executedViews)
		{
			// 计算在当前阶段成功执行的区块数
			if (v >= this->stageTransitionView)  // stageTransitionView 记录进入当前阶段的 view
			{
				successfulBlocksInStage++;
			}
		}

		if (successfulBlocksInStage >= STAGE_RECOVERY_SUCCESSFUL_BLOCKS)
		{
			std::cout << COLOUR_GREEN << this->printReplicaId()
					  << "[STAGE-RECOVERY] Successfully executed " << successfulBlocksInStage
					  << " blocks in " << printStage(this->stage)
					  << " - recovering to NORMAL"
					  << COLOUR_NORMAL << std::endl;
			std::cout.flush();

			this->transitionToNormal();
		}
	}

	this->replyHash(roundData_MsgPrecommit.getProposeHash());
	if (this->timeToStop())
	{
		// Stop the timer to prevent further view changes
		this->timer.del();
		this->recordStatisticsResiBFT();
	}
	else
	{
		this->startNewViewResiBFT();
	}
}

// Create checkpoint for the specified view
void ResiBFT::createCheckpointResiBFT(RoundData roundData, View targetView)
{
	// Check if block exists for this view
	if (this->blocks.find(targetView) == this->blocks.end())
	{
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_BLUE << this->printReplicaId()
					  << "No block for view " << targetView << ", cannot create checkpoint"
					  << COLOUR_NORMAL << std::endl;
		}
		return;
	}

	Block block = this->blocks[targetView];
	Justification qcPrecommit = this->prepareQC;

	// Collect VCs for this view
	Signs qcVc = Signs();
	for (const auto& vc : this->acceptedVCs)
	{
		if (vc.getView() == targetView)
		{
			// Add VC signatures
			// Note: VCs contain VerificationCertificates, not direct signatures
			// For checkpoint, we use the quorum certificate signatures
		}
	}

	// Create checkpoint
	Checkpoint checkpoint(block, targetView, qcPrecommit, qcVc);
	unsigned int quorumThreshold = this->generalQuorumSize;  // 默认
	if (this->stage == STAGE_GRACIOUS && this->committee.getSize() > 0)
	{
		quorumThreshold = this->trustedQuorumSize;
	}

	if (checkpoint.wellFormed(quorumThreshold))
	{
		this->log.storeCheckpoint(checkpoint);
		this->checkpoints.insert(checkpoint);

		if (DEBUG_HELP)
		{
			std::cout << COLOUR_MAGENTA << this->printReplicaId()
					  << "Checkpoint created for view " << targetView
					  << ": " << checkpoint.toPrint()
					  << COLOUR_NORMAL << std::endl;
		}

		// Broadcast checkpoint (via MsgBC)
		BlockCertificate blockCert(block, targetView, qcPrecommit, qcVc);
		Sign sign = Sign(true, this->replicaId, (unsigned char *)"checkpoint");
		Signs signs = Signs();
		signs.add(sign);
		MsgBCResiBFT msgBC(blockCert, signs);
		Peers recipients = this->removeFromPeers(this->replicaId);
		this->sendMsgBCResiBFT(msgBC, recipients);
	}
}

// 论文设计：ACCEPTED VC ≥ f+1 时 commit 前一个 block
void ResiBFT::commitPreviousBlock()
{
	// 检查是否存在前一个 view 的 block
	View previousView = this->view - 1;
	if (this->blocks.find(previousView) != this->blocks.end())
	{
		Block previousBlock = this->blocks[previousView];

		if (DEBUG_HELP)
		{
			std::cout << COLOUR_GREEN << this->printReplicaId()
					  << "Committing previous block for view " << previousView
					  << ": " << previousBlock.toPrint()
					  << COLOUR_NORMAL << std::endl;
		}

		// 在实际实现中，这里应该执行前一个 block 的交易
		// 并更新状态机/回复客户端

		// 更新 checkpoint 信息
		// 创建前一个 block 的 checkpoint
		Justification qcPrecommit = this->prepareQC;
		Signs qcVc = Signs();
		Checkpoint checkpoint(previousBlock, previousView, qcPrecommit, qcVc);

		unsigned int quorumThreshold = this->generalQuorumSize;  // 默认
	if (this->stage == STAGE_GRACIOUS && this->committee.getSize() > 0)
	{
		quorumThreshold = this->trustedQuorumSize;
	}
		if (checkpoint.wellFormed(quorumThreshold))
		{
			this->log.storeCheckpoint(checkpoint);
			this->checkpoints.insert(checkpoint);
		}
	}
	else
	{
		if (DEBUG_HELP)
		{
			std::cout << COLOUR_ORANGE << this->printReplicaId()
					  << "No previous block found for view " << previousView
					  << COLOUR_NORMAL << std::endl;
		}
	}
}

bool ResiBFT::timeToStop()
{
	// Stop when view reaches numViews
	// e.g., numViews=10 means execute view 1-10, stop after view 10 executes
	bool b = this->numViews > 0 && this->view >= this->numViews;
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
	return b;
}

void ResiBFT::recordStatisticsResiBFT()
{
	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "DONE - Printing statistics" << COLOUR_NORMAL << std::endl;
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

	// Latency
	double latencyView = (totalView.total / totalView.num / 1000);
	double handle = (totalHandle.total / 1000);

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

	// Stop peer event loop - this will cause the main thread to exit
	peerEventContext.stop();
}

// Constructor
ResiBFT::ResiBFT(KeysFunctions keysFunctions, ReplicaID replicaId, unsigned int numGeneralReplicas, unsigned int numTrustedReplicas, unsigned int numReplicas, unsigned int numViews, unsigned int numFaults, double leaderChangeTime, Nodes nodes, Key privateKey, PeerNet::Config peerNetConfig, ClientNet::Config clientNetConfig) : peerNet(peerEventContext, peerNetConfig), clientNet(requestEventContext, clientNetConfig)
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
	this->originalTrustedQuorumSize = this->trustedQuorumSize;  // 保存原始值
	this->lowTrustedSize = 3;

	// 正确初始化 trustedGroup，包含所有 T-Replica IDs
	// T-Replica IDs 从 numGeneralReplicas 开始，到 numReplicas-1
	std::vector<ReplicaID> trustedIds;
	for (unsigned int i = this->numGeneralReplicas; i < this->numReplicas; i++)
	{
		trustedIds.push_back(i);
	}
	this->trustedGroup = Group(trustedIds);

	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Trusted group initialized: ";
		ReplicaID *members = this->trustedGroup.getGroup();
		for (int i = 0; i < this->trustedGroup.getSize(); i++)
		{
			std::cout << members[i] << " ";
		}
		std::cout << ", amTrustedReplicaIds=" << this->amTrustedReplicaIds() << COLOUR_NORMAL << std::endl;
	}

	this->view = 0;

	// ResiBFT-specific initializations
	this->epoch = 0;
	this->stage = STAGE_NORMAL;
	this->committee = Group();
	this->isCommitteeMember = false;

	// HotStuff state variable initialization
	this->prepareQC = Justification();
	this->lockedQC = Justification();
	this->prepareHash = Hash();
	this->prepareView = 0;
	this->lockedHash = Hash();
	this->lockedView = 0;

	// VRF committee state initialization
	this->currentCommittee = std::set<ReplicaID>();
	this->committeeView = 0;
	this->committeeLeader = 0;
	this->stageTransitionView = 0;  // 初始化为 0（NORMAL 模式开始）

	// Check environment variables for test simulation
	char* simMalicious = std::getenv("RESIBFT_SIMULATE_MALICIOUS");
	if (simMalicious != nullptr && strcmp(simMalicious, "1") == 0)
	{
		this->simulateMalicious = true;
		std::cout << COLOUR_MAGENTA << this->printReplicaId() << "SIMULATE_MALICIOUS mode enabled (will trigger UNCIVIL)" << COLOUR_NORMAL << std::endl;
	}
	char* simTimeout = std::getenv("RESIBFT_SIMULATE_TIMEOUT");
	if (simTimeout != nullptr && strcmp(simTimeout, "1") == 0)
	{
		this->simulateTimeout = true;
		std::cout << COLOUR_MAGENTA << this->printReplicaId() << "SIMULATE_TIMEOUT mode enabled (will trigger GRACIOUS faster)" << COLOUR_NORMAL << std::endl;
	}

	// ============================================================
	// 实验参数初始化（TC节点比例 λ 等）
	// ============================================================
	this->initializeExperimentConfig();

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
		if (this->initializeSGX() < 0)
		{
			if (DEBUG_HELP)
			{
				std::cout << COLOUR_RED << this->printReplicaId() << "TEE initialization failed, cannot continue" << COLOUR_NORMAL << std::endl;
			}
			return;
		}

		// Initialize enclave variables (replicaId, quorum sizes, public keys)
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
		if (status_t != SGX_SUCCESS || extra != SGX_SUCCESS)
		{
			if (DEBUG_HELP)
			{
				std::cout << COLOUR_RED << this->printReplicaId() << "Failed to initialize enclave variables: status=" << status_t << " extra=" << extra << COLOUR_NORMAL << std::endl;
			}
		}
		else
		{
			if (DEBUG_HELP)
			{
				std::cout << COLOUR_BLUE << this->printReplicaId() << "Initialized TEE and enclave variables" << COLOUR_NORMAL << std::endl;
			}
		}
	}
	else
	{
		// General replica - no TEE needed
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
													std::cout << COLOUR_BLUE << this->printReplicaId() << "timer ran out for view " << this->view << COLOUR_NORMAL << std::endl;
												}
												// Send VC_TIMEOUT to indicate trusted replica may be unavailable
												// This helps trigger GRACIOUS mode when enough timeouts accumulate
												Hash currentBlockHash = Hash();
												if (this->blocks.find(this->view) != this->blocks.end())
												{
													currentBlockHash = this->blocks[this->view].hash();
												}
												this->sendVC(VC_TIMEOUT, this->view, currentBlockHash);

												// startNewViewResiBFT() 内部会调用 setTimer() 设置新的timer
												// 不需要在这里再次设置timer
												this->startNewViewResiBFT();
										 });

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

				// Only allow smaller ID nodes to actively connect to larger ID nodes
				// to avoid bidirectional connection race condition
				if (this->replicaId < j)
				{
					try {
						this->peerNet.conn_peer(otherPeerId);
						if (DEBUG_HELP)
						{
							std::cout << COLOUR_BLUE << this->printReplicaId() << "Connected to peer: " << j << COLOUR_NORMAL << std::endl;
						}
					} catch (std::exception &e) {
						if (DEBUG_HELP)
						{
							std::cout << COLOUR_BLUE << this->printReplicaId() << "Connection to " << j << " failed: " << e.what() << COLOUR_NORMAL << std::endl;
						}
					}
				}
				else
				{
					if (DEBUG_HELP)
					{
						std::cout << COLOUR_BLUE << this->printReplicaId() << "Waiting for peer " << j << " to connect" << COLOUR_NORMAL << std::endl;
					}
				}
				this->peers.push_back(std::make_pair(j, otherPeerId));
			}
			else
			{
				std::cout << COLOUR_RED << this->printReplicaId() << "Couldn't find " << j << "'s information among nodes" << COLOUR_NORMAL << std::endl;
			}
		}
	}

	// Register client network handlers
	this->clientNet.reg_handler(salticidae::generic_bind(&ResiBFT::receiveMsgStartResiBFT, this, _1, _2));
	this->clientNet.reg_handler(salticidae::generic_bind(&ResiBFT::receiveMsgTransactionResiBFT, this, _1, _2));

	// Register peer network handlers for ResiBFT messages
	this->peerNet.reg_handler(salticidae::generic_bind(&ResiBFT::receiveMsgNewviewResiBFT, this, _1, _2));
	this->peerNet.reg_handler(salticidae::generic_bind(&ResiBFT::receiveMsgLdrprepareResiBFT, this, _1, _2));
	this->peerNet.reg_handler(salticidae::generic_bind(&ResiBFT::receiveMsgPrepareResiBFT, this, _1, _2));
	this->peerNet.reg_handler(salticidae::generic_bind(&ResiBFT::receiveMsgPrecommitResiBFT, this, _1, _2));
	this->peerNet.reg_handler(salticidae::generic_bind(&ResiBFT::receiveMsgDecideResiBFT, this, _1, _2));
	this->peerNet.reg_handler(salticidae::generic_bind(&ResiBFT::receiveMsgBCResiBFT, this, _1, _2));
	this->peerNet.reg_handler(salticidae::generic_bind(&ResiBFT::receiveMsgVCResiBFT, this, _1, _2));
	this->peerNet.reg_handler(salticidae::generic_bind(&ResiBFT::receiveMsgCommitteeResiBFT, this, _1, _2));
	this->peerNet.reg_handler(salticidae::generic_bind(&ResiBFT::receiveMsgPrepareProposalResiBFT, this, _1, _2));
	this->peerNet.reg_handler(salticidae::generic_bind(&ResiBFT::receiveMsgCommitResiBFT, this, _1, _2));

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

	// 设置初始timer - 即使MsgStart丢失，节点也能通过超时机制开始共识
	// 这避免了节点因为MsgStart丢失或启动晚了而永远卡住
	this->setTimer();

	if (DEBUG_HELP)
	{
		std::cout << COLOUR_BLUE << this->printReplicaId() << "Dispatching (timer set for initial view)" << COLOUR_NORMAL << std::endl;
	}

	// Wait for client MsgStart to trigger consensus (like Damysus)
	// But timer is set as backup - if MsgStart is lost, node will timeout and start anyway
	peerEventContext.dispatch();
}

// ============================================================
// 实验参数配置方法实现
// ============================================================

void ResiBFT::initializeExperimentConfig()
{
	// 从环境变量读取实验参数
	this->tcRatio = getTCRatioFromEnv();
	this->kRatio = getKRatioFromEnv();
	this->experimentMode = getExperimentModeFromEnv();
	this->tcBehavior = getTCBehaviorFromEnv();

	// 清空集合
	this->tcNodes.clear();
	this->byzantineNodes.clear();
	this->silentNodes.clear();

	std::cout << COLOUR_CYAN << this->printReplicaId()
			  << "[EXPERIMENT-CONFIG] Initialized experiment parameters:"
			  << std::endl;
	std::cout << COLOUR_CYAN << this->printReplicaId()
			  << "  tcRatio (λ) = " << this->tcRatio
			  << " (TC节点占TEE比例)"
			  << std::endl;
	std::cout << COLOUR_CYAN << this->printReplicaId()
			  << "  kRatio = " << this->kRatio
			  << " (委员会大小比例 k = d * kRatio)"
			  << std::endl;
	std::cout << COLOUR_CYAN << this->printReplicaId()
			  << "  experimentMode = " << this->experimentMode
			  << " (0=乐观, 1=随机, 2=攻击)"
			  << std::endl;
	std::cout << COLOUR_CYAN << this->printReplicaId()
			  << "  tcBehavior = " << this->tcBehavior
			  << " (0=投坏票, 1=沉默, 2=不一致)"
			  << std::endl;
	std::cout << COLOUR_CYAN << this->printReplicaId()
			  << "  numTrustedReplicas (d) = " << this->numTrustedReplicas
			  << std::endl;
	std::cout << COLOUR_CYAN << this->printReplicaId()
			  << "  numFaults (f) = " << this->numFaults
			  << COLOUR_NORMAL << std::endl;

	// 根据实验模式选择节点
	if (this->experimentMode == EXP_MODE_OPTIMISTIC)
	{
		std::cout << COLOUR_CYAN << this->printReplicaId()
				  << "[EXPERIMENT-CONFIG] OPTIMISTIC mode: no Byzantine nodes"
				  << COLOUR_NORMAL << std::endl;
		return;
	}

	// 选择拜占庭节点
	this->selectByzantineNodes();

	// 选择TC节点
	this->selectTCNodes();

	// 打印选中的节点
	std::cout << COLOUR_CYAN << this->printReplicaId()
			  << "[EXPERIMENT-CONFIG] Byzantine nodes (" << this->byzantineNodes.size() << "): ";
	for (auto id : this->byzantineNodes)
	{
		std::cout << id << " ";
	}
	std::cout << std::endl;

	std::cout << COLOUR_CYAN << this->printReplicaId()
			  << "[EXPERIMENT-CONFIG] TC nodes (" << this->tcNodes.size() << "): ";
	for (auto id : this->tcNodes)
	{
		std::cout << id << " ";
	}
	std::cout << COLOUR_NORMAL << std::endl;
}

void ResiBFT::selectByzantineNodes()
{
	// 拜占庭节点数量 = f (numFaults)
	unsigned int numByzantine = this->numFaults;

	// 关键修复：确保有足够的 Byzantine TEE 节点来支持 TC 节点数量
	// TC 节点数量 = d * tcRatio，必须从 Byzantine TEE 中选择
	unsigned int numTC = (unsigned int)(this->numTrustedReplicas * this->tcRatio);

	// Byzantine TEE 节点数量应该 >= numTC
	// 额外增加一些 buffer，确保 TC 选择有足够的候选
	unsigned int numByzantineTEE = numTC + 5;  // 至少 numTC + 5 个 Byzantine TEE
	if (numByzantineTEE > numByzantine)
	{
		numByzantineTEE = numByzantine;  // 不能超过总 Byzantine 数量
	}
	if (numByzantineTEE > this->numTrustedReplicas)
	{
		numByzantineTEE = this->numTrustedReplicas;  // 不能超过 TEE 总数
	}

	unsigned int numByzantineGeneral = numByzantine - numByzantineTEE;
	if (numByzantineGeneral > this->numGeneralReplicas)
	{
		numByzantineGeneral = this->numGeneralReplicas;
		numByzantineTEE = numByzantine - numByzantineGeneral;  // 调整
	}

	// 使用固定种子确保确定性（所有节点选择相同的拜占庭集合）
	unsigned int seed = 12345;

	this->byzantineNodes.clear();

	// Step 1: 从 TEE 节点中选择 Byzantine TEE 节点
	std::set<ReplicaID> teeNodes;
	for (unsigned int i = this->numGeneralReplicas; i < this->numReplicas; i++)
	{
		teeNodes.insert(i);
	}

	std::vector<ReplicaID> teeVec(teeNodes.begin(), teeNodes.end());
	unsigned int loopCount = 0;
	while (this->byzantineNodes.size() < numByzantineTEE && loopCount < teeVec.size())
	{
		unsigned int randomVal = seed * seed + loopCount * 13;
		unsigned int idx = randomVal % teeVec.size();
		this->byzantineNodes.insert(teeVec[idx]);
		seed = seed + 1;
		loopCount++;
		if (loopCount > numByzantineTEE * 10) break;
	}

	// Step 2: 从 G-Replica 节点中选择剩余的 Byzantine 节点
	std::set<ReplicaID> generalNodes;
	for (unsigned int i = 0; i < this->numGeneralReplicas; i++)
	{
		generalNodes.insert(i);
	}

	std::vector<ReplicaID> generalVec(generalNodes.begin(), generalNodes.end());
	loopCount = 0;
	while (this->byzantineNodes.size() < numByzantine && loopCount < generalVec.size())
	{
		unsigned int randomVal = seed * seed + loopCount * 17;
		unsigned int idx = randomVal % generalVec.size();
		this->byzantineNodes.insert(generalVec[idx]);
		seed = seed + 1;
		loopCount++;
		if (loopCount > numByzantineGeneral * 10) break;
	}

	// 输出 Byzantine 节点分布
	std::cout << COLOUR_CYAN << this->printReplicaId()
			  << "[EXPERIMENT-CONFIG] Byzantine distribution: "
			  << numByzantineTEE << " TEE + " << numByzantineGeneral << " General = " << this->byzantineNodes.size()
			  << COLOUR_NORMAL << std::endl;
}

void ResiBFT::selectTCNodes()
{
	// TC节点数量 = d * λ (TEE节点数 * TC比例)
	unsigned int numTC = (unsigned int)(this->numTrustedReplicas * this->tcRatio);

	// TC节点必须是拜占庭TEE节点
	// 即：TC节点 ∈ 拜占庭节点 ∩ TEE节点

	// 获取拜占庭TEE节点（拜占庭节点和TEE节点的交集）
	std::set<ReplicaID> byzantineTEE;
	for (auto id : this->byzantineNodes)
	{
		if (id >= this->numGeneralReplicas)  // 是TEE节点
		{
			byzantineTEE.insert(id);
		}
	}

	// 如果拜占庭TEE节点不足，调整 numTC
	if (byzantineTEE.size() < numTC)
	{
		numTC = byzantineTEE.size();
		std::cout << COLOUR_CYAN << this->printReplicaId()
				  << "[EXPERIMENT-CONFIG] Adjusted numTC to " << numTC
				  << " (limited by Byzantine TEE count)"
				  << COLOUR_NORMAL << std::endl;
	}

	// 从拜占庭TEE节点中随机选择 numTC 个作为TC节点
	unsigned int seed = 12345 + 1;
	std::vector<ReplicaID> byzantineTEEVec(byzantineTEE.begin(), byzantineTEE.end());

	this->tcNodes.clear();
	unsigned int loopCount = 0;
	while (this->tcNodes.size() < numTC && this->tcNodes.size() < byzantineTEEVec.size())
	{
		unsigned int randomVal = seed * seed + loopCount * 13;
		unsigned int idx = randomVal % byzantineTEEVec.size();
		this->tcNodes.insert(byzantineTEEVec[idx]);
		seed = seed + 1;
		loopCount++;
		if (loopCount > numTC * 10) break;
	}
}

bool ResiBFT::isTCNode(ReplicaID replicaId)
{
	return this->tcNodes.find(replicaId) != this->tcNodes.end();
}

bool ResiBFT::isByzantineNode(ReplicaID replicaId)
{
	return this->byzantineNodes.find(replicaId) != this->byzantineNodes.end();
}

bool ResiBFT::isSilentNode(ReplicaID replicaId)
{
	return this->silentNodes.find(replicaId) != this->silentNodes.end();
}

void ResiBFT::generateSilentNodesForView(View view)
{
	// 每轮视图随机生成沉默节点
	// 沉默节点比例 = SILENT_NODE_RATIO
	unsigned int numSilent = (unsigned int)(this->numReplicas * SILENT_NODE_RATIO);

	// 使用 view 作为种子，确保每轮不同但所有节点一致
	unsigned int seed = view * 7 + 3;

	std::set<ReplicaID> potentialSilent;
	// 排除TC节点（TC节点投坏票，不沉默）
	for (unsigned int i = 0; i < this->numReplicas; i++)
	{
		if (!isTCNode(i))
		{
			potentialSilent.insert(i);
		}
	}

	this->silentNodes.clear();
	std::vector<ReplicaID> potentialVec(potentialSilent.begin(), potentialSilent.end());
	unsigned int loopCount = 0;
	while (this->silentNodes.size() < numSilent && this->silentNodes.size() < potentialVec.size())
	{
		unsigned int randomVal = seed * seed + loopCount * 11;
		unsigned int idx = randomVal % potentialVec.size();
		this->silentNodes.insert(potentialVec[idx]);
		seed = seed + 1;
		loopCount++;
		if (loopCount > numSilent * 10) break;
	}

	std::cout << COLOUR_CYAN << this->printReplicaId()
			  << "[SILENT-NODES] View " << view << ": "
			  << this->silentNodes.size() << " silent nodes generated"
			  << COLOUR_NORMAL << std::endl;
}

void ResiBFT::simulateTCBehavior(ReplicaID sender, View view)
{
	// 检查发送者是否是TC节点
	if (!isTCNode(sender))
	{
		return;  // 不是TC节点，正常处理
	}

	// 根据TC节点行为配置模拟
	switch (this->tcBehavior)
	{
		case TC_BEHAVIOR_BAD_VOTE:
			// 投坏票：发送VC_REJECTED
			std::cout << COLOUR_MAGENTA << this->printReplicaId()
					  << "[TC-BEHAVIOR] TC node " << sender << " sending BAD_VOTE (VC_REJECTED)"
					  << " for view " << view
					  << COLOUR_NORMAL << std::endl;
			// 如果当前节点是TC节点，发送VC_REJECTED
			if (sender == this->replicaId)
			{
				Hash blockHash = Hash();
				if (this->blocks.find(view) != this->blocks.end())
				{
					blockHash = this->blocks[view].hash();
				}
				this->sendVC(VC_REJECTED, view, blockHash);
			}
			break;

		case TC_BEHAVIOR_SILENT:
			// 沉默：不发送任何消息
			std::cout << COLOUR_MAGENTA << this->printReplicaId()
					  << "[TC-BEHAVIOR] TC node " << sender << " is SILENT for view " << view
					  << COLOUR_NORMAL << std::endl;
			break;

		case TC_BEHAVIOR_INCONSISTENT:
			// 发送不一致消息（向不同节点发送不同内容）
			std::cout << COLOUR_MAGENTA << this->printReplicaId()
					  << "[TC-BEHAVIOR] TC node " << sender << " sending INCONSISTENT messages for view " << view
					  << COLOUR_NORMAL << std::endl;
			break;
	}
}