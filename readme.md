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

A Rect ID is any integer in range (0,9999). Writing window geometry to a previously assigned ID will overwrite the previous geometry for that ID.

Step 4: For this example, let's manually move the window somewhere else on the desktop so we can put it back with winDozer

Step 5: We move the focused window to the rect/geometry described by the Rect ID we set earlier by typing directly at the window: `mtr1` followed by `<RCtrl>`


**Syntax**

Keystrokes considered *valid*:
- A-Z
- Numrow 0-9
- Numpad 0-9
- Fn 1-9

In the event that the buffer is evaluated containing multiple valid "words", winDozer will choose one in the following precedence:
###### (buffer conflicts should be rare but are certainly possible.)

1. Flush
2. Move Window 
3. Move This
4. Set Rect ID
5. Set Window ID
6. Get Rects
7. Get Windows


**Syntax detail**

- Flush

Manually flush the internal buffer

| F | L | U | S | H |
|---|---|---|---|---|

---

- Move Window

Move Window by it's Window ID to the rect described by a Rect ID

| M | W | {Win ID} | {Rect ID} |
|---|---|----------|-----------|

---

- Move This

Move This [window] to Rect

| M | T | R | {Rect ID} |
|---|---|---|-----------|

---

- Set Rect ID

Set Rect ID to geometry of focused window

| S | R | {Rect ID} |
|---|---|-----------|

---

- Set Window ID

Set the Window ID of the focused window.

| S | W | {Win ID} |
|---|---|----------|

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

---

- Get Windows

Print Window IDs & an associated title, if one can be gleaned from the window handle. 

| G | W |
|---|---|

---

- Help

| H | E | L | P |
|---|---|---|---|

---

**Command line arguments**

`dbf` : Disable buffer flush

If this flag is passed, `<RCtrl>` will only flush the buffer if the buffer contains "FLUSH". This is useful to repeat a previous action with fewer keystrokes, provided the buffer hasn't been polluted in the meantime.

`verbose` : Verbose console output

This flag satisfies a few conditionals that print some extra feedback to stdout, primarily regarding syntax evaluation.


`debug` <span style="color:#ccc">
 : Debug mode 
<br>
</span>

This flag is intended for development, and in most cases will flood stdout as the buffer is shifted with valid input. 

---

**The buffer**

Currently, winDozer's internal syntax buffer is implemented as a character array of length 7. This size is partially arbitrary, and may grow in size or undergo different implementations as the application is developed. The buffer is populated by continuously shifting in the latest *valid* `keydown` caught from a `WH_KEYBOARD_LL` hook, which is set and unhooked each time the application runs. 

By default, the buffer is flushed on `<RCtrl>`, which acts as winDozer's flavor of `<Enter>`.

---

**Known bugs**:
- MSI Afterburner & children (which installs a few hooks of its own) ignores the `WH_KEYBOARD_LL` hook.

**Todo**:
- Class(es) / Structure(s) / OOP
- Qt GUI
