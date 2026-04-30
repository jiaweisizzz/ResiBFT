# ResiBFT 四台服务器实验部署方案（独立执行版）

## 服务器配置

| 服务器 | IP | 序号 |
|--------|-----|------|
| Server 0 | 47.86.182.194 | 0 |
| Server 1 | 8.216.32.203 | 1 |
| Server 2 | 43.108.14.100 | 2 |
| Server 3 | 47.84.134.45 | 3 |

---

## 一、环境准备（所有服务器执行相同步骤）

### 1.1 编译

```bash
# 在每台服务器上执行
cd ~/ResiBFT/Hotsus
source /opt/intel/sgxsdk/environment  # 如果有SGX
make SGX_MODE=SIM ResiBFTServer ResiBFTClient ResiBFTKeys enclave.signed.so
```

### 1.2 生成密钥（所有服务器执行相同步骤）

```bash
# 在每台服务器上执行
cd ~/ResiBFT/Hotsus
mkdir -p somekeys

# 生成所有节点的密钥（每台服务器都需要）
for i in $(seq 0 $((N-1))); do
  ./ResiBFTKeys $i
done

# 或者：只在 Server 0 生成，然后复制到其他服务器
# Server 0:
for i in $(seq 0 99); do ./ResiBFTKeys $i; done
# 然后打包分发：
tar -czf somekeys.tar.gz somekeys/
# 其他服务器解压：
tar -xzf somekeys.tar.gz
```

---

## 二、生成配置文件（每台服务器单独执行）

### 2.1 使用 gen_config_auto.sh

```bash
# 在每台服务器上执行（参数相同）
cd ~/ResiBFT/Hotsus

# 100节点配置
./gen_config_auto.sh 100 47.86.182.194 8.216.32.203 43.108.14.100 47.84.134.45

# 200节点配置
./gen_config_auto.sh 200 47.86.182.194 8.216.32.203 43.108.14.100 47.84.134.45

# 241节点配置 (f=80)
./gen_config_auto.sh 241 47.86.182.194 8.216.32.203 43.108.14.100 47.84.134.45
```

**注意：所有服务器执行相同的命令，生成的 config 文件内容相同**

---

## 三、启动节点（每台服务器单独执行）

### 3.1 使用 run_resibft_server.sh

```bash
# Server 0 (47.86.182.194)
./run_resibft_server.sh 0 4 33 67 100 200 33 32

# Server 1 (8.216.32.203)
./run_resibft_server.sh 1 4 33 67 100 200 33 32

# Server 2 (43.108.14.100)
./run_resibft_server.sh 2 4 33 67 100 200 33 32

# Server 3 (47.84.134.45)
./run_resibft_server.sh 3 4 33 67 100 200 33 32
```

### 3.2 参数说明

```bash
./run_resibft_server.sh <server_index> <num_servers> <num_general> <num_trusted> <num_all> <num_views> <num_faults> <timeout>
```

| 参数 | 说明 | 示例值 |
|------|------|--------|
| server_index | 本服务器序号 (0-3) | 0, 1, 2, 3 |
| num_servers | 总服务器数 | 4 |
| num_general | General节点数 | 33 |
| num_trusted | Trusted节点数 | 67 |
| num_all | 总节点数 | 100 |
| num_views | 视图数 | 200 |
| num_faults | 故障数 f | 33 |
| timeout | 超时时间(秒) | 32 |

---

## 四、不同规模实验配置

### 4.1 乐观实验 (LAN)

| f | N | num_general | num_trusted | num_faults | d配置 |
|---|---|-------------|-------------|------------|-------|
| 20 | 61 | 20 | 41 | 20 | d=20/f/30/40/50 |
| 30 | 91 | 30 | 61 | 30 | d=20/30/45/60/75 |
| 40 | 121 | 40 | 81 | 40 | d=20/40/60/80/100 |

**启动命令示例（f=30, d=60）：**

