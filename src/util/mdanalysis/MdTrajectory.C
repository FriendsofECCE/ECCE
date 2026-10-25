#include <cmath>
#include <cstdint>
#include <cstring>

#include "util/MdTrajectory.H"

namespace {

// XDR is big-endian; the reads below are byte-wise so the host order is moot.
bool readInt(FILE *f, int32_t& v)
{
  unsigned char b[4];
  if (fread(b, 1, 4, f) != 4) return false;
  v = (int32_t)(((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) |
                ((uint32_t)b[2] << 8) | (uint32_t)b[3]);
  return true;
}

bool readReal(FILE *f, bool dbl, double& v)
{
  unsigned char b[8];
  const int n = dbl ? 8 : 4;
  if (fread(b, 1, n, f) != (size_t)n) return false;
  uint64_t u = 0;
  for (int i = 0; i < n; i++) u = (u << 8) | b[i];
  if (dbl) {
    memcpy(&v, &u, 8);
  } else {
    uint32_t w = (uint32_t)u;
    float x;
    memcpy(&x, &w, 4);
    v = x;
  }
  return true;
}

bool readReals(FILE *f, bool dbl, size_t n, std::vector<float>& out)
{
  out.resize(n);
  std::vector<unsigned char> raw(n * (dbl ? 8 : 4));
  if (n && fread(&raw[0], 1, raw.size(), f) != raw.size()) return false;
  for (size_t i = 0; i < n; i++) {
    if (dbl) {
      uint64_t u = 0;
      for (int k = 0; k < 8; k++) u = (u << 8) | raw[i * 8 + k];
      double d;
      memcpy(&d, &u, 8);
      out[i] = (float)d;
    } else {
      uint32_t w = ((uint32_t)raw[i * 4] << 24) | ((uint32_t)raw[i * 4 + 1] << 16) |
                   ((uint32_t)raw[i * 4 + 2] << 8) | raw[i * 4 + 3];
      memcpy(&out[i], &w, 4);
    }
  }
  return true;
}

const int kMagicTrr = 1993;
const int kMagicXtc = 1995;

// The size table of the xtc coder: the integer ranges its small-delta
// codes can address.
const int kFirstIdx = 9;
const int kMagicInts[] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 10, 12, 16, 20, 25, 32, 40, 50, 64, 80, 101,
  128, 161, 203, 256, 322, 406, 512, 645, 812, 1024, 1290, 1625, 2048, 2580,
  3250, 4096, 5060, 6501, 8192, 10321, 13003, 16384, 20642, 26007, 32768,
  41285, 52015, 65536, 82570, 104031, 131072, 165140, 208063, 262144, 330280,
  416127, 524287, 660561, 832255, 1048576, 1321122, 1664510, 2097152, 2642245,
  3329021, 4194304, 5284491, 6658042, 8388607, 10568983, 13316085, 16777216};
const int kLastIdx = (int)(sizeof(kMagicInts) / sizeof(kMagicInts[0])) - 1;

int sizeOfInt(unsigned size)
{
  unsigned num = 1;
  int bits = 0;
  while (size >= num && bits < 32) {
    bits++;
    num <<= 1;
  }
  return bits;
}

int sizeOfInts(int n, const unsigned *sizes)
{
  unsigned char bytes[32];
  unsigned tmp;
  int nbytes = 1, bits = 0, cnt;
  bytes[0] = 1;
  for (int i = 0; i < n; i++) {
    tmp = 0;
    for (cnt = 0; cnt < nbytes; cnt++) {
      tmp = bytes[cnt] * sizes[i] + tmp;
      bytes[cnt] = (unsigned char)(tmp & 0xff);
      tmp >>= 8;
    }
    while (tmp != 0) {
      bytes[cnt++] = (unsigned char)(tmp & 0xff);
      tmp >>= 8;
    }
    nbytes = cnt;
  }
  unsigned num = 1;
  nbytes--;
  while (bytes[nbytes] >= num) {
    bits++;
    num *= 2;
  }
  return bits + nbytes * 8;
}

struct BitReader
{
  const unsigned char *buf;
  size_t size, cnt;
  unsigned lastbits, lastbyte;
  bool bad;
  BitReader(const unsigned char *b, size_t n)
    : buf(b), size(n), cnt(0), lastbits(0), lastbyte(0), bad(false) {}

  unsigned nextByte()
  {
    if (cnt >= size) { bad = true; return 0; }
    return buf[cnt++];
  }

  int bits(int nbits)
  {
    const int mask = (nbits >= 32) ? -1 : (1 << nbits) - 1;
    int num = 0;
    while (nbits >= 8) {
      lastbyte = (lastbyte << 8) | nextByte();
      num |= (int)(lastbyte >> lastbits) << (nbits - 8);
      nbits -= 8;
    }
    if (nbits > 0) {
      if ((int)lastbits < nbits) {
        lastbits += 8;
        lastbyte = (lastbyte << 8) | nextByte();
      }
      lastbits -= nbits;
      num |= (int)((lastbyte >> lastbits) & ((1u << nbits) - 1));
    }
    return num & mask;
  }

