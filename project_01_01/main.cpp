#include <iostream>
#include <fstream>
#include <string>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <ctime>
#include <cstdlib>

using namespace std;

// BigInt class using bit-wise manipulation with 32-bit words
class BigInt {
private:
    // Only need space for 512-bit numbers since we do modular arithmetic
    enum { MAX_WORDS = 17 }; // 512 bits / 32 = 16 words + 1 for safety
    uint32_t data[MAX_WORDS];
    int size; // number of significant words

    void normalize() {
        while (size > 1 && data[size - 1] == 0) {
            size--;
        }
    }

public:
    BigInt() : size(1) {
        memset(data, 0, sizeof(data));
    }

    BigInt(uint64_t val) : size(1) {
        memset(data, 0, sizeof(data));
        data[0] = (uint32_t)(val & 0xFFFFFFFF);
        data[1] = (uint32_t)(val >> 32);
        if (data[1] != 0) size = 2;
    }

    BigInt(const string& hex) : size(1) {
        memset(data, 0, sizeof(data));
        fromHex(hex);
    }

    void fromHex(const string& hex) {
        memset(data, 0, sizeof(data));
        size = 1;
        
        if (hex.empty() || hex == "0") {
            return;
        }
        
        // Process hex string from right to left (LSB first)
        int bitPos = 0;
        for (int i = 0; i < (int)hex.length(); i++) {
            char c = hex[i];
            int val;
            if (c >= '0' && c <= '9') val = c - '0';
            else if (c >= 'A' && c <= 'F') val = c - 'A' + 10;
            else if (c >= 'a' && c <= 'f') val = c - 'a' + 10;
            else continue;
            
            // Add val * 16^i
            for (int bit = 0; bit < 4; bit++) {
                if (val & (1 << bit)) {
                    setBit(bitPos + bit);
                }
            }
            bitPos += 4;
        }
        normalize();
    }

    void setBit(int pos) {
        int wordIdx = pos / 32;
        int bitIdx = pos % 32;
        if (wordIdx < MAX_WORDS) {
            data[wordIdx] |= (1U << bitIdx);
            if (wordIdx >= size) size = wordIdx + 1;
        }
    }

    bool getBit(int pos) const {
        int wordIdx = pos / 32;
        int bitIdx = pos % 32;
        if (wordIdx >= size) return false;
        return (data[wordIdx] >> bitIdx) & 1;
    }

    bool isZero() const {
        return size == 1 && data[0] == 0;
    }

    bool isOne() const {
        return size == 1 && data[0] == 1;
    }

    bool isEven() const {
        return (data[0] & 1) == 0;
    }

    int bitLength() const {
        if (isZero()) return 0;
        int len = (size - 1) * 32;
        uint32_t top = data[size - 1];
        while (top > 0) {
            len++;
            top >>= 1;
        }
        return len;
    }

    bool operator==(const BigInt& other) const {
        if (size != other.size) return false;
        for (int i = 0; i < size; i++) {
            if (data[i] != other.data[i]) return false;
        }
        return true;
    }

    bool operator!=(const BigInt& other) const {
        return !(*this == other);
    }

    bool operator<(const BigInt& other) const {
        if (size != other.size) return size < other.size;
        for (int i = size - 1; i >= 0; i--) {
            if (data[i] != other.data[i]) {
                return data[i] < other.data[i];
            }
        }
        return false;
    }

    bool operator<=(const BigInt& other) const {
        return *this < other || *this == other;
    }

    bool operator>(const BigInt& other) const {
        return !(*this <= other);
    }

    bool operator>=(const BigInt& other) const {
        return !(*this < other);
    }

    BigInt operator+(const BigInt& other) const {
        BigInt result;
        uint64_t carry = 0;
        int maxSize = max(size, other.size);
        
        for (int i = 0; i < maxSize || carry; i++) {
            uint64_t sum = carry;
            if (i < size) sum += data[i];
            if (i < other.size) sum += other.data[i];
            result.data[i] = (uint32_t)(sum & 0xFFFFFFFF);
            carry = sum >> 32;
            result.size = i + 1;
        }
        
        result.normalize();
        return result;
    }

