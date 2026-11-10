#!/usr/bin/env python3
"""Regression tests using a standard-library Chudnovsky reference."""

from __future__ import annotations

import argparse
from decimal import Decimal, localcontext
from pathlib import Path
import subprocess
import sys


C3_OVER_24 = 640320**3 // 24


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


def calculate_pi(decimal_digits: int) -> str:
    terms = decimal_digits // 14 + 2
    _, q, t = binary_split(0, terms)
    with localcontext() as context:
        context.prec = decimal_digits + 30
        value = Decimal(q * 426880) * Decimal(10005).sqrt() / Decimal(t)
        return format(value, "f")


def executable(bin_dir: Path, name: str) -> Path:
    suffix = ".exe" if sys.platform == "win32" else ""
    return bin_dir / f"{name}{suffix}"


def run(
    program: Path, *arguments: str, check: bool = True, timeout: int = 60
) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [str(program), *arguments],
        check=check,
        capture_output=True,
        text=True,
        encoding="ascii",
        timeout=timeout,
    )


def check_write_failure(pi_program: Path) -> None:
    """写不出去的输出必须体现为退出码 1（需要 POSIX 的 /dev/full）。"""
    full = Path("/dev/full")
    if not full.exists():
        return

    with full.open("wb") as sink:
        result = subprocess.run(
            [str(pi_program), "16"],
            stdout=sink,
            stderr=subprocess.PIPE,
            timeout=60,
        )

    if result.returncode != 1:
        raise AssertionError(
            f"pi returned {result.returncode} although stdout could not be written"
        )


def expect_equal(label: str, actual: str, expected: str) -> None:
    if actual != expected:
        mismatch = next(
            (index for index, pair in enumerate(zip(actual, expected)) if pair[0] != pair[1]),
            min(len(actual), len(expected)),
        )
        raise AssertionError(
            f"{label}: mismatch at index {mismatch}; "
            f"actual={actual[mismatch:mismatch + 20]!r}, "
            f"expected={expected[mismatch:mismatch + 20]!r}"
        )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--bin-dir", type=Path, required=True)
    parser.add_argument(
        "--slow", action="store_true", help="also verify 50000 decimal digits"
    )
    arguments = parser.parse_args()
    bin_dir = arguments.bin_dir.resolve()

    pi_reference = calculate_pi(50020 if arguments.slow else 20020)
    pi_digits = pi_reference.replace(".", "")

    expect_equal("test2", run(executable(bin_dir, "test2")).stdout, "0.666667\n")
    expect_equal(
        "test3",
        run(executable(bin_dir, "test3")).stdout,
        pi_reference[: 2 + 34] + "\n",
    )

    with localcontext() as context:
        context.prec = 15020
        half_reference = format(Decimal(pi_reference) / 2, "f")
    expect_equal(
        "test4",
        run(executable(bin_dir, "test4")).stdout,
        half_reference[: 2 + 2800] + "\n",
    )
    expect_equal(
        "test5",
        run(executable(bin_dir, "test5")).stdout,
        pi_reference[: 2 + 2800] + "\n",
    )

    pi_program = executable(bin_dir, "pi")
    for decimal_digits in (0, 1, 2, 3, 4, 5, 7, 31, 100, 1000):
        expected = "3" if decimal_digits == 0 else pi_reference[: 2 + decimal_digits]
        expect_equal(
            f"pi {decimal_digits}",
            run(pi_program, str(decimal_digits)).stdout,
            expected + "\n",
        )

    expect_equal(
        "pi default",
        run(pi_program).stdout,
        pi_reference[: 2 + 15000] + "\n",
    )
    expect_equal(
        "pi 20000",
        run(pi_program, "20000").stdout,
        pi_reference[: 2 + 20000] + "\n",
    )
    expect_equal(
        "test7",
        run(executable(bin_dir, "test7")).stdout,
        pi_digits[:15000],
    )

    if arguments.slow:
        expect_equal(
            "pi 50000",
            run(pi_program, "50000", timeout=300).stdout,
            pi_reference[: 2 + 50000] + "\n",
        )

    usage_error_cases = (
        ("-1",),
        ("1.5",),
        ("abc",),
        ("1", "2"),
        ("99999999999999999999999999999999",),
    )
    for invalid_arguments in usage_error_cases:
        result = run(pi_program, *invalid_arguments, check=False)
        if result.returncode != 2:
            raise AssertionError(
                f"pi returned {result.returncode} for invalid arguments "
                f"{invalid_arguments!r}, expected 2"
            )

    # 恰好等于 64 位 size_t 上限的位数：在 32 位目标上无法解析（用法错误，2），
    # 在 64 位目标上能解析但精度过大（计算失败，1）。这里只要求它被拒绝。
    oversized = "18446744073709551615"
    result = run(pi_program, oversized, check=False)
    if result.returncode not in (1, 2):
        raise AssertionError(
            f"pi returned {result.returncode} for {oversized!r}, expected 1 or 2"
        )

    help_result = run(pi_program, "--help")
    if "Usage:" not in help_result.stderr:
        raise AssertionError("pi --help did not print usage information")
    if help_result.stdout:
        raise AssertionError("pi --help wrote the usage text to stdout")

    check_write_failure(pi_program)

    print("All PI15000 tests passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
