

<h1 align="center">

  ![drawing-1](https://github.com/user-attachments/assets/269cae95-b10c-4343-a973-be06e23c3679)

  Themis
  <br>
</h1>

<h4 align="center">A flexible IDE built with C and Raylib</h4>

<p align="center">
  <a href="#features">Features</a> •
  <a href="#compiling">Compiling</a> 
</p>

[output.webm](https://github.com/user-attachments/assets/d9791d5b-face-4bc2-9846-7b3f720813d0)


## Features

+ File picker
+ Syntax highlighting ( work in progress )
+ Multiple panes ( work in progress )
+ Compilation buffer with error-stepping ( work in progress )
+ LSP support ( TODO )
+ emacs, helix and vim keybind support ( work in progress )

## Compiling

```bash
# Clone this repository
$ git clone https://github.com/quiet-visage/themis

# Go into the repository
$ cd themis

# Create build directory
$ mkdir -p build

# Generate build files
$ cd build; cmake ..

# Compile
$ make -j8
```

> **Note**
> Currently the only supported OS is linux
