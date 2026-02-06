#pragma once

class Server;

class WindowsService {
 public:
  static int RunAsService(Server* server);
};
