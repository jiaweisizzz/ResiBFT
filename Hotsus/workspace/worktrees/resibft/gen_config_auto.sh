#!/bin/bash

# gen_config_auto.sh - 自动生成多服务器节点配置文件
# 用法: ./gen_config_auto.sh <num_nodes> <ip1> <ip2> <ip3> ...
# 示例: ./gen_config_auto.sh 200 47.239.159.160 39.105.182.143 1.2.3.4 5.6.7.8

NUM_NODES=$1
shift
SERVER_IPS=("$@")

NUM_SERVERS=${#SERVER_IPS[@]}

if [ $NUM_SERVERS -eq 0 ]; then
    echo "用法: ./gen_config_auto.sh <num_nodes> <ip1> <ip2> ..."
    echo "示例: ./gen_config_auto.sh 200 47.239.159.160 39.105.182.143 1.2.3.4"
    exit 1
fi

# 计算每台服务器的基础节点数和额外节点数
BASE_NODES=$((NUM_NODES / NUM_SERVERS))
EXTRA_NODES=$((NUM_NODES % NUM_SERVERS))

> config

node_id=0
for server_idx in $(seq 0 $((NUM_SERVERS - 1))); do
    # 前 EXTRA_NODES 台服务器多分配 1 个节点
    if [ $server_idx -lt $EXTRA_NODES ]; then
        nodes_on_server=$((BASE_NODES + 1))
    else
        nodes_on_server=$BASE_NODES
    fi

    SERVER_IP=${SERVER_IPS[$server_idx]}

    for i in $(seq $node_id $((node_id + nodes_on_server - 1))); do
        echo "id:$i host:$SERVER_IP port:$((8760+i)) port:$((9760+i))" >> config
    done

    node_id=$((node_id + nodes_on_server))

    echo "Server $((server_idx + 1)) ($SERVER_IP): $nodes_on_server 个节点 (ID: $((node_id - nodes_on_server)) - $((node_id - 1)))"
done

echo ""
echo "config 文件已生成，共 $(wc -l < config) 行"