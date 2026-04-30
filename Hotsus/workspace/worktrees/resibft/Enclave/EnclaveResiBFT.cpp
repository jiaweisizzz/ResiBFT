#include <set>
#include "EnclaveBasic.h"
#include "Enclave_t.h"

// Task 10: TEE functions for ResiBFT
// Updated to use lightweight structures for SGX parameter passing

// ResiBFT state variables (similar to Hotsus state)
Hash_t prepareHash_ResiBFT_t = initiateHash_t();    // Hash of the last prepared block
View prepareView_ResiBFT_t = 0;                     // View of [prepareHash_ResiBFT_t]
Hash_t lockedHash_ResiBFT_t = initiateHash_t();     // Hash of the last locked block
View lockedView_ResiBFT_t = 0;                      // View of [lockedHash_ResiBFT_t]
View view_ResiBFT_t = 0;                             // Current view
Phase phase_ResiBFT_t = PHASE_NEWVIEW;              // Current phase
Stage stage_ResiBFT_t = STAGE_NORMAL;               // Current stage (normal, gracious, uncivil)

// Helper function to create lightweight justification (single signature)
JustificationLight_t createJustificationLight_t(Hash_t proposeHash, View proposeView, Hash_t justifyHash, View justifyView, Phase phase)
{
    JustificationLight_t justification_t;
    justification_t.set = true;

    RoundData_t roundData_t;
    roundData_t.proposeHash = proposeHash;
    roundData_t.proposeView = proposeView;
    roundData_t.justifyHash = justifyHash;
    roundData_t.justifyView = justifyView;
    roundData_t.phase = phase;
    justification_t.roundData = roundData_t;

    // Sign the round data
    std::string roundDataText = roundData2string_t(roundData_t);
    SignSingle_t sign_t;
    sign_t.set = true;
    sign_t.signer = replicaId;
    std::copy(roundDataText.begin(), roundDataText.end(), std::begin(sign_t.signtext));
    // Note: signData_t returns Sign_t, we need to convert to SignSingle_t
    Sign_t fullSign = signData_t(roundDataText);
    sign_t.set = fullSign.set;
    sign_t.signer = fullSign.signer;
    std::copy(std::begin(fullSign.signtext), std::end(fullSign.signtext), std::begin(sign_t.signtext));

    justification_t.sign = sign_t;

    return justification_t;
}

// Helper function to convert VCType to string for signing
std::string vcType2string_t(VCType_t vcType_t)
{
    std::string text = "";
    text += std::to_string(vcType_t.vcType);
    return text;
}

// Helper function to convert VerificationCertificate to string for signing
std::string verificationCertificate2string_t(VerificationCertificate_t vc_t)
{
    std::string text = "";
    text += vcType2string_t(vc_t.type);
    text += std::to_string(vc_t.view);
    text += hash2string_t(vc_t.blockHash);
    text += std::to_string(vc_t.signer);
    return text;
}

// Helper function to convert BlockCertificate to string for verification
std::string blockCertificate2string_t(BlockCertificate_t blockCertificate_t)
{
    std::string text = "";
    text += block2string_t(blockCertificate_t.block);
    text += std::to_string(blockCertificate_t.view);
    text += justification2string_t(blockCertificate_t.qcPrecommit);
    text += signs2string_t(blockCertificate_t.qcVc);
    return text;
}

// TEE_verifyProposalResiBFT: Verify proposal structure and signatures
sgx_status_t TEE_verifyProposalResiBFT(Proposal_t *proposal_t, Signs_t *signs_t, bool *b)
{
    sgx_status_t status_t = SGX_SUCCESS;

    if (DEBUG_TEE)
    {
        TEE_Print((printReplicaId_t() + "ENCLAVE: TEE_verifyProposalResiBFT called").c_str());
    }

    // Verify the proposal using existing verification function
    *b = verifyProposal_t(proposal_t, signs_t);

    if (DEBUG_TEE)
    {
        if (*b)
        {
            TEE_Print((printReplicaId_t() + "ENCLAVE: TEE_verifyProposalResiBFT succeeded").c_str());
        }
        else
        {
            TEE_Print((printReplicaId_t() + "ENCLAVE: TEE_verifyProposalResiBFT failed").c_str());
        }
    }

    return status_t;
}

