# PI15000

[![CI](https://github.com/sanxiadaba/PI15000/actions/workflows/ci.yml/badge.svg)](https://github.com/sanxiadaba/PI15000/actions/workflows/ci.yml)

这个仓库从普通浮点除法开始，逐步推导到能够连续生成圆周率数字的
spigot 算法。最终的 `test7.c` 是标准 C11 版本的紧凑写法，可输出
15000 个有效数字；`test6.c` 是可读、可配置和经过测试的正式版本，
构建后的可执行文件名为 `pi`。

相关视频：[两行代码计算 PI 后 15,000 位](https://www.bilibili.com/video/BV1Tu4y1h7Qc/)

## 文件说明

| 文件 | 内容 |
| --- | --- |
| `test2.c` | 展示普通浮点除法的有限精度 |
| `test3.c` | 用长除法输出一个有理近似能够保证的 34 位小数 |
| `test4.c` | 用十进制数组计算 `pi / 2` 的 2800 位小数 |
| `test5.c` | 在 `test4.c` 基础上得到 pi 的 2800 位小数 |
| `test6.c` | 支持任意非负小数位数的正式 CLI，构建产物为 `pi` |
| `test7.c` | 保留代码高尔夫风格的 15000 位紧凑实现 |

编号从 `test2` 开始，`test1` 不在本仓库中；表格顺序就是视频里的推导顺序。

## 前置依赖

- C11 编译器：Windows 上用 MinGW-w64 gcc 4.9.4（i686）和 MSVC 14.44（x64，
  `/std:c11`）实测通过；Linux/macOS 的 gcc、clang 由 CI 覆盖，本地未实测
- GNU make
- Python 3：只有 `make test` / `make test-slow` 需要，且仅用标准库

## 构建和测试

Windows MinGW：

```powershell
mingw32-make
mingw32-make test
```

Linux/macOS：

```bash
make
make test
```

构建使用 C11，并开启 `-Wall -Wextra -Wpedantic -Wconversion -Werror`。
测试仅依赖 Python 3 标准库，通过 Chudnovsky 算法生成独立参考值，覆盖
完整 15000 位输出、短输出边界、退出码契约和非法参数。

`make test` 大约需要 7 秒；`make test-slow` 会额外核对 50000 位，本机约 24 秒：

```bash
make test-slow
```

测试脚本默认查找 `python3`，找不到时回退到 `python`；也可以显式指定：

```bash
make test PYTHON=python3.12
```

`make clean` 会删除构建产物。

仓库附带 GitHub Actions 工作流 `.github/workflows/ci.yml`：ubuntu（gcc、clang）、
macOS（clang）与 Windows（MSYS2 + MinGW64）各跑一遍 `make test`，其中
ubuntu + gcc 额外跑 `make test-slow`。四个 jobs 在首个提交上全部通过。

## 验证状态

| 环境 | 构建 | 输出校验 |
| --- | --- | --- |
| Windows + MinGW-w64 gcc 4.9.4（i686，`-O3`） | 零警告 | `make test`、`make test-slow` 全绿 |
| Windows + MSVC 14.44（x64，`/std:c11 /W3 /O2`） | 零警告 | 20000 位与 gcc 版逐字节一致 |
| Linux/macOS + gcc/clang | CI 全绿（含 `/dev/full` 写失败用例） | 同上 |

## 使用

`pi` 的参数表示小数点后的位数；省略参数时默认输出 15000 位小数。

```powershell
.\pi.exe 100
.\pi.exe 20000 > pi.txt
```

输出是标准形式，例如 `pi 5` 输出：

```text
3.14159
```

输出约定：

- 小数位是**截断**而不是四舍五入，所以 `pi 1` 输出 `3.1`。
- `pi 0` 只输出整数部分 `3`，不带小数点。
- 退出码：`0` 成功；`1` 计算失败、内存不足或写出失败；`2` 参数用法错误。
  `--help` 打印用法并以 `0` 退出，用法信息始终写到 `stderr`。
- Windows 下重定向到文件时行尾是 `CRLF`（`0D 0A`），与逐字节参考文件比对时需要注意。
- `test7` 是高尔夫写法：固定输出 15000 位，**不含小数点、不带结尾换行**，位数写死在源码里。

## 算法

算法以 10000 为基数，每轮生成 4 个数字。余数数组初始值为
`10000 / 5 = 2000`，每输出一组数字将级数深度减少 14，即平均每个数字
需要 3.5 个级数项。

时间复杂度为 `O(n^2)`，空间复杂度为 `O(n)`。这个实现适合教学和几万位
规模；计算几十万位以上时，建议使用 Chudnovsky 二分拆分算法以及
GMP/MPIR 等高性能大整数库。

实测耗时（Windows，MinGW-w64 gcc 4.9.4 i686，`-O3`）：

| 命令 | 位数 | 耗时 |
| --- | --- | --- |
| `pi 15000` | 15000 | 约 3.5 s |
| `pi 20000` | 20000 | 约 7.6 s |
| `pi 50000` | 50000 | 约 25 s |
| `test7` | 15000 | 约 1.9 s |

## 许可证

Apache License 2.0，见 [LICENSE](LICENSE)。
