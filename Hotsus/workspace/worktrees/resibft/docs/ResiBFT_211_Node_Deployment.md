# ResiBFT 211节点部署与测试方案

## 概述

本文档描述如何在4台服务器上部署211个ResiBFT节点，并测试三种共识模式：NORMAL、GRACIOUS、UNCIVIL。

---

## 1. 节点配置

### 1.1 节点分布

假设配置：1个General Replica + 210个Trusted Replicas = 211个节点

| 服务器 | 节点数量 | 节点ID范围 | 端口范围 |
|--------|---------|-----------|----------|
| Server 1 | 53 | 0-52 | 8760-8812, 9760-9812 |
| Server 2 | 53 | 53-105 | 8813-8865, 9813-9865 |
| Server 3 | 53 | 106-158 | 8866-8918, 9866-9918 |
| Server 4 | 52 | 159-210 | 8919-8970, 9919-9970 |

### 1.2 角色分配

根据代码逻辑：
```cpp
// getGeneralReplicaIds: 从 Node 0 开始分配 numGeneralReplicas 个 General Replicas
for (unsigned int i = 0; i < this->numGeneralReplicas; i++)
{
    generalReplicaIds.push_back(i);
}
```

| 参数值 | 分配结果 |
|--------|---------|
| `numGeneralReplicas = 1` | **Node 0** → General Replica |
| `numTrustedReplicas = 210` | **Nodes 1-210** → Trusted Replicas |

### 1.3 Threshold计算

| 类型 | 公式 | 值 |
|------|------|-----|
| VC_TIMEOUT threshold | floor(210/2) + 1 | **106** |
| VC_REJECTED threshold | floor(210/2) + 1 | **106** |
| Committee size | floor(210/2) + 1 | **106** |
| Trusted quorum | floor(210/2) + 1 | **106** |

### 1.4 角色分配规则与结果

根据代码中的分配逻辑：
```cpp
// ResiBFT.cpp - getGeneralReplicaIds()
std::vector<ReplicaID> ResiBFT::getGeneralReplicaIds()
{
    std::vector<ReplicaID> generalReplicaIds;
    for (unsigned int i = 0; i < this->numGeneralReplicas; i++)
    {
        generalReplicaIds.push_back(i);  // 从Node 0开始分配
    }
    return generalReplicaIds;
}
```

**211节点配置结果：**

| 参数 | 值 | 分配结果 |
|------|-----|---------|
| `numGeneralReplicas` | 1 | **Node 0** → General Replica |
| `numTrustedReplicas` | 210 | **Nodes 1-210** → Trusted Replicas |
| `numReplicas` | 211 | 总节点数 |

**各服务器上的角色分布：**

| 服务器 | 节点ID | 角色 | 数量 |
|--------|--------|------|------|
| Server 1 | **Node 0** | **General Replica** (唯一) | 1 |
| Server 1 | Nodes 1-52 | Trusted Replicas | 52 |
| Server 2 | Nodes 53-105 | Trusted Replicas | 53 |
| Server 3 | Nodes 106-158 | Trusted Replicas | 53 |
| Server 4 | Nodes 159-210 | Trusted Replicas | 52 |

**角色功能对比：**

| 功能 | General Replica (Node 0) | Trusted Replicas (Nodes 1-210) |
|------|-------------------------|----------------------------|
| TEE验证 | ✗ 不使用TEE | ✓ 使用TEE |
| 恶意检测 | ✗ 不检测malicious | ✓ 检测malicious标记 |
| UNCIVIL触发 | ✗ 不触发UNCIVIL | ✓ 可触发UNCIVIL |
| GRACIOUS Committee | ✗ 不参与committee选择 | ✓ 可能被选为committee成员 |
| Leader角色 | ✓ 可轮换做leader | ✓ 可轮换做leader |
| 共识参与 | ✓ 正常参与共识 | ✓ 正常参与共识 |

**验证命令：**

启动后可以从日志中验证角色分配：

