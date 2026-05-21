# 分支与联调规范

## 主分支保护

- `main` 只接受 PR 合并。
- 禁止直接 push 到 `main`。

## 功能分支命名

- 底盘：`feature/chassis-*`
- 视觉：`feature/vision-*`
- 修复：`fix/*`
- 文档：`docs/*`

示例：

- `feature/chassis-pid-turn`
- `feature/vision-target-detect-v1`

## 每周联调分支

- 命名：`integration/week1`、`integration/week2` ...
- 节奏：每周固定时间从 `main` 拉取并合入本周关键功能分支。
- 目标：提前发现接口不一致、时序和异常处理问题。

## PR 要求

- 标题清晰描述改动范围。
- 必须说明是否影响 `docs/interface.md`。
- 至少 1 位队友 Review 后再合并。

