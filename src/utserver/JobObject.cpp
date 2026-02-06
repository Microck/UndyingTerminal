#include "JobObject.hpp"

#ifdef _WIN32
#include <windows.h>

#include <iostream>

#include "Verbose.hpp"

JobObject::JobObject() = default;

JobObject::~JobObject() {
  Close();
}

bool JobObject::Create() {
  Close();
  handle_ = CreateJobObjectW(nullptr, nullptr);
  if (!handle_) {
    if (IsVerbose()) {
      std::cerr << "CreateJobObject failed: " << GetLastError() << "\n";
    }
    return false;
  }

  JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
  limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
  if (!SetInformationJobObject(handle_, JobObjectExtendedLimitInformation, &limits, sizeof(limits))) {
    if (IsVerbose()) {
      std::cerr << "SetInformationJobObject failed: " << GetLastError() << "\n";
    }
    Close();
    return false;
  }

  return true;
}

void JobObject::Close() {
  if (handle_) {
    CloseHandle(static_cast<HANDLE>(handle_));
    handle_ = nullptr;
  }
}
#else
JobObject::JobObject() = default;
JobObject::~JobObject() = default;

bool JobObject::Create() {
  return true;
}

void JobObject::Close() {}
#endif
