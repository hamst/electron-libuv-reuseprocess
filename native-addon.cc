#include "native-addon.h"
#include "get-uv-event-loop-napi.h"
#include "uv.h"
#include <chrono>
#include <iostream>

namespace {

struct Context {
  Context(Napi::FunctionReference callback)
      : cb(std::move(callback)), ts(std::chrono::high_resolution_clock::now()) {
    work.data = this;
  }
  Context(const Context &) = delete;
  Context &operator=(const Context &) = delete;

  Napi::FunctionReference cb;
  uv_work_t work = {};
  const std::chrono::time_point<std::chrono::high_resolution_clock> ts;
};

static void noop_execute(uv_work_t *) {}

static void execute(uv_work_t *req, int /*status*/) {
  Context *ctx_ptr = reinterpret_cast<Context *>(req->data);
  std::unique_ptr<Context> task(ctx_ptr);
  const auto now = std::chrono::high_resolution_clock::now();
  auto elapsed =
      std::chrono::duration_cast<std::chrono::microseconds>(now - task->ts)
          .count();
  auto duration =
      Napi::Number::New(task->cb.Env(), static_cast<double>(elapsed));
  task->cb.Call({duration});
}

} // namespace

Napi::Object NativeAddon::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(
      env, "NativeAddon", {InstanceMethod("dispatch", &NativeAddon::Dispatch)});

  Napi::FunctionReference *constructor = new Napi::FunctionReference();
  *constructor = Napi::Persistent(func);

  exports.Set("NativeAddon", func);
  env.SetInstanceData<Napi::FunctionReference>(constructor);
  return exports;
}

NativeAddon::NativeAddon(const Napi::CallbackInfo &info)
    : Napi::ObjectWrap<NativeAddon>(info), _stop(false),
      _thread(&NativeAddon::Runloop, this) {
  std::cerr << "NativeAddon ctor " << this << std::endl;
}

NativeAddon::~NativeAddon() {
  std::cerr << "NativeAddon dctor " << this << std::endl;
  {
    std::unique_lock<std::mutex> lock(_mutex);
    _stop = true;
  }
  _condition.notify_one();
  _thread.join();
}

void NativeAddon::Dispatch(const Napi::CallbackInfo &info) {
  auto cb = std::make_shared<Napi::FunctionReference>();
  *cb = Napi::Persistent(info[0].As<Napi::Function>());
  Enqueue([cb]() {
    uv_loop_t *loop = get_uv_event_loop(cb->Env());
    auto context = std::make_unique<Context>(std::move(*cb));
    if (0 == uv_queue_work(loop, &(context->work), noop_execute, execute)) {
      context.release();
    }
  });
}

void NativeAddon::Enqueue(task_t task) {
  {
    std::unique_lock<std::mutex> lock(_mutex);
    _tasks.push(std::move(task));
  }
  _condition.notify_one();
}

void NativeAddon::Runloop() {
  while (true) {
    task_t task;
    {
      std::unique_lock<std::mutex> lock(_mutex);
      _condition.wait(lock, [this] { return _stop || _tasks.size(); });
      if (_stop)
        return;
      task = std::move(_tasks.front());
      _tasks.pop();
    }
    task();
  }
}