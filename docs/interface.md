# 底盘-视觉接口定义（v0.1）

本文件定义视觉模块输出给底盘模块的统一数据结构与约束。任何字段改动必须先改本文件并在 PR 中说明。

## 1. 消息方向

- 方向：`vision -> chassis`
- 频率建议：`10~30 Hz`（联调阶段可先固定 10 Hz）

## 2. 字段定义

| 字段名 | 类型 | 单位 | 说明 |
|---|---|---|---|
| `timestamp_ms` | uint64 | ms | 视觉结果时间戳（单调递增） |
| `target_yaw_deg` | float | deg | 目标相对车体偏航角，左负右正（可按项目统一） |
| `target_speed_mps` | float | m/s | 建议线速度，底盘可二次限幅 |
| `target_distance_m` | float | m | 目标距离（无效时填 `-1`） |
| `status_code` | int | - | 状态码，见下表 |
| `confidence` | float | 0~1 | 视觉置信度 |
| `frame_id` | uint32 | - | 图像帧序号，便于追踪 |

## 3. 状态码约定

| `status_code` | 含义 | 底盘建议动作 |
|---|---|---|
| `0` | 正常跟踪 | 按 `target_yaw_deg` 与 `target_speed_mps` 控制 |
| `1` | 目标短时丢失 | 降速并保持最近方向短时搜索 |
| `2` | 目标长期丢失 | 停车并等待 |
| `3` | 障碍/危险 | 立即停车 |
| `4` | 视觉初始化中 | 低速/停车，等待稳定 |

## 4. 有效性与超时

- 底盘收到消息后若 `now_ms - timestamp_ms > 200 ms`，视为过期数据。
- 连续 `500 ms` 未收到新消息，进入安全停车模式。
- `confidence < 0.4` 时底盘可降权处理（如降速）。

## 5. 示例 JSON

```json
{
  "timestamp_ms": 1716200000123,
  "target_yaw_deg": -8.5,
  "target_speed_mps": 0.35,
  "target_distance_m": 1.2,
  "status_code": 0,
  "confidence": 0.91,
  "frame_id": 1024
}
```

## 6. 版本管理

- 当前版本：`v0.1`
- 变更规则：接口字段增删或语义变化必须升级版本并记录变更说明。

