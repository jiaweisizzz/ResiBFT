# ResiBFT 系统说明文档

## 目录

1. [概述](#概述)
2. [架构设计](#架构设计)
3. [目录结构](#目录结构)
4. [核心组件说明](#核心组件说明)
5. [共识流程](#共识流程)
   - [NORMAL 模式 HotStuff 3-phase](#normal-模式-hotstuff-3-phase)
   - [GRACIOUS/UNCIVIL 模式 Damysus 2-phase](#graciousuncivil-模式-damysus-2-phase)
6. [三种运行模式](#三种运行模式)
   - [NORMAL 模式](#1-normal-模式-正常共识)
   - [GRACIOUS 模式](#2-gracious-模式-优雅降级)
   - [UNCIVIL 模式](#3-uncivil-模式-恶意检测)
7. [Verification Certificate (VC) 详细处理机制](#verification-certificate-vc-详细处理机制)
8. [消息类型](#消息类型)
9. [数据结构](#数据结构)
   - [HotStuff 状态变量](#hotstuff-状态变量)
   - [Checkpoint 机制](#checkpoint-机制)
10. [TEE/SGX 集成](#teesgx-集成)
11. [启动与测试](#启动与测试)
    - [Server 和 Client 参数详解](#71-server-和-client-参数详解)
    - [启动日志输出详解](#72-启动日志输出详解)
    - [NORMAL 模式 HotStuff 流程日志详解](#73-normal-模式-hotstuff-流程日志详解)
    - [统计信息日志详解](#74-统计信息日志详解)
    - [启动脚本示例](#75-启动脚本示例)
12. [日志消息详解](#日志消息详解)
13. [生产环境配置](#生产环境配置)
    - [配置文件说明](#配置文件说明)
    - [测试模式 vs 生产模式差异](#测试模式-vs-生产模式差异)
    - [启用生产模式](#启用生产模式)
14. [性能监控日志](#性能监控日志)
    - [VC-VALIDATION 日志](#vc-validation-日志)
    - [VIEW-CHANGE 日志](#view-change-日志)
    - [GRACIOUS-TRANSITION 日志](#gracious-transition-日志)
15. [实验参数配置](#实验参数配置)
    - [实验方案参数说明](#实验方案参数说明)
    - [TC节点比例 λ 配置](#tc节点比例-λ-配置)
    - [委员会大小比例配置](#委员会大小比例配置)
    - [实验模式与节点行为](#实验模式与节点行为)
    - [环境变量配置方式](#环境变量配置方式)
    - [实验运行示例](#实验运行示例)
16. [常见问题](#常见问题)

---

## 概述

ResiBFT 是一个基于 HotStuff 共识框架的弹性拜占庭容错协议，支持 Trusted Replicas（可信副本）和 General Replicas（普通副本）的混合架构。通过 Verification Certificate (VC) 和 Committee 机制，系统能够在 Trusted Replicas 出现故障或恶意行为时自动切换运行模式，保证共识的安全性和活性。

### 核心特性

- **混合副本架构**：结合 Trusted Replicas（TEE保护）和 General Replicas
- **三种运行模式**：NORMAL、GRACIOUS、UNCIVIL
- **NORMAL 模式 HotStuff 3-phase**：所有副本参与投票，quorum = 2f+1
- **GRACIOUS/UNCIVIL 模式 Damysus 2-phase**：委员会投票
- **TEE/SGX 集成**：Trusted Replicas 使用 SGX Enclave 进行安全签名
- **VRF Committee 选择**：GRACIOUS 模式使用 VRF 随机选择委员会
- **Checkpoint 机制**：每 CHECKPOINT_INTERVAL 个 view 创建检查点
- **Verification Certificate**：用于检测节点状态和触发模式切换

### 论文设计实现

根据 NDSS2027 论文，ResiBFT 实现以下关键设计：

| 特性 | NORMAL 模式 | GRACIOUS/UNCIVIL 模式 |
|------|-------------|----------------------|
| 共识协议 | HotStuff 3-phase | Damysus 2-phase |
| 消息流程 | MsgPrepareProposal → MsgPrepare → MsgPrecommit → MsgCommit | MsgLdrprepare → MsgPrepare → MsgPrecommit → MsgDecide |
| 投票权 | 所有副本 (General + Trusted) | 委员会成员 (VRF 选择的 T-Replicas) |
| Quorum | q_G = 2f+1 | q_T = floor((k+1)/2) |
| 委员会选择 | 不需要 | VRF 随机选择（seed = 历史 block hashes） |
| 委员会大小 k | 不需要 | k ≥ k̄ = 30（从预设区间采样） |
| Checkpoint | 有 | 有 |
| VC 阈值 | f+1 | f+1 |

---

## 架构设计

### 整体架构图

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              ResiBFT System                                  │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │                         Client Layer                                 │    │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐               │    │
│  │  │  Client 0    │  │  Client 1    │  │  Client N    │               │    │
│  │  │ (MsgStart)   │  │(Transactions)│  │  (Reply)     │               │    │
│  │  └──────────────┘  └──────────────┘  └──────────────┘               │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
│                              │                                               │
│                              │ ClientNet                                     │
│                              ▼                                               │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │                         Replica Layer                                │    │
│  │                                                                       │    │
│  │  ┌─────────────────────┐      ┌─────────────────────┐               │    │
│  │  │   General Replica   │      │   Trusted Replicas  │               │    │
│  │  │      (Node 0)       │      │   (Nodes 1, 2, 3)   │               │    │
│  │  │                     │      │                     │               │    │
│  │  │  ┌───────────────┐  │      │  ┌───────────────┐  │               │    │
│  │  │  │  ResiBFT      │  │      │  │  ResiBFT      │  │               │    │
│  │  │  │  Core Logic   │  │      │  │  Core Logic   │  │               │    │
│  │  │  └───────────────┘  │      │  └───────────────┘  │               │    │
│  │  │                     │      │        │            │               │    │
│  │  │  No TEE/SGX         │      │        ▼            │               │    │
│  │  │  Dummy Signatures   │      │  ┌───────────────┐  │               │    │
│  │  │                     │      │  │ SGX Enclave   │  │               │    │
│  │  │                     │      │  │ (TEE Signing) │  │               │    │
│  │  │                     │      │  └───────────────┘  │               │    │
│  │  └─────────────────────┘      └─────────────────────┘               │    │
│  │                                                                       │    │
│  │  ┌───────────────────────────────────────────────────────────────┐  │    │
│  │  │                      PeerNet (P2P Network)                     │  │    │
│  │  │   MsgNewview │ MsgLdrprepare │ MsgPrepare │ MsgPrecommit │     │  │    │
│  │  │   MsgDecide │ MsgBC │ MsgVC │ MsgCommittee                     │  │    │
│  │  └───────────────────────────────────────────────────────────────┘  │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
│                                                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │                        Stage Management                              │    │
│  │                                                                       │    │
│  │    ┌─────────────┐    ┌──────────────┐    ┌───────────────┐          │    │
│  │    │   NORMAL    │───▶│   GRACIOUS   │───▶│    UNCIVIL    │          │    │
│  │    │  (Normal    │    │  (Committee  │    │  (Malicious   │          │    │
│  │    │  Consensus) │    │  Recovery)   │    │  Detection)   │          │    │
│  │    └─────────────┘    └──────────────┘    └───────────────┘          │    │
│  │         │                    │                    │                  │    │
│  │         │    TIMEOUT VC       │    REJECTED VC     │                  │    │
│  │         └────────────────────┼────────────────────┘                  │    │
│  │                              │                                        │    │
│  │         ACCEPTED VC          │                                        │    │
│  │         ◀────────────────────┘                                        │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 副本角色分配

```
┌──────────────────────────────────────────────────────────────┐
│                    Replica Roles                              │
├──────────────────────────────────────────────────────────────┤
│                                                              │
│  Node 0 (General Replica)                                    │
│  ├── 不使用 TEE/SGX                                          │
│  ├── 使用 Dummy Signatures                                   │
│  ├── 参与共识但签名验证较简单                                 │
│  └── 用于降低系统整体成本                                     │
│                                                              │
│  Node 1, 2, 3 (Trusted Replicas)                             │
│  ├── 使用 SGX Enclave                                        │
│  ├── TEE 保护签名生成                                        │
│  ├── 提供 Quorum 安全性保障                                  │
│  ├── GRACIOUS 委员会成员来源                                 │
│                                                              │
│  Quorum Sizes (论文设计):                                    │
│  ├── q_G = 2f+1 (NORMAL/UNCIVIL, 所有副本投票)              │
│  ├── q_T = floor((k+1)/2) (GRACIOUS, 委员会投票)            │
│                                                              │
│  VC Threshold: f+1                                           │
│                                                              │
│  Leader Selection: leader = view % numReplicas               │
│    - G-Replica Leader → NORMAL (HotStuff)                   │
│    - T-Replica Leader → VRF 委员会选择 → GRACIOUS           │
│                                                              │
└──────────────────────────────────────────────────────────────┘
```

---

## 目录结构

```
ResiBFT/
├── App/                          # 应用层代码
│   ├── ResiBFT.cpp               # ResiBFT 核心实现
│   ├── ResiBFT.h                 # ResiBFT 头文件
│   ├── ResiBFTBasic.cpp          # 基础工具函数
│   ├── ResiBFTBasic.h            # 基础工具函数头文件
│   │
│   ├── Server.cpp                # 服务端入口
│   ├── Client.cpp                # 客户端入口
│   │
│   ├── message.h                 # 消息类型定义
│   ├── Log.cpp                   # 日志管理
│   ├── Log.h                     # 日志管理头文件
│   │
│   ├── Block.cpp                 # 区块结构
│   ├── Block.h
│   ├── BlockCertificate.cpp      # 区块证书
│   ├── BlockCertificate.h
│   ├── Checkpoint.cpp            # 检查点
│   ├── Checkpoint.h
│   │
│   ├── VerificationCertificate.cpp  # 验证证书
│   ├── VerificationCertificate.h
│   │
│   ├── RoundData.cpp             # 轮次数据
│   ├── RoundData.h
│   ├── Justification.cpp         # 证明/证据
│   ├── Justification.h
│   │
│   ├── Accumulator.cpp           # 累加器
│   ├── Accumulator.h
│   ├── Proposal.cpp              # 提案
│   ├── Proposal.h
│   │
│   ├── Group.cpp                 # 组结构
│   ├── Group.h                   # 用于 Committee
│   │
│   ├── Sign.cpp                  # 签名
│   ├── Sign.h
│   ├── Signs.cpp                 # 签名集合
│   ├── Signs.h
│   │
│   ├── Hash.cpp                  # 哈希
│   ├── Hash.h
│   │
│   ├── Transaction.cpp           # 交易
│   ├── Transaction.h
│   │
│   ├── Node.cpp                  # 节点信息
│   ├── Node.h
│   ├── Nodes.cpp                 # 节点集合
│   ├── Nodes.h
│   │
│   ├── KeysFunctions.cpp         # 密钥函数
│   ├── KeysFunctions.h
│   │
│   ├── Statistics.cpp            # 统计信息
│   ├── Statistics.h
│   │
│   ├── types.h                   # 类型定义
│   ├── config.h                  # 配置常量
│   ├── parameters.h              # 参数配置
│   ├── production_config.h       # 生产环境配置
│   ├── experiment_config.h       # 实验参数配置 (新增)
│   │
│   └── sgx_utils/                # SGX 工具
│       ├── sgx_utils.cpp
│       ├── sgx_utils.h
│
├── Enclave/                      # SGX Enclave 代码
│   ├── Enclave.cpp               # Enclave 实现
│   ├── Enclave.h
│   ├── Enclave.edl               # Enclave 接口定义
│   ├── EnclaveBasic.cpp          # 基础函数
│   ├── EnclaveBasic.h
│   ├── EnclaveUsertypes.h        # 用户类型
│
├── salticidae/                   # 网络库
│   ├── include/                  # 头文件
│   ├── lib/                      # 库文件
│
├── Makefile                      # 构建脚本
├── config                        # 节点配置文件
│
└── docs/                         # 文档目录
    └── ResiBFT_Documentation.md  # 本文档
```

---

## 核心组件说明

### 1. ResiBFT 类 (App/ResiBFT.cpp)

核心共识实现类，包含所有共识逻辑：

```cpp
class ResiBFT {
private:
    // 基本设置
    ReplicaID replicaId;
    unsigned int numGeneralReplicas;   // 普通副本数量
    unsigned int numTrustedReplicas;   // 可信副本数量
    unsigned int numReplicas;          // 总节点数
    unsigned int numViews;             // 执行轮数
    unsigned int numFaults;            // 故障容忍数
    unsigned int trustedQuorumSize;    // 可信 Quorum 大小

    // ResiBFT 特有设置
    Epoch epoch;                       // Epoch 编号
    Stage stage;                       // 当前模式 (NORMAL/GRACIOUS/UNCIVIL)
    Group committee;                   // 当前 Committee
    bool isCommitteeMember;            // 是否为 Committee 成员

    // 状态变量
    View view;                         // 当前 View
    Log log;                           // 消息日志
    std::set<Checkpoint> checkpoints;  // 检查点集合
    std::set<VerificationCertificate> acceptedVCs;   // 接受的 VC
    std::set<VerificationCertificate> rejectedVCs;   // 拒绝的 VC
    std::set<VerificationCertificate> timeoutVCs;    // 超时 VC

public:
    // 消息处理
    void handleMsgNewviewResiBFT(MsgNewviewResiBFT msg);
    void handleMsgLdrprepareResiBFT(MsgLdrprepareResiBFT msg);
    void handleMsgPrepareResiBFT(MsgPrepareResiBFT msg);
    void handleMsgPrecommitResiBFT(MsgPrecommitResiBFT msg);
    void handleMsgDecideResiBFT(MsgDecideResiBFT msg);
    void handleMsgBCResiBFT(MsgBCResiBFT msg);
    void handleMsgVCResiBFT(MsgVCResiBFT msg);
    void handleMsgCommitteeResiBFT(MsgCommitteeResiBFT msg);

    // 模式切换
    void transitionToNormal();
    void transitionToGracious();
    void transitionToUncivil();

    // Committee 管理
    void selectCommittee();
    void revokeCommittee();
    void updateCommittee();

    // VC 检查
    bool checkAcceptedVCs();
    bool checkRejectedVCs();
    bool checkTimeoutVCs();
};
```

### 2. Log 类 (App/Log.cpp)

消息日志管理，存储所有接收到的消息：

```cpp
class Log {
private:
    // ResiBFT 消息存储
    std::map<View, std::set<MsgNewviewResiBFT>> newviewsResiBFT;
    std::map<View, std::set<MsgLdrprepareResiBFT>> ldrpreparesResiBFT;
    std::map<View, std::set<MsgPrepareResiBFT>> preparesResiBFT;
    std::map<View, std::set<MsgPrecommitResiBFT>> precommitsResiBFT;
    std::map<View, std::set<MsgDecideResiBFT>> decidesResiBFT;

    // ResiBFT 特有消息
    std::map<View, std::set<MsgVCResiBFT>> vcsResiBFT;
    std::map<View, std::set<MsgBCResiBFT>> bcsResiBFT;
    std::map<View, std::set<MsgCommitteeResiBFT>> committeesResiBFT;
    std::set<Checkpoint> checkpoints;
};
```

### 3. VerificationCertificate (App/VerificationCertificate.cpp)

验证证书，用于检测节点状态：

```cpp
class VerificationCertificate {
private:
    VCType type;       // VC_ACCEPTED / VC_REJECTED / VC_TIMEOUT
    View view;         // 相关 View
    Hash blockHash;    // 区块哈希
    ReplicaID signer;  // 签名者
    Sign sign;         // 签名
};
```

### 4. BlockCertificate (App/BlockCertificate.cpp)

区块证书，用于提交的区块证明：

```cpp
class BlockCertificate {
private:
    Block block;
    View view;
    Justification qcPrecommit;  // Precommit Quorum Certificate
    Signs qcVc;                  // VC Quorum Certificate
};
```

### 5. Group (App/Group.cpp)

组结构，用于 Committee 表示：

```cpp
class Group {
private:
    unsigned int size;
    ReplicaID group[MAX_NUM_GROUPMEMBERS];
};
```

---

## 共识流程

### NORMAL 模式 HotStuff 3-phase

NORMAL 模式使用 HotStuff 3-phase 协议，所有副本（General + Trusted）都参与投票。

```
┌─────────────────────────────────────────────────────────────────────────────┐
│              ResiBFT NORMAL Mode - HotStuff 3-phase Flow                      │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  View n 开始                                                                 │
│      │                                                                       │
│      ▼                                                                       │
│  ┌──────────────────────────────────────────────────────────────────────┐   │
│  │ Phase 1: NEWVIEW                                                      │   │
│  │                                                                        │   │
│  │  All Replicas (General + Trusted)                                     │   │
│  │      │                                                                 │   │
│  │      ▼                                                                 │   │
│  │  发送 MsgNewview(view=n, justifyHash=lockedHash)                     │   │
│  │      │                                                                 │   │
│  │      ▼                                                                 │   │
│  │  Leader (node = n % numReplicas)                                       │   │
│  │      │                                                                 │   │
│  │      ├─ G-Replica Leader: 继续 HotStuff 3-phase                       │   │
│  │      │                                                                 │   │
│  │      └─ T-Replica Leader: VRF 选择委员会 → GRACIOUS stage             │   │
│  │      │                                                                 │   │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

### GRACIOUS/UNCIVIL 模式 Damysus 2-phase

GRACIOUS 和 UNCIVIL 模式使用 Damysus 2-phase 协议，委员会成员投票。

---

## 三种运行模式

### 模式定义 (types.h)

```cpp
#define STAGE_NORMAL   0x0    // 正常共识模式
#define STAGE_GRACIOUS 0x1    // 优雅降级模式
#define STAGE_UNCIVIL  0x2    // 不文明/恶意检测模式

typedef uint8_t Stage;
```

### 日志格式说明

ResiBFT 日志采用统一格式：

```
[nodeId-view-stage]message content
```

**日志前缀说明：**
- `nodeId`: 节点ID (0=General Replica, 1-3=Trusted Replicas)
- `view`: 当前View编号
- `stage`: 当前模式 (NORMAL/GRACIOUS/UNCIVIL)

**示例：**
```
[0-1-NORMAL]Starting new view         # Node 0, View 1, NORMAL模式
[2-5-GRACIOUS]Selected committee       # Node 2, View 5, GRACIOUS模式
[1-4-UNCIVIL]Transitioning to UNCIVIL  # Node 1, View 4, UNCIVIL模式
```

---

### 1. NORMAL 模式 (正常共识)

**触发条件（论文设计）：**

- 系统初始状态
- Leader 为 G-Replica 时，使用 HotStuff 3-phase，所有副本投票
- 当 Leader 轮换到 T-Replica 时，触发 VRF 委员会选择 → 进入 GRACIOUS

**行为：**

- 所有副本（General + Trusted）参与投票
- Leader 按轮换规则选举：`leader = view % numReplicas`
- Quorum = 2f+1（所有副本签名）
- 使用 HotStuff safeNode 规则

**VC 处理（论文设计）：**
```cpp
// ACCEPTED VC: ≥ f+1 → commit 前一个 block
// 不是用于恢复到 NORMAL 模式，而是用于 commit block

bool ResiBFT::checkAcceptedVCs() {
    std::set<ReplicaID> uniqueAcceptedSigners;
    for (auto& vc : this->acceptedVCs) {
        if (vc.getView() == this->view || vc.getView() == this->view - 1) {
            uniqueAcceptedSigners.insert(vc.getSigner());
        }
    }

    if (uniqueAcceptedSigners.size() >= this->numFaults + 1) {  // 论文: f+1
        // Commit 前一个 block
        this->commitPreviousBlock();
        return true;
    }
    return false;
}
```

**NORMAL 模式典型日志：**

```
[0-0-NORMAL]Getting started                         # 系统启动
[0-0-NORMAL]Starting new view                       # 开始新View
[0-1-NORMAL]Initialized MsgNewview with view: 1     # 初始化Newview消息
[0-1-NORMAL]Sending: RESIBFT_MSGNEWVIEW[...] -> 1   # 发送Newview给Node 1
[0-1-NORMAL]Handling earlier messages               # 处理已接收的消息
[0-1-NORMAL]Handling MsgLdrprepare: [...]           # 处理Leader Prepare消息
[0-1-NORMAL]Verification: TEE=1, extends=1, view=1  # 验证结果(TEE验证通过)
[0-1-NORMAL]Stored block for view 1: [...]          # 存储区块
[0-1-NORMAL]Handling MsgPrepare: [...]              # 处理Prepare消息
[0-1-NORMAL]MsgPrepare signatures: SIGNS[...]       # Prepare签名集合
[0-1-NORMAL]Sending: RESIBFT_MSGPRECOMMIT[...]      # 发送Precommit消息
[0-1-NORMAL]Handling MsgDecide: [...]               # 处理Decide消息
[0-1-NORMAL]RESIBFT-EXECUTE(1/10:...)               # 执行区块(view/总轮数)
[0-1-NORMAL]STATISTICS[replicaId=0, executeViews=1] # 统计信息
```

**日志关键字解读：**

| 日志关键字 | 含义 |
|-----------|------|
| `Getting started` | 系统初始化完成 |
| `Starting new view` | 开始新的共识轮次 |
| `Initialized MsgNewview` | Leader初始化Newview消息 |
| `Sending: RESIBFT_MSGNEWVIEW -> X` | 发送Newview给节点X |
| `Handling MsgLdrprepare` | 收到Leader的Prepare提案 |
| `Verification: TEE=1` | TEE签名验证通过 |
| `Stored block for view X` | 存储View X的区块 |
| `Handling MsgPrepare` | 处理Prepare阶段消息 |
| `Sending: RESIBFT_MSGPRECOMMIT` | 发送Precommit消息 |
| `Handling MsgDecide` | 收到Decide消息，准备执行 |
| `RESIBFT-EXECUTE(X/Y:time)` | 执行区块，X=当前View，Y=总轮数 |
| `STATISTICS[...]` | 执行统计信息 |
| `timeToStop = X` | 检查是否结束 |

---

### 2. GRACIOUS 模式 (优雅降级)

**触发条件（论文设计）：**

GRACIOUS stage 在以下情况下触发：

1. **Leader 轮换到 T-Replica**：当 leader 轮换到 Trusted Replica 时，使用 VRF 选择委员会
2. **T-Replica 不可用检测**：收到 TIMEOUT VC ≥ f+1，表示 Trusted Replica 无法响应

**委员会选举（VRF）：**

```
- seed = hashes of previous lb view's committed blocks（不是 view）
- VRF 输出伪随机值 V(x) 和证明 π(x)
- k 个 V(x) 值最小的 T-Replicas 成为委员会成员
- k ≥ k̄ = 30（最小委员会大小，从预设区间采样）
```

**Quorum 大小：**
```
q_T = floor((k+1)/2)  // 委员会 quorum
```

**行为：**

- 使用 VRF (Verifiable Random Function) 随机选举 Committee（论文设计）
- Committee 成员承担共识职责
- Leader 在 Committee 内轮换：`leader = view % committeeSize`
- 非 Committee 成员不设置定时器，减少无效超时
- 使用 Damysus 2-phase 协议（MsgLdrprepare → MsgPrepare → MsgPrecommit → MsgDecide）

**VRF 委员会选择实现（论文设计）：**

```cpp
// ====== 生产环境配置 (production_config.h) ======
#define COMMITTEE_K_MIN 30           // k_bar: 最小委员会大小
#define COMMITTEE_K_MAX_DEFAULT 100  // 默认最大委员会大小
#define VRF_SEED_HISTORY_SIZE 3      // 组合的 checkpoint 数量
#define VRF_SEED_DELAY_VIEWS 30      // 种子稳定延迟

// ====== VRF Seed 计算（从历史 checkpoint hashes）======
unsigned int ResiBFT::computeVRFSeedFromCommittedBlocks(View view) {
    // 安全要求：种子必须在 committee 选择前已确定
    View minStableView = view - VRF_SEED_DELAY_VIEWS;  // 30 views delay

    std::vector<Checkpoint> recentCheckpoints;
    for (const auto& checkpoint : this->checkpoints) {
        if (checkpoint.getView() <= minStableView && checkpoint.getView() > 0) {
            recentCheckpoints.push_back(checkpoint);
        }
    }

    // 无 checkpoint 时使用 genesis hash
    if (recentCheckpoints.empty()) {
        Block genesisBlock = Block();
        return convertHashToSeed(genesisBlock.hash(), view);
    }

    // XOR 组合最近 VRF_SEED_HISTORY_SIZE (3) 个 checkpoint hashes
    std::sort(recentCheckpoints.begin(), recentCheckpoints.end(),
        [](const Checkpoint& a, const Checkpoint& b) { return a.getView() > b.getView(); });

    unsigned char combinedHash[SHA256_DIGEST_LENGTH];
    memset(combinedHash, 0, SHA256_DIGEST_LENGTH);

    size_t numToUse = std::min((size_t)VRF_SEED_HISTORY_SIZE, recentCheckpoints.size());
    for (size_t i = 0; i < numToUse; i++) {
        Hash checkpointHash = recentCheckpoints[i].hash();
        unsigned char* rawHash = checkpointHash.getHash();
        for (int j = 0; j < SHA256_DIGEST_LENGTH; j++) {
            combinedHash[j] ^= rawHash[j];
        }
    }

    // SHA256 再次哈希确保均匀分布
    unsigned char finalSeedHash[SHA256_DIGEST_LENGTH];
    SHA256(combinedHash, SHA256_DIGEST_LENGTH, finalSeedHash);

    // 转换为 unsigned int
    unsigned int seed = 0;
    for (int i = 0; i < 4; i++) {
        seed |= (finalSeedHash[i] << (8 * i));
    }
    return seed;
}

// ====== 委员会大小 k 采样 ======
unsigned int ResiBFT::sampleCommitteeSize(unsigned int seed, unsigned int numTrustedReplicas) {
    unsigned int k_min = COMMITTEE_K_MIN;  // 30
    unsigned int k_max = std::min(numTrustedReplicas, (unsigned int)COMMITTEE_K_MAX_DEFAULT);

    if (k_max < k_min) {
        return numTrustedReplicas;  // 使用所有可用 T-Replicas
    }

    unsigned int rangeSize = k_max - k_min + 1;
    unsigned int sampledK = k_min + (seed % rangeSize);
    return sampledK;
}

// ====== 委员会选择主函数 ======
Group ResiBFT::selectCommitteeByVRF(View view, unsigned int k) {
#ifdef PRODUCTION_MODE
    // 生产模式：使用历史 checkpoint hashes 计算 seed
    unsigned int seed = computeVRFSeedFromCommittedBlocks(view);
    k = sampleCommitteeSize(seed, this->numTrustedReplicas);
#else
    // 测试模式：使用 view 作为 seed
    unsigned int seed = view;
    unsigned int k_min = COMMITTEE_K_MIN;
    unsigned int k_max = this->numTrustedReplicas;
    if (k_max < k_min) {
        k = k_max;
    } else {
        k = k_min + (view % (k_max - k_min + 1));
    }
#endif

    // Trusted replicas: IDs >= numGeneralReplicas
    std::vector<ReplicaID> trustedReplicaIds;
    for (unsigned int i = this->numGeneralReplicas; i < this->numReplicas; i++) {
        trustedReplicaIds.push_back(i);
    }

    // VRF-like selection: 选择 k 个成员
    std::set<ReplicaID> selectedCommittee;
    unsigned int loopCount = 0;
    while (selectedCommittee.size() < k && selectedCommittee.size() < trustedReplicaIds.size()) {
        unsigned int randomVal = seed * seed + view * 7 + loopCount;
        unsigned int idx = randomVal % trustedReplicaIds.size();
        selectedCommittee.insert(trustedReplicaIds[idx]);
        seed = seed + 1;
        loopCount++;
        if (loopCount > k * 10) break;
    }

    return Group(selectedCommittee.size(), selectedCommittee);
}

void ResiBFT::transitionToGracious() {
    this->stage = STAGE_GRACIOUS;

    // VRF 委员会选择
    Group committeeGroup = this->selectCommitteeByVRF(this->view, COMMITTEE_K_MIN);

    this->committee = committeeGroup;
    this->committeeView = this->view;
    this->isCommitteeMember = isInCommittee(this->replicaId);
    this->committeeLeader = this->replicaId;  // 触发者成为 Leader

    // 更新 trustedQuorumSize: q_T = floor((k+1)/2)
    unsigned int committeeSize = committeeGroup.getSize();
    this->trustedQuorumSize = (committeeSize + 1) / 2;

    // 广播 Committee 消息
    MsgCommitteeResiBFT msgCommittee(committeeGroup, this->view, signs);
    this->sendMsgCommitteeResiBFT(msgCommittee, recipients);
}
```

**原有 selectCommittee 实现（基于 VC_TIMEOUT 签名者）：**
```cpp
void ResiBFT::transitionToGracious() {
    this->stage = STAGE_GRACIOUS;
    this->updateCommittee();
}

void ResiBFT::selectCommittee() {
    // 从 Trusted Replicas 中选举 Committee
    // 优先选择发送过 VC_TIMEOUT 的活跃节点
    unsigned int committeeSize = floor(this->numTrustedReplicas / 2) + 1;
    std::vector<ReplicaID> committeeMembers;

    // 基于 View 的确定性选举
    for (unsigned int i = 0; i < committeeSize; i++) {
        ReplicaID trustedId = 1 + ((this->view + i) % this->numTrustedReplicas);
        committeeMembers.push_back(trustedId);
    }

    this->committee = Group(committeeMembers);
    this->isCommitteeMember = isInCommittee(this->replicaId);

    // 广播 Committee 消息
    MsgCommitteeResiBFT msgCommittee(this->committee, this->view, signs);
    this->sendMsgCommitteeResiBFT(msgCommittee, recipients);
}

bool ResiBFT::checkTimeoutVCs() {
    // 统计当前View和上一View的唯一签名者
    std::set<ReplicaID> uniqueTimeoutSigners;
    for (auto& vc : this->timeoutVCs) {
        View vcView = vc.getView();
        if (vcView == this->view || vcView == this->view - 1) {
            uniqueTimeoutSigners.insert(vc.getSigner());
        }
    }

    unsigned int timeoutCount = uniqueTimeoutSigners.size();
    unsigned int graciousThreshold = floor(this->numTrustedReplicas / 2) + 1;

    if (timeoutCount >= graciousThreshold) {
        this->transitionToGracious();
        this->updateCommittee();
        return true;
    }
    return false;
}

unsigned int ResiBFT::getLeaderOf(View view) {
    // GRACIOUS模式下Leader是触发GRACIOUS的节点（固定）
    // 注意：当前实现不是委员会内部轮换，而是固定触发者
    if (this->stage == STAGE_GRACIOUS && this->committee.getSize() > 0) {
        return this->committeeLeader;  // 触发者成为Leader，不轮换
    }
    // NORMAL模式：标准轮换
    return view % this->numReplicas;
}

// 注意：论文设计建议委员会内部Leader轮换
// 当前实现：触发GRACIOUS的T-Replica成为固定的委员会Leader
// 未来优化：leader = committeeMembers[view % committeeSize]

void ResiBFT::setTimer() {
    // GRACIOUS模式下非Committee成员不设置定时器
    if (this->stage == STAGE_GRACIOUS && !this->isCommitteeMember) {
        return;
    }
    this->timer.del();
    this->timer.add(this->leaderChangeTime);
    this->timerView = this->view;
}
```

**GRACIOUS 模式典型日志：**

```
[0-1-NORMAL]timer ran out for view 1                # View 1定时器超时
[0-1-NORMAL]Sending: RESIBFT_MSGVC[VC[TIMEOUT,v=1,...]] # 发送TIMEOUT VC
[0-1-NORMAL]Sent VC[TIMEOUT] for view 1             # TIMEOUT VC已发送
[0-2-NORMAL]Handling MsgVC: VC[TIMEOUT,v=1,...]     # 收到TIMEOUT VC
[0-2-NORMAL]VC stored, type=TIMEOUT, timeout=1      # 存储TIMEOUT VC
[0-2-NORMAL]Handling MsgVC: VC[TIMEOUT,v=1,...]     # 收到另一个TIMEOUT VC
[0-2-NORMAL]Timeout VC threshold reached: 2 unique signers # 达到阈值(2个签名者)
[0-2-GRACIOUS]Transitioning to GRACIOUS stage       # 切换到GRACIOUS模式
[0-2-GRACIOUS]VRF selected committee for view 2: GROUP[2-3,1] # VRF 选择 Committee
[0-2-GRACIOUS]isCommitteeMember=0                   # Node 0不是Committee成员
[0-2-GRACIOUS]Sending: RESIBFT_MSGCOMMITTEE[...]    # 广播Committee消息
[0-2-GRACIOUS]Updated committee for view 2          # 更新Committee
[0-3-GRACIOUS]Not setting timer (not committee member) # 非Committee成员不设定时器
[0-3-GRACIOUS]GRACIOUS leader for view 3 is committee member 1 # View 3的Leader是Node 1
[0-3-GRACIOUS]Handling MsgLdrprepare: [...]         # 处理Committee Leader的Prepare
[0-7-GRACIOUS]RESIBFT-EXECUTE(7/10:...)             # 执行区块
[0-7-GRACIOUS]STATISTICS[executeViews = 9]          # 成功执行9轮
```

**日志关键字解读：**

| 日志关键字 | 含义 |
|-----------|------|
| `timer ran out for view X` | View X定时器超时 |
| `Sending: RESIBFT_MSGVC[VC[TIMEOUT,...]]` | 发送TIMEOUT VC消息 |
| `VC stored, type=TIMEOUT, timeout=N` | 存储TIMEOUT VC，当前总数N |
| `Timeout VC threshold reached: N unique signers` | 达到GRACIOUS阈值(N个不同签名者) |
| `Transitioning to GRACIOUS stage` | 进入GRACIOUS模式 |
| `Selected committee: GROUP[...]` | 选举出的Committee成员列表 |
| `isCommitteeMember=0/1` | 当前节点是否是Committee成员 |
| `Sending: RESIBFT_MSGCOMMITTEE[...]` | 广播Committee选举结果 |
| `Updated committee for view X` | 为View X更新Committee |
| `Not setting timer (not committee member)` | 非Committee成员跳过定时器 |
| `GRACIOUS leader for view X is committee member Y` | View X的Leader是Committee成员Y |
| `Committee accepted: GROUP[...]` | 接收到其他节点的Committee消息 |

**Committee消息格式：**
```
GROUP[2-3,1]  # Committee大小=2，成员是Node 3和Node 1
GROUP[2-2,3]  # Committee大小=2，成员是Node 2和Node 3
```

---

### 3. UNCIVIL 模式 (恶意检测)

**触发条件：**
- 收到足够 REJECTED VC (≥ floor(numTrusted/2) + 1 = 2)
- 表示 Trusted Replica 检测到恶意行为

**行为：**
- 检测到 Trusted Replica 恶意行为（如发送错误Proposal）
- 触发惩罚机制
- 系统标记恶意节点，但仍继续共识 

**代码实现：**

```cpp
void ResiBFT::transitionToUncivil() {
    this->stage = STAGE_UNCIVIL;
    // 触发恶意行为处理流程
}

bool ResiBFT::checkRejectedVCs() {
    // 统计当前View和上一View的唯一签名者
    std::set<ReplicaID> uniqueRejectedSigners;
    for (auto& vc : this->rejectedVCs) {
        View vcView = vc.getView();
        if (vcView == this->view || vcView == this->view - 1) {
            uniqueRejectedSigners.insert(vc.getSigner());
        }
    }

    unsigned int rejectedCount = uniqueRejectedSigners.size();
    unsigned int uncivilThreshold = floor(this->numTrustedReplicas / 2) + 1;

    if (rejectedCount >= uncivilThreshold) {
        this->transitionToUncivil();
        return true;
    }
    return false;
}
```

**VC消息处理逻辑：**
```cpp
void ResiBFT::handleMsgVCResiBFT(MsgVCResiBFT msgVC) {
    VCType vcType = vc.getType();
    View vcView = vc.getView();

    // VC_TIMEOUT和VC_REJECTED接受当前View或上一View的消息
    // 因为网络延迟可能导致消息晚到达
    bool shouldAcceptVC = false;
    if (vcType == VC_TIMEOUT || vcType == VC_REJECTED) {
        shouldAcceptVC = (vcView == this->view || vcView == this->view - 1);
    } else {
        shouldAcceptVC = (vcView >= this->view);
    }

    if (shouldAcceptVC) {
        // 存储并检查阈值
        if (vcType == VC_REJECTED) {
            this->rejectedVCs.insert(vc);
            this->checkRejectedVCs();  // 检查是否触发UNCIVIL
        }
    }
}
```

**UNCIVIL 模式典型日志：**

```
[0-4-GRACIOUS]Handling MsgLdrprepare: [...]          # 收到Leader Prepare
[0-4-GRACIOUS]Verification: TEE=0                    # TEE验证失败
[0-4-GRACIOUS]Sending: RESIBFT_MSGVC[VC[REJECTED,v=4,...]] # 发送REJECTED VC
[0-4-GRACIOUS]VC stored, type=REJECTED, rejected=2   # 存储REJECTED VC
[0-4-GRACIOUS]Rejected VC threshold reached: 2 unique signers # 达到阈值
[0-4-UNCIVIL]Transitioning to UNCIVIL stage          # 进入UNCIVIL模式
[0-4-UNCIVIL]VC stored, type=REJECTED, rejected=3    # 继续收集REJECTED VC
[0-4-UNCIVIL]Verification: TEE=1, leader=0           # 验证通过，标记Leader
[0-4-UNCIVIL]Stored block for view 4: [...]          # 存储区块
[0-4-UNCIVIL]Handling MsgPrepare: [...]              # 处理Prepare
[0-4-UNCIVIL]Sending: RESIBFT_MSGPRECOMMIT[...]      # 发送Precommit
[0-4-UNCIVIL]Sending: RESIBFT_MSGDECIDE[...]         # 发送Decide
[0-4-UNCIVIL]RESIBFT-EXECUTE(4/10:...)               # 执行区块
[0-4-UNCIVIL]STATISTICS[executeViews = 4]            # 执行统计
```

**日志关键字解读：**

| 日志关键字 | 含义 |
|-----------|------|
| `Verification: TEE=0` | TEE签名验证失败 |
| `Sending: RESIBFT_MSGVC[VC[REJECTED,...]]` | 发送REJECTED VC表示拒绝Proposal |
| `VC stored, type=REJECTED, rejected=N` | 存储REJECTED VC，当前总数N |
| `Rejected VC threshold reached: N unique signers` | 达到UNCIVIL阈值 |
| `Transitioning to UNCIVIL stage` | 进入UNCIVIL模式 |
| `DETECTED MALICIOUS LEADER` | 检测到恶意Leader |

**VC消息格式：**
```
VC[REJECTED,v=4,HASH[...],signer=3,SIGN[...]]  # 类型=REJECTED，View=4，签名者=3
VC[TIMEOUT,v=1,HASH[...],signer=2,SIGN[...]]   # 类型=TIMEOUT，View=1，签名者=2
VC[ACCEPTED,v=5,HASH[...],signer=1,SIGN[...]]  # 类型=ACCEPTED，View=5，签名者=1
```

---

## Verification Certificate (VC) 详细处理机制

### VC 类型定义

```cpp
#define VC_ACCEPTED  0x0   // 接受Proposal，表示节点行为正常
#define VC_REJECTED  0x1   // 拒绝Proposal，表示检测到恶意行为
#define VC_TIMEOUT   0x2   // 超时，表示节点不可用

typedef uint8_t VCType;
```

### VC 触发条件详解

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                          VC Trigger Conditions                               │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  VC_ACCEPTED 触发条件:                                                       │
│  ├── Proposal TEE验证成功                                                   │
│  ├── Block 正确扩展前一个区块 (block.extends(prevBlock))                    │
│  ├── proposeView == currentView                                             │
│  └── Leader 正确 (proposeLeader == expectedLeader)                          │
│                                                                              │
│  VC_REJECTED 触发条件:                                                       │
│  ├── Proposal TEE验证失败 (TEE签名不匹配)                                   │
│  ├── Block 无法正确扩展前一个区块                                           │
│  ├── proposeView != currentView                                             │
│  ├── Leader 不匹配                                                          │
│  └── 检测到恶意行为（如发送不一致消息）                                      │
│                                                                              │
│  VC_TIMEOUT 触发条件:                                                        │
│  ├── 定时器超时 (timer.del() + timer.add() 时间到期)                        │
│  ├── Leader 无法联系                                                        │
│  └── 网络延迟导致消息丢失                                                    │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

### VC 接受策略（处理网络延迟）

由于网络延迟，VC消息可能在View切换后才到达。系统采用不同的接受策略：

| VC类型 | 接受条件 | 原因 |
|--------|---------|------|
| VC_TIMEOUT | `vcView == currentView` 或 `vcView == currentView - 1` | 超时消息可能因网络延迟晚到达 |
| VC_REJECTED | `vcView == currentView` 或 `vcView == currentView - 1` | 恶意检测消息可能因网络延迟晚到达 |
| VC_ACCEPTED | `vcView >= currentView` | 正常接受消息，可以用于恢复 |

### 阈值计算（统计唯一签名者）

为了处理重复消息和网络延迟，阈值检查统计**唯一签名者**数量：

```cpp
// 阈值公式（论文设计）
threshold = numFaults + 1  // f+1

// 示例: numFaults = 1
// threshold = 1 + 1 = 2
// 需要来自2个不同节点的VC才能触发
```

**论文说明：**
- TIMEOUT VC ≥ f+1：表示 T-Replica 不可用
- REJECTED VC ≥ f+1：检测到恶意行为 → UNCIVIL
- ACCEPTED VC ≥ f+1：commit 前一个 block

**为什么统计唯一签名者：**
1. 防止同一节点发送多个VC造成虚假阈值
2. 跨View统计（当前View + 上一View）处理网络延迟
3. 使用 `std::set<ReplicaID>` 自动去重

### VC 处理完整流程

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                          VC Complete Processing Flow                         │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  收到 MsgVCResiBFT                                                           │
│      │                                                                       │
│      ▼                                                                       │
│  解析 VC: type, view, signer, hash                                          │
│      │                                                                       │
│      ▼                                                                       │
│  ┌────────────────────────────────────────────────────────────────┐         │
│  │ Step 1: Check shouldAcceptVC                                    │         │
│  │                                                                 │         │
│  │  VC_TIMEOUT/VC_REJECTED:                                        │         │
│  │    accept = (vcView == this->view || vcView == this->view - 1) │         │
│  │                                                                 │         │
│  │  VC_ACCEPTED:                                                   │         │
│  │    accept = (vcView >= this->view)                             │         │
│  └────────────────────────────────────────────────────────────────┘         │
│      │                                                                       │
│      ├── No: Log "Old VC discarded (view=X < current=Y)"                    │
│      │        Return                                                         │
│      │                                                                       │
│      └─ Yes: Continue                                                       │
│              │                                                               │
│              ▼                                                               │
│  ┌────────────────────────────────────────────────────────────────┐         │
│  │ Step 2: Store in appropriate set                               │         │
│  │                                                                 │         │
│  │  VC_TIMEOUT  → timeoutVCs.insert(vc)                           │         │
│  │  VC_REJECTED → rejectedVCs.insert(vc)                          │         │
│  │  VC_ACCEPTED → acceptedVCs.insert(vc)                          │         │
│  │                                                                 │         │
│  │  Log: "VC stored, type=XXX, accepted=N, rejected=M, timeout=K" │         │
│  └────────────────────────────────────────────────────────────────┘         │
│              │                                                               │
│              ▼                                                               │
│  ┌────────────────────────────────────────────────────────────────┐         │
│  │ Step 3: Check threshold (count unique signers)                 │         │
│  │                                                                 │         │
│  │  For each VC in set:                                           │         │
│  │    if vcView == this->view OR vcView == this->view - 1:        │         │
│  │      uniqueSigners.insert(vc.getSigner())                      │         │
│  │                                                                 │         │
│  │  count = uniqueSigners.size()                                  │         │
│  │  threshold = numFaults + 1  // 论文: f+1                        │         │
│  └────────────────────────────────────────────────────────────────┘         │
│              │                                                               │
│              ▼                                                               │
│  ┌────────────────────────────────────────────────────────────────┐         │
│  │ Step 4: Trigger action if threshold met                         │         │
│  │                                                                 │         │
│  │  VC_TIMEOUT  count >= f+1 → T-Replica unavailable detected     │         │
│  │  VC_REJECTED count >= f+1 → transitionToUncivil()              │         │
│  │  VC_ACCEPTED count >= f+1 → commit previous block              │         │
│  │                                                                 │         │
│  │  Log: "XXX VC threshold reached: N unique signers"             │         │
│  └────────────────────────────────────────────────────────────────┘         │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

### VC 日志分析示例

**GRACIOUS 触发完整日志序列：**
```
# View 1: Node 0 超时
[0-1-NORMAL]timer ran out for view 1
[0-1-NORMAL]Sending: RESIBFT_MSGVC[VC[TIMEOUT,v=1,HASH[...],signer=0,SIGN[...]]]
[0-1-NORMAL]Sent VC[TIMEOUT] for view 1

# View 2: 收集VC_TIMEOUT
[0-2-NORMAL]Handling MsgVC: VC[TIMEOUT,v=1,HASH[...],signer=0]
[0-2-NORMAL]VC stored, type=TIMEOUT, accepted=0, rejected=0, timeout=1
[0-2-NORMAL]Handling MsgVC: VC[TIMEOUT,v=1,HASH[...],signer=3]
[0-2-NORMAL]VC stored, type=TIMEOUT, accepted=0, rejected=0, timeout=2

# 阈值检查: 2个不同签名者 (signer=0, signer=3)
[0-2-NORMAL]Timeout VC threshold reached: 2 unique signers

# 模式切换
[0-2-GRACIOUS]Transitioning to GRACIOUS stage
[0-2-GRACIOUS]Selected committee: GROUP[2-3,1], isCommitteeMember=0
```

**UNCIVIL 触发完整日志序列：**
```
# View 4: 检测恶意行为
[0-4-GRACIOUS]Handling MsgLdrprepare: RESIBFT_MSGLDRPREPARE[...]
[0-4-GRACIOUS]Verification: TEE=1, extends=1, view=1, leader=1

# Node 0 拒绝Proposal
[0-4-GRACIOUS]Sending: RESIBFT_MSGVC[VC[REJECTED,v=4,HASH[...],signer=0]]
[0-4-GRACIOUS]VC stored, type=REJECTED, accepted=0, rejected=1, timeout=7

# 收集其他节点的REJECTED VC
[0-4-GRACIOUS]Handling MsgVC: VC[REJECTED,v=4,HASH[...],signer=3]
[0-4-GRACIOUS]VC stored, type=REJECTED, accepted=0, rejected=2, timeout=7

# 阈值检查: 2个不同签名者 (signer=0, signer=3)
[0-4-GRACIOUS]Rejected VC threshold reached: 2 unique signers

# 模式切换
[0-4-UNCIVIL]Transitioning to UNCIVIL stage
```

---

### 模式切换流程图（论文设计）

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        Stage Transition Flow                                 │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│                        ┌─────────────┐                                       │
│                        │   NORMAL    │                                       │
│                        │  (初始)     │                                       │
│                        │ HotStuff    │                                       │
│                        │ q_G = 2f+1  │                                       │
│                        └──────┬──────┘                                       │
│                               │                                              │
│               ┌───────────────┼───────────────┐                              │
│               │               │               │                              │
│               │               │               │                              │
│               ▼               │               ▼                              │
│        ┌─────────────┐        │        ┌──────────────┐                      │
│        │ Leader 轮换 │        │        │   REJECTED   │                      │
│        │ 到 T-Replica│        │        │    VC 检测   │                      │
│        │ + VRF 选择  │        │        │   ≥ f+1      │                      │
│        │ 委员会      │        │        └──────┬───────┘                      │
│        └──────┬──────┘        │               │                              │
│               │               │               │                              │
│               │               │               ▼                              │
│               │               │        ┌──────────────┐                      │
│               │               │        │    UNCIVIL   │                      │
│               │               │        │  恶意处理    │                      │
│               │               │        │ Damysus      │                      │
│               │               │        └──────┬───────┘                      │
│               │               │               │                              │
│               ▼               │               │                              │
│        ┌─────────────┐        │               │                              │
│        │   GRACIOUS  │        │               │                              │
│        │  Committee  │        │               │                              │
│        │  VRF 选举   │        │               │                              │
│        │ Damysus     │        │               │                              │
│        │ q_T = floor │        │               │                              │
│        │ ((k+1)/2)   │        │               │                              │
│        └──────┬──────┘        │               │                              │
│               │               │               │                              │
│               │               │               │                              │
│               ▼               │               │                              │
│        ┌─────────────┐        │               │                              │
│        │  ACCEPTED   │        │               │                              │
│        │  VC ≥ f+1   │        │               │                              │
│        │ Commit 前一 │        │               │                              │
│        │ Block       │        │               │                              │
│        └──────┬──────┘        │               │                              │
│               │               │               │                              │
│               ▼               │               │                              │
│        ┌─────────────┐◀───────┘───────────────┘                              │
│        │   NORMAL    │                                                       │
│        │  (继续)     │                                                       │
│        │  下一个 View│                                                       │
│        └─────────────┘                                                       │
│                                                                              │
│  注意：论文设计中 GRACIOUS 触发于 Leader 轮换到 T-Replica 时                 │
│       TIMEOUT VC ≥ f+1 用于检测 T-Replica 不可用                            │
│       ACCEPTED VC ≥ f+1 用于 commit 前一个 block                            │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 消息类型

### ResiBFT 消息头定义 (types.h)

```cpp
// NORMAL 模式 HotStuff 消息
#define HEADER_NEWVIEW_RESIBFT         0x21
#define HEADER_PREPARE_PROPOSAL_RESIBFT 0x29  // HotStuff Leader Proposal
#define HEADER_PREPARE_RESIBFT         0x23   // HotStuff Prepare Vote
#define HEADER_PRECOMMIT_RESIBFT       0x24   // HotStuff Precommit
#define HEADER_COMMIT_RESIBFT          0x30   // HotStuff Commit

// GRACIOUS/UNCIVIL 模式 Damysus 消息
#define HEADER_LDRPREPARE_RESIBFT      0x22
#define HEADER_DECIDE_RESIBFT          0x25

// ResiBFT 特有消息
#define HEADER_BC_RESIBFT              0x26   // Block Certificate
#define HEADER_VC_RESIBFT              0x27   // Verification Certificate
#define HEADER_COMMITTEE_RESIBFT       0x28   // Committee
```

### 消息结构 (message.h)

#### 1. MsgNewviewResiBFT (所有模式通用)
```cpp
struct MsgNewviewResiBFT {
    RoundData roundData;  // proposeHash, proposeView, justifyHash, justifyView, phase
    Signs signs;          // 签名集合
};
```

#### 2. MsgPrepareProposalResiBFT (NORMAL 模式 HotStuff)
```cpp
// Leader 发送 Proposal，包含 parent QC + Block
struct MsgPrepareProposalResiBFT {
    static const uint8_t opcode = HEADER_PREPARE_PROPOSAL_RESIBFT;
    Proposal<Justification> proposal;  // 包含 parent QC + block
    Signs signs;                       // Leader 签名
};
```

#### 3. MsgPrepareResiBFT (投票消息)
```cpp
struct MsgPrepareResiBFT {
    RoundData roundData;  // 轮次数据
    Signs signs;          // 投票签名
};
```

#### 4. MsgPrecommitResiBFT
```cpp
struct MsgPrecommitResiBFT {
    RoundData roundData;  // 轮次数据
    Signs signs;          // Quorum Certificate
};
```

#### 5. MsgCommitResiBFT (NORMAL 模式 HotStuff)
```cpp
// HotStuff Commit 消息，替代 MsgDecide
struct MsgCommitResiBFT {
    static const uint8_t opcode = HEADER_COMMIT_RESIBFT;
    Justification qcPrecommit;       // Precommit QC
    BlockCertificate blockCertificate;
    Signs signs;
};
```

#### 6. MsgLdrprepareResiBFT (GRACIOUS/UNCIVIL 模式)
```cpp
struct MsgLdrprepareResiBFT {
    Proposal<Accumulator> proposal;      // 提案
    BlockCertificate blockCertificate;   // 区块证书
    Signs signs;                         // 签名集合
};
```

#### 7. MsgDecideResiBFT (GRACIOUS/UNCIVIL 模式)
```cpp
struct MsgDecideResiBFT {
    Justification qcPrecommit;      // Precommit QC
    BlockCertificate blockCertificate;
    Signs signs;
};
```

#### 8. MsgVCResiBFT
```cpp
struct MsgVCResiBFT {
    VerificationCertificate vc;  // VC_ACCEPTED / VC_REJECTED / VC_TIMEOUT
    Signs signs;
};
```

#### 9. MsgBCResiBFT
```cpp
struct MsgBCResiBFT {
    BlockCertificate blockCertificate;
    Signs signs;
};
```

#### 10. MsgCommitteeResiBFT
```cpp
struct MsgCommitteeResiBFT {
    Group committee;    // Committee 成员列表 (VRF 选择)
    View view;          // 相关 View
    Signs signs;        // 签名
};
```

---

## 数据结构

### 1. RoundData (轮次数据)

```cpp
class RoundData {
    Hash proposeHash;    // 提案哈希
    View proposeView;    // 提案 View
    Hash justifyHash;    // 证明哈希
    View justifyView;    // 证明 View
    Phase phase;         // 阶段标识
};
```

### 2. Phase 定义

```cpp
#define PHASE_NEWVIEW   0x0
#define PHASE_PREPARE   0x1
#define PHASE_PRECOMMIT 0x2
#define PHASE_COMMIT    0x3
#define PHASE_DECIDE    0x8
```

### 3. Block (区块)

```cpp
class Block {
    bool set;                              // 是否有效
    Hash previousHash;                     // 前一区块哈希
    unsigned int size;                     // 交易数量
    Transaction transactions[MAX_NUM_TRANSACTIONS];
};
```

### 4. Accumulator (累加器)

用于 GRACIOUS/UNCIVIL 模式收集 MsgNewview 信息：

```cpp
class Accumulator {
    View proposeView;
    Hash justifyHash;
    View justifyView;
    unsigned int numNewviews;
};
```

### 5. Proposal (提案)

```cpp
template <typename T>
class Proposal {
    T value;              // 提案内容 (如 Accumulator 或 Justification)
    Block block;          // 区块
};
```

### HotStuff 状态变量

NORMAL 模式使用 HotStuff 安全机制，维护以下状态变量：

```cpp
class ResiBFT {
private:
    // HotStuff safety mechanism
    Justification prepareQC;    // Latest prepareQC (quorum MsgPrepare votes)
    Justification lockedQC;     // Latest lockedQC (quorum MsgPrecommit votes)
    Hash prepareHash;           // Hash of prepared block
    View prepareView;           // View of prepared block
    Hash lockedHash;            // Hash of locked block
    View lockedView;            // View of locked block

    // VRF committee selection
    std::set<ReplicaID> currentCommittee;  // Current committee members
    View committeeView;                    // View for which committee was selected
};
```

**HotStuff safeNode 规则：**

```cpp
bool ResiBFT::checkSafeNode(Justification qc) {
    // Safety: accept if qc.view > lockedView OR qc.hash == lockedHash
    // This ensures we never vote for conflicting blocks
    return (qc.getRoundData().getProposeView() > this->lockedView) ||
           (qc.getRoundData().getProposeHash() == this->lockedHash);
}
```

**HotStuff lockedQC 更新规则：**

```cpp
void ResiBFT::updateLockedQC(Justification newQC) {
    // Lock when we have prepareQC (quorum MsgPrepare votes)
    if (newQC.getRoundData().getProposeView() > this->lockedView) {
        this->lockedQC = newQC;
        this->lockedView = newQC.getRoundData().getProposeView();
        this->lockedHash = newQC.getRoundData().getProposeHash();
    }
}
```

### Checkpoint 机制

Checkpoint 用于 epoch 管理、状态同步和垃圾回收。

**Checkpoint 结构：**

```cpp
class Checkpoint {
private:
    Block block;           // 已提交的区块
    View view;             // View 编号
    Justification qcPrecommit;  // Precommit Quorum Certificate
    Signs qcVc;            // VC Quorum Certificate

public:
    // 检查 checkpoint 是否有效
    bool wellFormed(unsigned int quorumSize) const;
};
```

**Checkpoint 创建逻辑：**

```cpp
// 每 CHECKPOINT_INTERVAL 个 view 创建 checkpoint
#define CHECKPOINT_INTERVAL 10

void ResiBFT::createCheckpointResiBFT(RoundData roundData) {
    if (this->view > 0 && this->view % CHECKPOINT_INTERVAL == 0) {
        Block block = this->blocks[this->view];
        Justification qcPrecommit = this->prepareQC;
        Signs qcVc = Signs();

        Checkpoint checkpoint(block, this->view, qcPrecommit, qcVc);
        if (checkpoint.wellFormed(quorumThreshold)) {
            this->log.storeCheckpoint(checkpoint);
            this->checkpoints.insert(checkpoint);

            // 广播 MsgBC
            BlockCertificate blockCert(block, this->view, qcPrecommit, qcVc);
            MsgBCResiBFT msgBC(blockCert, signs);
            this->sendMsgBCResiBFT(msgBC, recipients);
        }
    }
}
```

**Checkpoint 存储和管理：**

```cpp
class Log {
private:
    std::set<Checkpoint> checkpoints;

public:
    unsigned int storeCheckpoint(Checkpoint checkpoint);
    std::set<Checkpoint> getCheckpoints();
    Checkpoint getHighestCheckpoint();
};
```

---

## TEE/SGX 集成

### Trusted vs General Replica 处理差异

```cpp
// Trusted Replica: 使用 TEE 签名
Signs ResiBFT::initializeMsgLdrprepareResiBFT(Proposal<Accumulator> proposal) {
    if (this->amGeneralReplicaIds()) {
        // General Replica: Dummy 签名
        Sign sign = Sign(true, this->replicaId, (unsigned char *)"dummy");
        Signs signs = Signs();
        signs.add(sign);
        return signs;
    }
    // Trusted Replica: TEE 签名
    // ... 调用 SGX Enclave ...
}

// General Replica 判断
bool ResiBFT::amGeneralReplicaIds() {
    return this->replicaId == 0;  // Node 0 是 General Replica
}

bool ResiBFT::amTrustedReplicaIds() {
    return this->replicaId >= 1 && this->replicaId < numReplicas;
}
```

### TEE 函数列表

| 函数名 | Trusted Replica | General Replica |
|--------|----------------|-----------------|
| `initializeMsgNewviewResiBFT` | TEE 签名 | Dummy 签名 |
| `initializeMsgLdrprepareResiBFT` | TEE 签名 | Dummy 签名 |
| `respondMsgLdrprepareProposalResiBFT` | TEE 签名 | Dummy 签名 |
| `saveMsgPrepareResiBFT` | TEE 签名 | Dummy 签名 |

---

## 启动与测试

### 1. 编译

```bash
# 设置 SGX 环境
source /opt/intel/sgxsdk/environment

# 编译
make SGX_MODE=SIM ResiBFTServer ResiBFTClient enclave.signed.so
```

### 2. 配置文件

创建 `config` 文件：

```bash
cat > config << 'EOF'
id=0 host=127.0.0.1 port=8760 cport=8761
id=1 host=127.0.0.1 port=8762 cport=8763
id=2 host=127.0.0.1 port=8764 cport=8765
id=3 host=127.0.0.1 port=8766 cport=8767
EOF
```

### 3. 启动服务端

```bash
mkdir -p results

# 启动所有节点
./ResiBFTServer 0 1 3 4 10 1 32 > results/node0.log 2>&1 &
./ResiBFTServer 1 1 3 4 10 1 32 > results/node1.log 2>&1 &
./ResiBFTServer 2 1 3 4 10 1 32 > results/node2.log 2>&1 &
./ResiBFTServer 3 1 3 4 10 1 32 > results/node3.log 2>&1 &

# 等待启动
sleep 3
```

**服务端参数说明：**

| 参数 | 位置 | 说明 | 示例值 |
|------|------|------|--------|
| replicaId | argv[1] | 节点ID | 0-3 |
| numGeneralReplicas | argv[2] | 普通副本数 | 1 |
| numTrustedReplicas | argv[3] | 可信副本数 | 3 |
| numReplicas | argv[4] | 总节点数 | 4 |
| numViews | argv[5] | 执行轮数 | 10 |
| numFaults | argv[6] | 故障容忍数 | 1 |
| leaderChangeTime | argv[7] | 超时时间(秒) | 32 |

### 4. 启动客户端

```bash
./ResiBFTClient 4 1 3 4 1 100 0 0
```

**客户端参数说明：**

| 参数 | 位置 | 说明 | 示例值 |
|------|------|------|--------|
| clientId | argv[1] | 客户端ID | 4 |
| numGeneralReplicas | argv[2] | 普通副本数 | 1 |
| numTrustedReplicas | argv[3] | 可信副本数 | 3 |
| numReplicas | argv[4] | 总节点数 | 4 |
| numFaults | argv[5] | 故障容忍数 | 1 |
| numTransactions | argv[6] | 交易数量 | 100 |
| sleepTime | argv[7] | 发送间隔(ms) | 0 |
| expIndex | argv[8] | 实验索引 | 0 |

### 5. 查看日志

```bash
# 查看节点日志
tail -f results/node0.log

# 查看所有节点状态
ps aux | grep ResiBFTServer

# 检查完成状态
ls results/done*
```

### 6. 停止服务

```bash
killall ResiBFTServer
```

### 7. 验证结果

```bash
# 查看统计信息
grep "STATISTICS" results/node0.log

# 查看执行轮数
grep "RESIBFT-EXECUTE" results/node0.log | wc -l
```

### 7.1 Server 和 Client 参数详解

#### Server 参数

```
./ResiBFTServer <replicaId> <numGeneralReplicas> <numTrustedReplicas> <numReplicas> <numViews> <numFaults> <leaderChangeTime>
```

| 参数 | 含义 | 示例值 | 说明 |
|------|------|--------|------|
| **replicaId** | 副本ID | `0` | 当前节点的唯一标识符，范围 `[0, numReplicas-1]` |
| **numGeneralReplicas** | General 副本数量 | `3` | General replicas 数量 = `(n-1)/3` |
| **numTrustedReplicas** | Trusted 副本数量 | `7` | Trusted replicas 数量 = `n - numGeneralReplicas` |
| **numReplicas** | 总节点数 | `10` | `n = numGeneralReplicas + numTrustedReplicas` |
| **numViews** | 执行的 view 数量 | `20` | 系统运行的 view 总数 |
| **numFaults** | 最大容错数 | `3` | `f = (n-1)/3`，系统可容忍的最大故障节点数 |
| **leaderChangeTime** | Leader 切换超时 | `10.0` | 单位：秒，超时后触发 view change |

#### Client 参数

```
./ResiBFTClient <clientId> <numGeneralReplicas> <numTrustedReplicas> <numReplicas> <numFaults> <numTransactions> <sleepTime> <experimentIndex>
```

| 参数 | 含义 | 示例值 | 说明 |
|------|------|--------|------|
| **clientId** | 客户端ID | `10` | 客户端唯一标识符，通常从 `numReplicas` 开始编号 |
| **numGeneralReplicas** | General 副本数量 | `3` | 需与 server 配置一致 |
| **numTrustedReplicas** | Trusted 副本数量 | `7` | 需与 server 配置一致 |
| **numReplicas** | 总节点数 | `10` | 需与 server 配置一致 |
| **numFaults** | 最大容错数 | `3` | 需与 server 配置一致 |
| **numTransactions** | 交易数量 | `100` | 客户端发送的交易总数 |
| **sleepTime** | 发送间隔 | `0` | 单位：毫秒，每笔交易之间的间隔 |
| **experimentIndex** | 实验索引 | `0` | 用于区分不同实验运行的日志文件 |

#### Quorum 大小计算

系统根据参数自动计算 quorum 阈值：

```cpp
// NORMAL 模式 quorum
generalQuorumSize = numReplicas - numFaults;     // q_G = n - f = 2f + 1

// GRACIOUS 模式委员会大小和 quorum
k = std::min(30, numTrustedReplicas);           // 委员会大小（测试环境适配）
trustedQuorumSize = (committeeSize + 1) / 2;    // q_T = floor((k+1)/2)

// UNCIVIL 触发阈值（根据委员会大小动态调整）
vcThreshold = (committeeSize / 2) + 1;          // GRACIOUS模式下
vcThreshold = numFaults + 1;                    // 其他模式下
```

**示例配置 (n=10, f=3, numTrustedReplicas=4)**:
- General replicas: `6`（ID 0-5）
- Trusted replicas: `4`（ID 6-9）
- 委员会大小 `k = min(30, 4) = 4`
- `generalQuorumSize = 10 - 3 = 7` (NORMAL 模式，所有副本投票需要 7 票)
- `trustedQuorumSize = (4+1)/2 = 2` (GRACIOUS 模式，委员会投票需要 2 票)
- UNCIVIL触发阈值 = `floor(4/2)+1 = 3` (需要 3 个 VC_REJECTED)

#### 参数关系图

```
n = numReplicas = numGeneralReplicas + numTrustedReplicas
f = numFaults = (n-1)/3

General replicas: ID ∈ [0, numGeneralReplicas-1]
Trusted replicas: ID ∈ [numGeneralReplicas, numReplicas-1]

Quorum thresholds:
├── NORMAL mode:    q_G = n - f = 2f + 1 (所有副本投票)
├── GRACIOUS mode:  k = min(30, numTrustedReplicas)
│                   q_T = floor((k+1)/2) (委员会投票)
├── UNCIVIL trigger: floor(k/2) + 1 (GRACIOUS模式下)
│                    f + 1 (其他模式)
└── VC threshold:   根据模式动态调整
```

**测试环境配置示例 (10节点)**:
```
numGeneralReplicas = 6  → G-Replica ID: 0-5
numTrustedReplicas = 4  → T-Replica ID: 6-9

委员会大小 k = 4
委员会成员 = {6, 7, 8, 9}

Quorum:
├── NORMAL:    q_G = 7 (所有副本)
├── GRACIOUS:  q_T = 2 (委员会)
└── UNCIVIL:   触发阈值 = 3 (VC_REJECTED)
```

#### 配置文件格式

`config` 文件定义节点网络配置：

```
id:<node_id> host:<ip_address> port:<server_port> port:<client_port>
```

**示例**:
```
id:0 host:47.86.232.213 port:8760 port:9760
id:1 host:47.86.232.213 port:8761 port:9761
id:2 host:47.86.232.213 port:8762 port:9762
...
id:9 host:47.86.232.213 port:8769 port:9769
```

- 第一个 port: 节点间通信端口 (PeerNet, P2P)
- 第二个 port: 客户端通信端口 (ClientNet)

### 7.2 启动日志输出详解

#### Server 启动日志示例

启动命令：`./ResiBFTServer 0 3 7 10 20 3 10`

```
[SERVER0]replicaId = 0
[SERVER0]numGeneralReplicas = 3
[SERVER0]numTrustedReplicas = 7
[SERVER0]numReplicas = 10
[SERVER0]numViews = 20
[SERVER0]numFaults = 3
[SERVER0]leaderChangeTime = 10
[SERVER0]Checking private key
[SERVER0]Checked private key
[SERVER0]Checking public keys
[SERVER0]Checked public keys
[SERVER0]Creating Enclave
[SERVER0]Enclave created successfully, eid=0x7f8...
[SERVER0]Initializing ResiBFT
[SERVER0]ResiBFT initialized
[SERVER0]Setting up timer
[SERVER0]Timer set up
[SERVER0]Starting network
[SERVER0]Network started on port 8760
[SERVER0]Waiting for client connections...
```

**日志解析：**
| 日志行 | 含义 |
|--------|------|
| `replicaId = 0` | 当前节点 ID 为 0 |
| `numGeneralReplicas = 3` | General 副本数量 3（ID: 0,1,2） |
| `numTrustedReplicas = 7` | Trusted 副本数量 7（ID: 3-9） |
| `numReplicas = 10` | 总节点数 10 |
| `numViews = 20` | 将执行 20 个 view |
| `numFaults = 3` | 最大容错数 f=3 |
| `leaderChangeTime = 10` | 10秒超时触发 view change |
| `Enclave created successfully` | SGX Enclave 初始化成功 |
| `Network started on port 8760` | P2P 网络端口 8760 |

#### Client 启动日志示例

启动命令：`./ResiBFTClient 10 3 7 10 3 100 0 0`

```
[CLIENT10]clientId = 10
[CLIENT10]numGeneralReplicas = 3
[CLIENT10]numTrustedReplicas = 7
[CLIENT10]numReplicas = 10
[CLIENT10]numFaults = 3
[CLIENT10]numClientTransactions = 100
[CLIENT10]sleepTime = 0
[CLIENT10]experimentIndex = 0
[CLIENT10]quorumSize = 7
[CLIENT10]Loading config file
[CLIENT10]Config loaded, 10 nodes found
[CLIENT10]Connecting to nodes...
[CLIENT10]Connected to node 0 at 47.86.232.213:9760
[CLIENT10]Connected to node 1 at 47.86.232.213:9761
...
[CLIENT10]All connections established
[CLIENT10]Sending transactions...
[CLIENT10]Sent transaction 0
[CLIENT10]Sent transaction 1
...
[CLIENT10]All transactions sent, waiting for replies...
```

**日志解析：**
| 日志行 | 含义 |
|--------|------|
| `clientId = 10` | 客户端 ID 10（从 numReplicas 开始编号） |
| `numClientTransactions = 100` | 将发送 100 笔交易 |
| `sleepTime = 0` | 发送间隔 0ms（连续发送） |
| `experimentIndex = 0` | 实验索引 0，用于日志文件命名 |
| `quorumSize = 7` | Client 侧 quorum 阈值 = n - f |
| `Connected to node X` | 成功连接到节点 X |

### 7.3 NORMAL 模式 HotStuff 流程日志详解

启动命令（10节点测试）：`./ResiBFTServer 0-9 各启动`

```
[0-0-NORMAL]Getting started
[0-0-NORMAL]Starting timer for view 0, timeout=10s
[0-0-NORMAL]Waiting for transactions...
[0-0-NORMAL]Received transaction from client 10: TRANS[0]
[0-0-NORMAL]Received transaction from client 10: TRANS[1]
...
[0-1-NORMAL]Starting new view
[0-1-NORMAL]Initialized MsgNewview with view: 1
[0-1-NORMAL]Sending: RESIBFT_MSGNEWVIEW[ROUNDDATA[HASH[],1,HASH[],0,PHASE_NEWVIEW],SIGNS[1-SIGN[1,0]]]
[0-1-NORMAL]Handling MsgNewview from replica 0
[0-1-NORMAL]Handling MsgNewview from replica 1
...
[0-1-NORMAL]MsgNewview count: 7, quorum reached (threshold=7)
[0-1-NORMAL]Leader is replica 1, I am leader: true
[0-1-NORMAL]Initiating MsgPrepareProposal as leader
[0-1-NORMAL]Sending: RESIBFT_MSGPREPAREPROPOSAL[PROPOSAL[QC[...],BLOCK[...]],SIGNS[...]]
```

**阶段 1: PrepareProposal (Leader 发送提案)**

```
[0-1-NORMAL]Handling MsgPrepareProposal from leader 1
[0-1-NORMAL]Proposal block: BLOCK[set=1,prevHash=HASH[abc123],size=10,{TRANS[0-9]}]
[0-1-NORMAL]SafeNode check: qc.view=0 > lockedView=0, ACCEPTED
[0-1-NORMAL]Stored block for view 1
[0-1-NORMAL]Voting with MsgPrepare (all replicas vote)
[0-1-NORMAL]Sending: RESIBFT_MSGPREPARE[ROUNDDATA[HASH[block1],1,HASH[],0,PHASE_PREPARE],SIGNS[1-SIGN[1,0]]]
```

**日志解析：**
- `SafeNode check`: HotStuff 安全检查，接受 view > lockedView 的提案
- `Voting with MsgPrepare`: 所有副本（包括 General 和 Trusted）都投票
- `SIGNS[1-SIGN[1,0]]`: 单个签名，节点 0 的投票

**阶段 2: Prepare (Leader 收集投票)**

```
[0-1-NORMAL]Handling MsgPrepare from replica 0
[0-1-NORMAL]MsgPrepare count: 1
[0-1-NORMAL]Handling MsgPrepare from replica 1
[0-1-NORMAL]MsgPrepare count: 2
...
[0-1-NORMAL]Handling MsgPrepare from replica 6
[0-1-NORMAL]MsgPrepare count: 7, quorum reached (threshold=7)
[0-1-NORMAL]Leader aggregating Prepare votes -> MsgPrecommit
[0-1-NORMAL]Aggregated signatures: SIGNS[7-SIGN[1,0]SIGN[1,1]SIGN[1,2]SIGN[1,3]SIGN[1,4]SIGN[1,5]SIGN[1,6]]
[0-1-NORMAL]Sending: RESIBFT_MSGPRECOMMIT[ROUNDDATA[...],SIGNS[7-SIGN[...]]]
```

**日志解析：**
- `MsgPrepare count: 7`: 收到 7 个投票
- `quorum reached (threshold=7)`: 达到 quorum（n-f=7）
- `Aggregated signatures: 7`: 聚合了 7 个节点的签名
- 这证明所有副本（General 0-2 + Trusted 3-6）都参与了投票

**阶段 3: Precommit (Replica 锁定)**

```
[0-1-NORMAL]Handling MsgPrecommit from leader 1
[0-1-NORMAL]Precommit QC signatures: 7, quorum verified
[0-1-NORMAL]Updating lockedQC: view=1, hash=HASH[block1]
[0-1-NORMAL]Locked: prepareQC becomes lockedQC
[0-1-NORMAL]Voting with MsgPrecommit
[0-1-NORMAL]Sending: RESIBFT_MSGPRECOMMIT[ROUNDDATA[...],SIGNS[1-SIGN[...]]]
```

**HotStuff 锁定机制：**
- `Updating lockedQC`: 收到 precommit QC 后，节点锁定该区块
- 这是 HotStuff 的关键安全机制：一旦锁定，不会再投票给冲突的区块

**阶段 4: Commit (Leader 聚合 precommit votes)**

```
[0-1-NORMAL]Handling MsgPrecommit vote from replica 0
[0-1-NORMAL]MsgPrecommit votes count: 1
...
[0-1-NORMAL]MsgPrecommit votes count: 7, quorum reached
[0-1-NORMAL]Leader aggregating Precommit votes -> MsgCommit
[0-1-NORMAL]Sending: RESIBFT_MSGCOMMIT[QC[...],SIGNS[7-SIGN[...]]]
```

**阶段 5: Execute**

```
[0-1-NORMAL]Handling MsgCommit from leader 1
[0-1-NORMAL]Commit QC signatures: 7, quorum verified
[0-1-NORMAL]Executing block for view 1
[0-1-NORMAL]RESIBFT-EXECUTE(1/20:BLOCK[set=1,size=10])
[0-1-NORMAL]Block executed, 10 transactions committed
[0-1-NORMAL]Starting new view 2
[0-1-NORMAL]Reset timer for view 2
```

**完整流程总结：**

```
MsgNewview(quorum=7) → MsgPrepareProposal → MsgPrepare(votes=7) → MsgPrecommit → MsgPrecommit(votes=7) → MsgCommit → EXECUTE
```

### 7.4 统计信息日志详解

执行完成后输出的统计信息：

```
[0-20-NORMAL]STATISTICS[
  executeViews = 26,
  executedTransactions = 260,
  numTrans = 10,
  numBlocks = 26,
  throughput = 10.4 Kops,
  latency = 96.15 ms,
  startTime = 2026-04-27 10:30:00,
  endTime = 2026-04-27 10:30:25
]
[0-20-NORMAL]DONE - Printing statistics
[0-20-NORMAL]Creating done file: results/done-0-26
```

**统计项含义：**
| 统计项 | 含义 | 示例值 |
|--------|------|--------|
| `executeViews` | 成功执行的 view 数量 | 26 |
| `executedTransactions` | 执行的交易总数 | 260 |
| `numTrans` | 每个 block 的平均交易数 | 10 |
| `numBlocks` | 执行的 block 数量 | 26 |
| `throughput` | 吞吐量（每秒操作数） | 10.4 Kops |
| `latency` | 平均延迟 | 96.15 ms |
| `startTime/endTime` | 执行时间范围 | 10:30:00 - 10:30:25 |

### 7.5 启动脚本示例

**Server 批量启动脚本 (run_resibft_server.sh)**:

```bash
#!/bin/bash
NUM_GENERAL=3
NUM_TRUSTED=7
NUM_ALL=10
NUM_VIEWS=20
NUM_FAULTS=3
LEADER_CHANGE_TIME=10

mkdir -p results

for i in {0..9}; do
    ./ResiBFTServer $i $NUM_GENERAL $NUM_TRUSTED $NUM_ALL $NUM_VIEWS $NUM_FAULTS $LEADER_CHANGE_TIME > results/node$i.log 2>&1 &
    echo "Started node $i"
done

echo "All nodes started, waiting for initialization..."
sleep 3

# 检查节点状态
ps aux | grep ResiBFTServer | grep -v grep | wc -l
```

**Client 启动脚本 (run_resibft_client.sh)**:

```bash
#!/bin/bash
CLIENT_ID=10
NUM_GENERAL=3
NUM_TRUSTED=7
NUM_ALL=10
NUM_FAULTS=3
NUM_TRANSACTIONS=100
SLEEP_TIME=0
EXP_INDEX=0

./ResiBFTClient $CLIENT_ID $NUM_GENERAL $NUM_TRUSTED $NUM_ALL $NUM_FAULTS $NUM_TRANSACTIONS $SLEEP_TIME $EXP_INDEX
```

### 8. 测试三种模式

#### 测试环境准备

```bash
# 创建脚本文件
cat > App_node0.sh << 'EOF'
#!/bin/bash
./ResiBFTServer 0 1 3 4 10 1 32
EOF

cat > App_node1.sh << 'EOF'
#!/bin/bash
./ResiBFTServer 1 1 3 4 10 1 32
EOF

cat > App_node2.sh << 'EOF'
#!/bin/bash
./ResiBFTServer 2 1 3 4 10 1 32
EOF

cat > App_node3.sh << 'EOF'
#!/bin/bash
./ResiBFTServer 3 1 3 4 10 1 32
EOF

cat > App_client.sh << 'EOF'
#!/bin/bash
./ResiBFTClient 4 1 3 4 1 100 0 0
EOF

chmod +x App_node*.sh App_client.sh
```

#### NORMAL 模式测试

**测试方法：**
```bash
# 清理环境
killall ResiBFTServer 2>/dev/null
rm -f results/done-*

# 启动所有节点（正常模式）
mkdir -p results
./App_node1.sh > results/node1.log 2>&1 &
./App_node2.sh > results/node2.log 2>&1 &
./App_node3.sh > results/node3.log 2>&1 &
./App_node0.sh > results/node0.log 2>&1 &
sleep 2
./App_client.sh
```

**预期日志输出：**
```
[0-0-NORMAL]Getting started
[0-1-NORMAL]Starting new view
[0-1-NORMAL]Initialized MsgNewview with view: 1
[0-1-NORMAL]Handling MsgLdrprepare: RESIBFT_MSGLDRPREPARE[...]
[0-1-NORMAL]Verification: TEE=1, extends=1, view=1
[0-1-NORMAL]Stored block for view 1: BLOCK[...]
[0-1-NORMAL]RESIBFT-EXECUTE(1/10:...)
[0-2-NORMAL]Starting new view
...
[0-10-NORMAL]RESIBFT-EXECUTE(10/10:...)
[0-10-NORMAL]timeToStop = 1; numViews = 10
[0-10-NORMAL]DONE - Printing statistics
```

**验证方法：**
```bash
# 检查模式标记
grep "NORMAL" results/node0.log | head -5

# 检查执行轮数
grep "executeViews" results/node0.log

# 检查完成状态
ls results/done-*
```

#### GRACIOUS 模式测试

**测试方法（方法一：使用环境变量模拟超时）：**
```bash
# 清理环境
killall ResiBFTServer 2>/dev/null
rm -f results/done-*

# 启动节点（使用超时模拟）
mkdir -p results
RESIBFT_SIMULATE_TIMEOUT=1 ./App_node1.sh > results/node1.log 2>&1 &
RESIBFT_SIMULATE_TIMEOUT=1 ./App_node2.sh > results/node2.log 2>&1 &
RESIBFT_SIMULATE_TIMEOUT=1 ./App_node3.sh > results/node3.log 2>&1 &
./App_node0.sh > results/node0.log 2>&1 &
sleep 2
./App_client.sh
```

**测试方法（方法二：只启动部分节点）：**
```bash
# 清理环境
killall ResiBFTServer 2>/dev/null
rm -f results/done-*

# 只启动Node 0和Node 1，其他节点不启动
mkdir -p results
./App_node1.sh > results/node1.log 2>&1 &
./App_node0.sh > results/node0.log 2>&1 &
sleep 2
./App_client.sh

# 系统会因为Node 2, 3不响应而超时
# 超时后发送VC_TIMEOUT，达到阈值后进入GRACIOUS
```

**预期日志输出：**
```
[0-1-NORMAL]timer ran out for view 1
[0-1-NORMAL]Sending: RESIBFT_MSGVC[VC[TIMEOUT,v=1,...]]
[0-2-NORMAL]Handling MsgVC: VC[TIMEOUT,v=1,...]
[0-2-NORMAL]VC stored, type=TIMEOUT, timeout=1
[0-2-NORMAL]Timeout VC threshold reached: 2 unique signers
[0-2-GRACIOUS]Transitioning to GRACIOUS stage
[0-2-GRACIOUS]Selected committee: GROUP[2-3,1]
[0-2-GRACIOUS]Sending: RESIBFT_MSGCOMMITTEE[...]
[0-3-GRACIOUS]Not setting timer (not committee member)
[0-3-GRACIOUS]GRACIOUS leader for view 3 is committee member 1
[0-7-GRACIOUS]RESIBFT-EXECUTE(7/10:...)
[0-9-GRACIOUS]RESIBFT-EXECUTE(9/10:...)
[0-9-GRACIOUS]STATISTICS[executeViews = 9]
```

**关键日志含义：**
- `timer ran out`: 定时器超时，节点未收到有效响应
- `VC[TIMEOUT]`: 发送超时VC，表示Trusted Replica不可用
- `threshold reached: 2 unique signers`: 收集到2个不同节点的TIMEOUT VC
- `Transitioning to GRACIOUS`: 进入GRACIOUS模式
- `Selected committee`: 选举出替代故障节点的Committee
- `Not setting timer`: 非Committee成员不设置定时器（避免无效轮换）
- `GRACIOUS leader is committee member X`: Leader从Committee中选择

**验证方法：**
```bash
# 检查GRACIOUS模式
grep "GRACIOUS" results/node0.log | head -10

# 检查Committee选举
grep "Selected committee" results/node0.log

# 检查执行轮数（GRACIOUS模式下应该能有效执行）
grep "executeViews" results/node0.log
```

#### UNCIVIL 模式测试

**测试方法（10节点配置，节点6模拟恶意）：**
```bash
# 清理环境
killall ResiBFTServer 2>/dev/null
mkdir -p results

# 启动 G-Replicas (节点 0-5)
for i in {0..5}; do
    ./ResiBFTServer $i 6 4 10 20 3 100 > results/node${i}.log 2>&1 &
done

# 节点 6 设置恶意模拟环境变量
export RESIBFT_SIMULATE_MALICIOUS=1
./ResiBFTServer 6 6 4 10 20 3 100 > results/node6.log 2>&1 &
unset RESIBFT_SIMULATE_MALICIOUS

# 节点 7-9 正常启动
for i in {7..9}; do
    ./ResiBFTServer $i 6 4 10 20 3 100 > results/node${i}.log 2>&1 &
done

# 等待启动
sleep 2

# 启动客户端
./ResiBFTClient 0 6 4 10 3 100 1000 1 > results/client.log 2>&1
```

**环境变量说明：**
- `RESIBFT_SIMULATE_MALICIOUS=1`: 设置节点6为模拟恶意节点
- 节点6在GRACIOUS模式下发送的Proposal会带`malicious=true`标记
- 其他T-Replica（节点7,8,9）检测到恶意标记后发送VC_REJECTED
- 当VC_REJECTED达到阈值时触发UNCIVIL

**配置参数说明：**
| 参数 | 值 | 说明 |
|------|-----|------|
| numGeneralReplicas | 6 | G-Replica数量（ID 0-5） |
| numTrustedReplicas | 4 | T-Replica数量（ID 6-9） |
| numReplicas | 10 | 总节点数 |
| numViews | 20 | 执行view数量 |
| numFaults | 3 | 故障容忍数f=3 |

**阈值计算：**
- 委员会大小 `k = 4`（测试环境适配，min(30, numTrustedReplicas)）
- 委员会quorum `q_T = floor((4+1)/2) = 2`
- UNCIVIL触发阈值 = `floor(4/2)+1 = 3`

**完整流程日志示例（节点7视角）：**
```
# Phase 1: NORMAL → GRACIOUS
[7-5-NORMAL]Leader rotating to T-Replica 6 - will trigger VRF committee selection
[7-6-NORMAL]Sending: RESIBFT_MSGNEWVIEW[...] -> 6
[7-6-NORMAL]Handling MsgCommittee: RESIBFT_MSGCOMMITTEE[GROUP[4-6,7,8,9],6,SIGNS[...]]
[7-6-GRACIOUS]Transitioning to GRACIOUS stage (received MsgCommittee)
[7-6-GRACIOUS]Committee accepted: k=4, q_T=2, committee=GROUP[4-6,7,8,9]

# Phase 2: 恶意检测
[7-6-GRACIOUS]Handling MsgLdrprepare: RESIBFT_MSGLDRPREPARE[...malicious=true]
[7-6-GRACIOUS]DETECTED MALICIOUS MARK: rejecting proposal from leader 6
[7-6-GRACIOUS]Verification: TEE=0, extends=1 (blockPrevHash=..., prepareHash=...)
[7-6-GRACIOUS]MsgLdrprepare verification FAILED: TEE
[7-6-GRACIOUS]Sending: RESIBFT_MSGVC[VC[REJECTED,v=6,...signer=7]]
[7-6-GRACIOUS]Sent VC[REJECTED] for view 6

# Phase 3: UNCIVIL 触发
[7-6-GRACIOUS]Handling MsgVC: VC[REJECTED,v=6,...signer=8]
[7-6-GRACIOUS]VC stored, type=REJECTED, accepted=0, rejected=2, timeout=0
[7-6-GRACIOUS]Handling MsgVC: VC[REJECTED,v=6,...signer=9]
[7-6-GRACIOUS]Rejected VC threshold reached: 3 unique signers (≥ 3) - transitioning to UNCIVIL
[7-6-UNCIVIL]Transitioning to UNCIVIL stage

# Phase 4: View 6 超时，切换 View 7
[7-6-UNCIVIL]timer ran out for view 6
[7-6-UNCIVIL]Sending: RESIBFT_MSGVC[VC[TIMEOUT,v=6,...]]
[7-6-UNCIVIL]Starting new view
[7-6-UNCIVIL]Initialized MsgNewview with view: 7

# Phase 5: View 7 共识（UNCIVIL模式）
[7-7-UNCIVIL]initiateMsgNewviewResiBFT: msgNewviews.size()=2, trustedQuorumSize=2
[7-7-UNCIVIL]Initiating MsgLdrprepare from MsgNewview
[7-7-UNCIVIL]Creating MsgLdrprepare proposal: malicious=0, view=7
[7-7-UNCIVIL]Sent MsgLdrprepare to replicas: RESIBFT_MSGLDRPREPARE[...malicious=false]
[7-7-UNCIVIL]Handling MsgPrepare: RESIBFT_MSGPREPARE[...]
[7-7-UNCIVIL]Leader reached quorum (2 signatures), initiating MsgPrepare broadcast
[7-7-UNCIVIL]RESIBFT-EXECUTE(7/20:...) STATISTICS[executeViews = 7]
```

**关键日志含义：**
| 日志 | 含义 |
|------|------|
| `malicious=true` | 节点6发送的提案带恶意标记 |
| `DETECTED MALICIOUS MARK` | 检测到恶意行为，拒绝提案 |
| `TEE=0` | TEE验证失败（模拟恶意导致） |
| `VC[REJECTED]` | 发送拒绝VC，表示检测到恶意 |
| `Rejected VC threshold reached: 3 ≥ 3` | 达到UNCIVIL触发阈值 |
| `timer ran out for view 6` | View 6超时（恶意导致无法完成） |
| `malicious=0, view=7` | View 7恢复正常，不再恶意 |

**验证方法：**
```bash
# 检查恶意模拟启用
grep "SIMULATE_MALICIOUS" results/node6.log

# 检查恶意标记
grep "malicious=true" results/node6.log

# 检查其他节点检测到恶意
grep "MALICIOUS MARK" results/node7.log results/node8.log results/node9.log

# 检查UNCIVIL触发
grep "UNCIVIL" results/node*.log

# 检查阈值触发
grep "Rejected VC threshold" results/node*.log

# 检查共识继续（View 7执行）
grep "EXECUTE.*7" results/node*.log
```

**预期验证结果：**
```
# 节点6显示恶意模拟启用
results/node6.log:[6-0-NORMAL]SIMULATE_MALICIOUS mode enabled (will trigger UNCIVIL)

# 节点7,8,9都检测到恶意并进入UNCIVIL
results/node7.log:[7-6-GRACIOUS]DETECTED MALICIOUS MARK: rejecting proposal from leader 6
results/node8.log:[8-6-GRACIOUS]DETECTED MALICIOUS MARK: rejecting proposal from leader 6
results/node9.log:[9-6-GRACIOUS]DETECTED MALICIOUS MARK: rejecting proposal from leader 6

# 所有T-Replica进入UNCIVIL
results/node6.log:[6-6-UNCIVIL]Transitioning to UNCIVIL stage
results/node7.log:[7-6-UNCIVIL]Transitioning to UNCIVIL stage
results/node8.log:[8-6-UNCIVIL]Transitioning to UNCIVIL stage
results/node9.log:[9-6-UNCIVIL]Transitioning to UNCIVIL stage

# View 7继续共识
results/node7.log:[7-7-UNCIVIL]RESIBFT-EXECUTE(7/20:...)
```

#### 三种模式测试结果对比

| 模式 | 测试条件 | 触发条件 | 共识协议 |
|------|---------|---------|---------|
| NORMAL | G-Replica Leader | 系统初始状态 | HotStuff 3-phase |
| GRACIOUS | T-Replica Leader | Leader轮换到T-Replica | Damysus 2-phase + TEE |
| UNCIVIL | 检测恶意Leader | VC_REJECTED ≥ floor(k/2)+1 | Damysus 2-phase（无TEE） |

**ResiBFT完整测试流程总结：**

| View | Leader | 模式 | 状态 |
|------|--------|------|------|
| 0-5 | G-Replica 0-5 | NORMAL | HotStuff共识成功 |
| 6 | T-Replica 6 | GRACIOUS | 委员会共识，检测恶意 |
| 6 | T-Replica 6,7,8,9 | → UNCIVIL | VC_REJECTED触发转换 |
| 7+ | T-Replica 7+ | UNCIVIL | 继续共识（无TEE验证） |

#### 日志调试技巧

**1. 查看特定View的完整流程：**
```bash
grep "view 4" results/node0.log | grep -E "Handling|Sending|Verification"
```

**2. 查看VC消息统计：**
```bash
grep "VC stored" results/node0.log
```

**3. 查看模式切换：**
```bash
grep "Transitioning" results/node0.log
```

**4. 查看消息丢弃：**
```bash
grep "discarded" results/node0.log
```

**5. 实时监控日志：**
```bash
tail -f results/node0.log | grep -E "NORMAL|GRACIOUS|UNCIVIL"
```

---

## 日志消息详解

### 共识消息格式

**MsgNewview 格式：**
```
RESIBFT_MSGNEWVIEW[ROUNDDATA[HASH[...],view,HASH[...],prevView,PHASE_NEWVIEW],SIGNS[...]]
```
- `ROUNDDATA`: 包含提案哈希、View编号、证明哈希、阶段信息
- `SIGNS`: 签名集合

**MsgLdrprepare 格式：**
```
RESIBFT_MSGLDRPREPARE[PROPOSAL[ACCUMULATOR[...],BLOCK[...]],BC[...],SIGNS[...]]
```
- `PROPOSAL`: Leader的提案，包含Accumulator和Block
- `BC`: 区块证书，证明前一区块已提交
- `SIGNS`: Leader的签名

**MsgPrepare 格式：**
```
RESIBFT_MSGPREPARE[ROUNDDATA[HASH[...],view,HASH[...],prevView,PHASE_PREPARE],SIGNS[N-SIGN[...]...]]
```
- `SIGNS`: 包含N个签名，形成Quorum Certificate

**MsgPrecommit/MsgDecide 格式：**
类似MsgPrepare，包含轮次数据和Quorum签名

### Block 格式

```
BLOCK[set,previousHash,size,{transactions}]
```
- `set`: 是否有效(1=有效，0=空块)
- `previousHash`: 前一区块哈希
- `size`: 交易数量
- `{transactions}`: 交易列表

### 常见日志问题诊断

| 问题现象 | 日志特征 | 解决方案 |
|---------|---------|---------|
| 共识卡住 | 无`RESIBFT-EXECUTE`输出 | 检查节点网络连接 |
| 模式切换失败 | 无`Transitioning`日志 | 检查VC阈值计算 |
| 签名验证失败 | `TEE=0`或`Verification FAILED` | 检查TEE/SGX配置 |
| 消息丢弃过多 | `Discarded`大量出现 | 检查View同步 |
| 超时频繁 | `timer ran out`频繁 | 检查网络延迟或leaderChangeTime参数 |

---

## 生产环境配置

### 配置文件说明

ResiBFT 支持测试环境和生产环境两种配置模式。生产环境配置在 `App/production_config.h` 中定义。

#### production_config.h 内容

```cpp
#ifndef PRODUCTION_CONFIG_H
#define PRODUCTION_CONFIG_H

// 生产模式开关
// 取消注释以启用生产模式
// #define PRODUCTION_MODE

// 委员会配置 (论文 Section 4.2)
#define COMMITTEE_K_MIN 30           // k_bar: 最小委员会大小
#define COMMITTEE_K_MAX_DEFAULT 100  // 默认最大委员会大小

// VRF seed 配置 (论文 Section 4.1)
#define VRF_SEED_HISTORY_SIZE 3      // 组合的 checkpoint 数量
#define VRF_SEED_DELAY_VIEWS 30      // 种子稳定延迟 (3 * CHECKPOINT_INTERVAL)

#endif
```

#### 测试模式 vs 生产模式差异

| 特性 | 测试模式 | 生产模式 |
|------|---------|---------|
| VRF Seed | `view` (简化) | 历史 checkpoint hashes |
| 委员会大小 k | 固定 min(30, T-Replicas) | 从 [30, 100] 采样 |
| MAX_NUM_NODES | 211 | 1000 |
| MAX_NUM_SIGNATURES | 210 | 999 |
| DEBUG_BASIC | true | false |
| DEBUG_HELP | true | false |

#### 启用生产模式

**方式1: 修改配置文件**
```cpp
// App/production_config.h
#define PRODUCTION_MODE  // 取消注释
```

**方式2: 编译时宏定义**
```bash
make clean
CXXFLAGS="-DPRODUCTION_MODE" make SGX_MODE=SIM ResiBFTServer ResiBFTClient enclave.signed.so
```

#### 生产环境运行命令

```bash
# 100节点配置示例
./run_resibft_server.sh 0 1 33 67 100 50 33 32

# 启动客户端测试
./ResiBFTClient 100 33 67 100 33 1000 0 0
```

---

## 性能监控日志

### 新增日志类型

为监控 GRACIOUS 阶段和主节点切换的性能开销，系统新增以下日志类型：

#### 1. VC-VALIDATION 日志

记录 Verification Certificate 处理的时间消耗：

```
[VC-VALIDATION] START: type=TIMEOUT, view=5, signer=2, currentView=5, stage=GRACIOUS
[VC-VALIDATION] END: type=TIMEOUT, acceptedVCs=0, rejectedVCs=0, timeoutVCs=3, time=15 μs
[VC-VALIDATION] DISCARD: oldVC (view=3 < current=5), time=5 μs
```

**字段说明：**
| 字段 | 说明 |
|------|------|
| `type` | VC类型 (ACCEPTED/REJECTED/TIMEOUT) |
| `acceptedVCs/rejectedVCs/timeoutVCs` | 各类VC当前数量 |
| `time` | 处理时间 (微秒) |

#### 2. VIEW-CHANGE 日志

记录主节点切换的时间消耗：

```
[VIEW-CHANGE] START: oldView=5, oldStage=GRACIOUS, oldLeader=2
[VIEW-CHANGE] Leader check: newLeader=3, isTrusted=true, time=2 μs
[VIEW-CHANGE] New view: view=6, newLeader=3, amLeader=false, initTime=50 μs, timerTime=10 μs
[VIEW-CHANGE] GRACIOUS transition time=500 μs
[VIEW-CHANGE] END: view=5->6, leader=3, stage=GRACIOUS, totalTime=600 μs
```

**字段说明：**
| 字段 | 说明 |
|------|------|
| `oldView/newView` | View变化 |
| `oldLeader/newLeader` | Leader变化 |
| `isTrusted` | 新Leader是否为T-Replica |
| `initTime` | 初始化时间 |
| `timerTime` | 定时器设置时间 |
| `graciousTime` | GRACIOUS转换时间 |
| `totalTime` | 总切换时间 |

#### 3. GRACIOUS-TRANSITION 日志

记录 GRACIOUS 模式转换的详细时间分解：

```
[GRACIOUS-TRANSITION] START: view=10, fromStage=NORMAL, toStage=GRACIOUS
[GRACIOUS-TRANSITION] Calling selectCommitteeByVRF(k=30), numTrustedReplicas=67
[GRACIOUS-TRANSITION] VRF selection time=200 μs, committeeSize=45
[GRACIOUS-TRANSITION] Committee: k=45, q_T=15->23 (PRODUCTION), isMember=true
[GRACIOUS-TRANSITION] Broadcasting MsgCommittee to numRecipients=99
[GRACIOUS-TRANSITION] END: vrfTime=200 μs, broadcastTime=50 μs, totalTime=300 μs
```

**字段说明：**
| 字段 | 说明 |
|------|------|
| `vrfTime` | VRF委员会选择时间 |
| `broadcastTime` | MsgCommittee广播时间 |
| `k` | 委员会大小 |
| `q_T` | trustedQuorumSize变化 |
| `(PRODUCTION)/(TEST)` | 当前运行模式 |

### 日志搜索示例

```bash
# 搜索所有VC处理日志
grep "\[VC-VALIDATION\]" results/node*.log

# 搜索View切换时间统计
grep "\[VIEW-CHANGE\] END" results/node*.log | grep "totalTime"

# 搜索GRACIOUS转换耗时
grep "\[GRACIOUS-TRANSITION\] END" results/node*.log | grep "totalTime"

# 统计平均VRF选择时间
grep "VRF selection time" results/node*.log | awk -F'time=' '{print $2}' | awk -F' μs' '{sum+=int($1)} END {print "avg:", sum/NR, "μs"}'
```

---

## 实验参数配置

### 实验方案参数说明

根据实验方案文档，ResiBFT 支持以下实验参数配置：

#### 核心参数定义

| 参数 | 符号 | 含义 | 实验范围 |
|------|------|------|---------|
| 故障节点数 | f | 拜占庭节点数量 | 20-80 |
| 总节点数 | N = 3f+1 | 所有副本数量 | 61-241 |
| TEE节点数 | d | Trusted Replicas数量 | 20, f, 1.5f, 2f, 2.5f |
| TC节点比例 | λ | TC节点占TEE比例 | 0.1, 0.2, 0.3 |
| 委员会大小比例 | k_ratio | 委员会占TEE比例 | d/3, d/2, 2d/3, 3d/4 |
| 负载 | payload | 区块大小 | 0KB, 256KB |

#### 节点类型定义

| 类型 | 定义 | 行为 |
|------|------|------|
| **TEE节点 (T-Replica)** | `replicaId >= numGeneralReplicas` | 使用TEE签名，可进入委员会 |
| **General节点 (G-Replica)** | `replicaId < numGeneralReplicas` | 普通签名，不进入委员会 |
| **拜占庭节点** | 从所有节点随机选择 f 个 | 可能恶意行为 |
| **TC节点** | 拜占庭TEE节点中投坏票的节点 | 投坏票/沉默/发送不一致消息 |
| **沉默节点** | 每轮随机生成的临时节点 | 不响应任何消息 |

### TC节点比例 λ 配置

#### 配置说明

TC节点（Trusted Corrupt节点）是拜占庭TEE节点中主动攻击的节点：

```
TC节点数 = d × λ
TC节点 ∈ 拜占庭节点 ∩ TEE节点
```

**实验方案：λ = [0.1, 0.2, 0.3]**

#### TC节点行为模式

```cpp
// TC行为定义 (experiment_config.h)
#define TC_BEHAVIOR_BAD_VOTE      0    // 投坏票：发送VC_REJECTED
#define TC_BEHAVIOR_SILENT        1    // 沉默：不响应消息
#define TC_BEHAVIOR_INCONSISTENT  2    // 发送不一致消息
```

#### 环境变量配置

```bash
# 设置TC节点比例
RESIBFT_TC_RATIO=0.2    # 20%的TEE节点为TC节点

# 设置TC节点行为
RESIBFT_TC_BEHAVIOR=0   # 投坏票模式
```

#### 代码实现

```cpp
// experiment_config.h
double tcRatio;                    // λ = TC节点数 / TEE节点总数
std::set<ReplicaID> tcNodes;       // TC节点集合
std::set<ReplicaID> byzantineNodes; // 拜占庭节点集合

void ResiBFT::selectTCNodes() {
    // TC节点数量 = d * λ
    unsigned int numTC = (unsigned int)(numTrustedReplicas * tcRatio);

    // TC节点必须是拜占庭TEE节点
    std::set<ReplicaID> byzantineTEE;
    for (auto id : byzantineNodes) {
        if (id >= numGeneralReplicas) {  // 是TEE节点
            byzantineTEE.insert(id);
        }
    }

    // 从拜占庭TEE节点中随机选择 numTC 个
    // ...
}
```

### 委员会大小比例配置

#### 两种模式对比

| 模式 | k 计算方式 | 适用场景 |
|------|-----------|---------|
| **固定范围模式** | k ∈ [30, 100] | 生产环境 |
| **比例模式** | k = d × k_ratio | 实验方案 |

#### 实验方案比例值

```cpp
// k = d/3
RESIBFT_K_RATIO=0.33

// k = d/2
RESIBFT_K_RATIO=0.5

// k = 2d/3
RESIBFT_K_RATIO=0.67

// k = 3d/4
RESIBFT_K_RATIO=0.75
```

#### 配置文件

```cpp
// production_config.h

// 模式1：固定范围（默认）
#define COMMITTEE_K_MIN 30
#define COMMITTEE_K_MAX_DEFAULT 100

// 模式2：比例计算（实验方案）
// 启用：定义 COMMITTEE_K_RATIO_MODE 或设置 RESIBFT_K_RATIO
#define COMMITTEE_K_RATIO 0.5
```

#### 代码实现

```cpp
unsigned int ResiBFT::sampleCommitteeSize(unsigned int seed, unsigned int numTrustedReplicas)
{
    double effectiveKRatio = getKRatioFromEnv();  // 优先使用环境变量

    // k = d × kRatio
    unsigned int sampledK = (unsigned int)(numTrustedReplicas * effectiveKRatio);

    // 确保 k ≥ k_bar = 30
    if (sampledK < COMMITTEE_K_MIN && numTrustedReplicas >= COMMITTEE_K_MIN) {
        sampledK = COMMITTEE_K_MIN;
    }

    return sampledK;
}
```

### 实验模式与节点行为

#### 实验模式定义

```cpp
// experiment_config.h
#define EXP_MODE_OPTIMISTIC   0    // 乐观模式：无拜占庭节点
#define EXP_MODE_RANDOM       1    // 随机模式：拜占庭行为随机
#define EXP_MODE_ATTACK       2    // 攻击模式：拜占庭节点共谋
```

#### 各模式节点行为

| 模式 | 拜占庭节点 | TC节点 | 沉默节点 | 行为特征 |
|------|-----------|--------|---------|---------|
| **乐观模式** | 无 | 无 | 无 | 无恶意行为 |
| **随机模式** | 随机选择 | 部分投坏票 | 每轮随机生成 | 拜占庭行为随机 |
| **攻击模式** | 共谋选择 | 全部投坏票/沉默 | 按策略生成 | 目标性攻击 |

#### 乐观实验 (LAN)

```
节点总量 N=3f+1：f=[20, 30, 40]
TEE节点总量 d：d=[20, f, 1.5f, 2f, 2.5f]
负载：(0KB, 256KB)
```

#### 攻击实验 (WAN)

```
节点总量 N=3f+1：f=[30, 40, 50, 60, 70, 80]
TEE节点总量 d：d=[20, f, 1.5f, 2f, 2.5f]
TC节点比例 λ：λ=[0.1, 0.2, 0.3]
委员会数量 k：k=[d/3, d/2, 2d/3, 3d/4]
负载：(0KB, 256KB)
每次实验：200轮视图
```

### 环境变量配置方式

#### 环境变量列表

| 环境变量 | 默认值 | 说明 |
|---------|-------|------|
| `RESIBFT_TC_RATIO` | 0.2 | TC节点比例 λ |
| `RESIBFT_K_RATIO` | 0.5 | 委员会大小比例 |
| `RESIBFT_EXP_MODE` | 0 | 实验模式 (0=乐观, 1=随机, 2=攻击) |
| `RESIBFT_TC_BEHAVIOR` | 0 | TC节点行为 (0=投坏票, 1=沉默, 2=不一致) |
| `RESIBFT_SIMULATE_MALICIOUS` | 0 | 模拟恶意Leader |
| `RESIBFT_SIMULATE_TIMEOUT` | 0 | 模拟超时触发GRACIOUS |

#### 启动时日志输出

```
[EXPERIMENT-CONFIG] Initialized experiment parameters:
  tcRatio (λ) = 0.2 (TC节点占TEE比例)
  kRatio = 0.5 (委员会大小比例 k = d * kRatio)
  experimentMode = 2 (0=乐观, 1=随机, 2=攻击)
  tcBehavior = 0 (0=投坏票, 1=沉默, 2=不一致)
  numTrustedReplicas (d) = 67
  numFaults (f) = 33
[EXPERIMENT-CONFIG] Byzantine nodes (33): 5 12 18 25 ...
[EXPERIMENT-CONFIG] TC nodes (6): 45 52 58 61 65 66
```

### 实验运行示例

#### 实验配置脚本

```bash
#!/bin/bash
# experiment_config.sh - 实验参数配置脚本

# 实验参数
EXP_MODE=$1        # 0=乐观, 1=随机, 2=攻击
F=$2               # 故障节点数
D=$3               # TEE节点数
TC_RATIO=$4        # TC节点比例
K_RATIO=$5         # 委员会大小比例

export RESIBFT_EXP_MODE=$EXP_MODE
export RESIBFT_TC_RATIO=$TC_RATIO
export RESIBFT_K_RATIO=$K_RATIO
export RESIBFT_TC_BEHAVIOR=0

# 计算节点数
N=$((3 * F + 1))
GENERAL=$((N - D))

echo "Starting experiment: N=$N, f=$F, d=$D, λ=$TC_RATIO, k_ratio=$K_RATIO"

# 启动节点
for i in $(seq 0 $((N-1))); do
    ./ResiBFTServer $i $GENERAL $D $N 200 $F 32 > results/node$i.log 2>&1 &
done

# 启动客户端
./ResiBFTClient $N $GENERAL $D $N $F 1000 0 0
```

#### 不同实验运行命令

```bash
# 乐观实验 (LAN)：f=20, d=40, 无TC节点
RESIBFT_EXP_MODE=0 RESIBFT_TC_RATIO=0 RESIBFT_K_RATIO=0.5 \
  ./experiment_config.sh 0 20 40 0 0.5

# 随机实验：f=30, d=60, λ=0.2
RESIBFT_EXP_MODE=1 RESIBFT_TC_RATIO=0.2 RESIBFT_K_RATIO=0.5 \
  ./experiment_config.sh 1 30 60 0.2 0.5

# 攻击实验 (WAN)：f=50, d=100, λ=0.3, k=d/2
RESIBFT_EXP_MODE=2 RESIBFT_TC_RATIO=0.3 RESIBFT_K_RATIO=0.5 RESIBFT_TC_BEHAVIOR=0 \
  ./experiment_config.sh 2 50 100 0.3 0.5

# TC节点沉默实验
RESIBFT_EXP_MODE=2 RESIBFT_TC_RATIO=0.2 RESIBFT_TC_BEHAVIOR=1 \
  ./experiment_config.sh 2 40 80 0.2 0.67
```

#### 批量实验脚本

```bash
#!/bin/bash
# batch_experiment.sh - 批量实验脚本

# 参数组合
for F in 20 30 40; do
  for D_RATIO in 1 1.5 2 2.5; do
    for TC_RATIO in 0.1 0.2 0.3; do
      for K_RATIO in 0.33 0.5 0.67 0.75; do

        D=$((F * D_RATIO))
        N=$((3 * F + 1))
        GENERAL=$((N - D))

        echo "===== Experiment: f=$F, d=$D, λ=$TC_RATIO, k_ratio=$K_RATIO ====="

        export RESIBFT_EXP_MODE=2
        export RESIBFT_TC_RATIO=$TC_RATIO
        export RESIBFT_K_RATIO=$K_RATIO
        export RESIBFT_TC_BEHAVIOR=0

        # 运行实验
        ./run_experiment.sh

        # 等待完成
        sleep 60

      done
    done
  done
done
```

---

## 常见问题

### Q1: 为什么 General Replica 不使用 TEE？

General Replica 用于降低系统成本，在硬件资源受限场景下提供基本共识参与能力。它的签名验证较简单，但 Trusted Replicas 的 Quorum 仍然保证安全性。

### Q2: trustedQuorumSize 如何计算？

```cpp
trustedQuorumSize = numTrustedReplicas / 2 + 1
// 例如: numTrustedReplicas = 3, trustedQuorumSize = 2
```

### Q3: Committee 如何选举？

使用 VRF (Verifiable Random Function) 从 Trusted Replicas 中选择：

**生产模式：**
```cpp
// 1. 从历史 checkpoint hashes 计算 seed
seed = computeVRFSeedFromCommittedBlocks(view);

// 2. 从 [k_min, k_max] 采样委员会大小
k = sampleCommitteeSize(seed, numTrustedReplicas);  // k ∈ [30, 100]

// 3. VRF-like 随机选择 k 个 T-Replicas
selectedCommittee = VRF_select(k, seed, trustedReplicaIds);
```

**测试模式：**
```cpp
// 简化版本：使用 view 作为 seed
seed = view;
k = min(30, numTrustedReplicas);
selectedCommittee = pseudo_random_select(k, seed, trustedReplicaIds);
```

### Q4: GRACIOUS 模式 Leader 如何选择？

当前实现：**触发 GRACIOUS 的 T-Replica 成为固定的委员会 Leader**

```cpp
// ResiBFT.cpp
void ResiBFT::transitionToGracious() {
    this->committeeLeader = this->replicaId;  // 触发者成为 Leader
}

unsigned int ResiBFT::getLeaderOf(View view) {
    if (this->stage == STAGE_GRACIOUS) {
        return this->committeeLeader;  // 固定，不轮换
    }
    return view % this->numReplicas;
}
```

**注意：** 论文设计建议委员会内部 Leader 轮换，当前实现为简化版本。

### Q5: 如何判断共识完成？

```cpp
bool ResiBFT::timeToStop() {
    return this->numViews > 0 && this->view >= this->numViews;
}
```

---

## 参考

- HotStuff 论文: "HotStuff: BFT Consensus with Linearity and Responsiveness"
- ResiBFT 相关论文
- SGX/TEE 文档
- Salticidae 网络库文档