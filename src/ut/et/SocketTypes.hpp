#pragma once
 
#include <cstdint>
 
namespace ut { 
using SocketHandle = std::uintptr_t;
constexpr SocketHandle kInvalidSocket = static_cast<SocketHandle>(-1);
}
