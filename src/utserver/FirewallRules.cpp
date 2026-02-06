#include "FirewallRules.hpp"

#ifdef _WIN32
#include <netfw.h>
#include <windows.h>

#include <comdef.h>

#include <string>

namespace {
bool AddRule(INetFwPolicy2* policy, int port) {
  INetFwRules* rules = nullptr;
  if (FAILED(policy->get_Rules(&rules)) || !rules) {
    return false;
  }

  INetFwRule* rule = nullptr;
  if (FAILED(CoCreateInstance(__uuidof(NetFwRule), nullptr, CLSCTX_INPROC_SERVER, __uuidof(INetFwRule), reinterpret_cast<void**>(&rule))) || !rule) {
    rules->Release();
    return false;
  }

  _bstr_t name(L"Undying Terminal");
  _bstr_t description(L"Allow Undying Terminal");
  _bstr_t ports(std::to_wstring(port).c_str());

  rule->put_Name(name);
  rule->put_Description(description);
  rule->put_Protocol(NET_FW_IP_PROTOCOL_TCP);
  rule->put_LocalPorts(ports);
  rule->put_Action(NET_FW_ACTION_ALLOW);
  rule->put_Direction(NET_FW_RULE_DIR_IN);
  rule->put_Enabled(VARIANT_TRUE);
  rule->put_Profiles(NET_FW_PROFILE2_ALL);

  const HRESULT hr = rules->Add(rule);
  rule->Release();
  rules->Release();
  return SUCCEEDED(hr);
}
}  // namespace

bool FirewallRules::EnsureRule(int port) {
  HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
  if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
    return false;
  }

  INetFwPolicy2* policy = nullptr;
  hr = CoCreateInstance(__uuidof(NetFwPolicy2), nullptr, CLSCTX_INPROC_SERVER, __uuidof(INetFwPolicy2), reinterpret_cast<void**>(&policy));
  if (FAILED(hr) || !policy) {
    return false;
  }

  const bool ok = AddRule(policy, port);
  policy->Release();
  return ok;
}
#else
bool FirewallRules::EnsureRule(int port) {
  (void)port;
  return true;
}
#endif
