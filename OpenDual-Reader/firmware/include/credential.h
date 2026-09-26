#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
enum class CredentialKind : uint8_t { HF_UID=1, LF_EM=2, LF_HID_RAW=3 };
struct Credential {
  CredentialKind kind{};
  uint8_t length = 0;
  uint8_t data[32]{};
  uint32_t seen = 0;
};
inline bool sameCredential(const Credential &a, const Credential &b) {
  return a.kind == b.kind && a.length == b.length && !memcmp(a.data,b.data,a.length);
}
inline int hexDigit(uint8_t c) {
  if (c >= '0' && c <= '9') return c-'0';
  if (c >= 'A' && c <= 'F') return c-'A'+10;
  if (c >= 'a' && c <= 'f') return c-'a'+10;
  return -1;
}
// Frame body includes type and space, excludes STX / CR / LF / ETX.
// HID payload is preserved byte-for-byte; ambiguous vendor packing is not guessed.
inline bool parseLf(const uint8_t *p, size_t n, Credential &out) {
  if (n < 3 || p[1] != ' ') return false;
  out = {};
  if (p[0] == '1' && n == 12) {
    out.kind = CredentialKind::LF_EM; out.length = 5;
    for (size_t i=0;i<5;i++) {
      int a=hexDigit(p[2+i*2]), b=hexDigit(p[3+i*2]);
      if(a<0 || b<0) return false;
      out.data[i]=uint8_t((a<<4)|b);
    }
    return true;
  }
  if (p[0] == '2' && (n == 14 || n == 26)) {
    for(size_t i=2;i<n;i++) if(hexDigit(p[i])<0) return false;
    out.kind=CredentialKind::LF_HID_RAW; out.length=uint8_t(n);
    memcpy(out.data,p,n); return true;
  }
  return false;
}
template<typename T, size_t N> struct BoundedQueue {
  T values[N]{}; size_t head=0, count=0;
  uint32_t dropped=0;
  bool push(const T &v) {
    if(count==N) { ++dropped; return false; }
    values[(head+count)%N]=v; ++count; return true;
  }
  T *front(){return count ? &values[head] : nullptr;}
  void pop(){if(count){head=(head+1)%N; --count;}}
  void clear(){head=count=0;}
};