```bash
# Node 0日志应该显示它是General Replica（不初始化TEE）
grep "Initializing TEE" results/node0.log  # 应该没有输出
grep "General" results/node0.log | head -5

# Node 1-210日志应该显示TEE初始化（Trusted Replicas）
grep "Initializing TEE" results/node1.log  # 应该有输出
grep "Initializing TEE" results/node100.log  # 应该有输出
grep "Initializing TEE" results/node200.log  # 应该有输出
```

**重要说明：**
- 只有Node 0是General Replica，其他210个节点（Node 1-210）都是Trusted Replicas
- General Replica不参与UNCIVIL检测和GRACIOUS Committee选择
- Trusted Replicas可以使用TEE验证和检测恶意行为

---

## 2. 配置文件生成

### 2.1 单机测试配置

```bash
# 生成config_211文件（所有节点在同一台机器）
for i in {0..210}; do
    port1=$((8760 + i))
    port2=$((9760 + i))
    echo "id:$i host:127.0.0.1 port:$port1 port:$port2"
done > config_211
```

### 2.2 多服务器配置

假设四台服务器IP：
- Server 1: 192.168.1.1
- Server 2: 192.168.1.2
- Server 3: 192.168.1.3
- Server 4: 192.168.1.4

```bash
# 生成跨服务器配置
cat > config_211 << 'EOF'
# Server 1节点 (0-52)
id:0 host:192.168.1.1 port:8760 port:9760
id:1 host:192.168.1.1 port:8761 port:9761
...
id:52 host:192.168.1.1 port:8812 port:9812

# Server 2节点 (53-105)
id:53 host:192.168.1.2 port:8760 port:9760
id:54 host:192.168.1.2 port:8761 port:9761
...
id:105 host:192.168.1.2 port:8812 port:9812

# Server 3节点 (106-158)
id:106 host:192.168.1.3 port:8760 port:9760
...
id:158 host:192.168.1.3 port:8812 port:9812

# Server 4节点 (159-210)
id:159 host:192.168.1.4 port:8760 port:9760
...
id:210 host:192.168.1.4 port:8811 port:9811
EOF
```

---

## 3. NORMAL模式测试

### 3.1 测试条件

- 所有节点正常运行
- 无恶意行为
- 无节点故障

### 3.2 启动脚本

**Server 1 (deploy_normal_server1.sh):**

```bash
#!/bin/bash
mkdir -p results

echo "Starting NORMAL mode - Server 1 nodes (0-52)"

# Node 0 - General Replica
./ResiBFTServer 0 1 210 211 100 1 32 > results/node0.log 2>&1 &

# Nodes 1-52 - Trusted Replicas
for i in {1..52}; do
    ./ResiBFTServer $i 1 210 211 100 1 32 > results/node$i.log 2>&1 &
    echo "Started node $i"
done

echo "Server 1 deployment complete"
```

**Server 2 (deploy_normal_server2.sh):**

```bash
#!/bin/bash
mkdir -p results

echo "Starting NORMAL mode - Server 2 nodes (53-105)"

for i in {53..105}; do
    ./ResiBFTServer $i 1 210 211 100 1 32 > results/node$i.log 2>&1 &
    echo "Started node $i"
done

echo "Server 2 deployment complete"
```

**Server 3 (deploy_normal_server3.sh):**

```bash
#!/bin/bash
mkdir -p results

echo "Starting NORMAL mode - Server 3 nodes (106-158)"

for i in {106..158}; do
    ./ResiBFTServer $i 1 210 211 100 1 32 > results/node$i.log 2>&1 &
    echo "Started node $i"
done

echo "Server 3 deployment complete"
```

**Server 4 (deploy_normal_server4.sh):**

```bash
#!/bin/bash
mkdir -p results

echo "Starting NORMAL mode - Server 4 nodes (159-210)"

for i in {159..210}; do
    ./ResiBFTServer $i 1 210 211 100 1 32 > results/node$i.log 2>&1 &
    echo "Started node $i"
done

echo "Server 4 deployment complete"
```

**启动Client (任意服务器):**
```bash
./ResiBFTClient 211 1 210 211 1 100 0 0
```

### 3.3 验证结果

