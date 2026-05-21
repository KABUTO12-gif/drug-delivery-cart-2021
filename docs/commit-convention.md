# 提交规范

建议采用简化 Conventional Commits：

- `feat:` 新功能
- `fix:` 修复问题
- `docs:` 文档更新
- `refactor:` 重构（不改行为）
- `test:` 测试相关
- `chore:` 构建/工具/杂项

示例：

- `feat(chassis): add velocity limit and dead-zone`
- `fix(vision): handle empty frame in detector`
- `docs(interface): add confidence timeout rule`

## 单次提交原则

- 一次提交只做一类事情，避免“混合提交”。
- 涉及接口变更时，代码与 `docs/interface.md` 同一 PR 提交。

