# SQV - Sepi's Quake Viewer

SQV (Sepi's Quake Viewer) is a lightweight program that renders Quake 1 `.MDL` files.

https://github.com/user-attachments/assets/4b13422c-c4dc-4cd8-9f55-34426d07f551

https://github.com/user-attachments/assets/d725cb44-4b51-4bee-9d44-4ac1fdb165ac


## Features
- Portable: Runs on all major desktop operating systems.
- Self-contained: Comes with all necessary dependencies as source files—no need to install external packages.
- Written in pure `C`, utilizing `sokol` for window creation and graphics.

## Status
SQV is still in early development, but I have some exciting ideas for its future evolution.

## Building
To build SQV, you need [`premake5`](https://premake.github.io/) installed on your system.

1. Generate the Makefiles:
   ```sh
   premake5 gmake2
   ```

2. Build the source code
   ```sh
   make
   ```


## Cool

- d3d mesh optimization functions: https://github.com/wine-mirror/wine/blob/master/dlls/d3dx9_36/mesh.c


## Notes:

i experimented with mold just for fun to see if i can squeeze more time on build time, here is the result:

# using gnu linker

| full clean | full repeat | target clean | target repeat |
|------------|-------------|--------------|---------------|
| 14.034     | 0.078       | 0.630        | 0.028         |
|            |             |

# using mold linker

| full clean | full repeat | target clean | target repeat |
|------------|-------------|--------------|---------------|
| 13.695     | 0.069       | 0.430        | 0.017         |

there is a slight gain in the time but not a huge one, this does not mean that mold is not good enogh, in
fact the way i structured the code base and build script makes the build time very short, so the benefits
gained from mold is negligible, however if the code grows and the build system becomes more complicated
i am pretty sure there are more substantial gains with mold, for now i think i will not use mold as it adds
more complexity to the project setup


┏━━━━━━━━┓
┃ metal  ┃◀━━━━━━━━━━━━━━━━━━━━━━━━┓
┣━━━━━━━━┫                         ┃
┃direct3d┃◀━━━━━━━━━━━━━━━━━━━━━━┓ ┃
┣━━━━━━━━┫                       ┃ ┃
┃ webgpu ┃◀━━━━━━━━━━━━━━━━━━━━┳━┻━┻━━━━━━━┓
┣━━━━━━━━┫          ┏━━━━━━━━━━┫ lib_sokol ┣━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃ opengl ┃◀━━━━━━━━━┛   ┏━━━━━━┻━━━━━━━━━━━┛                         ┃
┣━━━━━━━━┫              ┃      ┏━━━━━━━━━━━┓                         ┃                            ┏━━━━━━━━━┓
┃ vulkan ┃◀━━━━━━━━━━━━━┛      ┃  lib_hmm  ┣━━━━━━━━━━━━━━━━━━━━━┓   ┃                ┏━━━━━━━━━━▶┃ windows ┃
┗━━━━━━━━┛                     ┗━━━━━━━━━━━┛                     ▼   ▼                ┃           ┗━━━━━━━━━┛
                               ┏━━━━━━━━━━━┓                  ╔═════════╗       ┏━━━━━┻━━━━┓      ┏━━━━━━━━━┓
                               ┃lib_nuklear┃━━━━━━━━━━━━━━━━━▶║ app_pak ║◀━━━━━━┫ lib_sepi ┃      ┃  macos  ┃
                               ┗━━━━━━━━━━━┛                  ╚═════════╝       ┗━━━━━┳━━━━┛      ┗━━━━━━━━━┛
                               ┏━━━━━━━━━━━┓                     ▲   ▲                ┃           ┏━━━━━━━━━┓
                               ┃  lib_log  ┃━━━━━━━━━━━━━━━━━━━━━┛   ┃                ┗━━━━━━━━━━▶┃  linux  ┃
                               ┗━━━━━━━━━━━┛                         ┃                            ┗━━━━━━━━━┛
                               ┏━━━━━━━━━━━┓                         ┃
                               ┃  lib_stb  ┃━━━━━━━━━━━━━━━━━━━━━━━━━┛
                               ┗━━━━━━━━━━━┛

# note for myself
- i hate that linking to graphics apis bloats the binary when it is loaded in memory, i may end up
  using a software renderer to fight this, and it may even help with the portability, right now
  the following libs are interesting and be used to achieve this goal:
  - osmesa: fast 3d software renderer (opengl)
  - silk: fast 2d software rendere
  - rgfw: alternative to sokol
  - nuklear: immediate mode UI (what i am using now)
  - clay: very fast retained mode UI (a lot of people praised it)
