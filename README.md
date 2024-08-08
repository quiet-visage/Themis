

<h1 align="center">
  <br>
  <a href="icon"><img src="" alt="icon" width="200"></a>
  <br>
  Themis
  <br>
</h1>

<h4 align="center">A flexible IDE built with C++ and Raylib</h4>

<p align="center">
  <a href="#features">Features</a> •
  <a href="#compiling">Compiling</a> 
</p>

<TODO>

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

# Generate build files
$ mkdir -p build; cmake ..

# Compile
$ make -j8
```

> **Note**
> Currently the only supported OS is linux