```bash
# 所有服务器生成config
./gen_config_auto.sh 121 47.242.18.75 47.86.232.213 47.86.182.194

# 各服务器启动（server_index 不同）
# Server 0:
./run_resibft_server.sh 0 4 20 41 61 200 20 500

# Server 1:
./run_resibft_server.sh 1 4 20 41 61 200 20 500

# Server 2:
./run_resibft_server.sh 2 4 20 41 61 200 20 500

# Server 3:
./run_resibft_server.sh 3 4 20 41 61 200 20 500

./ResiBFTClient 0 20 41 61 20 100 0 0

  ┌─────┬──────────────────────────┬──────────────────────────┐
  │  d  │ num_general (G-Replicas) │ num_trusted (T-Replicas) │
  ├─────┼──────────────────────────┼──────────────────────────┤
  │ 20  │ 41                       │ 20                       │
  ├─────┼──────────────────────────┼──────────────────────────┤
  │ 30  │ 31                       │ 30                       │
  ├─────┼──────────────────────────┼──────────────────────────┤
  │ 40  │ 21                       │ 40                       │
  ├─────┼──────────────────────────┼──────────────────────────┤
  │ 50  │ 11                       │ 50                       │
  └─────┴──────────────────────────┴──────────────────────────┘

a1:
./run_resibft_server.sh 0 3 41 20 61 200 20 500
./run_resibft_server.sh 1 3 41 20 61 200 20 500
./run_resibft_server.sh 2 3 41 20 61 200 20 500
./ResiBFTClient 0 41 20 61 20 100 0 0
b1:
./run_resibft_server.sh 0 3 31 30 61 200 20 500
./run_resibft_server.sh 1 3 31 30 61 200 20 500
./run_resibft_server.sh 2 3 31 30 61 200 20 500
./ResiBFTClient 0 31 30 61 20 100 0 0
c1:
./run_resibft_server.sh 0 3 21 40 61 200 20 500
./run_resibft_server.sh 1 3 21 40 61 200 20 500
./run_resibft_server.sh 2 3 21 40 61 200 20 500
./ResiBFTClient 0 21 40 61 20 100 0 0
d1:
./run_resibft_server.sh 0 3 11 50 61 200 20 500
./run_resibft_server.sh 1 3 11 50 61 200 20 500
./run_resibft_server.sh 2 3 11 50 61 200 20 500
./ResiBFTClient 0 11 50 61 20 100 0 0

d=20/30/45/60/75
a2:
./run_resibft_server.sh 0 3 71 20 91 200 30 500
./run_resibft_server.sh 1 3 71 20 91 200 30 500
./run_resibft_server.sh 2 3 71 20 91 200 30 500
./ResiBFTClient 0 71 20 91 30 100 0 0
b2:
./run_resibft_server.sh 0 3 61 30 91 200 30 500
./run_resibft_server.sh 1 3 61 30 91 200 30 500
./run_resibft_server.sh 2 3 61 30 91 200 30 500
./ResiBFTClient 0 61 30 91 30 100 0 0
c2:
./run_resibft_server.sh 0 3 46 45 91 200 30 500
./run_resibft_server.sh 1 3 46 45 91 200 30 500
./run_resibft_server.sh 2 3 46 45 91 200 30 500
./ResiBFTClient 0 46 45 91 30 100 0 0
d2:
./run_resibft_server.sh 0 3 31 60 91 200 30 500
./run_resibft_server.sh 1 3 31 60 91 200 30 500
./run_resibft_server.sh 2 3 31 60 91 200 30 500
./ResiBFTClient 0 31 60 91 30 100 0 0
e2:
./run_resibft_server.sh 0 3 16 75 91 200 30 500
./run_resibft_server.sh 1 3 16 75 91 200 30 500
./run_resibft_server.sh 2 3 16 75 91 200 30 500
./ResiBFTClient 0 16 75 91 30 100 0 0

d=20/40/60/80/100
a3:
./run_resibft_server.sh 0 3 101 20 121 200 40 500
./run_resibft_server.sh 1 3 101 20 121 200 40 500
./run_resibft_server.sh 2 3 101 20 121 200 40 500
./ResiBFTClient 0 101 20 121 40 100 0 0
b3:
./run_resibft_server.sh 0 3 81 40 121 200 40 500
./run_resibft_server.sh 1 3 81 40 121 200 40 500
./run_resibft_server.sh 2 3 81 40 121 200 40 500
./ResiBFTClient 0 81 40 121 40 100 0 0
c3:
./run_resibft_server.sh 0 3 61 60 121 200 40 500
./run_resibft_server.sh 1 3 61 60 121 200 40 500
./run_resibft_server.sh 2 3 61 60 121 200 40 500
./ResiBFTClient 0 61 60 121 40 100 0 0
d3:
./run_resibft_server.sh 0 3 41 80 121 200 40 500
./run_resibft_server.sh 1 3 41 80 121 200 40 500
./run_resibft_server.sh 2 3 41 80 121 200 40 500
./ResiBFTClient 0 41 80 121 40 100 0 0
e3:
./run_resibft_server.sh 0 3 21 100 121 200 40 500
./run_resibft_server.sh 1 3 21 100 121 200 40 500
./run_resibft_server.sh 2 3 21 100 121 200 40 500
./ResiBFTClient 0 21 100 121 40 100 0 0
```

