#!/usr/bin/env python3
"""
RSA key generation test script for project_01_02.
Generates 100 test cases with varying bit sizes and compares Python results with C++ program.
"""

import sys
import os
import subprocess
import random
import time
from pathlib import Path

try:
    import gmpy2
    from gmpy2 import mpz
except ImportError:
    print("Error: gmpy2 is required. Install with: pip install gmpy2")
    sys.exit(1)


def int_to_hex_lsb_format(n):
    """
    Convert integer to hex string in LSB-to-MSB format.
    Format: (h_0*16^0 + h_1*16^1 + ... + h_k*16^k)
    where h_0 is the FIRST character (leftmost).
    
    Example: 91 = 11 + 80 = B*16^0 + 5*16^1 -> "B5"
    """
    if n == 0:
        return "0"
    
    hex_digits = []
    while n > 0:
        digit = n % 16
        if digit < 10:
            hex_digits.append(chr(ord('0') + digit))
        else:
            hex_digits.append(chr(ord('A') + digit - 10))
        n //= 16
    
    return ''.join(hex_digits)


def hex_to_int_lsb_format(hex_string):
    """
    Convert hex string from LSB-to-MSB format to integer.
    Format: (h_0*16^0 + h_1*16^1 + ... + h_k*16^k)
    where h_0 is the FIRST character (leftmost).
    """
    hex_string = hex_string.strip().upper()
    
    if not hex_string or hex_string == "0":
        return 0
    
    result = 0
    for i, char in enumerate(hex_string):
        if '0' <= char <= '9':
            digit = ord(char) - ord('0')
        elif 'A' <= char <= 'F':
            digit = ord(char) - ord('A') + 10
        else:
            continue
        
        result += digit * (16 ** i)
    
    return result


def generate_random_prime(bit_size_min, bit_size_max):
    """
    Generate a random prime number with bit size in range [bit_size_min, bit_size_max].
    """
    bit_size = random.randint(bit_size_min, bit_size_max)
    
    while True:
        # Generate random number with specified bit size
        n = random.getrandbits(bit_size)
        
        # Ensure it has the correct bit size (MSB should be 1)
        n |= (1 << (bit_size - 1))
        
        # Make it odd
        n |= 1
        
        # Check if prime
        if gmpy2.is_prime(n, 25):
            return mpz(n)


def generate_e(p, q):
    """
    Generate a valid public exponent e.
    e must satisfy: 1 < e < phi(n) and gcd(e, phi(n)) = 1
    where phi(n) = (p-1)(q-1)
    """
    phi = (p - 1) * (q - 1)
    
    # Common choices for e
    common_e_values = [3, 5, 17, 257, 65537]
    
    # Try common values first
    for e in common_e_values:
        if e < phi and gmpy2.gcd(e, phi) == 1:
            return mpz(e)
    
    # Generate random e
    max_attempts = 100
    for _ in range(max_attempts):
        e = random.randint(3, min(int(phi) - 1, 1000000))
        if e % 2 == 1 and gmpy2.gcd(e, phi) == 1:
            return mpz(e)
    
    # Fallback to a small odd number
    e = mpz(3)
    while gmpy2.gcd(e, phi) != 1:
        e += 2
    
    return e


def compute_d(p, q, e):
    """
    Compute the private key d given p, q, and e.
    d = e^(-1) mod phi(n), where phi(n) = (p-1)(q-1)
    
    Returns:
        d if it exists, -1 otherwise
    """
    phi = (p - 1) * (q - 1)
    
    # Check if e and phi are coprime
    if gmpy2.gcd(e, phi) != 1:
        return -1
    
    # Compute modular inverse
    try:
        d = gmpy2.invert(e, phi)
        return d
    except:
        return -1


def generate_test_case(test_num, bit_size_min, bit_size_max):
    """
    Generate a single test case.
    
    Returns:
        (p, q, e, expected_d)
    """
    p = generate_random_prime(bit_size_min, bit_size_max)
    q = generate_random_prime(bit_size_min, bit_size_max)
    
    # Ensure p != q
    while p == q:
        q = generate_random_prime(bit_size_min, bit_size_max)
    
    # Generate valid e
    e = generate_e(p, q)
    
    # Compute expected d
    expected_d = compute_d(p, q, e)
    
    return p, q, e, expected_d


def run_cpp_program(input_file, output_file, timeout=60):
    """
    Run the C++ program with the given input file and collect output.
    
    Returns:
        (success, output, elapsed_time)
    """
    cpp_program = "./main"
    
    try:
        start_time = time.time()
        result = subprocess.run(
            [cpp_program, input_file, output_file],
            timeout=timeout,
            capture_output=True,
            text=True
        )
        elapsed_time = time.time() - start_time
        
        if result.returncode != 0:
            return False, f"Program exited with code {result.returncode}\nStderr: {result.stderr}", elapsed_time
        
        # Read output file
        if not os.path.exists(output_file):
            return False, "Output file not created", elapsed_time
        
        with open(output_file, 'r') as f:
            output = f.read().strip()
        
        return True, output, elapsed_time
        
    except subprocess.TimeoutExpired:
        return False, f"Timeout ({timeout}s exceeded)", timeout
    except Exception as e:
        return False, f"Error running program: {str(e)}", 0


