#pragma once
 
namespace et {
constexpr int kProtocolVersion = 6;
constexpr unsigned char kClientServerNonceMsb = 0;
constexpr unsigned char kServerClientNonceMsb = 1;
constexpr int kMaxBackupBytes = 64 * 1024 * 1024;
}
