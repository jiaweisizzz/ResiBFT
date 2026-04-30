#ifndef EXPERIMENT_CONFIG_H
#define EXPERIMENT_CONFIG_H

// ============================================================
// 实验参数配置（支持环境变量覆盖）
// ============================================================
// 注意：环境变量读取函数在 ResiBFT.cpp 中实现
// 因为 SGX Enclave 无法使用 getenv()

// 默认值（可通过环境变量覆盖）
// 乐观实验默认配置：
#define DEFAULT_TC_RATIO 0.0           // TC节点比例 λ：乐观模式下无TC节点
#define DEFAULT_K_RATIO 0.33            // 委员会大小比例：k = d/2

// ============================================================
// 实验模式定义
// ============================================================

// 实验模式类型
#define EXP_MODE_OPTIMISTIC   0    // 乐观模式：无拜占庭节点
#define EXP_MODE_RANDOM       1    // 随机模式：拜占庭行为随机
#define EXP_MODE_ATTACK       2    // 攻击模式：拜占庭节点共谋

// TC节点行为定义
// TC节点是拜占庭TEE节点中"投坏票"的节点
#define TC_BEHAVIOR_BAD_VOTE      0    // 投坏票（拒绝正确的Proposal）
#define TC_BEHAVIOR_SILENT        1    // 沉默（不响应）
#define TC_BEHAVIOR_INCONSISTENT  2    // 发送不一致消息

// ============================================================
// 拜占庭节点配置
// ============================================================

// 拜占庭节点数量（默认 f 个）
#define DEFAULT_BYZANTINE_COUNT_FROM_F  // 使用 numFaults 作为拜占庭节点数

// 沉默节点配置（每轮视图随机生成）
#define SILENT_NODE_RATIO 0.1      // 每轮10%节点可能沉默

#endif