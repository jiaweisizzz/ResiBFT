# ResiBFT 四台服务器实验部署方案

## 服务器配置

| 服务器 | IP | 角色 |
|--------|-----|------|
| Server 0 | 47.86.182.194 | 主节点 |
| Server 1 | 8.216.32.203 | 副本节点 |
| Server 2 | 43.108.14.100 | 副本节点 |
| Server 3 | 47.84.134.45 | 副本节点 |

---

## 一、环境准备

### 1.1 服务器初始化（所有服务器执行）

```bash
# 安装依赖
yum install -y openssl-devel gcc g++ make git

# 安装 Intel SGX SDK（如果需要TEE功能）
# 参考: https://github.com/intel/linux-sgx

# 克隆代码
git clone <your-repo-url>
cd ResiBFT/Hotsus

# 编译
source /opt/intel/sgxsdk/environment  # 如果有SGX
make SGX_MODE=SIM ResiBFTServer ResiBFTClient ResiBFTKeys enclave.signed.so
```

### 1.2 创建密钥

```bash
# 在 Server 0 执行，生成所有节点密钥
mkdir -p somekeys
for i in 0 1 2 3; do
  ./ResiBFTKeys $i
done

# 将密钥复制到其他服务器
scp somekeys/ec256_private* somekeys/ec256_public* root@8.216.32.203:~/ResiBFT/Hotsus/somekeys/
scp somekeys/ec256_private* somekeys/ec256_public* root@43.108.14.100:~/ResiBFT/Hotsus/somekeys/
scp somekeys/ec256_private* somekeys/ec256_public* root@47.84.134.45:~/ResiBFT/Hotsus/somekeys/
```

---

## 二、实验方案参数

### 2.1 乐观实验 (LAN) 参数

| 参数 | 范围 | 说明 |
|------|------|------|
| f | 20, 30, 40 | 故障节点数 |
| N = 3f+1 | 61, 91, 121 | 总节点数 |
| d | 20, f, 1.5f, 2f, 2.5f | TEE节点数 |
| payload | 0KB, 256KB | 区块负载 |

**参数组合表：**

| f | N=3f+1 | d=20 | d=f | d=1.5f | d=2f | d=2.5f |
|---|--------|------|-----|--------|------|--------|
| 20 | 61 | 20 | 20 | 30 | 40 | 50 |
| 30 | 91 | 20 | 30 | 45 | 60 | 75 |
| 40 | 121 | 20 | 40 | 60 | 80 | 100 |

### 2.2 攻击实验 (WAN) 参数

| 参数 | 范围 | 说明 |
|------|------|------|
| f | 30, 40, 50, 60, 70, 80 | 故障节点数 |
| N = 3f+1 | 91-241 | 总节点数 |
| d | 20, f, 1.5f, 2f, 2.5f | TEE节点数 |
| λ (TC比例) | 0.1, 0.2, 0.3 | TC节点占TEE比例 |
| k比例 | d/3, d/2, 2d/3, 3d/4 | 委员会大小 |
| payload | 0KB, 256KB | 区块负载 |
| 视图数 | 200 | 每次实验轮数 |

**参数组合表：**

| f | N=3f+1 | d范围 | λ范围 | k比例 |
|---|--------|-------|-------|-------|
| 30 | 91 | 20~75 | 0.1~0.3 | d/3~3d/4 |
| 40 | 121 | 20~100 | 0.1~0.3 | d/3~3d/4 |
| 50 | 151 | 20~125 | 0.1~0.3 | d/3~3d/4 |
| 60 | 181 | 20~150 | 0.1~0.3 | d/3~3d/4 |
| 70 | 211 | 20~175 | 0.1~0.3 | d/3~3d/4 |
| 80 | 241 | 20~200 | 0.1~0.3 | d/3~3d/4 |

---

## 三、节点分配方案

### 3.1 按规模分配

#### f=20 (N=61)

| 服务器 | 节点ID | 数量 |
|--------|--------|------|
| 47.86.182.194 | 0-15 | 16 |
| 8.216.32.203 | 16-31 | 16 |
| 43.108.14.100 | 32-46 | 15 |
| 47.84.134.45 | 47-60 | 14 |

