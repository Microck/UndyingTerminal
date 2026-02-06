#pragma once

class JobObject {
 public:
  JobObject();
  ~JobObject();

  JobObject(const JobObject&) = delete;
  JobObject& operator=(const JobObject&) = delete;

  bool Create();
  void Close();

 private:
#ifdef _WIN32
  void* handle_ = nullptr;
#endif
};
