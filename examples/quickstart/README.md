# HonTo QuickStart

This example shows a small standalone game project that uses an installed HonTo Engine SDK.

## Build

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:/HonToSDK"
cmake --build build --config Release
```

## What it shows

- a code-first game scene
- gravity and tile collision
- tween animation
- a second utility window
- a scene transition
