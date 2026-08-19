# Exps/Exp_Intra-ST

进程内（intra-process）SingleThreaded 单线程调度策略实验。

## 基本信息

| 项目 | 内容 |
|------|------|
| 开发人员 | siweimutong |
| 联系方式 | luoxiantong@bjut.edu.cn |
| 实验类型 | 进程内调度策略对比 |
| 调度策略 | SingleThreaded |

## 目录结构

- `src/` — 实验源码
- `include/` — 头文件
- `build_all.sh` — 编译脚本（Waf 编译 + 静态链接）
- `run.sh` — QEMU 运行脚本（`./run.sh all` 依次运行 n=2,3,4,5）
- `wscript` — Waf 构建规格
- `Result/` — 实验结果（CSV / 原始输出）

## 编译与运行

```bash
# 编译（需先运行 RTcolcon 构建底层库）
./build_all.sh

# 运行全部链长实验（QEMU）
./run.sh all

# 运行指定链长
./run.sh 2
```

## 说明

- 编译链接依赖底层库静态归档（`../../rclcpp*/.../tmp/build/`），
  请先在工程根目录运行 `bash RTcolcon -j 4 -t 8 force` 完成底层库构建。
