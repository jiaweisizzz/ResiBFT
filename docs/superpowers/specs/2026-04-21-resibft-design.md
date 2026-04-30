# ResiBFT 协议实现设计文档

## 概述

本文档描述在 Hotsus 项目中新增 ResiBFT 共识协议的设计方案。ResiBFT 是一种基于部分 TEE 支持的 BFT 共识协议，具有三种运行阶段（NORMAL、GRACIOUS、UNCIVIL）、动态委员会管理和基于 Epoch 的视图结构。

## 项目背景

- **基础项目**: Hotsus (PT-BFT 共识实现)
- **语言**: C++，支持 SGX/TEE
- **依赖库**: Salticidae 网络库、OpenSSL 加密库、Intel SGX SDK
- **现有协议**: Hotstuff、Damysus、Hotsus

## 核心概念

### 1. Epoch 和 View 结构

- **Epoch**: 更高层的时间单位，在委员会撤销时递增 (e → e+1)
- **View**: Epoch 内的子单位，在正常的 Leader 轮换时递增 (v → v+1)
- **初始值**: epoch = e0, view = v0

### 2. 副本角色

- **T-Replicas** (`{Rr}`): 可信副本，通过 SGX 远程认证验证
- **G-Replicas**: 普通副本（非 TEE 副本）
- **Committee** (`{Rc}`): 从 T-Replicas 中选出的子集，由 Leader 选择用于内部共识
- **内部副本**: 参与 consensus 轮次的 Committee 成员

### 3. 运行阶段

| 阶段 | 描述 | 触发条件 |
|------|------|----------|
| NORMAL | 默认运行，基于 Committee 的 Hotstuff | 初始状态 |
| GRACIOUS | 基于 Damysus 的优雅过渡共识 | Committee 成员收到 accepted-vcs |
| UNCIVIL | 委员会撤销，回退到完整 Hotstuff | Leader 验证失败或收到 f+1 rejected/timeout-vcs |

### 4. 验证证书 (VerificationCertificate)

三种验证证书类型：
- **accepted-vc**: 区块验证通过
- **rejected-vc**: 区块验证失败
- **timeout-vc**: 验证超时

VerificationCertificate 触发阶段转换和 checkpoint 更新。

### 5. Checkpoint 和 BlockCertificate

- **Checkpoint**: 恢复点，包含 `{B, v, qc(PRECOMMIT), qc(vc)}`
- **BlockCertificate**: 区块证书，用于轻量级验证，广播给所有副本

## 文件结构

### 新增文件

```
App/
├── ResiBFT.h              # 主类头文件
├── ResiBFT.cpp            # 主类实现（约1500行）
├── ResiBFTBasic.h         # Stage 枚举和基础类型
├── ResiBFTBasic.cpp       # 基础类型实现
├── Checkpoint.h           # Checkpoint 类
├── Checkpoint.cpp         # Checkpoint 实现
├── VerificationCertificate.h   # 验证证书类
├── VerificationCertificate.cpp  # VerificationCertificate 实现
├── BlockCertificate.h           # 区块证书类
├── BlockCertificate.cpp         # BlockCertificate 实现
```

### 修改文件

```
App/types.h                # 新增 HEADER 常量和 Epoch 类型
App/message.h              # 新增消息结构
App/parameters.h           # 可能需要参数调整
Enclave/Enclave.edl        # 新增 ECALL 函数
Enclave/EnclaveUsertypes.h # 新增类型定义
Enclave/EnclaveResiBFT.cpp # ResiBFT 的 Enclave 实现
```

## 数据结构

### Stage 枚举 (ResiBFTBasic.h)

```cpp
enum Stage {
    STAGE_NORMAL = 0,
    STAGE_GRACIOUS = 1,
    STAGE_UNCIVIL = 2
};
```

### VerificationCertificate 结构 (VerificationCertificate.h)

```cpp
class VerificationCertificate {
private:
    VCType type;          // ACCEPTED, REJECTED, TIMEOUT
    View view;
    Hash blockHash;
    ReplicaID signer;
    Sign sign;
public:
    // serialize/unserialize, getters, toString
};
```

### Checkpoint 结构 (Checkpoint.h)

```cpp
class Checkpoint {
private:
    Block block;               // B_i-1
    View view;                 // v_i-1
    Justification qcPrecommit; // qc(PRECOMMIT)
    Signs qcVc;                // qc(vc(v_i-1))
public:
    // serialize/unserialize, getters, hash, toString
};
```

### BlockCertificate 结构 (BlockCertificate.h)

```cpp
class BlockCertificate {
private:
    Block block;               // B_i
    Justification qcPrecommit; // qc(PRECOMMIT)
    Signs qcVc;                // qc(vc)
public:
    // serialize/unserialize, getters, verify, toString
};
```

## 消息类型 (message.h)

