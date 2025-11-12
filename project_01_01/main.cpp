#pragma GCC optimize("O3")
#include <iostream>
#include <fstream>
#include <string>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <ctime>
#include <cstdlib>

using namespace std;

class BigInt {
private:
    static constexpr int MAX_WORDS = 64;
    uint32_t data[MAX_WORDS];
    int size;

    void normalize() {
        while (size > 1 && data[size - 1] == 0) size--;
        if (size == 0) size = 1;
    }

public:
    BigInt() : size(1) {
        memset(data, 0, sizeof(data));
    }

    BigInt(uint64_t val) : size(1) {
        memset(data, 0, sizeof(data));
        data[0] = (uint32_t)(val & 0xFFFFFFFF);
        if (val > 0xFFFFFFFF) {
            data[1] = (uint32_t)(val >> 32);
            size = 2;
        }
        normalize();
    }

    // input format h_0*16^0 + h_1*16^1 + ... (first char is LSB)
    BigInt(const string& hex) : size(1) {
        memset(data, 0, sizeof(data));
        
        if (hex.empty() || hex == "0") {
            return;
        }
        
        for (int i = 0; i < (int)hex.length(); i++) {
            char c = hex[i];  // h_i is at position i (left to right)
            int digit;
            if (c >= '0' && c <= '9') digit = c - '0';
            else if (c >= 'A' && c <= 'F') digit = c - 'A' + 10;
            else if (c >= 'a' && c <= 'f') digit = c - 'a' + 10;
            else continue;
            
            // h_i contributes to bit positions [4*i, 4*i+3]
            int bitPos = i * 4;
            int wordPos = bitPos / 32;
            int bitInWord = bitPos % 32;
            
            if (wordPos < MAX_WORDS) {
                data[wordPos] |= ((uint32_t)digit << bitInWord);
                if (bitInWord > 28 && wordPos + 1 < MAX_WORDS) {
                    data[wordPos + 1] |= ((uint32_t)digit >> (32 - bitInWord));
                }
            }
        }
        
        size = MAX_WORDS;
        normalize();
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

    bool getBit(int pos) const {
        int wordPos = pos / 32;
        int bitPos = pos % 32;
        if (wordPos >= size) return false;
        return (data[wordPos] >> bitPos) & 1;
    }

    void setBit(int pos) {
        int wordPos = pos / 32;
        int bitPos = pos % 32;
        if (wordPos < MAX_WORDS) {
            data[wordPos] |= (1U << bitPos);
            if (wordPos >= size) size = wordPos + 1;
        }
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
        return !(other < *this);
    }

    bool operator>(const BigInt& other) const {
        return other < *this;
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
        // this stuff only supports unsigned =)
        // will be changed if needed in other exs
        if (*this < other) {
            return BigInt(0);
        }
        
        BigInt result;
        int64_t borrow = 0;
        
        for (int i = 0; i < size; i++) {
            int64_t diff = (int64_t)data[i] - borrow;
            if (i < other.size) diff -= other.data[i];
            
            if (diff < 0) {
                diff += 0x100000000LL; // eg 16^8 = 2^32
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

    BigInt shiftLeft(int n) const {
        if (n == 0 || isZero()) return *this;
        
        BigInt result;
        int wordShift = n / 32;
        int bitShift = n % 32;
        
        result.size = size + wordShift + (bitShift > 0 ? 1 : 0);
        if (result.size > MAX_WORDS) result.size = MAX_WORDS;
        
        if (bitShift == 0) {
            for (int i = 0; i < size && i + wordShift < MAX_WORDS; i++) {
                result.data[i + wordShift] = data[i];
            }
        } else {
            uint64_t carry = 0;
            for (int i = 0; i < size && i + wordShift < MAX_WORDS; i++) {
                uint64_t temp = ((uint64_t)data[i] << bitShift) | carry;
                result.data[i + wordShift] = (uint32_t)(temp & 0xFFFFFFFF);
                carry = temp >> 32;
            }
            if (wordShift + size < MAX_WORDS && carry) {
                result.data[wordShift + size] = (uint32_t)carry;
            }
        }
        
        result.normalize();
        return result;
    }

    BigInt shiftRight(int n) const {
        if (n == 0 || isZero()) return *this;
        
        BigInt result;
        int wordShift = n / 32;
        int bitShift = n % 32;
        
        if (wordShift >= size) {
            return BigInt(0);
        }
        
        for (int i = wordShift; i < size; i++) {
            result.data[i - wordShift] = data[i];
        }
        
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

    BigInt operator*(const BigInt& other) const {
        BigInt result;
        result.size = min(size + other.size, MAX_WORDS);
        
        for (int i = 0; i < size && i < MAX_WORDS; i++) {
            uint64_t carry = 0;
            for (int j = 0; j < other.size && i + j < MAX_WORDS; j++) {
                uint64_t prod = (uint64_t)data[i] * other.data[j];
                uint64_t sum = (uint64_t)result.data[i + j] + prod + carry;
                result.data[i + j] = (uint32_t)(sum & 0xFFFFFFFF);
                carry = sum >> 32;
            }
            if (i + other.size < MAX_WORDS && carry) {
                result.data[i + other.size] += (uint32_t)carry;
            }
        }
        
        result.normalize();
        return result;
    }

    void divMod(const BigInt& divisor, BigInt& quotient, BigInt& remainder) const {
        quotient = BigInt(0);
        remainder = BigInt(0);
        
        if (divisor.isZero()) return;
        if (*this < divisor) {
            remainder = *this;
            return;
        }
        
        if (divisor.isOne()) {
            quotient = *this;
            return;
        }
        
        // Fast path: single-word divisor
        if (divisor.size == 1 && divisor.data[0] != 0) {
            uint64_t div = divisor.data[0];
            uint64_t rem = 0;
            
            quotient.size = size;
            for (int i = size - 1; i >= 0; i--) {
                rem = (rem << 32) | data[i];
                quotient.data[i] = (uint32_t)(rem / div);
                rem = rem % div;
            }
            quotient.normalize();
            
            remainder.data[0] = (uint32_t)rem;
            remainder.size = 1;
            remainder.normalize();
            return;
        }
        
        // Binary long division for multi-word
        int dividendBits = bitLength();
        
        for (int i = dividendBits - 1; i >= 0; i--) {
            remainder = remainder.shiftLeft(1);
            if (getBit(i)) {
                remainder.data[0] |= 1;
                remainder.normalize();
            }
            
            if (remainder >= divisor) {
                remainder = remainder - divisor;
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

    // Modular addition with single reduction
    static BigInt addMod(const BigInt& a, const BigInt& b, const BigInt& n) {
        BigInt result = a + b;
        if (result >= n) {
            result = result - n;
        }
        return result;
    }

    // Modular multiplication using optimized multiplication + reduction
    static BigInt mulMod(const BigInt& a, const BigInt& b, const BigInt& n) {
        if (n.isOne()) return BigInt(0);
        
        // Use optimized multiplication
        BigInt product = a * b;
        
        // Fast reduction for result
        return product % n;
    }

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

    // Generate random BigInt
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

    friend ostream& operator<<(ostream& os, const BigInt& n);
};

istream& operator>>(istream& is, BigInt& n) {
    string hex;
    is >> hex;
    n = BigInt(hex);
    return is;
}

ostream& operator<<(ostream& os, const BigInt& n) {
    if (n.isZero()) {
        os << "0";
        return os;
    }

    // Calculate total number of hex digits needed
    int totalBits = n.bitLength();
    int totalHexDigits = (totalBits + 3) / 4;  // Round up to nearest hex digit

    // Output each hex digit from LSB to MSB (left to right)
    for (int i = 0; i < totalHexDigits; i++) {
        // Extract 4 bits starting at position i*4
        int bitPos = i * 4;
        int wordPos = bitPos / 32;
        int bitInWord = bitPos % 32;

        int digit = 0;
        if (wordPos < n.size) {
            digit = (n.data[wordPos] >> bitInWord) & 0xF;

            // Handle case where hex digit spans two words
            if (bitInWord > 28 && wordPos + 1 < n.size) {
                int bitsFromNextWord = bitInWord - 28;
                digit |= (n.data[wordPos + 1] & ((1 << bitsFromNextWord) - 1)) << (32 - bitInWord);
            }
        }

        // Output as hex character
        if (digit < 10) {
            os << (char)('0' + digit);
        }
        else {
            os << (char)('A' + digit - 10);
        }
    }

    return os;
}
// Miller-Rabin 1st
bool millerRabinTest(const BigInt& n, const BigInt& a) {
    BigInt n_minus_1 = n - BigInt(1);
    
    int s = 0;
    BigInt d = n_minus_1;
    while (d.isEven()) {
        s++;
        d = d.shiftRight(1);
    }
    
    BigInt x = BigInt::powerMod(a, d, n);
    
    if (x == BigInt(1) || x == n_minus_1) {
        return true;
    }
    
    for (int i = 0; i < s - 1; i++) {
        x = BigInt::mulMod(x, x, n);
        if (x == n_minus_1) {
            return true;
        }
        if (x == BigInt(1)) {
            return false;
        }
    }
    
    return false;
}

bool millerRabin(const BigInt& n, int iterations = 20) {
    if (n < BigInt(2)) return false;
    if (n == BigInt(2) || n == BigInt(3)) return true;
    if (n.isEven()) return false;
    
    // Deterministic witnesses for better reliability
    uint64_t deterministicWitnesses[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37};
    int numDeterministic = sizeof(deterministicWitnesses) / sizeof(deterministicWitnesses[0]);
    
    for (int i = 0; i < numDeterministic && i < iterations; i++) {
        BigInt a(deterministicWitnesses[i]);
        if (a >= n) break;
        
        if (!millerRabinTest(n, a)) {
            return false;
        }
    }
    
    // Random witnesses for additional iterations
    for (int i = numDeterministic; i < iterations; i++) {
        BigInt a;
        
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

// Trial division for small primes
bool trialDivision(const BigInt& n) {
    if (n == BigInt(2) || n == BigInt(3) || n == BigInt(5) || n == BigInt(7)) return true;
    
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
    
    if (!trialDivision(n)) return false;
    
    return millerRabin(n, 20);
}

int main(int argc, char* argv[]) {
    srand(time(0));
    
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <input_file> <output_file>" << endl;
        return 1;
    }
    
    ifstream inFile(argv[1]);
    if (!inFile) {
        cerr << "Cannot open input file: " << argv[1] << endl;
        return 1;
    }
    
    string hexString;
    inFile >> hexString;
    inFile.close();
    
    // Remove whitespace
    hexString.erase(remove_if(hexString.begin(), hexString.end(), ::isspace), hexString.end());
    
    BigInt n(hexString);
    
    bool result = isPrime(n);
    
    ofstream outFile(argv[2]);
    if (!outFile) {
        cerr << "Cannot open output file: " << argv[2] << endl;
        return 1;
    }
    
    outFile << (result ? "1" : "0") << endl;
    outFile.close();
    
    return 0;
}