// TEE_verifyJustificationResiBFT: Verify justification signatures
sgx_status_t TEE_verifyJustificationResiBFT(Justification_t *justification_t, bool *b)
{
    sgx_status_t status_t = SGX_SUCCESS;

    if (DEBUG_TEE)
    {
        TEE_Print((printReplicaId_t() + "ENCLAVE: TEE_verifyJustificationResiBFT called").c_str());
    }

    // Verify the justification using existing verification function
    *b = verifyJustification_t(justification_t);

    if (DEBUG_TEE)
    {
        if (*b)
        {
            TEE_Print((printReplicaId_t() + "ENCLAVE: TEE_verifyJustificationResiBFT succeeded").c_str());
        }
        else
        {
            TEE_Print((printReplicaId_t() + "ENCLAVE: TEE_verifyJustificationResiBFT failed").c_str());
        }
    }

    return status_t;
}

// TEE_verifyBlockCertificateResiBFT: Verify block certificate structure and signatures
sgx_status_t TEE_verifyBlockCertificateResiBFT(BlockCertificate_t *blockCertificate_t, bool *b)
{
    sgx_status_t status_t = SGX_SUCCESS;

    if (DEBUG_TEE)
    {
        TEE_Print((printReplicaId_t() + "ENCLAVE: TEE_verifyBlockCertificateResiBFT called").c_str());
    }

    // Verify the block certificate structure
    // 1. Check that the block is set
    if (!blockCertificate_t->block.set)
    {
        if (DEBUG_TEE)
        {
            TEE_Print((printReplicaId_t() + "ENCLAVE: TEE_verifyBlockCertificateResiBFT - block not set").c_str());
        }
        *b = false;
        return status_t;
    }

    // 2. Verify the qcPrecommit justification
    if (!verifyJustification_t(&blockCertificate_t->qcPrecommit))
    {
        if (DEBUG_TEE)
        {
            TEE_Print((printReplicaId_t() + "ENCLAVE: TEE_verifyBlockCertificateResiBFT - qcPrecommit verification failed").c_str());
        }
        *b = false;
        return status_t;
    }

    // 3. Verify the qcVc signatures (placeholder for now)
    // In full implementation, would verify each signature in qcVc
    if (blockCertificate_t->qcVc.size < getGeneralQuorumSize_t())
    {
        if (DEBUG_TEE)
        {
            TEE_Print((printReplicaId_t() + "ENCLAVE: TEE_verifyBlockCertificateResiBFT - insufficient qcVc signatures").c_str());
        }
        *b = false;
        return status_t;
    }

    *b = true;

    if (DEBUG_TEE)
    {
        TEE_Print((printReplicaId_t() + "ENCLAVE: TEE_verifyBlockCertificateResiBFT succeeded").c_str());
    }

    return status_t;
}

// TEE_setViewResiBFT: Set the current view in enclave
sgx_status_t TEE_setViewResiBFT(View *view)
{
    sgx_status_t status_t = SGX_SUCCESS;

    if (DEBUG_TEE)
    {
        TEE_Print((printReplicaId_t() + "ENCLAVE: TEE_setViewResiBFT called, setting view to " + std::to_string(*view)).c_str());
    }

    view_ResiBFT_t = *view;

    if (DEBUG_TEE)
    {
        TEE_Print((printReplicaId_t() + "ENCLAVE: TEE_setViewResiBFT completed").c_str());
    }

    return status_t;
}

