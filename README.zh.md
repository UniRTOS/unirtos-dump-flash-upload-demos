# unirtos-dump-flash-upload-demos

中文 | [English](README.md)

本仓库推荐通过 unirtos-cli 的 demo 工作流使用，以保证创建、环境拉取和编译流程一致。

## 功能描述

本 Demo 展示 UniRTOS 上系统异常时如何捕获并上传 dump 数据。通过配置 dump 模式将关键故障信息保存到 Flash，在下次启动时检测已保存的 dump 数据，读取后通过 HTTP POST 上传到远程服务器。

- 演示通过 `qosa_dev_set_dumpcfg` 配置 `QOSA_DUMPCFG_ENABLE_FLASH`（模式 3），使设备在发生故障时将 dump 关键信息保存到 Flash（地址 `0x3ed000`，大小 4 KB）
- 演示通过 `qosa_dev_dump_occure_check` 在重启后检测 Flash 中是否存在已保存的 dump 数据
- 演示通过 `qosa_builtin_flash_read` 从内置 NOR Flash 读取 dump 数据
- 演示通过 `qurl` API 以 HTTP multipart POST 方式将 dump 数据上传到远程服务器
- 演示上传成功后通过 `qosa_builtin_flash_erase` 擦除 dump Flash 区域，防止重复上传

> **注意：** dump Flash 区域位于 FOTA 备份分区内，地址为 `0x3ed000`，大小 4 KB。  
> 关于实际物理地址，请参考版本固件包 DBG 文件夹下的 `partition.txt` 文件。  
> 日志打印的 `QUEC_FLASH_DUMP_ADDR` 减去偏移 `0x800000` 即为实际物理 Flash 地址。  
> 使用本 Demo 前，请将 `dump_flash_upload_demo.c` 中的 `DUMP_UPLOAD_URL` 替换为实际服务器地址。

## 快速上手

### 1. 安装 UniRTOS 工具链

- [开发准备](https://www.quectel.com.cn/unirtos/docs?docs_page=快速上手/开发准备/开发准备.html)
- [安装交叉编译工具链](https://www.quectel.com.cn/unirtos/docs?docs_page=快速上手/环境搭建/环境搭建.html)
- [安装 Python3](https://www.python.org/downloads/)
- [安装 git](https://git-scm.com)
- 安装 unirtos-cli：`pip install unirtos-cli`

以上工具安装完成后，确认以下命令可用：

```bash
python --version    # Python3
git --version
unirtos --version   # 1.0.5 及以上版本
unirtos-cli version # 1.0.11 及以上版本
```

### 2. 使用 unirtos-cli 拉取 demo

先查看可用 demo 与版本：

```bash
unirtos-cli ls-demos
```

创建本 demo 工程：

```bash
unirtos-cli new -r unirtos-dump-flash-upload-demos
```

如需指定版本：

```bash
unirtos-cli new -r unirtos-dump-flash-upload-demos -v 1.0.0
```

### 3. 配置服务器地址

打开 `dump_flash_upload_demo.c`，将占位地址替换为实际服务器地址：

```c
#define DUMP_UPLOAD_URL  "http://your.server.ip:port/api/dump/upload"
```

### 4. 进入工程并编译

```bash
cd unirtos-dump-flash-upload-demos-1.0.0
unirtos-cli env-setup
unirtos-cli build
```

## Demo 流程

```
启动
 └─ dump_flash_upload_demo_init()
     └─ dump_flash_upload_process() [任务]
         ├─ 1. qosa_dev_set_dumpcfg(QOSA_DUMPCFG_ENABLE_FLASH)
         │      → 下次发生故障时，dump 关键信息将保存到 Flash 0x3ed000
         ├─ 2. qosa_dev_dump_occure_check()
         │      → 返回 FALSE：无 dump 数据 → 任务退出
         │      → 返回 TRUE ：检测到 dump → 继续执行
         ├─ 3. qosa_builtin_flash_read(0x3ed000, buf, 4KB)
         ├─ 4. HTTP POST 将 dump 数据上传到 DUMP_UPLOAD_URL（qurl）
         └─ 5. qosa_builtin_flash_erase(0x3ed000, 4KB)  ← 上传成功后擦除
```

## 常用命令

```bash
# 打开 SDK 菜单配置
unirtos-cli menuconfig

# 清理构建产物
unirtos-cli clean
```

## 技术社区

技术社区：https://forumschinese.quectel.com/c/66-category/66

## 贡献指南

欢迎参与共建，建议按以下方式提交：
- 提交前先执行一次基础验证：env-setup、build、clean。
- 使用清晰的提交说明，描述改动目的、影响范围和验证结果。
- 新增功能或行为变化时，同步更新 README 与相关文档。
- 通过 Issue 或 Pull Request 提交问题修复与功能改进。
