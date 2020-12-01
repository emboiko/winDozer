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

Move this to rect

| M | T | R | {Rect ID} |
|---|---|---|-----------|

---

Set rect ID to geometry of focused window

| S | R | {Rect ID} |
|---|---|-----------|

---


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


###### Not implemented yet:

Move Window

| M | W | {Win ID} | {Rect ID} |
|---|---|----------|-----------|

Focus Window

| F | W | {Win ID} |
|---|---|----------|

Set Window

| S | W | {Win ID} |
|---|---|----------|

Help

| H | E | L | P |
|---|---|---|---|

---

**Known bugs**:
- MSI Afterburner & children (which installs a few hooks of its own) ignores the `WH_KEYBOARD_LL` hook.

**Todo**:
- Class(es) / Structure(s) / OOP
- Qt GUI
- Probably a `configuration.h`
- Enforce single instance
- Main Flags
    - Verbose/log/debug
    - Buffer Flush
- Buffer Flags
    - (W)indow