#### f=30 (N=91)

| 服务器 | 节点ID | 数量 |
|--------|--------|------|
| 47.86.182.194 | 0-22 | 23 |
| 8.216.32.203 | 23-45 | 23 |
| 43.108.14.100 | 46-68 | 23 |
| 47.84.134.45 | 69-90 | 22 |

#### f=40 (N=121)

| 服务器 | 节点ID | 数量 |
|--------|--------|------|
| 47.86.182.194 | 0-30 | 31 |
| 8.216.32.203 | 31-60 | 30 |
| 43.108.14.100 | 61-90 | 30 |
| 47.84.134.45 | 91-120 | 30 |

#### f=50 (N=151)

| 服务器 | 节点ID | 数量 |
|--------|--------|------|
| 47.86.182.194 | 0-37 | 38 |
| 8.216.32.203 | 38-75 | 38 |
| 43.108.14.100 | 76-113 | 38 |
| 47.84.134.45 | 114-150 | 37 |

#### f=60 (N=181)

| 服务器 | 节点ID | 数量 |
|--------|--------|------|
| 47.86.182.194 | 0-45 | 46 |
| 8.216.32.203 | 46-90 | 45 |
| 43.108.14.100 | 91-135 | 45 |
| 47.84.134.45 | 136-180 | 45 |

#### f=80 (N=241)

| 服务器 | 节点ID | 数量 |
|--------|--------|------|
| 47.86.182.194 | 0-60 | 61 |
| 8.216.32.203 | 61-121 | 61 |
| 43.108.14.100 | 122-182 | 61 |
| 47.84.134.45 | 183-240 | 58 |

---

## 四、实验脚本

### 4.1 乐观实验脚本

```bash
#!/bin/bash
# optimistic_experiment.sh - 乐观实验 (LAN)

SERVERS=("47.86.182.194" "8.216.32.203" "43.108.14.100" "47.84.134.45")
F=$1           # 20, 30, 40
D=$2           # TEE节点数
PAYLOAD=$3     # 0 或 256

N=$((3 * F + 1))
GENERAL=$((N - D))
VIEWS=200

echo "=== 乐观实验: f=$F, N=$N, d=$D, payload=$PAYLOAD KB ==="

# 生成config文件
> config
for i in $(seq 0 $((N-1))); do
  SERVER_IDX=$((i * 4 / N))
  PORT=$((8760 + i % 100))
  CPORT=$((9760 + i % 100))
  echo "id:$i host:${SERVERS[$SERVER_IDX]} port:$PORT cport:$CPORT"
done >> config

# 分发到所有服务器
for srv in "${SERVERS[@]}"; do
  scp config root@$srv:~/ResiBFT/Hotsus/
done

# 各服务器启动节点
for srv_idx in 0 1 2 3; do
  START_ID=$((srv_idx * N / 4))
  END_ID=$((srv_idx * N / 4 + N / 4 - 1))

  ssh root@${SERVERS[$srv_idx]} << EOF
cd ~/ResiBFT/Hotsus
mkdir -p results
killall ResiBFTServer 2>/dev/null
export RESIBFT_EXP_MODE=0
export RESIBFT_TC_RATIO=0
export RESIBFT_K_RATIO=0.5
for i in \$(seq $START_ID $END_ID); do
  ./ResiBFTServer \$i $GENERAL $D $N $VIEWS $F 32 > results/node\$i.log 2>&1 &
  sleep 0.3
done
echo "Server $srv_idx started nodes $START_ID to $END_ID"
EOF
done

echo "等待节点启动..."
sleep 10

# 启动客户端
ssh root@${SERVERS[0]} << EOF
cd ~/ResiBFT/Hotsus
./ResiBFTClient $N $GENERAL $D $N $F 1000 $PAYLOAD 0
EOF

echo "实验完成"
```

### 4.2 攻击实验脚本