| Header | 消息 | 用途 |
|--------|------|------|
| 0x21 | MsgNewviewResiBFT | 内部副本发送最高准备区块给 Leader |
| 0x22 | MsgLdrprepareResiBFT | Leader 提议新区块并附带 bc |
| 0x23 | MsgPrepareResiBFT | 内部副本对提议投票 |
| 0x24 | MsgPrecommitResiBFT | 副本发送 precommit 投票 |
| 0x25 | MsgDecideResiBFT | Leader 广播 qc(PRECOMMIT) |
| 0x26 | MsgBCResiBFT | 广播区块证书给所有副本 |
| 0x27 | MsgVCResiBFT | 验证证书发送给下一个 Leader |
| 0x28 | MsgCommitteeResiBFT | Leader 公布委员会选择 |

## ResiBFT 类结构

### 成员变量

```cpp
// 基础设置（继承 Hotstuff/Damysus 模式）
KeysFunctions keysFunction;
ReplicaID replicaId;
unsigned int numGeneralReplicas, numTrustedReplicas, numReplicas;
unsigned int numViews, numFaults;
double leaderChangeTime;
Nodes nodes;
Key privateKey;
unsigned int generalQuorumSize;  // qG
unsigned int trustedQuorumSize;  // qT
View view;
Epoch epoch;
Stage stage;

// 委员会管理
Group committee;
Group trustedReplicas;
bool isTrustedReplica;
bool isCommitteeMember;

// Checkpoint 和 BlockCertificate
Checkpoint checkpoint;
std::map<View, BlockCertificate> blockCertificates;

// 验证证书
std::set<VerificationCertificate> acceptedVCs, rejectedVCs, timeoutVCs;

// 网络和状态（标准模式）
PeerNet peerNet;
ClientNet clientNet;
Clients clients;
Peers peers;
salticidae::TimerEvent timer;
ExecutionQueue executionQueue;
std::list<Transaction> transactions;
std::map<View, Block> blocks;
std::mutex mutexTransaction, mutexHandle;
Log log;
bool started, stopped;
```

### 关键方法

#### 阶段转换
- `transitionToNormal()`: 进入 NORMAL 阶段，基于 Committee 运行 Hotstuff
- `transitionToGracious()`: 进入 GRACIOUS 阶段，运行 Damysus
- `transitionToUncivil()`: 进入 UNCIVIL 阶段，撤销委员会，重置 epoch

#### 委员会管理
- `selectCommittee()`: Leader 从 T-Replicas 中选择委员会
- `revokeCommittee()`: 验证失败时撤销委员会
- `updateCommittee(Group)`: 更新委员会成员

#### 消息处理（遵循现有模式）
- Receive: `receiveMsgXxxResiBFT()`
- Send: `sendMsgXxxResiBFT()`
- Handle: `handleMsgXxxResiBFT()`
- Initiate: `initiateMsgXxxResiBFT()` (Leader)
- Respond: `respondMsgXxxResiBFT()` (副本)

#### 验证
- `validateBlock(Block)`: 使用 CHECKER（T-Replicas）或 SAFENODE predicate（G-Replicas）验证区块
- `sendVC(VCType, View, Hash)`: 发送验证证书给下一个 Leader
- `checkAcceptedVCs()`: 检查是否收到超过 f+1 个 accepted-vcs
- `checkRejectedVCs()`: 检查是否收到至少 f+1 个 rejected-vcs
- `checkTimeoutVCs()`: 检查是否收到至少 f+1 个 timeout-vcs

#### TEE 函数（ECALLs）
- `TEE_verifyProposalResiBFT()`: 在 enclave 中验证提议
- `TEE_verifyJustificationResiBFT()`: 在 enclave 中验证 justification
- `TEE_verifyBCResiBFT()`: 在 enclave 中验证区块证书
- `TEE_initializeMsgNewviewResiBFT()`: 创建 NEWVIEW justification
- `TEE_initializeAccumulatorResiBFT()`: 从 NEWVIEW 创建 accumulator
- `TEE_createVCResiBFT()`: 在 enclave 中创建 VC

## 协议流程

### NORMAL 阶段（T-Replicas）

```
1. 如果是 Leader 且 committee 为空:
   - selectCommittee() → 广播 MsgCommitteeResiBFT
   - 基于 {Rc} = {R1} 运行 Hotstuff, j = j+1
2. 否则:
   - 基于 {Rc} 和 {R1} 运行 Hotstuff, j = j+1
```

### NORMAL 阶段（G-Replicas）

```
- 基于 {Rc} 和 {R1} 运行 Hotstuff, j = j+1
- 监听 BC 以更新 checkpoint
```

### GRACIOUS 阶段（Committee 成员）

```
1. 如果是 Leader 且检查 accepted-vcs:
   - 运行 Damysus，广播 bc，发送 vc, j = j+1
2. 否则:
   - 运行 Damysus，发送 vc, j = j+1
```

### GRACIOUS 阶段（非 Committee T-Replicas & G-Replicas）

```
- 初始化 checkpoint cp
- 监听 bc 以更新 cp，发送 vc, j = j+1
```

### UNCIVIL 阶段（T-Replicas）