// TEE_initializeMsgNewviewResiBFT: Initialize newview justification with current state (lightweight output)
sgx_status_t TEE_initializeMsgNewviewResiBFT(JustificationLight_t *justification_t)
{
    sgx_status_t status_t = SGX_SUCCESS;

    if (DEBUG_TEE)
    {
        TEE_Print((printReplicaId_t() + "ENCLAVE: TEE_initializeMsgNewviewResiBFT called").c_str());
    }

    // Initialize with dummy hash for propose, and use prepare hash for justify
    Hash_t dummyHash = initiateDummyHash_t();
    // HotStuff: proposeView should be the current view (node is ready for view k)
    View proposeView = view_ResiBFT_t;

    *justification_t = createJustificationLight_t(dummyHash, proposeView, prepareHash_ResiBFT_t, prepareView_ResiBFT_t, PHASE_NEWVIEW);

    // Note: Do NOT update view_ResiBFT_t here - it should be updated when view actually changes
    // (e.g., in startNewViewResiBFT or when handling MsgCommit)

    if (DEBUG_TEE)
    {
        TEE_Print((printReplicaId_t() + "ENCLAVE: TEE_initializeMsgNewviewResiBFT completed").c_str());
    }

    return status_t;
}

// TEE_initializeAccumulatorResiBFT: Initialize accumulator from lightweight input
sgx_status_t TEE_initializeAccumulatorResiBFT(AccumulatorInput_t *input_t, Accumulator_t *accumulator_t)
{
    sgx_status_t status_t = SGX_SUCCESS;

    if (DEBUG_TEE)
    {
        TEE_Print((printReplicaId_t() + "ENCLAVE: TEE_initializeAccumulatorResiBFT called").c_str());
    }

    View proposeView = input_t->proposeView;
    View highView = 0;
    Hash_t highHash_t = initiateHash_t();
    std::set<ReplicaID> signers;

    // Iterate through input items to find the highest justifyView
    for (unsigned int i = 0; i < input_t->size; i++)
    {
        AccumulatorInputItem_t item = input_t->items[i];
        ReplicaID signer = item.signer;
        View justifyView = item.justifyView;
        Hash_t justifyHash = item.justifyHash;

        // Track unique signers and find highest view
        if (signers.find(signer) == signers.end())
        {
            signers.insert(signer);
            if (justifyView >= highView)
            {
                highView = justifyView;
                highHash_t = justifyHash;
            }
        }
    }

    // Set the accumulator with collected information
    accumulator_t->set = true;
    accumulator_t->proposeView = proposeView;
    accumulator_t->prepareHash = highHash_t;
    accumulator_t->prepareView = highView;
    accumulator_t->size = signers.size();

    if (DEBUG_TEE)
    {
        TEE_Print((printReplicaId_t() + "ENCLAVE: TEE_initializeAccumulatorResiBFT completed with size " + std::to_string(signers.size())).c_str());
    }

    return status_t;
}

// TEE_initializeMsgLdrprepareResiBFT: Sign the proposal hash (lightweight)
sgx_status_t TEE_initializeMsgLdrprepareResiBFT(Hash_t *proposalHash_t, SignSingle_t *sign_t)
{
    sgx_status_t status_t = SGX_SUCCESS;

    if (DEBUG_TEE)
    {
        TEE_Print((printReplicaId_t() + "ENCLAVE: TEE_initializeMsgLdrprepareResiBFT called").c_str());
    }

    // Sign the proposal hash
    std::string hashText = hash2string_t(*proposalHash_t);
    Sign_t fullSign = signData_t(hashText);

    sign_t->set = fullSign.set;
    sign_t->signer = fullSign.signer;
    std::copy(std::begin(fullSign.signtext), std::end(fullSign.signtext), std::begin(sign_t->signtext));

    if (DEBUG_TEE)
    {
        TEE_Print((printReplicaId_t() + "ENCLAVE: TEE_initializeMsgLdrprepareResiBFT completed").c_str());
    }

    return status_t;
}

// TEE_respondProposalResiBFT: Create justification for prepare message (lightweight output)
sgx_status_t TEE_respondProposalResiBFT(Hash_t *proposeHash_t, Accumulator_t *accumulator_t, JustificationLight_t *justification_t)
{
    sgx_status_t status_t = SGX_SUCCESS;

    if (DEBUG_TEE)
    {
        TEE_Print((printReplicaId_t() + "ENCLAVE: TEE_respondProposalResiBFT called").c_str());
    }

    // Create justification with PREPARE phase
    *justification_t = createJustificationLight_t(
        *proposeHash_t,
        accumulator_t->proposeView,
        accumulator_t->prepareHash,
        accumulator_t->prepareView,
        PHASE_PREPARE
    );

    if (DEBUG_TEE)
    {
        TEE_Print((printReplicaId_t() + "ENCLAVE: TEE_respondProposalResiBFT completed").c_str());
    }

    return status_t;
}