def main():
    print("=" * 80)
    print("RSA Key Generation Test Script (project_01_02)")
    print("=" * 80)
    
    # Check if C++ program exists
    cpp_program = "./main"
    if not os.path.exists(cpp_program):
        print(f"Error: {cpp_program} not found!")
        print("Please compile the program first: g++ -O3 -o main main.cpp")
        sys.exit(1)
    
    # Create test directories
    test_dir = Path("generated_tests")
    test_dir.mkdir(exist_ok=True)
    
    # Test distribution
    total_tests = 100
    test_distribution = [
        (10, 256, 512),   # 10% of tests: 256-512 bits
        (20, 128, 256),   # 20% of tests: 128-256 bits
        (30, 64, 128),    # 30% of tests: 64-128 bits
        (40, 8, 64),      # 40% of tests: < 64 bits
    ]
    
    # Generate test cases
    print(f"\nGenerating {total_tests} test cases...")
    print("-" * 80)
    
    test_cases = []
    test_idx = 0
    
    for count, bit_min, bit_max in test_distribution:
        print(f"Generating {count} tests with {bit_min}-{bit_max} bit primes...")
        for i in range(count):
            p, q, e, expected_d = generate_test_case(test_idx, bit_min, bit_max)
            test_cases.append({
                'num': test_idx,
                'p': p,
                'q': q,
                'e': e,
                'expected_d': expected_d,
                'bit_range': f"{bit_min}-{bit_max}"
            })
            test_idx += 1
    
    print(f"\nGenerated {len(test_cases)} test cases successfully!")
    
    # Run tests
    print("\n" + "=" * 80)
    print("Running tests...")
    print("=" * 80)
    
    passed = 0
    failed = 0
    errors = 0
    total_time = 0
    
    for i, test in enumerate(test_cases):
        test_num = test['num']
        
        # Write input file
        input_file = test_dir / f"test_{test_num:03d}.inp"
        output_file = test_dir / f"test_{test_num:03d}.out"
        
        with open(input_file, 'w') as f:
            f.write(f"{int_to_hex_lsb_format(test['p'])}\n")
            f.write(f"{int_to_hex_lsb_format(test['q'])}\n")
            f.write(f"{int_to_hex_lsb_format(test['e'])}\n")
        
        # Run C++ program
        success, output, elapsed = run_cpp_program(str(input_file), str(output_file))
        total_time += elapsed
        
        if not success:
            errors += 1
            print(f"Test {test_num:3d} [{test['bit_range']:>8} bits]: ERROR - {output}")
            continue
        
        # Parse output
        if output == "-1":
            cpp_d = -1
        else:
            try:
                cpp_d = hex_to_int_lsb_format(output)
            except:
                errors += 1
                print(f"Test {test_num:3d} [{test['bit_range']:>8} bits]: ERROR - Invalid output format: {output}")
                continue
        
        # Compare results
        expected_d = test['expected_d']
        
        if cpp_d == expected_d:
            passed += 1
            status = "PASS"
            if (i + 1) % 10 == 0:
                print(f"Test {test_num:3d} [{test['bit_range']:>8} bits]: {status} ({elapsed:.3f}s)")
        else:
            failed += 1
            status = "FAIL"
            print(f"Test {test_num:3d} [{test['bit_range']:>8} bits]: {status} ({elapsed:.3f}s)")
            print(f"  Expected: {expected_d if expected_d == -1 else int_to_hex_lsb_format(expected_d)}")
            print(f"  Got:      {output}")
            
            # Save detailed info for failed test
            with open(test_dir / f"test_{test_num:03d}.log", 'w') as f:
                f.write(f"Test {test_num} FAILED\n")
                f.write(f"Bit range: {test['bit_range']}\n")
                f.write(f"p = {test['p']}\n")
                f.write(f"q = {test['q']}\n")
                f.write(f"e = {test['e']}\n")
                f.write(f"Expected d = {expected_d}\n")
                f.write(f"Got d = {cpp_d}\n")
    
    # Summary
    print("\n" + "=" * 80)
    print("Test Summary")
    print("=" * 80)
    print(f"Total tests:  {total_tests}")
    print(f"Passed:       {passed} ({100*passed/total_tests:.1f}%)")
    print(f"Failed:       {failed} ({100*failed/total_tests:.1f}%)")
    print(f"Errors:       {errors} ({100*errors/total_tests:.1f}%)")
    print(f"Total time:   {total_time:.2f}s")
    print(f"Average time: {total_time/total_tests:.3f}s per test")
    print("=" * 80)
    
    if failed == 0 and errors == 0:
        print("✓ All tests passed!")
        return 0
    else:
        print("✗ Some tests failed or encountered errors.")
        print(f"Check {test_dir}/ for detailed logs of failed tests.")
        return 1


if __name__ == '__main__':
    sys.exit(main())