```bash
#!/bin/bash
# attack_experiment.sh - 攻击实验 (WAN)

SERVERS=("47.86.182.194" "8.216.32.203" "43.108.14.100" "47.84.134.45")
F=$1           # 30, 40, 50, 60, 70, 80
D=$2           # TEE节点数
TC_RATIO=$3    # 0.1, 0.2, 0.3
K_RATIO=$4     # 0.33, 0.5, 0.67, 0.75
PAYLOAD=$5     # 0 或 256
TC_BEHAVIOR=${6:-0}  # 0=投坏票, 1=沉默

N=$((3 * F + 1))
GENERAL=$((N - D))
VIEWS=200

echo "=== 攻击实验: f=$F, N=$N, d=$D, λ=$TC_RATIO, k_ratio=$K_RATIO, payload=$PAYLOAD KB, tc_behavior=$TC_BEHAVIOR ==="

# 生成config文件
> config
for i in $(seq 0 $((N-1))); do
  SERVER_IDX=$((i * 4 / N))
  PORT=$((8760 + i % 100))
  CPORT=$((9760 + i % 100))
  echo "id:$i host:${SERVERS[$SERVER_IDX]} port:$PORT cport:$CPORT"
done >> config

# 分发到所有服务器
for srv in "${SERVERS[@]}"; do
  scp config root@$srv:~/ResiBFT/Hotsus/
done

# 各服务器启动节点（带攻击参数）
for srv_idx in 0 1 2 3; do
  START_ID=$((srv_idx * N / 4))
  END_ID=$((srv_idx * N / 4 + N / 4 - 1))

  ssh root@${SERVERS[$srv_idx]} << EOF
cd ~/ResiBFT/Hotsus
mkdir -p results
killall ResiBFTServer 2>/dev/null
export RESIBFT_EXP_MODE=2
export RESIBFT_TC_RATIO=$TC_RATIO
export RESIBFT_K_RATIO=$K_RATIO
export RESIBFT_TC_BEHAVIOR=$TC_BEHAVIOR
for i in \$(seq $START_ID $END_ID); do
  ./ResiBFTServer \$i $GENERAL $D $N $VIEWS $F 32 > results/node\$i.log 2>&1 &
  sleep 0.3
done
echo "Server $srv_idx started nodes $START_ID to $END_ID (attack mode)"
EOF
done

echo "等待节点启动..."
sleep 15

# 启动客户端
ssh root@${SERVERS[0]} << EOF
cd ~/ResiBFT/Hotsus
./ResiBFTClient $N $GENERAL $D $N $F 1000 $PAYLOAD 0
EOF

echo "攻击实验完成"
```

### 4.3 批量实验脚本

```bash
#!/bin/bash
# batch_all_experiments.sh - 执行所有实验方案

SERVERS=("47.86.182.194" "8.216.32.203" "43.108.14.100" "47.84.134.45")
LOG_DIR="experiment_results"
mkdir -p $LOG_DIR

# ========================================
# 乐观实验 (LAN)
# ========================================
echo "========== 乐观实验 (LAN) =========="

for F in 20 30 40; do
  for D_RATIO in 1 1.5 2 2.5; do
    for PAYLOAD in 0 256; do
      D=$((F * D_RATIO))
      if [ $D -lt 20 ]; then D=20; fi

      EXP_NAME="optimistic_f${F}_d${D}_payload${PAYLOAD}"
      echo "Running: $EXP_NAME"

      ./optimistic_experiment.sh $F $D $PAYLOAD

      # 收集结果
      for srv in "${SERVERS[@]}"; do
        ssh root@$srv "cat ~/ResiBFT/Hotsus/results/*.log" >> $LOG_DIR/${EXP_NAME}.log
      done

      # 停止所有节点
      for srv in "${SERVERS[@]}"; do
        ssh root@$srv "killall ResiBFTServer"
      done

      sleep 10
    done
  done
done

# ========================================
# 攻击实验 (WAN)
# ========================================
echo "========== 攻击实验 (WAN) =========="

for F in 30 40 50 60 70 80; do
  for D_RATIO in 1 1.5 2 2.5; do
    D=$((F * D_RATIO))
    if [ $D -lt 20 ]; then D=20; fi

    for TC_RATIO in 0.1 0.2 0.3; do
      for K_RATIO in 0.33 0.5 0.67 0.75; do
        for PAYLOAD in 0 256; do
          for TC_BEHAVIOR in 0 1; do

            EXP_NAME="attack_f${F}_d${D}_lambda${TC_RATIO}_k${K_RATIO}_payload${PAYLOAD}_tc${TC_BEHAVIOR}"
            echo "Running: $EXP_NAME"

            ./attack_experiment.sh $F $D $TC_RATIO $K_RATIO $PAYLOAD $TC_BEHAVIOR

            # 收集结果
            for srv in "${SERVERS[@]}"; do
              ssh root@$srv "cat ~/ResiBFT/Hotsus/results/*.log" >> $LOG_DIR/${EXP_NAME}.log
            done

            # 停止所有节点
            for srv in "${SERVERS[@]}"; do
              ssh root@$srv "killall ResiBFTServer"
            done

            sleep 15
          done
        done
      done
    done
  done
done

echo "所有实验完成！结果保存在 $LOG_DIR"
```