    BigInt operator-(const BigInt& other) const {
        if (*this < other) {
            return BigInt(0); // Return 0 for negative results
        }
        
        BigInt result;
        int64_t borrow = 0;
        
        for (int i = 0; i < size; i++) {
            int64_t diff = (int64_t)data[i] - borrow;
            if (i < other.size) diff -= other.data[i];
            
            if (diff < 0) {
                diff += 0x100000000LL;
                borrow = 1;
            } else {
                borrow = 0;
            }
            result.data[i] = (uint32_t)diff;
            result.size = i + 1;
        }
        
        result.normalize();
        return result;
    }

    // Left shift by n bits (multiply by 2^n)
    BigInt shiftLeft(int n) const {
        if (n == 0 || isZero()) return *this;
        
        BigInt result;
        int wordShift = n / 32;
        int bitShift = n % 32;
        
        if (wordShift >= MAX_WORDS) {
            return BigInt(0);
        }
        
        // Shift by whole words first
        for (int i = 0; i < size; i++) {
            if (i + wordShift < MAX_WORDS) {
                result.data[i + wordShift] = data[i];
            }
        }
        
        // Then shift by remaining bits
        if (bitShift > 0) {
            uint32_t carry = 0;
            for (int i = wordShift; i < size + wordShift && i < MAX_WORDS; i++) {
                uint64_t temp = ((uint64_t)result.data[i] << bitShift) | carry;
                result.data[i] = (uint32_t)(temp & 0xFFFFFFFF);
                carry = (uint32_t)(temp >> 32);
            }
            if (size + wordShift < MAX_WORDS && carry) {
                result.data[size + wordShift] = carry;
            }
        }
        
        int newSize = size + wordShift + (bitShift > 0 ? 1 : 0);
        result.size = (newSize < MAX_WORDS) ? newSize : MAX_WORDS;
        result.normalize();
        return result;
    }

    // Right shift by n bits (divide by 2^n)
    BigInt shiftRight(int n) const {
        if (n == 0 || isZero()) return *this;
        
        BigInt result;
        int wordShift = n / 32;
        int bitShift = n % 32;
        
        if (wordShift >= size) {
            return BigInt(0);
        }
        
        // Shift by whole words first
        for (int i = wordShift; i < size; i++) {
            result.data[i - wordShift] = data[i];
        }
        
        // Then shift by remaining bits
        if (bitShift > 0) {
            for (int i = 0; i < size - wordShift; i++) {
                result.data[i] = result.data[i] >> bitShift;
                if (i + 1 < size - wordShift) {
                    result.data[i] |= (result.data[i + 1] & ((1U << bitShift) - 1)) << (32 - bitShift);
                }
            }
        }
        
        result.size = size - wordShift;
        result.normalize();
        return result;
    }



    // Division and modulo - optimized version
    void divMod(const BigInt& divisor, BigInt& quotient, BigInt& remainder) const {
        quotient = BigInt(0);
        remainder = BigInt(0);
        
        if (divisor.isZero()) return;
        if (*this < divisor) {
            remainder = *this;
            return;
        }
        
        // Handle special case where divisor is 1
        if (divisor.isOne()) {
            quotient = *this;
            return;
        }
        
        // Optimized: process multiple bits when possible
        BigInt dividend = *this;
        BigInt div = divisor;
        
        // Find how many bits we need to process
        int dividendBits = dividend.bitLength();
        int divisorBits = div.bitLength();
        
        if (dividendBits < divisorBits) {
            remainder = *this;
            return;
        }
        
        // Start from the most significant bit
        for (int i = dividendBits - 1; i >= 0; i--) {
            remainder = remainder.shiftLeft(1);
            if (dividend.getBit(i)) {
                remainder.data[0] |= 1;
                if (remainder.size == 1 && remainder.data[0] == 1) {
                    remainder.size = 1; // Keep normalized
                } else {
                    remainder.normalize();
                }
            }
            
            if (remainder >= div) {
                remainder = remainder - div;
                quotient.setBit(i);
            }
        }
        
        quotient.normalize();
        remainder.normalize();
    }

