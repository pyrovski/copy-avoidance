#ifndef WIRE_H

#include <cstdint>

/* TODO: need a wire-compatible serialization scheme. 
   Flatbuffers?
*/

using Req = struct {
    int64_t offset;
    uint32_t size;
};

#endif