```
1. 如果是 Leader 且检查 accepted-vcs 失败:
   - 删除 committee
   - 运行 Hotstuff, {Rc} = 0, e = e+1, j = 0
2. 否则:
   - 基于 {Rc} = 0 运行 Hotstuff, e = e+1, j = 0
```

### UNCIVIL 阶段（G-Replicas）

```
- 基于 {Rc} = 0 运行 Hotstuff, e = e+1, j = 0
```

## 共识轮次

### NEWVIEW 阶段
- 内部副本发送 `(NEWVIEW, (H_b, v_i), (H_u, v_u), ph)` 给 Leader R_L
- Leader 等待 qT 个 NEWVIEW 消息和 qG 个 vc(v_i-1)s

### PREPARE 阶段
- Leader 验证 v_i-1 的验证结果
- 通过 ACCUMULATOR 选择最高的 view/hash
- 提议新区块 B_i
- 广播 `(PROPOSAL, (H(B_i), v_i), (H_u*, v_u*), ph, B_i, qc(NEWVIEW), qc(vc(v_i-1)))`
- 如果收到 f+1 个 rejected-vcs 或 timeout-vcs: 转换到 UNCIVIL

### PRECOMMIT 阶段
- Leader 收到 qT 个 PREPARE 消息
- 标记 B_i 为已准备
- 广播 `qc(PRECOMMIT)` 给内部副本

### DECIDE 阶段
- Leader 收到 qT 个 PRECOMMIT 消息
- 暂定执行 B_i
- 广播 `qc(PRECOMMIT)` 和 bc 给所有副本

### VALIDATION 阶段
- 所有副本收到 bc
- 提交 B_i-1
- T-Replicas 使用 CHECKER 验证 B_i
- G-Replicas 使用 SAFENODE predicate 验证 B_i
- 发送 accepted-vc 或 rejected-vc 给下一个 Leader
- 更新 checkpoint cp

## Enclave 函数（Enclave.edl 新增）

```cpp
// ResiBFT 新增 ECALLs
public sgx_status_t TEE_verifyProposalResiBFT([in] Proposal_t *proposal_t, [in] Signs_t *signs_t, [out] bool *b);
public sgx_status_t TEE_verifyJustificationResiBFT([in] Justification_t *justification_t, [out] bool *b);
public sgx_status_t TEE_verifyBCResiBFT([in] BC_t *bc_t, [out] bool *b);
public sgx_status_t TEE_initializeMsgNewviewResiBFT([out] Justification_t *justification_t);
public sgx_status_t TEE_initializeAccumulatorResiBFT([in] Justifications_t *justifications_t, [out] Accumulator_t *accumulator_t);
public sgx_status_t TEE_createVCResiBFT([in] VCType_t *type, [in] View *view, [in] Hash_t *blockHash, [out] VC_t *vc_t);
public sgx_status_t TEE_validateBlockResiBFT([in] Block_t *block_t, [out] bool *b);
```

## 实现策略

1. 创建所有新数据结构文件（VC、Checkpoint、BC、ResiBFTBasic）
2. 修改现有文件（types.h、message.h、EnclaveUsertypes.h）
3. 实现 ResiBFT.h 类声明
4. 实现 ResiBFT.cpp，遵循 Hotsus.cpp 的现有模式
5. 在 Enclave.edl 中添加 ECALL
6. 在 EnclaveResiBFT.cpp 中实现 Enclave 函数
7. 更新 Makefile 以包含新文件
8. 测试编译

## 代码风格合规性

遵循现有 Hotsus 项目约定：
- 头文件保护: `#ifndef XXX_H / #define XXX_H / #endif`
- 成员变量: 驼峰命名（如 `replicaId`, `numViews`）
- 函数: 描述性命名（如 `handleMsgNewviewResiBFT`）
- 分区注释: `// Print functions`, `// Receive messages` 等
- 序列化: `serialize()` / `unserialize()` 方法
- 网络: Salticidae PeerNet/ClientNet 模式
- 线程安全: `mutexTransaction`, `mutexHandle`

## 预估代码量

| 文件 | 行数 |
|------|------|
| ResiBFT.h | ~150 |
| ResiBFT.cpp | ~1500 |
| ResiBFTBasic.h/cpp | ~50 |
| VerificationCertificate.h/cpp | ~100 |
| Checkpoint.h/cpp | ~100 |
| BlockCertificate.h/cpp | ~100 |
| message.h 新增 | ~300 |
| Enclave 新增 | ~200 |
| **总计** | ~2400 |

## 测试策略

- 使用现有 Makefile 进行编译测试
- 集成到 experiments.py（添加 `--RunResiBFT` 选项）
- Docker 本地模式测试
- Aliyun 服务器模式测试

## 成功标准

1. 代码编译无错误
2. 所有新类遵循现有代码风格
3. 消息结构与 Salticidae 兼容
4. Enclave 函数在 EDL 中正确定义
5. ResiBFT 类构造函数匹配现有模式