    BigInt operator/(const BigInt& other) const {
        BigInt q, r;
        divMod(other, q, r);
        return q;
    }

    BigInt operator%(const BigInt& other) const {
        BigInt q, r;
        divMod(other, q, r);
        return r;
    }

    // Modular addition: (a + b) mod n
    static BigInt addMod(const BigInt& a, const BigInt& b, const BigInt& n) {
        BigInt result = a + b;
        if (result >= n) {
            result = result % n;
        }
        return result;
    }

    // Helper structure for extended multiplication
    struct ExtendedBigInt {
        static const int EXT_MAX_WORDS = 34;
        uint32_t data[EXT_MAX_WORDS];
        int size;
        
        ExtendedBigInt() : size(1) {
            memset(data, 0, sizeof(data));
        }
        
        void normalize() {
            while (size > 1 && data[size - 1] == 0) {
                size--;
            }
        }
        
        bool getBit(int pos) const {
            int wordIdx = pos / 32;
            int bitIdx = pos % 32;
            if (wordIdx >= size) return false;
            return (data[wordIdx] >> bitIdx) & 1;
        }
        
        int bitLength() const {
            if (size == 1 && data[0] == 0) return 0;
            int len = (size - 1) * 32;
            uint32_t top = data[size - 1];
            while (top > 0) {
                len++;
                top >>= 1;
            }
            return len;
        }
    };
    
    // Modular multiplication: (a * b) mod n using extended buffer
    static BigInt mulMod(const BigInt& a, const BigInt& b, const BigInt& n) {
        if (n.isOne()) return BigInt(0);
        
        // Multiply into extended buffer
        ExtendedBigInt product;
        
        for (int i = 0; i < a.size; i++) {
            uint64_t carry = 0;
            for (int j = 0; j < b.size; j++) {
                int k = i + j;
                if (k >= ExtendedBigInt::EXT_MAX_WORDS) break;
                
                uint64_t prod = (uint64_t)a.data[i] * b.data[j];
                uint64_t sum = (uint64_t)product.data[k] + prod + carry;
                product.data[k] = (uint32_t)(sum & 0xFFFFFFFF);
                carry = sum >> 32;
            }
            
            int k = i + b.size;
            while (carry && k < ExtendedBigInt::EXT_MAX_WORDS) {
                uint64_t sum = (uint64_t)product.data[k] + carry;
                product.data[k] = (uint32_t)(sum & 0xFFFFFFFF);
                carry = sum >> 32;
                k++;
            }
        }
        
        product.size = a.size + b.size;
        if (product.size > ExtendedBigInt::EXT_MAX_WORDS) {
            product.size = ExtendedBigInt::EXT_MAX_WORDS;
        }
        product.normalize();
        
        // Perform modulo using long division
        BigInt quotient, remainder;
        
        int productBits = product.bitLength();
        for (int i = productBits - 1; i >= 0; i--) {
            remainder = remainder.shiftLeft(1);
            if (product.getBit(i)) {
                remainder.data[0] |= 1;
                remainder.normalize();
            }
            
            if (remainder >= n) {
                remainder = remainder - n;
            }
        }
        
        return remainder;
    }

    // Modular exponentiation: (base^exp) mod n
    static BigInt powerMod(const BigInt& base, const BigInt& exp, const BigInt& n) {
        if (n.isOne()) return BigInt(0);
        
        BigInt result(1);
        BigInt b = base % n;
        
        int bits = exp.bitLength();
        for (int i = 0; i < bits; i++) {
            if (exp.getBit(i)) {
                result = mulMod(result, b, n);
            }
            b = mulMod(b, b, n);
        }
        
        return result;
    }

    // Generate random BigInt with similar bit length
    static BigInt random(const BigInt& n) {
        BigInt result;
        int bits = n.bitLength();
        
        for (int i = 0; i < bits; i++) {
            if (rand() % 2) {
                result.setBit(i);
            }
        }
        result.normalize();
        
        if (result >= n && !n.isZero()) {
            result = result % n;
        }
        
        return result;
    }
};

