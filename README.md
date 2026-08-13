# unirtos-dump-flash-upload-demos

[中文](README.zh.md) | English

This repository is recommended to be used via the unirtos-cli demo workflow to ensure a consistent process for project creation, environment setup, and compilation.

## Feature Description

This demo demonstrates how to capture and upload dump data on UniRTOS when a system fault occurs. It configures the dump mode to save key fault information to flash, detects the saved dump data on the next boot, reads it out, and uploads it to a remote server via HTTP POST.

- Demonstrates configuring `qosa_dev_set_dumpcfg` with `QOSA_DUMPCFG_ENABLE_FLASH` (mode 3) to save dump key info to flash (address `0x3ed000`, 4 KB) on fault
- Demonstrates using `qosa_dev_dump_occure_check` to detect whether dump data was saved to flash after a restart
- Demonstrates reading dump data from the built-in NOR flash with `qosa_builtin_flash_read`
- Demonstrates uploading dump data to a remote server via HTTP multipart POST using the `qurl` API
- Demonstrates erasing the dump flash region with `qosa_builtin_flash_erase` after a successful upload to prevent duplicate uploads

> **Note:** The dump flash area resides inside the FOTA backup region at address `0x3ed000` (length 4 KB).  
> Regarding the actual physical address, refer to the `partition.txt` file in the DBG folder of the firmware package.  
> The printed `QUEC_FLASH_DUMP_ADDR` minus the offset `0x800000` gives the actual physical flash address.  
> Before using this demo, replace `DUMP_UPLOAD_URL` in `dump_flash_upload_demo.c` with your actual server address.

## Quick Start

### 1. Install the UniRTOS Toolchain

- [Development Preparation](https://www.quectel.com.cn/unirtos/docs?docs_page=快速上手/开发准备/开发准备.html)
- [Install the Cross-Compilation Toolchain](https://www.quectel.com.cn/unirtos/docs?docs_page=快速上手/环境搭建/环境搭建.html)
- [Install Python3](https://www.python.org/downloads/)
- [Install git](https://git-scm.com)
- Install unirtos-cli: `pip install unirtos-cli`

Once all the above tools are installed, verify the following commands are available:

```bash
python --version    # Python3
git --version
unirtos --version   # version 1.0.5 or above
unirtos-cli version # version 1.0.11 or above
```

### 2. Pull the Demo Using unirtos-cli

List available demos and versions:

```bash
unirtos-cli ls-demos
```

Create this demo project:

```bash
unirtos-cli new -r unirtos-dump-flash-upload-demos
```

To specify a version:

```bash
unirtos-cli new -r unirtos-dump-flash-upload-demos -v 1.0.0
```

### 3. Configure the Server URL

Open `dump_flash_upload_demo.c` and replace the placeholder with your actual server address:

```c
#define DUMP_UPLOAD_URL  "http://your.server.ip:port/api/dump/upload"
```

### 4. Enter the Project and Build

```bash
cd unirtos-dump-flash-upload-demos-1.0.0
unirtos-cli env-setup
unirtos-cli build
```

## Demo Flow

```
Boot
 └─ dump_flash_upload_demo_init()
     └─ dump_flash_upload_process() [task]
         ├─ 1. qosa_dev_set_dumpcfg(QOSA_DUMPCFG_ENABLE_FLASH)
         │      → On next fault, dump key info is saved to flash 0x3ed000
         ├─ 2. qosa_dev_dump_occure_check()
         │      → Returns FALSE if no dump → task exits
         │      → Returns TRUE  if dump detected → continue
         ├─ 3. qosa_builtin_flash_read(0x3ed000, buf, 4KB)
         ├─ 4. HTTP POST dump data to DUMP_UPLOAD_URL (qurl)
         └─ 5. qosa_builtin_flash_erase(0x3ed000, 4KB)  ← clear after upload
```

## Common Commands

```bash
# Open the SDK menu configuration
unirtos-cli menuconfig

# Clean build artifacts
unirtos-cli clean
```

## Technical Community

Technical Community: https://forumschinese.quectel.com/c/66-category/66

## Contribution Guidelines

Contributions are welcome. Please follow these guidelines when submitting:
- Run a basic validation before submitting: env-setup, build, clean.
- Use clear commit messages describing the purpose of the change, its scope of impact, and verification results.
- When adding new features or changing behavior, update the README and related documentation accordingly.
- Submit bug fixes and feature improvements via Issues or Pull Requests.