### 4.4 实验指标收集脚本

```bash
#!/bin/bash
# collect_metrics.sh - 收集实验指标

LOG_FILE=$1

echo "=== 实验指标分析: $LOG_FILE ==="

# 蒙特卡洛指标
RAPID_VIEWS=$(grep -c "rapid" $LOG_FILE || echo 0)
COMMON_VIEWS=$(grep -c "common" $LOG_FILE || echo 0)
VIEW_SWITCHES=$(grep -c "VIEW-CHANGE" $LOG_FILE || echo 0)
GRACIOUS_TRANS=$(grep -c "GRACIOUS-TRANSITION" $LOG_FILE || echo 0)
UNCIVIL_TRANS=$(grep -c "UNCIVIL" $LOG_FILE || echo 0)

echo "Rapid视图数: $RAPID_VIEWS"
echo "Common视图数: $COMMON_VIEWS"
echo "Common/Rapid比例: $(echo "scale=2; $COMMON_VIEWS / $RAPID_VIEWS" | bc)"
echo "视图切换次数: $VIEW_SWITCHES"
echo "GRACIOUS触发次数: $GRACIOUS_TRANS"
echo "UNCIVIL触发次数: $UNCIVIL_TRANS"

# 实际部署指标
EXECUTES=$(grep -c "RESIBFT-EXECUTE" $LOG_FILE || echo 0)
TOTAL_TIME=$(grep "totalTime" $LOG_FILE | awk -F'time=' '{sum+=int($2)} END {print sum}')

echo "执行区块数: $EXECUTES"
echo "总处理时间: $TOTAL_TIME μs"

# TC节点统计
TC_BAD_VOTES=$(grep -c "TC-BEHAVIOR.*BAD_VOTE" $LOG_FILE || echo 0)
TC_SILENT=$(grep -c "TC-BEHAVIOR.*SILENT" $LOG_FILE || echo 0)

echo "TC投坏票次数: $TC_BAD_VOTES"
echo "TC沉默次数: $TC_SILENT"
```

---

## 五、运行命令示例

### 5.1 乐观实验

```bash
# 最小规模测试（先验证环境）
./optimistic_experiment.sh 20 20 0

# f=20, d=20, payload=0KB
./optimistic_experiment.sh 20 20 0

# f=20, d=40 (2f), payload=0KB
./optimistic_experiment.sh 20 40 0

# f=30, d=60 (2f), payload=256KB
./optimistic_experiment.sh 30 60 256

# f=40, d=100 (2.5f), payload=0KB
./optimistic_experiment.sh 40 100 0
```

### 5.2 攻击实验