// Miller-Rabin primality test with deterministic witnesses for better reliability
bool millerRabinTest(const BigInt& n, const BigInt& a) {
    if (n <= BigInt(1)) return false;
    if (n == BigInt(2)) return true;
    if (n.isEven()) return false;
    
    // Write n-1 as 2^r * d
    BigInt d = n - BigInt(1);
    int r = 0;
    while (d.isEven()) {
        d = d.shiftRight(1);
        r++;
    }
    
    // Compute a^d mod n
    BigInt x = BigInt::powerMod(a, d, n);
    
    if (x == BigInt(1) || x == n - BigInt(1)) {
        return true; // Probably prime
    }
    
    // Repeatedly square x
    for (int i = 0; i < r - 1; i++) {
        x = BigInt::mulMod(x, x, n);
        if (x == n - BigInt(1)) {
            return true; // Probably prime
        }
        if (x == BigInt(1)) {
            return false; // Definitely composite
        }
    }
    
    return false; // Definitely composite
}

bool millerRabin(const BigInt& n, int iterations = 20) {
    if (n < BigInt(2)) return false;
    if (n == BigInt(2) || n == BigInt(3)) return true;
    if (n.isEven()) return false;
    
    // Use deterministic witnesses for small numbers (more reliable)
    // These are known to be good witnesses
    uint64_t deterministicWitnesses[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37};
    int numDeterministic = 12;
    
    for (int i = 0; i < numDeterministic && i < iterations; i++) {
        BigInt a(deterministicWitnesses[i]);
        if (a >= n) break;
        
        if (!millerRabinTest(n, a)) {
            return false;
        }
    }
    
    // For additional iterations, use random witnesses
    for (int i = numDeterministic; i < iterations; i++) {
        BigInt a;
        
        // Generate random witness in range [2, n-2]
        if (n < BigInt(4)) {
            a = BigInt(2);
        } else {
            BigInt range = n - BigInt(3);
            a = BigInt::random(range) + BigInt(2);
            if (a >= n - BigInt(1)) a = BigInt(2);
        }
        
        if (!millerRabinTest(n, a)) {
            return false;
        }
    }
    
    return true;
}

// Small primes for trial division
bool trialDivision(const BigInt& n) {
    // Quick checks for small primes
    if (n == BigInt(2) || n == BigInt(3) || n == BigInt(5) || n == BigInt(7)) return true;
    
    // Check divisibility by small primes (avoid expensive modulo for tiny numbers)
    int smallPrimes[] = {3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97};
    
    for (int p : smallPrimes) {
        BigInt prime(p);
        if (n == prime) return true;
        if ((n % prime).isZero()) return false;
    }
    return true;
}

bool isPrime(const BigInt& n) {
    if (n < BigInt(2)) return false;
    if (n == BigInt(2)) return true;
    if (n.isEven()) return false;
    
    // Trial division with small primes
    if (!trialDivision(n)) return false;
    
    // Miller-Rabin test with fewer iterations (10 gives error < 2^-40)
    return millerRabin(n, 40);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <input_file> <output_file>" << endl;
        return 1;
    }
    
    srand(time(NULL));
    
    string inputFile = argv[1];
    string outputFile = argv[2];
    
    ifstream fin(inputFile);
    if (!fin) {
        cerr << "Cannot open input file: " << inputFile << endl;
        return 1;
    }
    
    string hexNumber;
    getline(fin, hexNumber);
    fin.close();
    
    // Remove whitespace
    hexNumber.erase(remove_if(hexNumber.begin(), hexNumber.end(), ::isspace), hexNumber.end());
    
    BigInt num(hexNumber);
    
    bool prime = isPrime(num);
    
    ofstream fout(outputFile);
    if (!fout) {
        cerr << "Cannot open output file: " << outputFile << endl;
        return 1;
    }
    
    fout << (prime ? 1 : 0) << endl;
    fout.close();
    
    return 0;
}