```bash
# 检查所有节点是否保持NORMAL状态
grep "NORMAL" results/node*.log | grep "RESIBFT-EXECUTE" | wc -l

# 检查是否有异常状态转换
grep -E "GRACIOUS|UNCIVIL" results/node*.log

# 统计共识完成数量
grep "RESIBFT-EXECUTE" results/node*.log | tail -20
```

**预期日志格式:**
```
[0-1-NORMAL]Getting started
[0-1-NORMAL]Starting new view
[0-1-NORMAL]RESIBFT-EXECUTE(1/100:...) STATISTICS[replicaId = 0, executeViews = 1, ...]
```

---

## 4. GRACIOUS模式测试

### 4.1 测试条件

- 部分节点超时或故障
- VC_TIMEOUT数量达到threshold (106)
- 系统进入recovery模式

### 4.2 触发方法

**方法A：Kill部分节点制造故障**

```bash
#!/bin/bash
# test_gracious_kill.sh

# 先正常启动所有节点（参考NORMAL模式脚本）
# 等待60秒让系统稳定运行
sleep 60

# Kill 111个节点（超过threshold=106）
# 这些节点将无法响应，导致超时
echo "Killing nodes 100-210 to simulate faults..."

# Server 3部分节点
for i in {100..158}; do
    pkill -f "ResiBFTServer $i" &
done

# Server 4全部节点
for i in {159..210}; do
    pkill -f "ResiBFTServer $i" &
done

wait
echo "Fault simulation complete"
```

**方法B：设置更短超时时间**

```bash
# 使用更短的leaderChangeTime（如8秒）
./ResiBFTServer $i 1 210 211 100 1 8 > results/node$i.log 2>&1 &
```

### 4.3 预期行为

根据论文Algorithm 1 Lines 16-30：

| 角色 | 行为 |
|------|------|
| Committee成员 | 运行Damysus共识，轮流做leader |
| 非Committee T-Replica | 监听BlockCertificate，更新checkpoint |
| G-Replica | 监听BlockCertificate，更新checkpoint |

**Committee选择逻辑：**
- Committee size = 106
- 从活跃节点（发送VC_TIMEOUT的节点）中选择
- 排除故障节点

### 4.4 验证结果

```bash
# 检查GRACIOUS状态转换
grep "Transitioning to GRACIOUS" results/node*.log | head -10

# 检查Committee选择
grep "Selected committee" results/node*.log | head -10

# 检查Committee成员共识
grep "committee member" results/node*.log | head -20

# 检查非Committee成员行为
grep "not committee member" results/node*.log | head -10
```

**预期日志格式:**
```
[X-Y-GRACIOUS]Timeout VC threshold reached: 106 unique signers - transitioning to GRACIOUS
[X-Y-GRACIOUS]Selected committee: GROUP[2-...], isCommitteeMember=1/0, activeReplicas=106
[X-Y-GRACIOUS]GRACIOUS leader for view Y is committee member Z
[X-Y-GRACIOUS]RESIBFT-EXECUTE(Y/100:...)
```

---

## 5. UNCIVIL模式测试

### 5.1 测试条件

- 恶意leader发送带恶意标记的proposal
- Trusted Replicas检测到恶意行为
- VC_REJECTED数量达到threshold (106)
- 系统进入UNCIVIL模式运行基本Hotstuff

### 5.2 启动脚本

**设置恶意leader:**
```bash
#!/bin/bash
# test_uncivil.sh

mkdir -p results

echo "Starting UNCIVIL mode test..."

# Node 0 - General Replica (正常，不参与恶意检测)
./ResiBFTServer 0 1 210 211 100 1 32 > results/node0.log 2>&1 &
echo "Started node 0 (General Replica)"

# Node 1 - 恶意Leader
RESIBFT_SIMULATE_MALICIOUS=1 ./ResiBFTServer 1 1 210 211 100 1 32 > results/node1.log 2>&1 &
echo "Started node 1 (Malicious Leader)"

# Nodes 2-210 - Trusted Replicas (会检测恶意)
for i in {2..210}; do
    ./ResiBFTServer $i 1 210 211 100 1 32 > results/node$i.log 2>&1 &
done

echo "All nodes started. Waiting for consensus..."
echo "When Node 1 becomes leader (view % 211 = 1), others will detect malicious behavior"
```