```bash
# f=30, d=60, λ=0.1, k=d/2, payload=0KB, TC投坏票
./attack_experiment.sh 30 60 0.1 0.5 0 0

# f=30, d=60, λ=0.2, k=2d/3, payload=0KB, TC投坏票
./attack_experiment.sh 30 60 0.2 0.67 0 0

# f=40, d=80, λ=0.3, k=d/3, payload=256KB, TC投坏票
./attack_experiment.sh 40 80 0.3 0.33 256 0

# f=50, d=100, λ=0.2, k=3d/4, payload=0KB, TC沉默
./attack_experiment.sh 50 100 0.2 0.75 0 1

# f=60, d=120, λ=0.3, k=d/2, payload=256KB
./attack_experiment.sh 60 120 0.3 0.5 256 0

# f=80, d=200, λ=0.2, k=2d/3, payload=0KB
./attack_experiment.sh 80 200 0.2 0.67 0 0
```

### 5.3 批量实验

```bash
# 执行所有实验方案
./batch_all_experiments.sh

# 收集单个实验指标
./collect_metrics.sh experiment_results/attack_f40_d80_lambda0.3_k0.33_payload0_tc0.log
```

---

## 六、监控与调试

### 6.1 检查节点状态

```bash
# 检查节点运行状态
ssh root@47.86.182.194 "ps aux | grep ResiBFTServer | wc -l"

# 检查所有服务器
for srv in 47.86.182.194 8.216.32.203 43.108.14.100 47.84.134.45; do
  echo "Server $srv:"
  ssh root@$srv "ps aux | grep ResiBFTServer | wc -l"
done
```

### 6.2 查看日志

```bash
# 实时查看日志
ssh root@47.86.182.194 "tail -f ~/ResiBFT/Hotsus/results/node0.log"

# 搜索关键日志
ssh root@47.86.182.194 "grep 'RESIBFT-EXECUTE' ~/ResiBFT/Hotsus/results/*.log | wc -l"

# 搜索GRACIOUS转换
ssh root@47.86.182.194 "grep 'GRACIOUS-TRANSITION' ~/ResiBFT/Hotsus/results/*.log"

# 搜索TC节点行为
ssh root@47.86.182.194 "grep 'TC-BEHAVIOR' ~/ResiBFT/Hotsus/results/*.log"
```

### 6.3 停止实验

```bash
# 停止单个服务器
ssh root@47.86.182.194 "killall ResiBFTServer"

# 停止所有服务器
for srv in 47.86.182.194 8.216.32.203 43.108.14.100 47.84.134.45; do
  ssh root@$srv "killall ResiBFTServer"
done
```

---

## 七、环境变量说明

| 环境变量 | 默认值 | 说明 |
|---------|-------|------|
| `RESIBFT_EXP_MODE` | 0 | 实验模式：0=乐观, 1=随机, 2=攻击 |
| `RESIBFT_TC_RATIO` | 0.2 | TC节点比例 λ |
| `RESIBFT_K_RATIO` | 0.5 | 委员会大小比例 |
| `RESIBFT_TC_BEHAVIOR` | 0 | TC节点行为：0=投坏票, 1=沉默 |
| `RESIBFT_SIMULATE_MALICIOUS` | 0 | 模拟恶意Leader |
| `RESIBFT_SIMULATE_TIMEOUT` | 0 | 模拟超时触发GRACIOUS |

---

## 八、测试指标

### 8.1 蒙特卡洛实验指标

| 指标 | 说明 |
|------|------|
| rapid视图数 | 快速共识的视图数 |
| common视图数 | 正常共识的视图数 |
| common/rapid比例 | 共识效率 |
| 视图切换触发次数 | View Change发生频率 |
| 平均消息复杂度 | 每轮消息数量 |
| rapid视图成功率 | 成功完成的比例 |
| rapid视图活性失去率 | 失活频率 |
| rapid视图安全性失去率 | 安全性问题频率 |

### 8.2 实际部署实验指标

| 指标 | 说明 |
|------|------|
| 吞吐量 | 交易数/时间 |
| 延迟 (50th) | 50%交易的延迟 |
| 延迟 (90th) | 90%交易的延迟 |
| 延迟 (99th) | 99%交易的延迟 |
| 通信开销 | 总字节/交易数 |