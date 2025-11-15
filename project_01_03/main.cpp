//#pragma GCC optimize("O3")
//#include <bits/stdc++.h>
#include <cstdint>
#include <cstring>
#include <string>
#include <fstream>
#include <iostream>
#include <algorithm>

using namespace std;

class BigInt {
private:
    static constexpr int MAX_WORDS = 256;
    uint32_t data[MAX_WORDS]{};
    int size = 1;

    void normalize() {
        while (size > 1 && data[size - 1] == 0) size--;
        if (size == 0) size = 1;
    }
public:
    BigInt() = default;
    BigInt(uint64_t v) {
        data[0] = (uint32_t)(v & 0xFFFFFFFFu);
        if (v > 0xFFFFFFFFu) {
            data[1] = (uint32_t)(v >> 32);
            size = 2;
        }
        normalize();
    }

    BigInt(const string& hex) {
        if (hex.empty() || hex == "0") { size = 1; data[0] = 0; return; }
        for (int i = 0; i < (int)hex.size(); ++i) {
            char c = hex[i];
            int d = (c>='0'&&c<='9')? c-'0' : (c>='A'&&c<='F')? c-'A'+10
                   : (c>='a'&&c<='f')? c-'a'+10 : -1;
            if (d < 0) continue;
            int bitPos = i * 4;
            int w = bitPos / 32, b = bitPos % 32;
            if (w < MAX_WORDS) {
                data[w] |= ((uint32_t)d << b);
                if (b > 28 && w + 1 < MAX_WORDS)
                    data[w + 1] |= ((uint32_t)d >> (32 - b));
            }
        }
        size = MAX_WORDS; normalize();
    }


    bool isZero() const { return size == 1 && data[0] == 0; }
    bool isOne()  const { return size == 1 && data[0] == 1; }
    bool isEven() const { return (data[0] & 1u) == 0; }

    int bitLength() const {
        if (isZero()) return 0;
        int len = (size - 1) * 32; uint32_t top = data[size - 1];
        while (top) { ++len; top >>= 1; } return len;
    }
    bool getBit(int pos) const {
        int w = pos / 32, b = pos % 32; if (w >= size) return false;
        return (data[w] >> b) & 1u;
    }
    void setBit(int pos) {
        int w = pos / 32, b = pos % 32;
        if (w < MAX_WORDS) { data[w] |= (1u << b); if (w >= size) size = w + 1; }
    }

    bool operator<(const BigInt& o) const {
        if (size != o.size) return size < o.size;
        for (int i = size - 1; i >= 0; --i) if (data[i] != o.data[i]) return data[i] < o.data[i];
        return false;
    }
    bool operator> (const BigInt& o) const { return o < *this; }
    bool operator<=(const BigInt& o) const { return !(o < *this); }
    bool operator>=(const BigInt& o) const { return !(*this < o); }
    bool operator==(const BigInt& o) const {
        if (size != o.size) return false;
        for (int i = 0; i < size; ++i) if (data[i] != o.data[i]) return false;
        return true;
    }