### 5.3 恶意检测机制

当Node 1成为leader时（view满足 `view % 211 == 1`）：

```
Node 1发送proposal:
  malicious=true  (因为RESIBFT_SIMULATE_MALICIOUS=1)

其他Trusted Replicas收到后:
  verifyProposalResiBFT()检测malicious=true
  → 返回false
  → 发送VC_REJECTED
  → 日志: "DETECTED MALICIOUS MARK: rejecting proposal from leader 1"
```

### 5.4 UNCIVIL模式行为

根据论文Algorithm 1 Lines 32-40：

| 角色 | 行为 |
|------|------|
| Leader | 检测`check accepted-vcs`失败 → delete committee → 运行基本Hotstuff |
| 其他T-Replica | 运行基本Hotstuff（不使用TEE验证） |
| G-Replica | 运行基本Hotstuff |

**重要：UNCIVIL模式下不检测malicious标记**

```cpp
// 代码逻辑
if (this->stage == STAGE_UNCIVIL)
{
    return true;  // 接受所有proposal，运行基本Hotstuff
}
```

### 5.5 验证结果

```bash
# 检查恶意检测
grep "DETECTED MALICIOUS MARK" results/node*.log | head -20

# 检查VC_REJECTED发送
grep "VC\[REJECTED" results/node*.log | head -20

# 检查UNCIVIL状态转换
grep "Transitioning to UNCIVIL" results/node*.log | head -10

# 检查UNCIVIL模式下的共识（应该能达成，因为忽略恶意标记）
grep -E "\[.*-.*-UNCIVIL\].*RESIBFT-EXECUTE" results/node*.log | head -10

# 检查UNCIVIL模式下忽略恶意标记
grep "UNCIVIL mode: ignoring malicious mark" results/node*.log | head -10
```

**预期日志格式:**
```
[X-Y-NORMAL/GRACIOUS]DETECTED MALICIOUS MARK: rejecting proposal from leader 1
[X-Y-NORMAL/GRACIOUS]Sending: RESIBFT_MSGVC[VC[REJECTED,v=Y,...]
[X-Y-NORMAL/GRACIOUS]Rejected VC threshold reached: 106 - transitioning to UNCIVIL
[X-Y-UNCIVIL]Transitioning to UNCIVIL stage
[X-Y-UNCIVIL]UNCIVIL mode: ignoring malicious mark (no TEE)
[X-Y-UNCIVIL]Verification: TEE=1 (接受proposal)
[X-Y-UNCIVIL]RESIBFT-EXECUTE(Y/100:...)
```

---

## 6. 状态转换流程图

