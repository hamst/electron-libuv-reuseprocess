#include <functional>
#include <mutex>
#include <napi.h>
#include <queue>
#include <thread>

class NativeAddon : public Napi::ObjectWrap<NativeAddon> {
public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  NativeAddon(const Napi::CallbackInfo &info);
  ~NativeAddon();

private:
  void Dispatch(const Napi::CallbackInfo &info);

private:
  using task_t = std::function<void()>;
  void Enqueue(task_t);
  void Runloop();
  std::queue<task_t> _tasks;
  std::mutex _mutex;
  std::condition_variable _condition;
  bool _stop;
  std::thread _thread;
};