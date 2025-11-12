#pragma GCC optimize("O3")
#include <iostream>
#include <fstream>
#include <string>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <ctime>
#include <cstdlib>
#include <iomanip>

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

    BigInt& operator=(const BigInt& other) {
        if (this != &other) {
            memcpy(data, other.data, sizeof(data));
            size = other.size;
        }
        return *this;
    }
    
    BigInt(const BigInt& other) : size(other.size) {
        memcpy(data, other.data, sizeof(data));
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
            }
            else {
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
        }
        else {
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
    friend istream& operator>>(istream& is, BigInt& n);
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

BigInt phi_euler(const BigInt& p, const BigInt& q) {
    BigInt one(1);
    return (p - one) * (q - one);
}

BigInt gcd(const BigInt& a, const BigInt& b) {
    BigInt res(1), x = a, y = b;

    while (x.isEven() && y.isEven()) {
        x = x.shiftRight(1);
        y = y.shiftRight(1);
        res = res.shiftLeft(1);
    }

    while (!x.isZero()) {
        while (x.isEven()) {
            x = x.shiftRight(1);
        }
        while (y.isEven()) {
            y = y.shiftRight(1);
        }
        if (x >= y) {
            x = x - y;
        }
        else {
            y = y - x;
        }
    }
    return res * y;
}

BigInt modInverse(const BigInt& e, const BigInt& phi) {
    BigInt g = gcd(e, phi);
    if (!g.isOne()) {
        return BigInt(0);  // No inverse exists
    }

    BigInt r0 = phi, r1 = e;
    BigInt s0(0), s1(1);

    while (!r1.isZero()) {
        BigInt q = r0 / r1;
        BigInt r2 = r0 - q * r1;

        BigInt qTimesS1 = q * s1;
        BigInt s2;

        if (qTimesS1 <= s0) {
            s2 = s0 - qTimesS1;
        }
        else {
            // s0 - q*s1 is negative, so add phi to make it positive
            BigInt diff = qTimesS1 - s0;
            s2 = phi - (diff % phi);
        }

        r0 = r1;
        r1 = r2;
        s0 = s1;
        s1 = s2;
    }

    return s0 % phi;
}

int main() {
    srand((unsigned int)time(0));

    BigInt p, q, e;
    cin >> p >> q >> e;
    BigInt d = modInverse(e, phi_euler(p, q));
    if (d.isZero()) {
        cout << -1 << endl;
    }
    else {
        cout << d << endl;
    }
}