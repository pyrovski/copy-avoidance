#ifndef WIRE_H

#include "req_generated.h"
/* TODO: need a wire-compatible serialization scheme.
   Flatbuffers?
*/

using Req = Server::Req;

struct LReq {
  int64_t offset;
  uint32_t size;
};

#endif