  void ints(int n, int nbits, const unsigned *sizes, int *nums)
  {
    unsigned char bytes[32];
    int nbytes = 0;
    bytes[0] = bytes[1] = bytes[2] = bytes[3] = 0;
    while (nbits > 8) {
      bytes[nbytes++] = (unsigned char)bits(8);
      nbits -= 8;
    }
    if (nbits > 0) bytes[nbytes++] = (unsigned char)bits(nbits);
    for (int i = n - 1; i > 0; i--) {
      unsigned num = 0;
      for (int j = nbytes - 1; j >= 0; j--) {
        num = (num << 8) | bytes[j];
        const unsigned p = num / sizes[i];
        bytes[j] = (unsigned char)p;
        num -= p * sizes[i];
      }
      nums[i] = (int)num;
    }
    nums[0] = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);
  }
};

// The compressed coordinates of one xtc frame (natoms > 9); false if the
// stream is inconsistent.
bool decompress(const std::vector<unsigned char>& data, int natoms,
                std::vector<float>& x, float precision, const int minint[3],
                const int maxint[3], int smallidx)
{
  BitReader r(&data[0], data.size());
  unsigned sizeint[3], bitsizeint[3] = {0, 0, 0};
  int bitsize = 0;
  for (int k = 0; k < 3; k++) sizeint[k] = (unsigned)(maxint[k] - minint[k] + 1);
  if ((sizeint[0] | sizeint[1] | sizeint[2]) > 0xffffff) {
    for (int k = 0; k < 3; k++) bitsizeint[k] = sizeOfInt(sizeint[k]);
    bitsize = 0;
  } else {
    bitsize = sizeOfInts(3, sizeint);
  }
  if (smallidx < kFirstIdx || smallidx > kLastIdx) return false;
  int smaller = kMagicInts[kFirstIdx > smallidx - 1 ? kFirstIdx : smallidx - 1] / 2;
  int smallnum = kMagicInts[smallidx] / 2;
  unsigned sizesmall[3] = {(unsigned)kMagicInts[smallidx],
                           (unsigned)kMagicInts[smallidx],
                           (unsigned)kMagicInts[smallidx]};
  const float inv = 1.0f / precision;
  x.assign((size_t)natoms * 3, 0.0f);
  size_t out = 0;
  int i = 0, run = 0;
  while (i < natoms) {
    int thiscoord[3], prev[3];
    if (bitsize == 0) {
      for (int k = 0; k < 3; k++) thiscoord[k] = r.bits((int)bitsizeint[k]);
    } else {
      r.ints(3, bitsize, sizeint, thiscoord);
    }
    i++;
    for (int k = 0; k < 3; k++) {
      thiscoord[k] += minint[k];
      prev[k] = thiscoord[k];
    }
    int flag = r.bits(1), isSmaller = 0;
    if (flag == 1) {
      run = r.bits(5);
      isSmaller = run % 3;
      run -= isSmaller;
      isSmaller--;
    }
    if (run > 0) {
      if (i + run / 3 > natoms) return false;
      int cur[3];
      for (int k = 0; k < run; k += 3) {
        r.ints(3, smallidx, sizesmall, cur);
        i++;
        for (int c = 0; c < 3; c++) cur[c] += prev[c] - smallnum;
        if (k == 0) {
          // The first small atom is stored before the large one (water).
          for (int c = 0; c < 3; c++) std::swap(cur[c], prev[c]);
          for (int c = 0; c < 3; c++) x[out++] = prev[c] * inv;
        } else {
          for (int c = 0; c < 3; c++) prev[c] = cur[c];
        }
        for (int c = 0; c < 3; c++) x[out++] = cur[c] * inv;
      }
    } else {
      for (int c = 0; c < 3; c++) x[out++] = thiscoord[c] * inv;
    }
    smallidx += isSmaller;
    if (isSmaller < 0) {
      smallnum = smaller;
      smaller = smallidx > kFirstIdx ? kMagicInts[smallidx - 1] / 2 : 0;
    } else if (isSmaller > 0) {
      smaller = smallnum;
      smallnum = kMagicInts[smallidx] / 2;
    }
    if (smallidx < kFirstIdx || smallidx > kLastIdx) return false;
    sizesmall[0] = sizesmall[1] = sizesmall[2] = (unsigned)kMagicInts[smallidx];
    if (r.bad) return false;
  }
  return out == (size_t)natoms * 3;
}

}  // namespace

MdTrajectoryReader::MdTrajectoryReader() : p_file(0), p_xtc(false) {}

MdTrajectoryReader::~MdTrajectoryReader()
{
  if (p_file) fclose(p_file);
}

bool MdTrajectoryReader::open(const std::string& path, std::string& error)
{
  if (p_file) fclose(p_file);
  p_file = fopen(path.c_str(), "rb");
  if (!p_file) {
    error = "cannot open " + path;
    return false;
  }
  int32_t magic = 0;
  if (!readInt(p_file, magic) ||
      (magic != kMagicTrr && magic != kMagicXtc)) {
    error = path + " is neither a .trr nor an .xtc file";
    fclose(p_file);
    p_file = 0;
    return false;
  }
  p_xtc = magic == kMagicXtc;
  rewind(p_file);
  return true;
}

