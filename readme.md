# winDozer
###### A pocket-sized, vim-esque language for the Windows DWM that's very much a work in progress

<div align="center">
<img src="https://i.imgur.com/Fzu3Ym2.png?1">
</div>

**Install & Run**

```
git clone ... winDozer
cd winDozer
winDozer.exe
```

**Getting Started**

1. `winDozer.exe`
2. Focus a window and place it *somewhere* (use winDozer terminal to learn the flow)
3. Set a Rect ID: **S**et **R**ect (ID) `sr1`, then `<RCtrl>`
4. Either move the focused window or focus a different one
5. Move the window to a Rect ID: **M**ove **T**his *to* **R**ect (ID) `mtr1`, then `<RCtrl>`

**Breakdown**

Step 1: We launch an instance of winDozer

Step 2: A given window is placed somewhere on the desktop. As an example, we can use the winDozer terminal window. 

Step 3: We assign the geometry of the currently focused window to a **Rect ID** by typing directly at the window: `sr{rectID}` followed by `<RCtrl>`

A Rect ID is any integer in range (0-9999), *although this is not strictly enforced.* Writing window geometry to a previously assigned ID will overwrite the previous geometry for that ID.

Step 4: For this example, let's manually move the window somewhere else on the desktop so we can put it back with winDozer

Step 5: We move the focused window to the rect/geometry described by the Rect ID we set earlier by typing directly at the window: `mtr1` followed by `<RCtrl>`


**Syntax**

<a name="validkeys"></a>