// TEE_saveMsgPrepareResiBFT: Save prepare and create precommit justification (lightweight)
sgx_status_t TEE_saveMsgPrepareResiBFT(RoundData_t *roundData_t, JustificationLight_t *justification_t)
{
    sgx_status_t status_t = SGX_SUCCESS;

    if (DEBUG_TEE)
    {
        TEE_Print((printReplicaId_t() + "ENCLAVE: TEE_saveMsgPrepareResiBFT called").c_str());
    }

    // Update phase to PRECOMMIT
    RoundData_t precommitRoundData = *roundData_t;
    precommitRoundData.phase = PHASE_PRECOMMIT;

    // Create precommit justification
    *justification_t = createJustificationLight_t(
        precommitRoundData.proposeHash,
        precommitRoundData.proposeView,
        precommitRoundData.justifyHash,
        precommitRoundData.justifyView,
        PHASE_PRECOMMIT
    );

    // Update internal state
    prepareHash_ResiBFT_t = roundData_t->proposeHash;
    prepareView_ResiBFT_t = roundData_t->proposeView;

    if (DEBUG_TEE)
    {
        TEE_Print((printReplicaId_t() + "ENCLAVE: TEE_saveMsgPrepareResiBFT completed").c_str());
    }

    return status_t;
}

// TEE_createVCResiBFT: Create verification certificate
sgx_status_t TEE_createVCResiBFT(VCType_t *type, View *view, Hash_t *blockHash, VerificationCertificate_t *vc_t)
{
    sgx_status_t status_t = SGX_SUCCESS;

    if (DEBUG_TEE)
    {
        TEE_Print((printReplicaId_t() + "ENCLAVE: TEE_createVCResiBFT called").c_str());
    }

    // Set the VC type
    vc_t->type.vcType = type->vcType;

    // Set the view
    vc_t->view = *view;

    // Set the block hash
    vc_t->blockHash = *blockHash;

    // Set the signer (this replica)
    vc_t->signer = replicaId;

    // Create the signature
    std::string vcText = verificationCertificate2string_t(*vc_t);
    vc_t->sign = signData_t(vcText);

    if (DEBUG_TEE)
    {
        TEE_Print((printReplicaId_t() + "ENCLAVE: TEE_createVCResiBFT completed for view " + std::to_string(*view)).c_str());
    }

    return status_t;
}

// TEE_validateBlockResiBFT: Validate block structure and transactions
sgx_status_t TEE_validateBlockResiBFT(Block_t *block_t, bool *b)
{
    sgx_status_t status_t = SGX_SUCCESS;

    if (DEBUG_TEE)
    {
        TEE_Print((printReplicaId_t() + "ENCLAVE: TEE_validateBlockResiBFT called").c_str());
    }

    // Check that the block is set
    if (!block_t->set)
    {
        if (DEBUG_TEE)
        {
            TEE_Print((printReplicaId_t() + "ENCLAVE: TEE_validateBlockResiBFT - block not set").c_str());
        }
        *b = false;
        return status_t;
    }

    // Validate block size is within bounds
    if (block_t->size > MAX_NUM_TRANSACTIONS)
    {
        if (DEBUG_TEE)
        {
            TEE_Print((printReplicaId_t() + "ENCLAVE: TEE_validateBlockResiBFT - block size exceeds maximum").c_str());
        }
        *b = false;
        return status_t;
    }

    // Placeholder: In full implementation, would validate:
    // 1. Transaction signatures
    // 2. Previous hash chain integrity
    // 3. Transaction ordering and dependencies
    // 4. State transition validity

    *b = true;

    if (DEBUG_TEE)
    {
        TEE_Print((printReplicaId_t() + "ENCLAVE: TEE_validateBlockResiBFT succeeded").c_str());
    }

    return status_t;
}