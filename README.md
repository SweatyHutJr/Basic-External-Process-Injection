# Basic External Process Injection

A minimal Windows example of **external process injection** written in C++. Injects
position-independent shellcode into a process of your choosing (by PID) and pops
`calc.exe` to demonstrate that the payload executed inside the target.

> ⚠️ **For educational and research use only.** Do not use this on any process or machine
> you do not own or have explicit written permission to test. Unauthorized injection is
> illegal in most jurisdictions and violates the terms of most software.

## What it does

Injects raw shellcode into a target process from a separate (external) injector using
only standard Win32 APIs:

1. `OpenProcess` — obtain a handle to the target PID with `PROCESS_ALL_ACCESS`
2. `VirtualAllocEx` — allocate an `RWX` region in the target for the shellcode
3. `WriteProcessMemory` — write the shellcode bytes into that allocation
4. `CreateRemoteThread` — spawn a thread in the target pointing at the shellcode
5. `WaitForSingleObject` — wait for the payload to run
6. `VirtualFreeEx` / `CloseHandle` — clean up

The shellcode itself calls `WinExec("calc.exe", SW_SHOW)` (or `system("calc")`) inside
the target, so you see a Calculator window appear when injection succeeds.

## Features

- Clean, commented implementation of the standard external shellcode-injection chain
- User-supplied **target PID** at runtime
- No external dependencies beyond the Windows SDK
- Ships with a Visual Studio solution (`External Process Injection.slnx`)
- Error-checked at every Win32 call

## Requirements

- **Windows**
- **Visual Studio 2022** with the *Desktop development with C++* workload
- Target architecture must **match** the injector (x64 injector → x64 target)

## Build

1. Open `External Process Injection.slnx` in Visual Studio 2022.
2. Set the configuration to **Release** and platform to **x64** (or **x86** — must match
   your target).
3. Build the solution (`Ctrl+Shift+B`).
4. The executable is output to `x64\Release\` (or your chosen config folder).

## Usage

1. Launch the target process you want to inject into (one you own — e.g. `notepad.exe`).
2. Find its PID (Task Manager → Details tab, or `tasklist` in a terminal).
3. Run the injector and enter the PID when prompted.
4. On success, `calc.exe` opens — that's the shellcode executing inside the target.

If no calculator appears, the injection failed or the shellcode crashed — check the
injector's error output (each Win32 call is checked and prints `GetLastError()` on
failure).

## How it works

The injector allocates a page in the target with `VirtualAllocEx(..., PAGE_EXECUTE_READWRITE)`,
writes the shellcode into it, and starts a remote thread at that address with
`CreateRemoteThread`. The shellcode is position-independent and self-contained — it
resolves `WinExec` and calls it, so no DLL path or `LoadLibrary` is needed. Because the
thread runs inside the target, the Calculator it spawns is a child of the *target*
process, not the injector.

## Notes

- **Architecture must match.** Shellcode written for x64 won't run in a 32-bit target
  (and vice-versa). Injector, shellcode, and target bitness must all agree.
- **RWX allocation is noisy.** `PAGE_EXECUTE_READWRITE` pages and `CreateRemoteThread`
  into another process are two of the most heavily monitored events by EDR/AV. This is
  the *basic* method and is expected to be detected.
- **Protected / elevated processes** require the injector to run elevated and may still
  fail with `ERROR_ACCESS_DENIED`.
- The shellcode is a trivial `calc` payload — swap the byte array to change what runs.

---

> **Disclaimer:** This project is provided for learning about Windows process memory,
> shellcode execution, and the Win32 injection APIs. The author is not responsible for
> any misuse. Only use it on processes and systems you own or are authorized to test.
