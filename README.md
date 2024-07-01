# firebase-functions-future-async-nested

This repo is an example/test app to repro a problem I am seeing using https://github.com/firebase/firebase-cpp-sdk where `firebase::Future<firebase::functions::HttpsCallableResult>`
appears to deadlock when nested inside a `Future::OnCompletion`.

The problem happens on both the Emulator and the live project.

Opened firebase-cpp-sdk Issue: https://github.com/firebase/firebase-cpp-sdk/issues/1621

Example...

Works:
```C++
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
```

Deadlocks:
```C++
    future = FooAsync([](int value) {
        cout << "tid=" << pthread_self() << " FooAsync #3: value=" << value << endl;
        FooAsync([](int value) { // <- deadlocks a few levels into this call and firebase never logs it called the firebase function
            cout << "tid=" << pthread_self() << " FooAsync #4: value=" << value << endl;
        });
    });
```

Log of the above 6 calls:
```bash
% ./FirebaseExample 
WARNING: Database URL not set in the Firebase config.
DEBUG: Creating Firebase App __FIRAPP_DEFAULT for Firebase C++ 12.1.0
DEBUG: Validating semaphore creation.
DEBUG: Added app name=__FIRAPP_DEFAULT: options, api_key=..., app_id=..., database_url=, messaging_sender_id=..., storage_bucket=..., project_id=... (0x...)
Using Functions Emulator at 127.0.0.1:5001
tid=0x1fefccc00 +Foo(...)
tid=0x1fefccc00 +Cloud::Call(`foo`, ...)
tid=0x1fefccc00 Call: `foo` +Call(...)
DEBUG: Calling Cloud Function with url: 127.0.0.1:5001/functions-futures-async-nested/us-central1/foo
data: {"data":null}
tid=0x1fefccc00 Call: `foo` -Call(...)
tid=0x1fefccc00 ProcessEvents(100)
DEBUG: Cloud Function response body = {"result":200}
tid=0x1fefccc00 +OnCallCompleted(...)
tid=0x1fefccc00 -OnCallCompleted(...)
tid=0x1fefccc00 -Cloud::Call(`foo`, ...)
tid=0x1fefccc00 -Foo(...)
tid=0x1fefccc00 Foo #1: value=200
tid=0x1fefccc00 +Foo(...)
tid=0x1fefccc00 +Cloud::Call(`foo`, ...)
tid=0x1fefccc00 Call: `foo` +Call(...)
DEBUG: Calling Cloud Function with url: 127.0.0.1:5001/functions-futures-async-nested/us-central1/foo
data: {"data":null}
tid=0x1fefccc00 Call: `foo` -Call(...)
tid=0x1fefccc00 ProcessEvents(100)
DEBUG: Cloud Function response body = {"result":200}
tid=0x1fefccc00 +OnCallCompleted(...)
tid=0x1fefccc00 -OnCallCompleted(...)
tid=0x1fefccc00 -Cloud::Call(`foo`, ...)
tid=0x1fefccc00 -Foo(...)
tid=0x1fefccc00 Foo #2: value=200
tid=0x1fefccc00 +FooAsync(...)
tid=0x1fefccc00 +Cloud::Call(`foo`, ...)
tid=0x1fefccc00 Call: `foo` +Call(...)
DEBUG: Calling Cloud Function with url: 127.0.0.1:5001/functions-futures-async-nested/us-central1/foo
data: {"data":null}
tid=0x1fefccc00 Call: `foo` -Call(...)
DEBUG: Cloud Function response body = {"result":200}
tid=0x16b7fb000 +OnCallCompleted(...)
tid=0x16b7fb000 FooAsync #1: value=200
tid=0x16b7fb000 -OnCallCompleted(...)
tid=0x1fefccc00 -Cloud::Call(`foo`, ...)
tid=0x1fefccc00 -FooAsync(...)
tid=0x1fefccc00 +FooAsync(...)
tid=0x1fefccc00 +Cloud::Call(`foo`, ...)
tid=0x1fefccc00 Call: `foo` +Call(...)
DEBUG: Calling Cloud Function with url: 127.0.0.1:5001/functions-futures-async-nested/us-central1/foo
data: {"data":null}
tid=0x1fefccc00 Call: `foo` -Call(...)
DEBUG: Cloud Function response body = {"result":200}
tid=0x16b7fb000 +OnCallCompleted(...)
tid=0x16b7fb000 FooAsync #2: value=200
tid=0x16b7fb000 -OnCallCompleted(...)
tid=0x1fefccc00 -Cloud::Call(`foo`, ...)
tid=0x1fefccc00 -FooAsync(...)
tid=0x1fefccc00 +FooAsync(...)
tid=0x1fefccc00 +Cloud::Call(`foo`, ...)
tid=0x1fefccc00 Call: `foo` +Call(...)
DEBUG: Calling Cloud Function with url: 127.0.0.1:5001/functions-futures-async-nested/us-central1/foo
data: {"data":null}
tid=0x1fefccc00 Call: `foo` -Call(...)
DEBUG: Cloud Function response body = {"result":200}
tid=0x16b7fb000 +OnCallCompleted(...)
tid=0x16b7fb000 FooAsync #3: value=200
tid=0x16b7fb000 +FooAsync(...)
tid=0x16b7fb000 +Cloud::Call(`foo`, ...)
^C
```

Note at the end that the app hung indefinitely before the 6th `DEBUG: Calling Cloud Function` was logged and I had to Ctrl-Break.

I suspect this has something to do with the muxtex(s)/semaphore(s) used to track pending Futures and their OnCompletion.

One **admittedly not directly related** hint I got about this was from https://github.com/firebase/firebase-cpp-sdk/issues/51#issuecomment-654535769
> In my case I resolved the issue by ensuring the HttpsCallableReference was not destroyed until the future had resolved. The destructor for this class releases the future and calls rest::CleanupTransportCurl() which I imagine is the cause of the problem.

It seems like the official code could stand for a bit more verbose logging around these areas... but what do I know?

I am not certain if these issues are [closely?] related:
* https://github.com/firebase/firebase-cpp-sdk/issues/61
* ...

## To test/run/use...

### firebase functions
```bash
cd firebase
```

To run in emulator:
```bash
firebase emulators:start --only functions
```

To deploy to firebase cloud:
```bash
firebase deploy --only functions
```

### app

To build:
```bash
cd app

# once, or whenever firebase-cpp-sdk changes
pushd libs
./get-firebase-cpp-sdk.sh
popd

mkdir -p build && cd $_

# Define an Android app in your firebase project and download its google-services.json
cp /path/to/your/google-services.json .

# once, or whenever CMakeLists.txt changes
cmake ..

# whenever code changes
cmake --build .
```

To run [and use firebase emulator by default]:
```bash
./FirebaseExample
```

To run and not use emulator [use firebase cloud]:
```bash
./FirebaseExample --no-emulator
```
