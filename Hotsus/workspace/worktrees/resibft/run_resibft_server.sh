#!/bin/bash

# run_resibft_server.sh - 启动 ResiBFT 节点
# 用法: ./run_resibft_server.sh <server_index> <num_servers> <num_general> <num_trusted> <num_all> <num_views> <num_faults> <leader_change_time>
#
# server_index: 本服务器序号 (从 0 开始)
# 示例: 在第 2 台服务器上运行
#   ./run_resibft_server.sh 1 4 33 67 100 50 33 32

SERVER_INDEX=$1
NUM_SERVERS=$2
NUM_GENERAL=$3
NUM_TRUSTED=$4
NUM_ALL=$5
NUM_VIEWS=$6
NUM_FAULTS=$7
LEADER_CHANGE_TIME=$8

if [ $# -lt 8 ]; then
    echo "用法: ./run_resibft_server.sh <server_index> <num_servers> <num_general> <num_trusted> <num_all> <num_views> <num_faults> <leader_change_time>"
    echo ""
    echo "参数说明:"
    echo "  server_index       本服务器序号 (从 0 开始)"
    echo "  num_servers        总服务器数"
    echo "  num_general        普通节点数"
    echo "  num_trusted        可信节点数"
    echo "  num_all            总节点数 (= num_general + num_trusted)"
    echo "  num_views          视图数"
    echo "  num_faults         故障数"
    echo "  leader_change_time 领导切换超时 (秒)"
    echo ""
    echo "ResiBFT 参数计算:"
    echo "  num_general  = (num_all - 1) / 3        (普通节点)"
    echo "  num_trusted  = num_all - num_general    (可信节点)"
    echo "  num_faults   = (num_all - 1) / 3        (最大容错数)"
    echo ""
    echo "示例配置:"
    echo "  100节点: ./run_resibft_server.sh 0 4 33 67 100 50 33 32"
    echo "  200节点: ./run_resibft_server.sh 0 4 66 134 200 50 66 32"
    exit 1
fi

# 计算本服务器的节点范围
BASE_NODES=$((NUM_ALL / NUM_SERVERS))
EXTRA_NODES=$((NUM_ALL % NUM_SERVERS))

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

echo "========================================"
echo "ResiBFT 节点启动脚本"
echo "========================================"
echo "本服务器序号: $SERVER_INDEX (共 $NUM_SERVERS 台)"
echo "节点范围: $START_ID - $END_ID (共 $NODES_ON_SERVER 个)"
echo "参数: general=$NUM_GENERAL trusted=$NUM_TRUSTED all=$NUM_ALL faults=$NUM_FAULTS"
echo "========================================"

# 生成 parameters.h（注释掉 BASIC_* 宏，让 Makefile 的 -D 标志生效）
echo '#ifndef PARAMETERS_H' > App/parameters.h
echo '#define PARAMETERS_H' >> App/parameters.h
echo '// #define BASIC_RESIBFT  // 由 Makefile 的 -DBASIC_RESIBFT 标志定义' >> App/parameters.h
echo "#define MAX_NUM_NODES $NUM_ALL" >> App/parameters.h
echo "#define MAX_NUM_SIGNATURES $((NUM_ALL - 1))" >> App/parameters.h
echo '#define MAX_NUM_TRANSACTIONS 400' >> App/parameters.h
echo '#define MAX_NUM_GROUPMEMBERS 100' >> App/parameters.h
echo '#define PAYLOAD_SIZE 0' >> App/parameters.h
echo '#endif' >> App/parameters.h

echo "parameters.h 已生成"

# 编译
echo "========================================"
echo "编译 ResiBFT..."
echo "========================================"
source /opt/intel/sgxsdk/environment
make clean 2>/dev/null || true
make SGX_MODE=SIM ResiBFTServer ResiBFTClient ResiBFTKeys enclave.signed.so
echo "编译完成"

# 启动节点
mkdir -p results
echo "========================================"
echo "启动节点..."
echo "========================================"

for i in $(seq $START_ID $END_ID); do
    ./ResiBFTServer $i $NUM_GENERAL $NUM_TRUSTED $NUM_ALL $NUM_VIEWS $NUM_FAULTS $LEADER_CHANGE_TIME > results/node$i.log 2>&1 &
    sleep 1
    if [ $((i % 10)) -eq $((END_ID % 10)) ] && [ $i -ne $END_ID ]; then
        sleep 2
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
echo "  查看节点日志: tail -f results/node0.log"
echo ""
echo "启动 Client:"
echo "  ./ResiBFTClient <client_id> $NUM_GENERAL $NUM_TRUSTED $NUM_ALL $NUM_FAULTS <num_transactions> <sleep_time> <exp_index>"
echo "  示例: ./ResiBFTClient 4 $NUM_GENERAL $NUM_TRUSTED $NUM_ALL $NUM_FAULTS 400 0 0"