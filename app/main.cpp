#include <iostream>
#include <memory>
#include <cstring>
#include <firebase/app.h>
#include <firebase/functions.h>
#include <firebase/log.h>

using namespace std;

bool ProcessEvents(int msec);
void WaitForCompletion(const firebase::FutureBase &future);

unique_ptr<firebase::App> app_;
unique_ptr<firebase::functions::Functions> functions_;

firebase::Variant OnCallCompleted(
    const firebase::Future<firebase::functions::HttpsCallableResult> &future,
    std::function<void(const firebase::Variant &data, int error,
               const std::string &)>
        completion_callback);

firebase::Variant Call(const char *name, const firebase::Variant &data,
        std::function<void(const firebase::Variant &data, int error,
                   const std::string &)>
            completion_callback)
{
    cout << "tid=" << pthread_self() << " +Cloud::Call(`" << name << "`, ...)" << endl;
    firebase::Variant result;
    if (functions_) {
        auto httpcallref = functions_->GetHttpsCallable(name);
        cout << "tid=" << pthread_self() << " Call: `" << name << "` +Call(...)" << endl;
        auto future = httpcallref.Call(data);
        cout << "tid=" << pthread_self() << " Call: `" << name << "` -Call(...)" << endl;
        WaitForCompletion(future);
        result = OnCallCompleted(future, nullptr);
    }
    cout << "tid=" << pthread_self() << " -Cloud::Call(`" << name << "`, ...)" << endl;
    return result;
}

firebase::Future<firebase::functions::HttpsCallableResult> CallAsync(const char *name, const firebase::Variant &data,
        std::function<void(const firebase::Variant &data, int error,
                   const std::string &)>
            completion_callback)
{
    cout << "tid=" << pthread_self() << " +Cloud::Call(`" << name << "`, ...)" << endl;
    firebase::Future<firebase::functions::HttpsCallableResult> future;
    if (functions_) {
        auto httpcallref = functions_->GetHttpsCallable(name);
        cout << "tid=" << pthread_self() << " Call: `" << name << "` +Call(...)" << endl;
        future = httpcallref.Call(data);
        cout << "tid=" << pthread_self() << " Call: `" << name << "` -Call(...)" << endl;
        future.OnCompletion(
            [completion_callback](
                const firebase::Future<
                    firebase::functions::
                        HttpsCallableResult>
                    &future) {
                OnCallCompleted(future,
                        completion_callback);
            });
    }
    cout << "tid=" << pthread_self() << " -Cloud::Call(`" << name << "`, ...)" << endl;
    return future;
}

firebase::Variant OnCallCompleted(
    const firebase::Future<firebase::functions::HttpsCallableResult> &future,
    std::function<void(const firebase::Variant &data, int error,
               const std::string &)>
        completion_callback)
{
    cout << "tid=" << pthread_self() << " +OnCallCompleted(...)" << endl;
    firebase::Variant data;
    auto status = future.status();
    cout << "tid=" << pthread_self() << " OnCallCompleted: status=" << status << endl;
    if (status == firebase::kFutureStatusComplete) {
        auto httpcallresult = future.result();
        auto error = future.error();
        auto error_message = future.error_message();
        if (error == 0) {
            data = httpcallresult->data();
        }
        if (completion_callback) {
            completion_callback(data, error, error_message);
        }
    }
    cout << "tid=" << pthread_self() << " -OnCallCompleted(...)" << endl;
    return data;
}

int Foo()
{
    cout << "tid=" << pthread_self() << " +Foo(...)" << endl;
    auto value = Call("foo", firebase::Variant(), nullptr).int64_value();
    cout << "tid=" << pthread_self() << " -Foo(...)" << endl;
    return value;
}

firebase::Future<firebase::functions::HttpsCallableResult> FooAsync(std::function<void(int)> completion_callback)
{
    cout << "tid=" << pthread_self() << " +FooAsync(...)" << endl;
    auto future = CallAsync("foo", firebase::Variant(),
         [completion_callback](const firebase::Variant &data,
                     int error, const std::string &) {
             int value = (error == 0) ? data.int64_value() : -1;
             if (completion_callback) {
                 completion_callback(value);
             }
         });
    cout << "tid=" << pthread_self() << " -FooAsync(...)" << endl;
    return future;
}

int main(int argc, char* argv[]) {
    firebase::SetLogLevel(firebase::kLogLevelVerbose);

    app_ = unique_ptr<firebase::App>(firebase::App::Create());

    functions_ = unique_ptr<firebase::functions::Functions>(firebase::functions::Functions::GetInstance(app_.get()));

    bool use_emulator = true;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--no-emulator") == 0) {
            use_emulator = false;
        }
    }

    if (use_emulator) {
        auto emulator_host = "127.0.0.1:5001";
        cout << "Using Functions Emulator at " << emulator_host << endl;
        functions_->UseFunctionsEmulator(emulator_host);
    } else {
        cout << "Using Functions Non-Emulator" << endl;
    }

    auto value = Foo();
    cout << "tid=" << pthread_self() << " Foo #1: value=" << value << endl;
    value = Foo();
    cout << "tid=" << pthread_self() << " Foo #2: value=" << value << endl;
    auto future = FooAsync([](int value) {
        cout << "tid=" << pthread_self() << " FooAsync #1: value=" << value << endl;
    });
    future = FooAsync([](int value) {
        cout << "tid=" << pthread_self() << " FooAsync #2: value=" << value << endl;
    });
    future = FooAsync([](int value) {
        cout << "tid=" << pthread_self() << " FooAsync #3: value=" << value << endl;
        FooAsync([](int value) {
            cout << "tid=" << pthread_self() << " FooAsync #4: value=" << value << endl;
        });
    });

    WaitForCompletion(future);

    return 0;
}

#ifndef _WIN32
#include <unistd.h>
#endif // _WIN32

static bool quit = false;

bool ProcessEvents(int msec)
{
    cout << "tid=" << pthread_self() << " ProcessEvents(" << msec << ")" << endl;
#ifdef _WIN32
    Sleep(msec);
#else
    usleep(msec * 1000);
#endif // _WIN32
    return quit;
}

void WaitForCompletion(const firebase::FutureBase &future)
{
    while (future.status() == firebase::kFutureStatusPending) {
        ProcessEvents(100);
    }
}
