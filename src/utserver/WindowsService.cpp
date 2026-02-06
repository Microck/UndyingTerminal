#include "WindowsService.hpp"

#ifdef _WIN32
#include <windows.h>

#include "Server.hpp"

namespace {
SERVICE_STATUS_HANDLE g_status_handle = nullptr;
Server* g_server = nullptr;

void SetServiceState(DWORD state, DWORD win32_exit_code = NO_ERROR) {
  SERVICE_STATUS status{};
  status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
  status.dwCurrentState = state;
  status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
  status.dwWin32ExitCode = win32_exit_code;
  SetServiceStatus(g_status_handle, &status);
}

DWORD WINAPI ServiceCtrlHandler(DWORD control, DWORD, void*, void*) {
  switch (control) {
    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:
      if (g_server) {
        g_server->Stop();
      }
      SetServiceState(SERVICE_STOPPED);
      return NO_ERROR;
    default:
      return NO_ERROR;
  }
}

void WINAPI ServiceMain(DWORD, LPWSTR*) {
  g_status_handle = RegisterServiceCtrlHandlerExW(L"UndyingTerminal", ServiceCtrlHandler, nullptr);
  if (!g_status_handle) {
    return;
  }

  SetServiceState(SERVICE_START_PENDING);
  if (g_server && g_server->Start()) {
    SetServiceState(SERVICE_RUNNING);
  } else {
    SetServiceState(SERVICE_STOPPED, ERROR_SERVICE_SPECIFIC_ERROR);
  }
}
}  // namespace

int WindowsService::RunAsService(Server* server) {
  g_server = server;
  SERVICE_TABLE_ENTRYW service_table[] = {
      {const_cast<LPWSTR>(L"UndyingTerminal"), ServiceMain},
      {nullptr, nullptr}};

  if (!StartServiceCtrlDispatcherW(service_table)) {
    return 1;
  }
  return 0;
}
#else
#include "Server.hpp"

int WindowsService::RunAsService(Server* server) {
  (void)server;
  return 1;
}
#endif
