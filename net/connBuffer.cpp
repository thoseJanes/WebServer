#include "connBuffer.h"

using namespace webserver;

const char ConnBuffer::kCRLF[3] = "\r\n";
int ConnBuffer::kPrependSize_ = 8;
int ConnBuffer::kInitialSize_ = 1024;