### 4.2 攻击实验 (WAN)

| f | N | num_general | num_trusted | num_faults |
|---|---|-------------|-------------|------------|
| 30 | 91 | 30 | 61 | 30 |
| 40 | 121 | 40 | 81 | 40 |
| 50 | 151 | 50 | 101 | 50 |
| 60 | 181 | 60 | 121 | 60 |
| 70 | 211 | 70 | 141 | 70 |
| 80 | 241 | 80 | 161 | 80 |

---

## 四.5、启动客户端

### 客户端参数说明

```bash
./ResiBFTClient <client_id> <num_general> <num_trusted> <num_replicas> <num_faults> <num_transactions> <sleep_time> <exp_index>
```

| 参数 | 说明 |
|------|------|
| client_id | 客户端ID（通常等于总节点数N） |
| num_general | General节点数 |
| num_trusted | Trusted节点数 |
| num_replicas | 总节点数 |
| num_faults | 故障数 |
| num_transactions | 发送交易数量 |
| sleep_time | 发送间隔(ms) |
| exp_index | 实验索引 |

### 不同配置的 Client 命令

| f | N | num_general | num_trusted | Client命令 |
|---|---|-------------|-------------|-----------|
| 20 | 61 | 20 | 41 | `./ResiBFTClient 61 20 41 61 20 1000 0 0` |
| 30 | 91 | 30 | 61 | `./ResiBFTClient 91 30 61 91 30 1000 0 0` |
| 40 | 121 | 40 | 81 | `./ResiBFTClient 121 40 81 121 40 1000 0 0` |
| 50 | 151 | 50 | 101 | `./ResiBFTClient 151 50 101 151 50 1000 0 0` |
| 60 | 181 | 60 | 121 | `./ResiBFTClient 181 60 121 181 60 1000 0 0` |
| 70 | 211 | 70 | 141 | `./ResiBFTClient 211 70 141 211 70 1000 0 0` |
| 80 | 241 | 80 | 161 | `./ResiBFTClient 241 80 161 241 80 1000 0 0` |

**注意：客户端只需要在 Server 0 启动**

---

## 五、攻击实验启动脚本（支持环境变量）

### 5.1 新增启动脚本：run_attack_experiment.sh

```bash
#!/bin/bash

# run_attack_experiment.sh - 攻击实验启动脚本
# 用法: ./run_attack_experiment.sh <server_index> <num_servers> <f> <d> <tc_ratio> <k_ratio> <payload> <tc_behavior>

SERVER_INDEX=$1
NUM_SERVERS=$2
F=$3
D=$4
TC_RATIO=$5
K_RATIO=$6
PAYLOAD=$7
TC_BEHAVIOR=${8:-0}

# 计算参数
N=$((3 * F + 1))
GENERAL=$((N - D))
VIEWS=200
TIMEOUT=32

echo "========================================"
echo "ResiBFT 攻击实验"
echo "========================================"
echo "服务器序号: $SERVER_INDEX (共 $NUM_SERVERS 台)"
echo "参数: f=$F, N=$N, d=$D, λ=$TC_RATIO, k_ratio=$K_RATIO"
echo "payload=$PAYLOAD KB, tc_behavior=$TC_BEHAVIOR"
echo "========================================"

# 设置环境变量
export RESIBFT_EXP_MODE=2
export RESIBFT_TC_RATIO=$TC_RATIO
export RESIBFT_K_RATIO=$K_RATIO
export RESIBFT_TC_BEHAVIOR=$TC_BEHAVIOR

# 计算节点范围
BASE_NODES=$((N / NUM_SERVERS))
EXTRA_NODES=$((N % NUM_SERVERS))

if [ $SERVER_INDEX -lt $EXTRA_NODES ]; then
    NODES_ON_SERVER=$((BASE_NODES + 1))
else
    NODES_ON_SERVER=$BASE_NODES
fi

START_ID=$((SERVER_INDEX * BASE_NODES))
if [ $SERVER_INDEX -lt $EXTRA_NODES ]; then
    START_ID=$((START_ID + SERVER_INDEX))
else
    START_ID=$((START_ID + EXTRA_NODES))
fi
END_ID=$((START_ID + NODES_ON_SERVER - 1))

echo "节点范围: $START_ID - $END_ID (共 $NODES_ON_SERVER 个)"

# 启动节点
mkdir -p results
for i in $(seq $START_ID $END_ID); do
    ./ResiBFTServer $i $GENERAL $D $N $VIEWS $F $TIMEOUT > results/node$i.log 2>&1 &
    sleep 0.5
done

echo ""
echo "节点 $START_ID-$END_ID 已启动"
echo "环境变量: RESIBFT_EXP_MODE=$RESIBFT_EXP_MODE, RESIBFT_TC_RATIO=$RESIBFT_TC_RATIO"
```

