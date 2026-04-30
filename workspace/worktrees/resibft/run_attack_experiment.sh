#!/bin/bash

# run_attack_experiment.sh - 攻击实验启动脚本
# 用法: ./run_attack_experiment.sh <server_index> <num_servers> <f> <d> <tc_ratio> <k_ratio> <payload> <tc_behavior>
#
# 示例: ./run_attack_experiment.sh 0 4 40 80 0.3 0.33 256 0

SERVER_INDEX=$1
NUM_SERVERS=$2
F=$3
D=$4
TC_RATIO=$5
K_RATIO=$6
PAYLOAD=$7
TC_BEHAVIOR=${8:-0}

if [ $# -lt 7 ]; then
    echo "用法: ./run_attack_experiment.sh <server_index> <num_servers> <f> <d> <tc_ratio> <k_ratio> <payload> [tc_behavior]"
    echo ""
    echo "参数说明:"
    echo "  server_index   本服务器序号 (0-3)"
    echo "  num_servers    总服务器数 (4)"
    echo "  f              故障节点数 (30, 40, 50, 60, 70, 80)"
    echo "  d              TEE节点数 (20, f, 1.5f, 2f, 2.5f)"
    echo "  tc_ratio       TC节点比例 λ (0.1, 0.2, 0.3)"
    echo "  k_ratio        委员会大小比例 (0.33, 0.5, 0.67, 0.75)"
    echo "  payload        区块负载 KB (0, 256)"
    echo "  tc_behavior    TC节点行为 (0=投坏票, 1=沉默)"
    echo ""
    echo "示例:"
    echo "  ./run_attack_experiment.sh 0 4 40 80 0.3 0.33 256 0"
    echo "  ./run_attack_experiment.sh 1 4 50 100 0.2 0.5 0 1"
    exit 1
fi

# 计算参数
N=$((3 * F + 1))
GENERAL=$((N - D))
VIEWS=200
TIMEOUT=500  # 攻击实验用较短 timeout，方便观察 UNCIVIL 触发和 view change

echo "========================================"
echo "ResiBFT 攻击实验"
echo "========================================"
echo "服务器序号: $SERVER_INDEX (共 $NUM_SERVERS 台)"
echo "实验参数:"
echo "  f = $F, N = $N, d = $D"
echo "  num_general = $GENERAL, num_trusted = $D"
echo "  λ (TC比例) = $TC_RATIO"
echo "  k_ratio = $K_RATIO"
echo "  payload = $PAYLOAD KB"
echo "  tc_behavior = $TC_BEHAVIOR (0=投坏票, 1=沉默)"
echo "========================================"

# 设置环境变量
export RESIBFT_EXP_MODE=2
export RESIBFT_TC_RATIO=$TC_RATIO
export RESIBFT_K_RATIO=$K_RATIO
export RESIBFT_TC_BEHAVIOR=$TC_BEHAVIOR

echo "环境变量已设置:"
echo "  RESIBFT_EXP_MODE=$RESIBFT_EXP_MODE"
echo "  RESIBFT_TC_RATIO=$RESIBFT_TC_RATIO"
echo "  RESIBFT_K_RATIO=$RESIBFT_K_RATIO"
echo "  RESIBFT_TC_BEHAVIOR=$RESIBFT_TC_BEHAVIOR"
echo ""

# 生成 parameters.h（根据节点数量动态设置）
echo "========================================"
echo "生成 parameters.h..."
echo "========================================"
echo '#ifndef PARAMETERS_H' > App/parameters.h
echo '#define PARAMETERS_H' >> App/parameters.h
echo '// #define BASIC_RESIBFT  // 由 Makefile 的 -DBASIC_RESIBFT 标志定义' >> App/parameters.h
echo "#define MAX_NUM_NODES $N" >> App/parameters.h
echo "#define MAX_NUM_SIGNATURES $((N - 1))" >> App/parameters.h
echo '#define MAX_NUM_TRANSACTIONS 400' >> App/parameters.h
echo "#define MAX_NUM_GROUPMEMBERS $D" >> App/parameters.h  # 委员会最大成员数 = TEE节点数
echo "#define PAYLOAD_SIZE $PAYLOAD" >> App/parameters.h
echo '#endif' >> App/parameters.h

echo "parameters.h 已生成:"
echo "  MAX_NUM_NODES = $N"
echo "  MAX_NUM_SIGNATURES = $((N - 1))"
echo "  MAX_NUM_GROUPMEMBERS = $D"
echo "  PAYLOAD_SIZE = $PAYLOAD"
echo ""

# 启用 COMMITTEE_K_RATIO_MODE（使用 k_ratio 计算委员会大小）
echo "========================================"
echo "配置 committee k_ratio 模式..."
echo "========================================"
# 检查 production_config.h 是否存在
if [ -f "App/production_config.h" ]; then
    # 添加 COMMITTEE_K_RATIO_MODE 定义（如果尚未启用）
    if ! grep -q "^#define COMMITTEE_K_RATIO_MODE" App/production_config.h; then
        # 找到注释掉的行，取消注释
        sed -i 's/^\/\/ #define COMMITTEE_K_RATIO_MODE/#define COMMITTEE_K_RATIO_MODE/' App/production_config.h
        echo "已启用 COMMITTEE_K_RATIO_MODE"
    else
        echo "COMMITTEE_K_RATIO_MODE 已启用"
    fi
else
    echo "警告: App/production_config.h 不存在"
fi
echo ""

# 编译
echo "========================================"
echo "编译 ResiBFT..."
echo "========================================"
source /opt/intel/sgxsdk/environment 2>/dev/null || true
make clean 2>/dev/null || true
make SGX_MODE=SIM ResiBFTServer ResiBFTClient ResiBFTKeys enclave.signed.so
echo "编译完成"
echo ""

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
echo ""

# 启动节点
mkdir -p results
echo "启动节点..."
for i in $(seq $START_ID $END_ID); do
    ./ResiBFTServer $i $GENERAL $D $N $VIEWS $F $TIMEOUT > results/node$i.log 2>&1 &
    sleep 0.5
    if [ $((i % 10)) -eq $((END_ID % 10)) ] && [ $i -ne $END_ID ]; then
        echo "已启动到节点 $i..."
    fi
done

echo ""
echo "========================================"
echo "节点 $START_ID-$END_ID 已启动"
echo "日志目录: results/"
echo "========================================"
echo ""
echo "常用命令:"
echo "  查看运行状态: ps aux | grep ResiBFTServer"
echo "  查看节点数量: ps aux | grep ResiBFTServer | wc -l"
echo "  停止所有节点: killall ResiBFTServer"
echo "  查看节点日志: tail -f results/node$START_ID.log"
echo "  查看实验配置: grep 'EXPERIMENT-CONFIG' results/*.log"
echo "  查看TC行为: grep 'TC-BEHAVIOR' results/*.log"
echo "  查看委员会配置: grep 'COMMITTEE-K' results/*.log"
echo ""
echo "启动客户端 (在 Server 0 执行):"
echo "  ./ResiBFTClient $N $GENERAL $D $N $F 1000 $PAYLOAD 0"