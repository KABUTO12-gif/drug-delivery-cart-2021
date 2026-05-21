# Drug Delivery Cart 2021

送药小车项目协作仓库（底盘控制 + 视觉控制）。

## 目录结构

```
.
├── chassis/                  # 底盘控制代码（运动控制、驱动、通信）
├── vision/                   # 视觉控制代码（检测、定位、目标跟踪）
└── docs/                     # 方案、接口、调参和协作规范
    ├── interface.md
    ├── branching.md
    └── commit-convention.md
```

## 协作流程（强制）

1. 不直接推送 `main`。
2. 开发在功能分支进行：
   - 底盘：`feature/chassis-*`
   - 视觉：`feature/vision-*`
3. 通过 Pull Request 合并到 `main`。
4. 每周建立联调分支：`integration/weekN`，做接口和系统联调。

## 快速开始

1. 先阅读 [docs/interface.md](docs/interface.md) 并确认字段定义。
2. 底盘同学在 `chassis/` 开发，视觉同学在 `vision/` 开发。
3. 提交前遵循 [docs/commit-convention.md](docs/commit-convention.md)。

