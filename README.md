# SQV - Sepi's Quake Viewer

SQV (Sepi's Quake Viewer) is a lightweight program that renders Quake 1 `.MDL` files.  

[showoff](showoff.mov)

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
