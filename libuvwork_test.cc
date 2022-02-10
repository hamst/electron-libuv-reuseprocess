#include <chrono>
#include <napi.h>
#include <node.h>
#include <thread>
#include <v8.h>

using namespace Napi;

class EchoWorker : public AsyncWorker {
public:
  EchoWorker(Function &callback, std::string &echo)
      : AsyncWorker(callback), echo(echo) {}

  ~EchoWorker() {}

  void Execute() override {
    std::this_thread::sleep_for(std::chrono::milliseconds(4));
  }

  void OnOK() override {
    HandleScope scope(Env());
    Callback().Call({Env().Null(), String::New(Env(), echo)});
  }

private:
  std::string echo;
};

Napi::Value Method(const Napi::CallbackInfo &info) {
  Function cb = info[1].As<Function>();
  std::string in = info[0].As<String>();
  EchoWorker *wk = new EchoWorker(cb, in);
  wk->Queue();
  return info.Env().Undefined();
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set(Napi::String::New(env, "echo"), Napi::Function::New(env, Method));
  return exports;
}

NODE_API_MODULE(hello, Init)