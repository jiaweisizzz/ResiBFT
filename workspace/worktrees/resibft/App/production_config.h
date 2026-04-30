#ifndef PRODUCTION_CONFIG_H
#define PRODUCTION_CONFIG_H

// 生产模式开关
// 取消注释以启用生产模式
// #define PRODUCTION_MODE

// ============================================================
// 委员会大小 k 配置（支持两种模式）
// ============================================================

// 模式1：固定范围采样（默认）
// k 从 [COMMITTEE_K_MIN, COMMITTEE_K_MAX] 范围内随机采样
#define COMMITTEE_K_MIN 10           // k_bar: 最小委员会大小
#define COMMITTEE_K_MAX_DEFAULT 100  // 默认最大委员会大小

// 模式2：相对比例计算（实验方案要求）
// k = d * COMMITTEE_K_RATIO，其中 d 为 TEE节点总数
// 启用方式：
//   1. 定义 COMMITTEE_K_RATIO_MODE（取消下方注释）
//   2. 或设置环境变量 RESIBFT_K_RATIO（优先级更高）
// 可选比例值：1/3, 1/2, 2/3, 3/4（对应实验方案 k=[d/3, d/2, 2d/3, 3d/4]）
#define COMMITTEE_K_RATIO_MODE    // 取消注释启用比例模式
#define COMMITTEE_K_RATIO 0.5       // 默认比例：k = d/2

// ============================================================
// TC节点比例配置（实验方案要求）
// ============================================================
// TC节点是拜占庭TEE节点中投坏票的节点
// λ = TC节点数 / TEE节点总数
// 启用方式：设置环境变量 RESIBFT_TC_RATIO
// 实验方案：λ=[0.1, 0.2, 0.3]
#define TC_RATIO 0.2                // 默认：20%的TEE节点为TC节点

// ============================================================
// VRF seed 配置 (论文 Section 4.1)
// ============================================================
#define VRF_SEED_HISTORY_SIZE 3      // 组合的 checkpoint 数量
#define VRF_SEED_DELAY_VIEWS 30      // 种子稳定延迟 (3 * CHECKPOINT_INTERVAL)

// ============================================================
// 状态恢复配置（从 GRACIOUS/UNCIVIL 恢复到 NORMAL）
// ============================================================
// 论文设计：当共识恢复正常时，应从特殊模式恢复到 NORMAL
#define STAGE_RECOVERY_SUCCESSFUL_BLOCKS 10  // 成功完成 N 个区块后可恢复
#define STAGE_RECOVERY_ACCEPTED_VC_THRESHOLD 2  // ACCEPTED VC 达到阈值后可恢复

#endif