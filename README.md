# Plane OS
Control software for my fixed-wing drone running Linux project.
## Installing
Clone this repository
```sh
git clone --recursive https://github.com/hmstr7/plane-os-redux.git 
cd plane-os-redux/
```
## Build
Build the `main` executable using `cmake`:
```sh
mkdir build && cd build/
cmake .. && make
```

## Usage
Run the program from the build directory with `sudo` privileges (required for multiple libraries):
```sh
sudo ./main
```

To see available options use
```
./main --help
```

## Documentation
This project uses Doxygen.
You can generate the documentation using
```sh
doxygen Doxyfile
```

## Credits
See [CREDITS](CREDITS.md)