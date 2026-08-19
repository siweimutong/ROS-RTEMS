# RTExecutor

RTExecutor 独立基准工程：验证由 RTEMS 内核原生回调调度驱动的实时执行器
（消除中间件层嵌套调度）。

## 基本信息

| 项目 | 内容 |
|------|------|
| 开发人员 | siweimutong |
| 联系方式 | luoxiantong@bjut.edu.cn |

## 文件说明

- `rtexecutor.c` / `rtexecutor.cpp` — RTExecutor 基准实现
- `STExecutor/` — 单线程执行器对照
- `run.sh` — 编译运行脚本
- `wscript` — Waf 构建规格
