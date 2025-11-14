# RSA Key Generation Test Script

## Overview
This test script (`test_rsa.py`) generates 100 test cases for the RSA key generation program (project_01_02) and compares the C++ program output with Python's reference implementation using `gmpy2`.

## Prerequisites

1. **Python 3** with `gmpy2` library installed:
   ```bash
   pip install gmpy2
   ```

2. **Compiled C++ program**:
   ```bash
   g++ -O3 -o main main.cpp
   ```

## Test Distribution

The script generates 100 tests with the following distribution:

- **10%** (10 tests): p and q are 256-512 bit primes
- **20%** (20 tests): p and q are 128-256 bit primes  
- **30%** (30 tests): p and q are 64-128 bit primes
- **40%** (40 tests): p and q are <64 bit primes

## Input/Output Format

### Input Format (`.inp` file)
Three lines, each containing a hexadecimal number in LSB-to-MSB format:
```
<p in hex LSB format>
<q in hex LSB format>
<e in hex LSB format>
```

Where:
- `p`, `q`: Prime numbers
- `e`: Public exponent (e < phi(n) and gcd(e, phi(n)) = 1)

### Output Format (`.out` file)
Single line:
- If `d` exists: hexadecimal string in LSB-to-MSB format
- If `d` doesn't exist: `-1`

### Hex Format Explanation
The format is: `(h_0*16^0 + h_1*16^1 + ... + h_k*16^k)`

Example:
- String `"B5"` represents: `B*16^0 + 5*16^1 = 11 + 80 = 91`
- Number `91` converts to: `"B5"`

## Usage

```bash
cd project_01_02
./test_rsa.py
```

Or:
```bash
python3 test_rsa.py
```

## Test Timeout

Each test has a 60-second timeout. If a test exceeds this limit, it will be marked as an error.

## Output

The script will:
1. Generate 100 test cases with various bit sizes
2. Run your C++ program on each test case
3. Compare outputs with expected results
4. Display:
   - Individual test results (errors and failures)
   - Summary statistics (pass/fail/error counts)
   - Total and average execution times

### Generated Files

Test files are saved in `generated_tests/` directory:
- `test_XXX.inp`: Input file
- `test_XXX.out`: Output file from your program
- `test_XXX.log`: Detailed log for failed tests (only created for failures)

## Understanding Results

- **PASS**: Your program output matches the expected result
- **FAIL**: Your program produced incorrect output
- **ERROR**: Your program crashed, timed out, or produced invalid output

## Troubleshooting

1. **"gmpy2 not found"**: Install with `pip install gmpy2`
2. **"./main not found"**: Compile your program first
3. **Test failures**: Check `generated_tests/test_XXX.log` files for details
4. **Timeout errors**: Your algorithm might be too slow for large numbers

## Algorithm Notes

Your C++ program should:
1. Read p, q, e from input file (hex LSB format)
2. Calculate `phi(n) = (p-1)(q-1)`
3. Compute `d = e^(-1) mod phi(n)` using Extended Euclidean Algorithm
4. Output d in hex LSB format, or -1 if no inverse exists