```
┌─────────────────────────────────────────────────────────────────────┐
│                     ResiBFT状态转换流程                              │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  ┌─────────────┐                                                    │
│  │   NORMAL    │                                                    │
│  │  (初始状态) │                                                    │
│  └──────┬──────┘                                                    │
│         │                                                           │
│         ├──────────────────────────────────────────┐                │
│         │                                          │                │
│         │ 检测到恶意leader                          │ 超时           │
│         │ (VC_REJECTED ≥ 106)                      │ (VC_TIMEOUT ≥ 106)│
│         │                                          │                │
│         ▼                                          ▼                │
│  ┌─────────────┐                           ┌─────────────┐         │
│  │   UNCIVIL   │                           │   GRACIOUS  │         │
│  │ 运行基本    │                           │ Committee   │         │
│  │ Hotstuff    │                           │ Recovery    │         │
│  │ (无TEE验证) │                           │             │         │
│  └──────┬──────┘                           └──────┬──────┘         │
│         │                                          │                │
│         │ 超时                                     │ 共识达成       │
│         │ (VC_TIMEOUT ≥ 106)                      │ (checkpoint)   │
│         │                                          │                │
│         ▼                                          ▼                │
│         │         ┌─────────────┐                 │                │
│         └────────►│   GRACIOUS  │◄────────────────┘                │
│                   │  (Recovery) │                                  │
│                   └──────┬──────┘                                  │
│                          │                                         │
│                          │ Checkpoint达成一致                       │
│                          │ 发送VC_ACCEPTED                         │
│                          │                                         │
│                          ▼                                         │
│                   ┌─────────────┐                                  │
│                   │   NORMAL    │                                  │
│                   │  (恢复)     │                                  │
│                   └─────────────┘                                  │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 7. 参数说明

### 7.1 ResiBFTServer参数

```
./ResiBFTServer <replicaId> <numGeneralReplicas> <numTrustedReplicas> <numReplicas> <numViews> <numFaults> <leaderChangeTime>
```

| 参数 | 211节点配置值 | 说明 |
|------|--------------|------|
| replicaId | 0-210 | 节点ID |
| numGeneralReplicas | 1 | General Replica数量 |
| numTrustedReplicas | 210 | Trusted Replica数量 |
| numReplicas | 211 | 总节点数 |
| numViews | 100 | 运行的view数量 |
| numFaults | 1 | 预期故障数 |
| leaderChangeTime | 32 | 超时时间（秒） |

### 7.2 ResiBFTClient参数

```
./ResiBFTClient <clientId> <numGeneralReplicas> <numTrustedReplicas> <numReplicas> <numClients> <numTransactions> <timerStart> <timerStop>
```

| 参数 | 示例值 | 说明 |
|------|-------|------|
| clientId | 211 | Client ID（从节点数开始） |
| numGeneralReplicas | 1 | General Replica数量 |
| numTrustedReplicas | 210 | Trusted Replica数量 |
| numReplicas | 211 | 总节点数 |
| numClients | 1 | Client数量 |
| numTransactions | 1000 | 发送的交易数 |
| timerStart | 0 | 定时器开始 |
| timerStop | 0 | 定时器结束 |

---

## 8. 常见问题与解决方案

### 8.1 节点连接问题

**问题**：节点无法连接到其他节点

**解决**：
```bash
# 检查网络连通性
ping <server_ip>

# 检查防火墙
sudo ufw allow <port_range>
# 或
sudo iptables -A INPUT -p tcp --dport 8760:9000 -j ACCEPT
```

### 8.2 Threshold未达到

**问题**：GRACIOUS/UNCIVIL未触发

**原因**：threshold=106需要足够多的节点发送VC

**解决**：
- 增加故障节点数量（超过106个）
- 或减少numTrustedReplicas降低threshold
- 或等待更长时间让更多节点超时

### 8.3 性能问题

**问题**：211节点共识速度慢

**解决**：
- 使用更快的网络连接
- 增加leaderChangeTime减少不必要的超时
- 优化TEE调用频率

---

## 9. 日志分析脚本

```bash
#!/bin/bash
# analyze_logs.sh

echo "=== ResiBFT Log Analysis ==="

echo ""
echo "--- NORMAL Mode Check ---"
normal_count=$(grep "NORMAL" results/node*.log | grep "RESIBFT-EXECUTE" | wc -l)
echo "Normal consensus executions: $normal_count"

echo ""
echo "--- GRACIOUS Mode Check ---"
gracious_count=$(grep "GRACIOUS" results/node*.log | grep "RESIBFT-EXECUTE" | wc -l)
echo "Gracious consensus executions: $gracious_count"
grep "Selected committee" results/node*.log | head -5

echo ""
echo "--- UNCIVIL Mode Check ---"
uncivil_count=$(grep "UNCIVIL" results/node*.log | grep "RESIBFT-EXECUTE" | wc -l)
echo "Uncivil consensus executions: $uncivil_count"
grep "DETECTED MALICIOUS" results/node*.log | head -5

echo ""
echo "--- State Transitions ---"
grep "Transitioning to" results/node*.log | sort | uniq -c

echo ""
echo "--- Statistics ---"
grep "STATISTICS" results/node*.log | tail -10

echo ""
echo "=== Analysis Complete ==="
```

---

## 10. 参考文献

- Algorithm 1: High-level implementation of ResiBFT (论文Lines 1-40)
- 论文第3-4节：Protocol Description
- ResiBFT_Documentation.md：详细实现文档