Keystrokes considered *valid*:
- A-Z
- Numrow 0-9
- Numpad 0-9
- Fn 1-9 (Same as numrow & numpad)
- `<RCtrl>` (See [Command Line Arguments](#commandline))
    - Arrow keys Left, Right, Up, Down (During an [adjustment](#adjustment))

---

In the event that the buffer is evaluated containing multiple valid "words", winDozer will choose one in the following precedence:
###### (This list can be safely ignored unless you're having problems. Buffer conflicts should be rare but are certainly possible.)

1. Flush
2. Move Window & Move This
3. Set Rect ID
4. Set Window ID
5. Erase Rect ID
6. Erase Window ID
7. Focus Window
8. Adjust Window & Adjust This
9. Get Rects
10. Get Windows
11. Help

**Syntax detail**

- Move Window

Move Window by it's Window ID to the rect described by a Rect ID

| M | W | {Win ID} | {Rect ID} |
|---|---|----------|-----------|

`MW5R10`

---

- Move This [window]

Move This [window] to Rect {Rect ID}

| M | T | R | {Rect ID} |
|---|---|---|-----------|

`MTR1`

---

- Set Rect ID

Set Rect ID to geometry of focused window. Rect IDs are persisted to the disk between instances of winDozer.

| S | R | {Rect ID} |
|---|---|-----------|

`SR99`

---

- Set Window ID

Set the Window ID of the focused window. Window IDs are not persisted to the disk and currently must be set for an individual instance of winDozer. 

| S | W | {Win ID} |
|---|---|----------|

`SW5`

---

- Erase Rect ID

Unregister a Rect ID. (Deleting `$APPDATA/winDozer/settings.txt` between instances of winDozer will unregister *every* Rect ID)

| D | R | {Rect ID} |
|---|---|-----------|

`ER99`

---

- Erase Window ID

Unregister a Window ID. (Restarting winDozer will unregister *every* Window ID) Closing a window will automatically unregister it. 

| D | W | {Win ID} |
|---|---|----------|

`EW5`

---

- Focus Window

Focus window by its assigned {Win ID} 

| F | W | {win ID} |
|---|---| ---------|

`FW5`

<a name="adjustment"></a>

---

- Adjust This [window]

*For those of us who are particularly obsessive-compulsive:*

Adjusts the focused window by shifting it 1px at a time. If this syntax is evaluated from the buffer following `<RCtrl>`, input is limited to only the arrow keys until a subsequent `<RCtrl>` is input. 

| A | T |
|---|---|

`AT`

`<RCtrl>`

*then*

```
<Arrow Key Up/Down/Left/Right>
<Arrow Key Up/Down/Left/Right>
<Arrow Key Up/Down/Left/Right>
```

*then*

`<RCtrl>`

---

- Adjust Window

*See `Adjust This` (Above)*

Adjusts the window by shifting it 1px at a time. If this syntax is evaluated from the buffer following `<RCtrl>`, input is limited to only the arrow keys until a subsequent `<RCtrl>` is input. This syntax will lift the window if minimized, but will not necessarily focus the window.

| A | W | {win ID} |
|---|---|----------|

`AW25`

---


- Get Rects

Print Rect IDs & their geometry to stdout, ordered by:

(

Left coordinate,

Top coordinate,

Bottom coordinate,

Right coordinate

)

| G | R |
|---|---|

`GR`

---

- Get Windows

Print Window IDs, window class, and an associated title, if one can be gleaned from the window handle. 

| G | W |
|---|---|

`GW`

---

- Help

Print a simple help dialog to stdout

| H | E | L | P |
|---|---|---|---|

`HELP`

---

- Flush

Manually flush the internal buffer, for use with `dbf`

| F | L | U | S | H |
|---|---|---|---|---|

`FLUSH`

---

<a name="commandline"></a>

**Command line arguments**

`dbf` : Disable buffer flush

If this flag is passed, `<RCtrl>` will only flush the buffer if the buffer contains "FLUSH". This is useful to repeat a previous action with fewer keystrokes, provided the buffer hasn't been polluted in the meantime.

`verbose` : Verbose console output

This flag satisfies a few conditionals that print some extra feedback to stdout, primarily regarding syntax evaluation.

`vks` : Virtual Key Submit

Set a non-default submit key. For example, `windozer vks162` will set `<LCtrl>` as winDozer's *submit* key.

This flag should be called with an integer representing a [virtual key](https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes). Overwrite [valid keystrokes](#validkeys) with this at your own risk. Setting integers that do not represent existing win32 virtual-key codes will effectively disable the *submit* function, and the buffer will never be evaluated. 

`debug` <span style="color:#ccc">
 : Debug mode 
<br>
</span>

This flag is intended for development, and in most cases will flood stdout as the buffer is shifted with valid input. 

---

**The buffer**

Currently, winDozer's internal syntax buffer is implemented as a character array of length 7. This size is partially arbitrary, and may grow in size or undergo different implementations as the application is developed. The buffer is populated by continuously shifting in the latest *valid* `keydown` caught from a `WH_KEYBOARD_LL` hook, which is set and unhooked each time the application runs. 

By default, the buffer is flushed on `<RCtrl>`, which acts as winDozer's flavor of `<Enter>`. This is configurable with the `vks` and `dbf` flags.

---

**Invalid Windows**

winDozer will discard attempts to Move or set a Window ID on the following windows:

- The start menu
- The system tray
- The desktop itself

---

**Deliberately unimplemented behavior**

Windows GUI has plenty of hotkeys and macros for power users such as `alt+tab`, `win+tab`, `win+shift+M`, and `win+<arrow>`. winDozer doesn't try to wrap behavior that is already made convenient in Windows. There is no syntax, for example, to minimize the focused window because there is already an *equally fast, equally convenient, prexisting way* to do the exact same thing. However, sometimes you really want that window adjusted by *one pixel* and nothing more. If this is a repetitive task, using the mouse can be exhausting.

---

**Known bugs**:
- Programs that winDozer cannot doze:
    - MSI Afterburner & children
    - Task Manager
    - Just about anything that installs a prior hook and doesn't go on to return a call to `CallNextHookEx()`.


**Todo**:
- Parse syntax without [doing so much of this](https://www.youtube.com/watch?v=poz6W0znOfk) and implement [something more robust](https://en.wikipedia.org/wiki/Interpreter_pattern). 
- Query `ImmersiveShell` for instance of `IVirtualDesktopManagerInternal` and implement those undocumented methods in winDozer. These features are subject to break between Windows builds and will probably warrant an `experimental` command line flag. It would be nice to be able to (C)hange [view to] (D)esktop {desktopID}, among other possibilities.
- Set a non-default `BUFFSIZE` at runtime.
