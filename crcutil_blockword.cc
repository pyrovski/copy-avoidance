#include <stdint.h>
#include "crcutil/generic_crc.h"

static constexpr uint32_t CRC_POLY = 0xEDB88320U;

class Gcrc : public crcutil::GenericCrc<uint32_t, uint32_t, size_t, 2> {
 public:
  Gcrc(uint32_t poly, size_t degree, bool canonical)
      : GenericCrc(poly, degree, canonical) {}
  Gcrc() = delete;
  ~Gcrc() = default;
  uint32_t CrcBlockwords(const void *data, size_t bytes,
                         const uint32_t &start) const {
    return GenericCrc::CrcBlockword(data, bytes, start);
  }
};

static Gcrc gCrc(CRC_POLY, 32, true);

uint32_t crc32(uint32_t prev, void *data, size_t length) {
  return gCrc.CrcBlockwords(data, length, prev);
}
