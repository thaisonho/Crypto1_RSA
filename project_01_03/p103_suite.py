#!/usr/bin/env python3
import argparse
import math
import os
import random
import subprocess
import sys
from pathlib import Path


# =======================
#  Helper: hex <-> int
# =======================
def to_le_hex(n: int) -> str:
    """Đổi số nguyên sang hex little endian (in HOA, không 0x, không leading zero)."""
    if n == 0:
        return "0"
    s = format(n, "X")  # big-endian hex, uppercase
    return s[::-1]      # đảo chuỗi -> little endian


def from_le_hex(s: str) -> int:
    """(Không dùng cho bài này; để đủ cặp.)"""
    s = s.strip()
    if s == "" or s == "0":
        return 0
    return int(s[::-1], 16)


# =======================
#  Test generation
# =======================
def gen_tests(out_dir: Path, seed: int = 42, total_tests: int = 5000) -> None:
    """
    Sinh test cho bài 01_03.

    - Tổng số test: total_tests (mặc định 5000).
    - Mỗi 100 test dùng 1 seed khác nhau: seed + batch_index.
    - Trong mỗi batch 100 test:
        40 test: bits(N) <= 128
        30 test: 128 < bits(N) <= 256
        20 test: 256 < bits(N) <= 512
        10 test: 512 < bits(N) <= 1024

    Mỗi file .inp:
        Dòng 1: N  (hex little endian, uppercase)
        Dòng 2: k  (hex little endian, uppercase)
        Dòng 3: x  (hex little endian, uppercase)

    Mỗi file .out:
        Dòng 1: y = x^k mod N  (hex little endian, uppercase)
    """
    out_dir.mkdir(parents=True, exist_ok=True)

    batch_size = 100
    if total_tests % batch_size != 0:
        raise ValueError("total_tests phải chia hết cho 100 để giữ đúng phân bố.")

    num_batches = total_tests // batch_size

    # (số test trong 1 batch, min_bits_exclusive, max_bits_inclusive)
    config_per_batch = [
        (40, 0,   128),   # N <= 128 bit
        (30, 128, 256),   # 128 < N <= 256
        (20, 256, 512),   # 256 < N <= 512
        (10, 512, 1024),  # 512 < N <= 1024
    ]

    idx = 0
    for batch in range(num_batches):
        # seed mới cho mỗi 100 test
        random.seed(seed + batch)

        for count, low_excl, high_inc in config_per_batch:
            lo_bits = max(low_excl + 1, 2)
            hi_bits = high_inc

            for _ in range(count):
                # chọn độ dài bit cho N
                L = random.randint(lo_bits, hi_bits)
                # sinh N có đúng L bit, bit cao nhất = 1
                N = random.getrandbits(L - 1) | (1 << (L - 1))
                if N < 3:
                    N = 3

                # k: 1 <= k < N
                k = random.randint(1, N - 1)

                # x: 1 <= x < N, cố gắng sao cho gcd(x, N) = 1
                while True:
                    x = random.randint(1, N - 1)
                    if math.gcd(x, N) == 1:
                        break

                y = pow(x, k, N)

                name = f"test_{idx:04d}"   # 0000 .. 4999
                idx += 1

                inp_path = out_dir / f"{name}.inp"
                out_path = out_dir / f"{name}.out"

                with inp_path.open("w") as f:
                    f.write(to_le_hex(N) + "\n")
                    f.write(to_le_hex(k) + "\n")
                    f.write(to_le_hex(x) + "\n")

                with out_path.open("w") as f:
                    f.write(to_le_hex(y) + "\n")

    print(f"Generated {idx} tests in {out_dir}")


# =======================
#  Verification
# =======================
def iter_tests(test_dir: Path):
    for inp in sorted(test_dir.glob("*.inp")):
        name = inp.stem
        out = test_dir / f"{name}.out"
        if out.exists():
            yield name, inp, out


def verify(prog: Path, test_dir: Path, out_dir: Path, time_limit: float = 60.0) -> int:
    """
    Chạy prog trên toàn bộ test trong test_dir, ghi output vào out_dir,
    rồi so sánh với .out chuẩn. Trả về số test sai.
    """
    out_dir.mkdir(parents=True, exist_ok=True)
    failures = 0

    for name, inp, expected in iter_tests(test_dir):
        run_out = out_dir / f"{name}.out"

        # ./bin/p103 input output
        res = subprocess.run(
            [str(prog), str(inp), str(run_out)],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            timeout=time_limit,
        )

        if res.returncode != 0:
            print(f"[{name}] ERROR: return code {res.returncode}")
            if res.stderr:
                print(res.stderr.strip())
            failures += 1
            continue

        exp = expected.read_text().strip()
        got = run_out.read_text().strip()

        if exp != got:
            print(f"[{name}] MISMATCH")
            print("  expected:", exp)
            print("  got     :", got)
            failures += 1
        else:
            # Nếu muốn yên lặng thì comment dòng dưới.
            print(f"[{name}] OK")

    print(f"Total failures: {failures}")
    return failures


# =======================
#  CLI
# =======================
def main(argv=None):
    parser = argparse.ArgumentParser(
        description="Test generator & verifier cho project_01_03 (RSA powmod)."
    )
    sub = parser.add_subparsers(dest="cmd", required=True)

    # gen / genmix
    p_gen = sub.add_parser("gen", help="Sinh test.")
    p_gen.add_argument("out_dir", help="Thư mục chứa test (.inp/.out).")
    p_gen.add_argument("--seed", type=int, default=42, help="Seed gốc (mặc định 42).")
    p_gen.add_argument(
        "--total",
        type=int,
        default=5000,
        help="Tổng số test (mặc định 5000, phải chia hết cho 100).",
    )

    p_genmix = sub.add_parser("genmix", help="Alias của 'gen'.")
    p_genmix.add_argument("out_dir", help="Thư mục chứa test (.inp/.out).")
    p_genmix.add_argument("--seed", type=int, default=42)
    p_genmix.add_argument("--total", type=int, default=5000)

    # verify
    p_ver = sub.add_parser("verify", help="Chạy chương trình C++ và so sánh kết quả.")
    p_ver.add_argument("prog", help="Đường dẫn tới binary (vd: bin/p103).")
    p_ver.add_argument("test_dir", help="Thư mục test (.inp/.out).")
    p_ver.add_argument("out_dir", help="Thư mục để ghi output của chương trình.")
    p_ver.add_argument("--limit", type=float, default=60.0, help="Timeout mỗi test (giây).")

    args = parser.parse_args(argv)

    if args.cmd in ("gen", "genmix"):
        gen_tests(Path(args.out_dir), seed=args.seed, total_tests=args.total)
    elif args.cmd == "verify":
        failures = verify(Path(args.prog), Path(args.test_dir), Path(args.out_dir), args.limit)
        if failures:
            sys.exit(1)
    else:
        parser.error("unknown command")


if __name__ == "__main__":
    main()
