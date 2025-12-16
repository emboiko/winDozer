# winDozer

### Install & Run

**Option 1: Download a release**

- Grab the latest `.zip` from GitHub Releases (contains `winDozer.exe`).
- Unzip and run `winDozer.exe`.

**Option 2: Build from source**

See [building](#building)

```
git clone ... winDozer
cd winDozer
build.bat   # outputs winDozer.exe in repo root
winDozer.exe [flags]
```

### Getting Started

1. `winDozer.exe`
2. Focus a window and place it _somewhere_ (use winDozer terminal to learn the flow)
3. Set a Rect ID: **S**et **R**ect (ID) `sr1`, then `<Submit>` (default: `<RCtrl>`)
4. Either move the focused window or focus a different one
5. Move the window to a Rect ID: **M**ove **T**his _to_ **R**ect (ID) `mtr1`, then `<Submit>` (default: `<RCtrl>`)

### Breakdown

Step 1: We launch an instance of winDozer

Step 2: A given window is placed somewhere on the desktop. As an example, we can use the winDozer terminal window.

Step 3: We assign the geometry of the currently focused window to a **Rect ID** by typing directly at the window: `sr{rectID}` followed by `<Submit>` (default: `<RCtrl>`)

By default, a Rect ID is any integer in range (0-9999), _although this is not strictly enforced._ Writing window geometry to a previously assigned ID will overwrite the previous geometry for that ID. Rect IDs are persisted to disk in Windows INI format at `$APPDATA/winDozer/settings.ini`. (See [Command Line Arguments](#commandline))

Step 4: For this example, let's manually move the window somewhere else on the desktop so we can put it back with winDozer

Step 5: We move the focused window to the rect/geometry described by the Rect ID we set earlier by typing directly at the window: `mtr1` followed by `<Submit>` (default: `<RCtrl>`)

### Syntax

<a name="validkeys"></a>

Keystrokes considered _valid_:

- A-Z
- Numrow 0-9
- Numpad 0-9
- Fn 1-9 (Same as numrow & numpad)
- `<RCtrl>` (See [Command Line Arguments](#commandline))
  - Arrow keys Left, Right, Up, Down (During an [adjustment](#adjustment))
  - `<Modifier>` (During an [adjustment](#adjustment) - enables resize mode, default: `<LCtrl>`)
- `<Submit>` (default: `<RCtrl>`, configurable via `vks` flag)

---

<a name="precedence"></a>

Buffer evaluation and conflict resolution

- The buffer slides left as you type; newest characters land on the right.
- On `<Submit>`, the interpreter tokenizes the whole buffer, builds a pattern string, and picks the **rightmost (most recent) matching command** via `rfind`.
- Arguments are also gathered from right-to-left so the newest args win (e.g., `AW1S5AW2S50` selects window `2`, step `50`).
- If two patterns start at the same position, the order in `WinDozerInput.cpp`'s pattern list breaks ties, but the rightmost rule handles most conflicts.

**Note on stale commands and accidental submits:**

The buffer can contain valid command syntax mixed with invalid characters. For example, a standard size buffer (20 characters) might look like `_CT_________________` where `_` represents other characters that don't form valid syntax. If you accidentally press `<Submit>`, the valid command (`CT` in this case) will execute, even though it may have been typed much earlier in another context and forgotten.

This is one reason why smaller buffer sizes are recommended. Larger buffers increase the risk of accidentally executing stale commands if `<Submit>` is pressed unintentionally. The default submit key (`<RCtrl>`) was chosen specifically because it's rarely used in most workflows, reducing the chance of accidental submits. If `<RCtrl>` conflicts with your workflow, you can change it using the `vks` flag (see [Command Line Arguments](#commandline)).

**Syntax detail**

- Move Window

Move Window by its Window ID to the rect described by a Rect ID

| M   | W   | {Win ID} | R   | {Rect ID} |
| --- | --- | -------- | --- | --------- |

`MW5R10`

---

- Move This [window]

Move This [window] to Rect {Rect ID}

| M   | T   | R   | {Rect ID} |
| --- | --- | --- | --------- |

`MTR1`

---

- Set Rect ID

Set Rect ID to geometry of focused window. Rect IDs are persisted to disk in INI format between instances of winDozer.

| S   | R   | {Rect ID} |
| --- | --- | --------- |

`SR99`

---

- Set Window ID

Set the Window ID of the focused window. Window IDs are not persisted to disk and must be set for each instance of winDozer.

| S   | W   | {Win ID} |
| --- | --- | -------- |

`SW5`

---

- Erase Rect ID

Unregister a Rect ID. (Delete `$APPDATA/winDozer/settings.ini` and relaunch to unregister _every_ Rect ID)

| E   | R   | {Rect ID} |
| --- | --- | --------- |

`ER99`

---

- Erase Window ID

Unregister a Window ID. (Restarting winDozer will unregister _every_ Window ID) Closing a window will automatically unregister it.

| E   | W   | {Win ID} |
| --- | --- | -------- |

`EW5`

---

- Focus Window

Focus window by its assigned {Win ID}

| F   | W   | {Win ID} |
| --- | --- | -------- |

`FW5`

<a name="adjustment"></a>

---

- Adjust This [window]

_For those of us who are particularly obsessive-compulsive:_

Adjusts the focused window by shifting its location or size by a specified number of pixels at a time (default: 1px). If this syntax is evaluated from the buffer following `<Submit>`, input is limited to only the arrow keys until a subsequent `<Submit>` is input.

**Hold `<Modifier>` (default: `<LCtrl>`) while using arrow keys to resize the window instead of moving it.**

| A   | T   | [S{step}] |
| --- | --- | --------- |

`AT` (adjusts by 1px)  
`ATS10` (adjusts by 10px)

`<Submit>`

_then_

```
<Arrow Key Up/Down/Left/Right>        (moves window)
<Modifier> + <Arrow Key Up/Down/Left/Right>  (resizes window, default: <LCtrl>)
```

_then_

`<Submit>`

---

- Adjust Window

_See [`Adjust This`](#adjustment)_

Adjusts the window by shifting its location or size by a specified number of pixels at a time (default: 1px). If this syntax is evaluated from the buffer following `<Submit>`, input is limited to only the arrow keys until a subsequent `<Submit>` is input. This syntax will lift the window if minimized, but will not necessarily focus the window.

**Hold `<Modifier>` (default: `<LCtrl>`) while using arrow keys to resize the window instead of moving it.**

| A   | W   | {Win ID} | [S{step}] |
| --- | --- | -------- | --------- |

`AW25` (adjusts window 25 by 1px)  
`AW25S5` (adjusts window 25 by 5px)

---

- Copy Geometry

Copy and store the focused window's geometry (dimensions only) for later use. The geometry is stored in memory and will be lost when winDozer exits.

| C   | G   |
| --- | --- |

`CG`

---

- Paste Geometry

Apply the stored geometry (size only) to the focused window. The window will be resized to match the stored dimensions while keeping its current position. Use `CG` first to copy a window's geometry.

| V   | G   |
| --- | --- |

`VG`

---

- Pin This [window]

Toggle the always-on-top status of the focused window. When pinned, the window will stay above all other non-pinned windows. Running the command again will unpin the window, or attempt to unpin provided it was pinned in the first place.

| P   | T   |
| --- | --- |

`PT`

---

- Center This [window]

Center the focused window on its current display. The window is centered within the display's work area (excluding the taskbar). If the window spans multiple displays, it is centered on the display containing the window's center point. This command does not resize the window.

| C   | T   |
| --- | --- |

`CT`

---

- Save Layout Snapshot

Capture the current layout of all open, non-minimized windows and store it as a snapshot. The snapshot includes each window's class name, title, geometry, maximized state, Z-order (window stacking order), and the currently focused window. Snapshots are stored in memory and will be lost when winDozer exits. Minimized windows are automatically excluded from snapshots.

| S   | S   | {Snapshot ID} |
| --- | --- | ------------- |

`SS1`

---

- Restore Layout Snapshot

Restore a previously saved layout snapshot. winDozer uses a two-step matching process to find windows:

1. **Primary matching by window handle (HWND)**: First, winDozer attempts to match windows using their stored window handle. This allows windows to be restored even if their title has changed (e.g., switching browser tabs). The window handle is verified to still be valid and the window class is checked to ensure it's the same window.

2. **Fallback matching by class + title**: If the window handle is no longer valid (window was closed and reopened), winDozer falls back to matching by window class name and title.

Windows that cannot be matched (closed and not reopened with the same class+title) will be skipped. Windows not included in the snapshot will be ignored.

**Restoration Process:**

1. **Window positions and sizes** are restored first (for non-maximized windows).
2. **Z-order (window stacking)** is restored, maintaining the original window stacking order from top to bottom.
3. **Maximized state** is restored for windows that were maximized when the snapshot was taken. Windows are first restored to their normal position, then maximized.
4. **Focus** is restored to the window that had focus when the snapshot was saved. If that window is no longer available (closed), focus falls back to the topmost window in the restored layout.

The command reports how many windows were restored and how many were not found if the `verbose` argument is used.

| R   | S   | {Snapshot ID} |
| --- | --- | ------------- |

`RS1`

---

- Get Rects

Print Rect IDs & their geometry to stdout, showing X, Y, Width, and Height for each rect.

| G   | R   |
| --- | --- |

`GR`

---

- Get Windows

Print Window IDs, window class, and an associated title, if one can be gleaned from the window handle.

| G   | W   |
| --- | --- |

`GW`

---

- Get Snapshots

Print all saved layout snapshots with their IDs, window counts, and details for each window in each snapshot (class, title, position, and size).

| G   | S   |
| --- | --- |

`GS`

---

- Help

Print a simple help dialog to stdout

| H   | E   | L   | P   |
| --- | --- | --- | --- |

`HELP`

---

- Flush

Manually flush the internal buffer, for use with `dbf`

| F   | L   | U   | S   | H   |
| --- | --- | --- | --- | --- |

`FLUSH`

---

- Exit

Exit winDozer cleanly. This saves settings, unhooks keyboard and window event hooks, cleans up the lock file, and exits the application. Equivalent to pressing `Ctrl+C` in the terminal.

| E   | X   | I   | T   |
| --- | --- | --- | --- |

`EXIT`

---

<a name="commandline"></a>

### Command line arguments

- `dbf` : Disable buffer flush

If this flag is passed, `<Submit>` will only flush the buffer if the buffer contains "FLUSH". This is useful to repeat a previous action with fewer keystrokes, provided the buffer hasn't been polluted in the meantime.

- `verbose` : Verbose console output

This flag satisfies a few conditionals that print some extra feedback to stdout when winDozer does things.

- `cleanup` : Cleanup mode

Enables winDozer to do its best to clean up after itself. Upon `<Submit>`, a number of backspace keystrokes will be synthesized corresponding to the number of characters contained in the parsed syntax (if the command was valid). This helps cut down on backspacing manually if you frequently use winDozer from text fields.

Note: Only the length of the valid syntax is erased, for example, `MMTR41` will leave behind a single 'M' character in the user's focused text field.

Note: This cleanup is only an attempt, and if the focus changes between buffer population and evaluation, the cleanup will take place in the newly focused window, which won't contain anything to clean up. Furthermore, the newly focused window may or may not contain bindings for backspace if its a meaningful operation in that context. TLDR: Be careful about sending backspaces to the wrong window with this flag if your focused window changes between _typing_ your command and _submitting/evaluating_ it.

Note: If the focused window is a modern/UWP app, the cleanup takes place with a delay between backspaces to avoid accelerated deletion from exceeding the keyboard repeat rate. Otherwise, there is no delay, and keystrokes are sent as fast as possible.

- `vks` : Virtual Key Submit

Set a non-default submit key. For example, `windozer vks162` will set `<LCtrl>` as winDozer's _submit_ key.

This flag should be called with an integer representing a [virtual key](https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes). Overwrite [valid keystrokes](#validkeys) with this at your own risk. Setting integers that do not represent existing win32 virtual-key codes will effectively disable the _submit_ function, and the buffer will never be evaluated.

- `vkm` : Virtual Key Modifier

Set a non-default modifier key for resize mode during window adjustment. For example, `windozer vkm162` will set `<LCtrl>` as the modifier (this is also the default (VK_LCONTROL)).

This flag should be called with an integer representing a [virtual key](https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes). The modifier key is held down while using arrow keys during adjustment mode to toggle resize instead of move.

- `bs` : Buffer Sizer

Set a non-default buffer size. For example, `windozer bs10` will set winDozer's internal buffer to 10 characters in length.

This flag should be used with an integer >= 7. The default buffer size is 20, which supports all command syntaxes including long IDs (e.g., `MW1000R556655` requires 13 characters). Very large buffer sizes (>100) may result in more frequent [precedence issues](#precedence) and are not recommended. Increase this if you like using large IDs for windows and rects.

- `debug` : Debug mode

This flag is intended for development, and in most cases will flood stdout as the buffer is shifted with valid input.

---

### Invalid windows

winDozer will discard attempts to Move, Adjust, Resize, or set a Window ID on the following windows:

- The start menu
- The system tray
- The desktop itself
- Other core/system windows

---

### Deliberately unimplemented behavior

Windows GUI has plenty of hotkeys and macros for power users such as `alt+tab`, `win+tab`, `win+shift+M`, and `win+<arrow>`. winDozer doesn't try to wrap behavior that is already made convenient in Windows. There is no syntax, for example, to minimize the focused window because there is already an _equally fast, equally convenient, prexisting way_ to do the exact same thing. However, sometimes you really want that window adjusted by _one pixel_ and nothing more. If this is a repetitive task, using the mouse can be exhausting.

---

### Run on startup

Simply create a shortcut to winDozer and place it in the Startup folder:

**For all users:**

C:\ProgramData\Microsoft\Windows\Start Menu\Programs\Startup\

**For the login user only:**

C:\Users\user_folder\AppData\Roaming\Microsoft\Windows\Start Menu\Programs\Startup\

Arguments for this startup instance can be specified in the `target` field in the shortcut properties

---

<a name="building"></a>

### Building, hacking, and contributing

winDozer is a C++17 application that requires a Windows-compatible C++ compiler. The project uses MinGW-w64 (via MSYS2) for compilation.

**Prerequisites:**

- Windows 10 or later
- MinGW-w64 (recommended: [MSYS2](https://www.msys2.org/))
- `g++` compiler (included with MinGW-w64)

**Build Instructions:**

1. Clone the repository:

   ```bash
   git clone <repository-url> winDozer
   cd winDozer
   ```

2. Ensure `g++` is in your PATH, or install MSYS2 and ensure `msys64\mingw64\bin\g++.exe` exists

3. Run the build script:

   ```bash
   build.bat
   ```

4. The compiled executable `winDozer.exe` will be created in the project root

**Build Flags:**

The build script compiles with the following flags:

- `-Wall -Wextra` - Enable additional warnings
- `-static-libgcc -static-libstdc++` - Static linking for portability
- `-std=c++17` - C++17 standard

**Project Structure:**

```
winDozer/
├── src/
│   ├── main.cpp              # Entry point, hook setup, message loop
│   ├── WinDozer.cpp          # Core window manipulation logic
│   ├── WinDozerInput.cpp    # Keyboard input, buffer management, command parsing
│   ├── WinDozerUtils.cpp    # Configuration, file I/O, initialization
│   └── headers/
│       └── WinDozer.h        # Main struct and function declarations
├── build.bat                 # Build script
├── .clang-format            # Code formatting configuration
└── readme.md                # This file
```

Contributions are welcome! When contributing:

1. **Code Style:** The project uses `clang-format` for consistent formatting. Format your code before submitting:

   ```bash
   clang-format -i --style=file src/*.cpp src/headers/*.h
   ```

   **Variable Naming:** Use explicit, descriptive variable names when the meaning isn't immediately obvious. Short names are acceptable in clear contexts (e.g., `point.x`, `point.y`, or `it` for iterators), but prefer expanded names for clarity (e.g., `monitorInfo` instead of `mi`, `position` instead of `pos`, `exception` instead of `e`, `command` instead of `cmd`).

2. **Testing:** Test your changes thoroughly, especially:

   - Multi-monitor setups with different DPI settings
   - Various window types and edge cases
   - Command syntax parsing with different buffer sizes

3. **Documentation:** Update the readme if you add new features or change existing behavior. Changes to internal mechanisms should be documented in the code itself.

4. **Pull Requests:**
   - Keep changes focused and well-documented.
   - Ensure the project builds successfully with `build.bat`
   - Test on Windows 10/11 before submitting

---

### Buffer & Interpreter

Currently, winDozer's internal syntax buffer is implemented as a character vector with a default length of 20. The buffer size can be customized with the `bs` argument if needed. The buffer is populated by continuously shifting left and appending new characters as _valid_ `keydown` events are caught from a `WH_KEYBOARD_LL` hook, which is set when the application starts and unhooked when it exits.

By default, the buffer is flushed on `<RCtrl>` (the submit key), which acts as winDozer's flavor of `<Enter>`. The submit key can be changed via the `vks` command-line argument and is displayed in the help dialog. Buffer flushing behavior can be modified with the `dbf` flag (see [Command Line Arguments](#commandline)).

- **Sliding buffer:** Each keystroke shifts the buffer left and appends the new char on the right.
- **Interpreter flow:** On `<Submit>` the interpreter tokenizes the entire buffer, builds a pattern string, finds the **rightmost matching command** (`rfind`), then extracts arguments from right-to-left so the most recent args win.
- **Example:** `AW1S5AW2S50` resolves to `ADJUST_WINDOW` with `winID=2`, `step=50` (the rightmost command and args are chosen).
- **Interpreter strategy:** A lightweight tokenizer + pattern matcher (deterministic, rightmost-wins dispatcher); not a grammar/AST- just enough to replace our legacy regex parsing with predictable precedence.

### Command Structure

winDozer commands generally follow a `[ACTION][TARGET][OPTIONS...]` pattern:

- **ACTION:** The operation to perform (e.g., `M` = Move, `A` = Adjust, `P` = Pin, `C` = Center/Copy)
- **TARGET:** What to operate on (e.g., `T` = This [focused window], `W{ID}` = Window by ID, `R{ID}` = Rect by ID)
- **OPTIONS:** Optional modifiers (e.g., `S{step}` = step size)

**Full-word commands:** System-level meta-commands use full words (e.g., `HELP`, `FLUSH`, `EXIT`) to distinguish them from window manipulation commands. These are typically operations on winDozer itself rather than on windows.

---

### Known bugs

- Windows that winDozer cannot doze:

  - MSI Afterburner & children
  - Task Manager
  - Just about anything that installs a prior hook and doesn't go on to return a call to `CallNextHookEx()`.

- Tile-based applications (Microsoft Store, Calculator, etc) can be unpredictable and rarely fit well in the footprint of a non-tiled window.

---

### Future Features / Wishlist

**Virtual Desktop Support**

Currently, winDozer operates within the context of a single virtual desktop. In the future, we'd like to explore adding support for Windows virtual desktops, allowing commands to work across multiple desktops.

This would require leveraging undocumented Windows COM interfaces:

- `IVirtualDesktopManager` (documented, but limited - only works for windows owned by the calling process)
- `IVirtualDesktopManagerInternal` (undocumented - allows moving windows from other processes)
- `IApplicationViewCollection` (undocumented - required for getting views for windows)

Useful resources:

- [Stack Overflow discussion on IVirtualDesktopManager limitations](https://stackoverflow.com/questions/32659505/windows-10-ivirtualdesktopmanagermovewindowtodesktop)
- [VirtualDesktopAccessor - C++ implementation and DLL](https://github.com/Ciantic/VirtualDesktopAccessor/)

Potential features could include:

- Moving windows between virtual desktops
- Snapshot/restore layouts across multiple desktops
- Commands to switch virtual desktops
- Commands to move windows to specific virtual desktops
