# GameStudioNative — C++ backend

Native C++ implementation of the dashboard's analytical core (build health,
render-budget percentiles, best-fit scheduler, retention/LTV fit). The WPF app
calls it through **P/Invoke** (`GameStudioDashboard/Native/NativeInterop.cs`).

The integration is **optional**: if the DLL is not found, `NativeBridge` reports
it and the app transparently uses the managed C# implementations in
`Services/Algorithms.cs`. The Settings view shows which backend is live.

## How the C# ⇄ C++ boundary works

- The C++ side exports plain `extern "C"` functions (see `include/gsd_api.h`) —
  no C++ name mangling, no C++/CLI needed.
- Data crosses as primitives, flat arrays, and POD structs. Struct fields are
  ordered doubles-first then ints so the C layout matches the C#
  `[StructLayout(LayoutKind.Sequential)]` mirrors on x64/arm64.
- The library is named `GameStudioNative` with no `lib` prefix so
  `DllImport("GameStudioNative")` resolves the same on Windows/Linux/macOS.
- `gsd_abi_version()` guards against loading a stale/incompatible DLL.

## Build (Windows, recommended — matches the WPF target)

Using Visual Studio (open `GameStudioNative.vcxproj`, build **Release | x64**), or CMake:

```bat
cd native
cmake -B build -A x64
cmake --build build --config Release
```

The `.csproj` copies `native/build/**/GameStudioNative.dll` next to the app on
build, so a subsequent `dotnet build` of the WPF project picks it up automatically.
If you build the DLL elsewhere, just drop `GameStudioNative.dll` beside
`GameStudioDashboard.exe`.

## Build (Linux/macOS — for testing the native code only)

```sh
cd native
cmake -B build && cmake --build build
```

Produces `GameStudioNative.so`/`.dylib`. Note the WPF app itself still only runs
on Windows; this is just to exercise/verify the C++ core cross-platform.

## Keeping the two backends in sync

`gsd_algorithms.cpp` is a line-for-line port of `Services/Algorithms.cs`. If you
change one, change the other and bump `GSD_ABI` in `gsd_api.h` +
`NativeInterop.ExpectedAbi` when the ABI (struct layout / signatures) changes.
