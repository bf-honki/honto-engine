# __HONTO_PROJECT_NAME__

`__HONTO_PROJECT_NAME__` is a starter game project for the HonTo Engine SDK.

## Build

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:/HonToSDK"
cmake --build build --config Release
```

If Windows Smart App Control blocks unsigned executables on your machine, configure signing during CMake configure:

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:/HonToSDK" -DHONTO_SIGN_CERT_SHA1="YOUR_CERT_SHA1"
cmake --build build --config Release
```

If you are using Visual Studio, open the generated solution in `build` and run `__HONTO_PROJECT_NAME__`.

## What this project shows

- starting a game with `honto::hontoGame(...)`
- building a scene with the `honto::hontoStage` API
- creating actors, UI, animation, collision, and a second window