### 5.2 使用攻击实验脚本

```bash
# f=30, d=60, λ=0.2, k=0.5 (d/2), payload=0, TC投坏票

# Server 0:
./run_attack_experiment.sh 0 4 30 60 0.2 0.5 0 0

# Server 1:
./run_attack_experiment.sh 1 4 30 60 0.2 0.5 0 0

# Server 2:
./run_attack_experiment.sh 2 4 30 60 0.2 0.5 0 0

# Server 3:
./run_attack_experiment.sh 3 4 30 60 0.2 0.5 0 0
```

---

## 六、完整实验执行流程

### 6.1 乐观实验执行流程

**步骤1：所有服务器生成config**
```bash
# 在每台服务器执行
cd ~/ResiBFT/Hotsus
./gen_config_auto.sh 100 47.86.182.194 8.216.32.203 43.108.14.100 47.84.134.45
```

**步骤2：各服务器启动节点**
```bash
# Server 0:
./run_resibft_server.sh 0 4 33 67 100 200 33 32

# Server 1:
./run_resibft_server.sh 1 4 33 67 100 200 33 32

# Server 2:
./run_resibft_server.sh 2 4 33 67 100 200 33 32

# Server 3:
./run_resibft_server.sh 3 4 33 67 100 200 33 32
```

**步骤3：在 Server 0 启动客户端**
```bash
# Server 0:
./ResiBFTClient 100 33 67 100 33 1000 0 0
```

**步骤4：所有服务器停止节点**
```bash
# 在每台服务器执行:
killall ResiBFTServer
```

### 6.2 攻击实验执行流程

**步骤1：所有服务器生成config**
```bash
./gen_config_auto.sh 121 47.86.182.194 8.216.32.203 43.108.14.100 47.84.134.45
```

**步骤2：各服务器启动节点（带攻击参数）**
```bash
# Server 0:
./run_attack_experiment.sh 0 4 40 81 0.3 0.33 256 0

# Server 1:
./run_attack_experiment.sh 1 4 40 81 0.3 0.33 256 0

# Server 2:
./run_attack_experiment.sh 2 4 40 81 0.3 0.33 256 0

# Server 3:
./run_attack_experiment.sh 3 4 40 81 0.3 0.33 256 0


./run_attack_experiment.sh 0 3 40 81 0.3 0.33 0 0
./run_attack_experiment.sh 1 3 40 81 0.3 0.33 0 0
./run_attack_experiment.sh 2 3 40 81 0.3 0.33 0 0
```

**步骤3：在 Server 0 启动客户端**
```bash
./ResiBFTClient 121 40 81 121 40 100 0 0
```

---

## 七、常用命令

### 7.1 查看状态

```bash
# 查看节点数量
ps aux | grep ResiBFTServer | wc -l

# 查看运行状态
ps aux | grep ResiBFTServer

# 查看日志
tail -f results/node0.log

# 查看实验配置日志
grep "EXPERIMENT-CONFIG" results/*.log
grep "TC-BEHAVIOR" results/*.log
grep "COMMITTEE-K" results/*.log
```

### 7.2 停止节点

```bash
# 停止所有节点
killall ResiBFTServer

# 停止特定节点
kill -9 <pid>
```