bool MdTrajectoryReader::next(MdFrame& frame, std::string& error)
{
  error.clear();
  if (!p_file) return false;
  return p_xtc ? nextXtc(frame, error) : nextTrr(frame, error);
}

bool MdTrajectoryReader::nextTrr(MdFrame& fr, std::string& error)
{
  int32_t magic;
  if (!readInt(p_file, magic)) return false;
  if (magic != kMagicTrr) { error = "damaged .trr frame header"; return false; }
  int32_t slen, n;
  if (!readInt(p_file, slen) || !readInt(p_file, n) || n < 0 || n > 64) {
    error = "damaged .trr frame header";
    return false;
  }
  fseek(p_file, (n + 3) / 4 * 4, SEEK_CUR);
  int32_t h[13];  // ir e box vir pres top sym x v f natoms step nre
  for (int i = 0; i < 13; i++)
    if (!readInt(p_file, h[i])) { error = "truncated .trr header"; return false; }
  const int boxSize = h[2], xSize = h[7], vSize = h[8], fSize = h[9];
  fr.natoms = h[10];
  fr.step = h[11];
  int rs = 4;
  if (boxSize) rs = boxSize / 9;
  else if (fr.natoms && xSize) rs = xSize / (3 * fr.natoms);
  else if (fr.natoms && vSize) rs = vSize / (3 * fr.natoms);
  else if (fr.natoms && fSize) rs = fSize / (3 * fr.natoms);
  if (rs != 4 && rs != 8) { error = "unknown .trr precision"; return false; }
  const bool dbl = rs == 8;
  double t, lambda;
  if (!readReal(p_file, dbl, t) || !readReal(p_file, dbl, lambda)) {
    error = "truncated .trr header";
    return false;
  }
  fr.time = t;
  std::vector<float> tmp;
  for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) fr.box[i][j] = 0;
  if (boxSize) {
    if (!readReals(p_file, dbl, 9, tmp)) { error = "truncated .trr frame"; return false; }
    for (int i = 0; i < 9; i++) fr.box[i / 3][i % 3] = tmp[i];
  }
  fseek(p_file, h[3] + h[4] + h[5] + h[6], SEEK_CUR);  // virial, pressure, top, sym
  fr.x.clear(); fr.v.clear();
  const size_t n3 = (size_t)fr.natoms * 3;
  if (xSize && !readReals(p_file, dbl, n3, fr.x)) { error = "truncated .trr frame"; return false; }
  if (vSize && !readReals(p_file, dbl, n3, fr.v)) { error = "truncated .trr frame"; return false; }
  if (fSize) fseek(p_file, fSize, SEEK_CUR);
  return true;
}

bool MdTrajectoryReader::nextXtc(MdFrame& fr, std::string& error)
{
  int32_t magic, natoms, step;
  if (!readInt(p_file, magic)) return false;
  if (magic != kMagicXtc) { error = "damaged .xtc frame header"; return false; }
  double t;
  std::vector<float> box;
  if (!readInt(p_file, natoms) || !readInt(p_file, step) ||
      !readReal(p_file, false, t) || !readReals(p_file, false, 9, box) ||
      natoms < 1) {
    error = "truncated .xtc frame header";
    return false;
  }
  fr.natoms = natoms;
  fr.step = step;
  fr.time = t;
  for (int i = 0; i < 9; i++) fr.box[i / 3][i % 3] = box[i];
  fr.v.clear();
  int32_t n2;
  if (!readInt(p_file, n2) || n2 != natoms) { error = "damaged .xtc frame"; return false; }
  if (natoms <= 9) {
    if (!readReals(p_file, false, (size_t)natoms * 3, fr.x)) { error = "truncated .xtc frame"; return false; }
    return true;
  }
  double prec;
  int32_t minint[3], maxint[3], smallidx, nbytes;
  if (!readReal(p_file, false, prec)) { error = "truncated .xtc frame"; return false; }
  for (int k = 0; k < 3; k++) if (!readInt(p_file, minint[k])) { error = "truncated .xtc frame"; return false; }
  for (int k = 0; k < 3; k++) if (!readInt(p_file, maxint[k])) { error = "truncated .xtc frame"; return false; }
  if (!readInt(p_file, smallidx) || !readInt(p_file, nbytes) || nbytes < 0) {
    error = "truncated .xtc frame";
    return false;
  }
  std::vector<unsigned char> data((size_t)nbytes + 8, 0);
  if (nbytes && fread(&data[0], 1, nbytes, p_file) != (size_t)nbytes) { error = "truncated .xtc frame"; return false; }
  fseek(p_file, (4 - nbytes % 4) % 4, SEEK_CUR);
  data.resize(nbytes);
  if (!decompress(data, natoms, fr.x, (float)prec, minint, maxint, smallidx)) {
    error = "damaged .xtc coordinates";
    return false;
  }
  return true;
}