    BigInt operator+(const BigInt& o) const {
        BigInt r; uint64_t carry = 0; int m = max(size, o.size);
        for (int i = 0; i < m || carry; ++i) {
            uint64_t s = carry;
            if (i < size) s += data[i];
            if (i < o.size) s += o.data[i];
            r.data[i] = (uint32_t)(s & 0xFFFFFFFFu);
            carry = s >> 32; r.size = i + 1;
        }
        r.normalize(); return r;
    }
    BigInt operator-(const BigInt& o) const {
        if (*this < o) return BigInt(0);
        BigInt r; int64_t borrow = 0;
        for (int i = 0; i < size; ++i) {
            int64_t d = (int64_t)data[i] - borrow - (i < o.size ? o.data[i] : 0);
            if (d < 0) { d += (1LL<<32); borrow = 1; } else borrow = 0;
            r.data[i] = (uint32_t)d; r.size = i + 1;
        }
        r.normalize(); return r;
    }
    BigInt shiftLeft(int n) const {
        if (n == 0 || isZero()) return *this;
        BigInt r; int ws = n/32, bs = n%32; r.size = min(MAX_WORDS, size + ws + (bs?1:0));
        if (bs == 0) for (int i=0;i<size && i+ws<MAX_WORDS;++i) r.data[i+ws]=data[i];
        else{
            uint64_t carry=0;
            for (int i=0;i<size && i+ws<MAX_WORDS;++i){
                uint64_t t= ((uint64_t)data[i] << bs)|carry;
                r.data[i+ws]=(uint32_t)(t & 0xFFFFFFFFu); carry = t >> 32;
            }
            if (ws + size < MAX_WORDS && carry) r.data[ws+size]=(uint32_t)carry;
        }
        r.normalize(); return r;
    }
    BigInt shiftRight(int n) const {
        if (n == 0 || isZero()) return *this;
        BigInt r; int ws = n/32, bs = n%32; if (ws >= size) return BigInt(0);
        for (int i=ws;i<size;++i) r.data[i-ws]=data[i];
        if (bs){
            for (int i=0;i<size-ws;++i){
                r.data[i] >>= bs;
                if (i+1 < size-ws) r.data[i] |= (r.data[i+1] & ((1u<<bs)-1)) << (32-bs);
            }
        }
        r.size = size - ws; r.normalize(); return r;
    }
    BigInt operator*(const BigInt& o) const {
        BigInt r; r.size = min(MAX_WORDS, size + o.size);
        for (int i=0; i<size && i<MAX_WORDS; ++i){
            uint64_t carry=0;
            for (int j=0; j<o.size && i+j<MAX_WORDS; ++j){
                uint64_t prod = (uint64_t)data[i] * o.data[j];
                uint64_t sum  = (uint64_t)r.data[i+j] + prod + carry;
                r.data[i+j] = (uint32_t)(sum & 0xFFFFFFFFu);
                carry = sum >> 32;
            }
            if (i + o.size < MAX_WORDS) r.data[i+o.size] += (uint32_t)carry;
        }
        r.normalize(); return r;
    }
    void divMod(const BigInt& d, BigInt& q, BigInt& r) const {
        q = BigInt(0); r = BigInt(0);
        if (d.isZero()) return;
        if (*this < d) { r = *this; return; }
        if (d.isOne()) { q = *this; return; }
        if (d.size == 1 && d.data[0]) {
            uint64_t div = d.data[0], rem = 0;
            q.size = size;
            for (int i=size-1; i>=0; --i){
                rem = (rem<<32) | data[i];
                q.data[i] = (uint32_t)(rem / div);
                rem %= div;
            }
            q.normalize(); r.data[0] = (uint32_t)rem; r.size = 1; r.normalize(); return;
        }
        int bits = bitLength();
        for (int i = bits-1; i >= 0; --i){
            r = r.shiftLeft(1);
            if (getBit(i)) { r.data[0] |= 1; r.normalize(); }
            if (r >= d) { r = r - d; q.setBit(i); }
        }
        q.normalize(); r.normalize();
    }
    BigInt operator/(const BigInt& o) const { BigInt q,r; divMod(o,q,r); return q; }
    BigInt operator%(const BigInt& o) const { BigInt q,r; divMod(o,q,r); return r; }

    static BigInt mulMod(const BigInt& a, const BigInt& b, const BigInt& n) {
        if (n.isOne()) return BigInt(0);
        BigInt prod = a * b;
        return prod % n;
    }
    static BigInt powerMod(const BigInt& base, const BigInt& exp, const BigInt& n) {
        if (n.isOne()) return BigInt(0);
        BigInt result(1), b = base % n;
        int bits = exp.bitLength();
        for (int i = 0; i < bits; ++i) {
            if (exp.getBit(i)) result = mulMod(result, b, n);
            b = mulMod(b, b, n);
        }
        return result;
    }

    friend istream& operator>>(istream& is, BigInt& n);
    friend ostream& operator<<(ostream& os, const BigInt& n);
};

istream& operator>>(istream& is, BigInt& n) {
    string s; is >> s; n = BigInt(s); return is;
}
ostream& operator<<(ostream& os, const BigInt& n) {
    if (n.isZero()) { os << "0"; return os; }
    int totalHex = (n.bitLength() + 3) / 4;
    for (int i=0; i<totalHex; ++i){
        int bitPos = i*4, w = bitPos/32, b = bitPos%32, d = 0;
        if (w < n.size) {
            d = (n.data[w] >> b) & 0xF;
            if (b > 28 && w + 1 < n.size) {
                int k = b - 28;
                d |= (n.data[w+1] & ((1<<k)-1)) << (32 - b);
            }
        }
        os << (char)(d < 10 ? '0'+d : 'A'+(d-10));
    }
    return os;
}

int main(int argc, char* argv[]) {
    ios::sync_with_stdio(false); cin.tie(nullptr);

    if (argc != 3) { cerr << "Usage: " << argv[0] << " <input> <output>\n"; return 1; }
    ifstream in(argv[1]); if (!in) { cerr << "Cannot open input\n"; return 1; }
    ofstream out(argv[2]); if (!out){ cerr << "Cannot open output\n"; return 1; }

    BigInt N, k, x;
    in >> N >> k >> x;

    BigInt y = BigInt::powerMod(x, k, N);
    out << y << '\n';
    return 0;
}
