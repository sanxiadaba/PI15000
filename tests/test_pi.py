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


def run(program: Path, *arguments: str, check: bool = True) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [str(program), *arguments],
        check=check,
        capture_output=True,
        text=True,
        encoding="ascii",
        timeout=60,
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
    arguments = parser.parse_args()
    bin_dir = arguments.bin_dir.resolve()

    pi_reference = calculate_pi(20020)
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

    invalid_cases = (("-1",), ("1.5",), ("abc",), ("1", "2"), ("18446744073709551615",))
    for invalid_arguments in invalid_cases:
        result = run(pi_program, *invalid_arguments, check=False)
        if result.returncode == 0:
            raise AssertionError(f"pi accepted invalid arguments: {invalid_arguments!r}")

    help_result = run(pi_program, "--help")
    if "Usage:" not in help_result.stderr:
        raise AssertionError("pi --help did not print usage information")

    print("All PI15000 tests passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
