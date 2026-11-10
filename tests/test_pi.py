#!/usr/bin/env python3
"""用 Chudnovsky 独立算一份参考值，逐字节比对各程序的输出。"""

from __future__ import annotations

import argparse
from decimal import Decimal, localcontext
from pathlib import Path
import subprocess
import sys

C3_OVER_24 = 640320**3 // 24

FAST_SIZES = (0, 1, 2, 3, 4, 5, 7, 31, 100, 1000, 15000, 20000)
SLOW_SIZES = (54936, 100000)


def binary_split(start: int, end: int) -> tuple[int, int, int]:
    if end - start == 1:
        if start == 0:
            p = q = 1
        else:
            p = (6 * start - 5) * (2 * start - 1) * (6 * start - 1)
            q = start**3 * C3_OVER_24
        t = p * (13591409 + 545140134 * start)
        return p, q, -t if start & 1 else t

    middle = (start + end) // 2
    p1, q1, t1 = binary_split(start, middle)
    p2, q2, t2 = binary_split(middle, end)
    return p1 * p2, q1 * q2, t1 * q2 + p1 * t2


def pi_digits(count: int) -> str:
    """pi 的前 count 位数字，去掉小数点：count=6 时是 "314159"。"""
    _, q, t = binary_split(0, count // 14 + 2)
    with localcontext() as context:
        context.prec = count + 30
        value = Decimal(q * 426880) * Decimal(10005).sqrt() / Decimal(t)
        return format(value, "f").replace(".", "")[:count]


def expect_pi(digits: str, decimals: int) -> str:
    if decimals == 0:
        return "3\n"
    return digits[0] + "." + digits[1 : 1 + decimals] + "\n"


def half_pi(digits: str) -> str:
    with localcontext() as context:
        context.prec = 3000
        value = Decimal(digits[:3000]) / 2
        return format(value, "f").replace(".", "")


def run(program: Path, *arguments: str, check: bool = True, timeout: int = 120):
    # 用法提示是中文，源码按 UTF-8 编译进二进制，所以这里也按 UTF-8 解。
    return subprocess.run(
        [str(program), *arguments],
        check=check,
        capture_output=True,
        text=True,
        encoding="utf-8",
        timeout=timeout,
    )


def expect_equal(label: str, actual: str, expected: str) -> None:
    if actual == expected:
        return
    index = next(
        (i for i, (a, b) in enumerate(zip(actual, expected)) if a != b),
        min(len(actual), len(expected)),
    )
    raise AssertionError(
        f"{label}: 第 {index} 个字符不对；"
        f"实际 {actual[index:index + 20]!r}，期望 {expected[index:index + 20]!r}"
    )


def check_exit(program: Path, arguments: tuple[str, ...], expected: int) -> None:
    result = run(program, *arguments, check=False)
    if result.returncode != expected:
        raise AssertionError(
            f"{program.name} {arguments} 退出码 {result.returncode}，期望 {expected}"
        )


def check_write_failure(program: Path) -> None:
    """写不出去的输出必须体现为退出码 1（需要 POSIX 的 /dev/full）。"""
    full = Path("/dev/full")
    if not full.exists():
        return
    with full.open("wb") as sink:
        result = subprocess.run(
            [str(program), "16"], stdout=sink, stderr=subprocess.PIPE, timeout=120
        )
    if result.returncode != 1:
        raise AssertionError(f"写输出失败时退出码是 {result.returncode}，期望 1")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--bin-dir", type=Path, required=True)
    parser.add_argument("--slow", action="store_true", help="另外核对 54936 和 100000 位")
    arguments = parser.parse_args()
    bin_dir = arguments.bin_dir.resolve()

    sizes = SLOW_SIZES if arguments.slow else ()
    digits = pi_digits(max(FAST_SIZES + sizes) + 1)

    def program(name: str) -> Path:
        return bin_dir / (name + (".exe" if sys.platform == "win32" else ""))

    expect_equal("01_float", run(program("demos/01_float")).stdout, "0.666667\n")
    expect_equal(
        "02_long_division",
        run(program("demos/02_long_division")).stdout,
        expect_pi(digits, 34),
    )
    half = half_pi(digits)
    expect_equal(
        "03_half_pi",
        run(program("demos/03_half_pi")).stdout,
        half[0] + "." + half[1:2801] + "\n",
    )
    expect_equal(
        "04_pi", run(program("demos/04_pi")).stdout, expect_pi(digits, 2800)
    )

    pi_program = program("pi")
    for decimals in FAST_SIZES:
        expect_equal(
            f"pi {decimals}",
            run(pi_program, str(decimals)).stdout,
            expect_pi(digits, decimals),
        )
    expect_equal("pi 默认", run(pi_program).stdout, expect_pi(digits, 15000))

    golf = run(program("pi_golf")).stdout
    expect_equal("pi_golf", golf, digits[:15000])

    for bad in ("-1", "1.5", "abc", "99999999999999999999999999999999"):
        check_exit(pi_program, (bad,), 2)
    check_exit(pi_program, ("1", "2"), 2)

    help_result = run(pi_program, "--help")
    if "用法" not in help_result.stdout or help_result.stderr:
        raise AssertionError("pi --help 应该把用法写到 stdout，stderr 保持干净")
    check_exit(pi_program, ("-h",), 0)

    check_write_failure(pi_program)

    for decimals in sizes:
        expect_equal(
            f"pi {decimals}",
            run(pi_program, str(decimals)).stdout,
            expect_pi(digits, decimals),
        )

    print("全部通过。")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