### 7.3 检查共识进度

```bash
# 查看执行区块数
grep -c "RESIBFT-EXECUTE" results/*.log

# 查看GRACIOUS转换
grep -c "GRACIOUS-TRANSITION" results/*.log

# 查看View切换
grep "VIEW-CHANGE" results/*.log
```

---

## 八、实验参数快速参考

### 8.1 乐观实验参数计算

```
N = 3f + 1
num_general = (N - 1) / 3 = f
num_trusted = N - num_general = 2f + 1
num_faults = f
```

### 8.2 攻击实验参数计算

```
N = 3f + 1
d = TEE节点数 (20, f, 1.5f, 2f, 2.5f)
λ = TC节点比例 (0.1, 0.2, 0.3)
k = d × k_ratio (k_ratio: 0.33, 0.5, 0.67, 0.75)
num_general = N - d
num_faults = f
```

### 8.3 典型配置示例

| 配置 | f | N | d | num_general | 命令参数 |
|------|---|---|---|-------------|---------|
| 乐观100节点 | 33 | 100 | 67 | 33 | `0 4 33 67 100 200 33 32` |
| 乐观200节点 | 66 | 200 | 134 | 66 | `0 4 66 134 200 200 66 32` |
| 攻击f=50 | 50 | 151 | 100 | 51 | `run_attack 0 4 50 100 0.2 0.5 0 0` |
| 攻击f=80 | 80 | 241 | 200 | 41 | `run_attack 0 4 80 200 0.3 0.67 256 0` |

---

## 九、环境变量说明

| 环境变量 | 默认值 | 说明 |
|---------|-------|------|
| `RESIBFT_EXP_MODE` | 0 | 实验模式：0=乐观, 1=随机, 2=攻击 |
| `RESIBFT_TC_RATIO` | 0.2 | TC节点比例 λ |
| `RESIBFT_K_RATIO` | 0.5 | 委员会大小比例 |
| `RESIBFT_TC_BEHAVIOR` | 0 | TC节点行为：0=投坏票, 1=沉默 |

**注意：** 环境变量在 `run_attack_experiment.sh` 中自动设置，无需手动设置。

---

## 十、攻击模式验证方法

### 10.1 检查攻击模式是否生效

```bash
# 一行检查所有关键信息
grep -E "experimentMode|tcRatio|kRatio|TC nodes|Byzantine nodes|COMMITTEE-K" results/*.log | head -20
```

期望输出：
```
[EXPERIMENT-CONFIG] experimentMode = 2 (0=乐观, 1=随机, 2=攻击)
[EXPERIMENT-CONFIG] tcRatio (λ) = 0.2
[EXPERIMENT-CONFIG] kRatio = 0.5
[EXPERIMENT-CONFIG] Byzantine nodes (28): ...
[EXPERIMENT-CONFIG] TC nodes (9): ...
[COMMITTEE-K] Ratio mode: d=60, kRatio=0.5, k=30 (k = d * 0.5)
```

### 10.2 检查 TC 节点攻击行为

```bash
# 查看 TC 节点投坏票行为
grep "TC-BEHAVIOR" results/*.log | head -10
```

期望输出（投坏票模式）：
```
[TC-BEHAVIOR] TC node rejecting MsgLdrprepare - sending VC_REJECTED for view 30
[TC-BEHAVIOR] TC node staying SILENT for view 31
```

### 10.3 检查委员会 k 计算

```bash
# 查看委员会 k 计算（Ratio mode）
grep "COMMITTEE-K" results/*.log | head -5
```

期望输出：
```
[COMMITTEE-K] Ratio mode: d=60, kRatio=0.5, k=30
```

### 10.4 关键指标对照表

| 检查项 | 期望值 | 说明 |
|--------|--------|------|
| experimentMode | **2** | 0=乐观, 1=随机, 2=攻击 |
| tcRatio (λ) | **0.2** | TC节点比例 |
| kRatio | **0.5** | 委员会比例 |
| COMMITTEE-K | **Ratio mode, k=30** | k = d × kRatio |
| TC nodes count | **≈12** | d×λ = 60×0.2 |
| TC-BEHAVIOR | **有输出** | TC节点正在攻击 |
| GRACIOUS 触发 | **> 0** | 攻击应触发 GRACIOUS |