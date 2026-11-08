# Plane OS
Control software for my fixed-wing drone running Linux project.
## Prerequisites
While most of the libraries used are included as git submodules, 
some of them have to be installed separately:
- `libboost` 1.92.0 or higher
- `ftxui` (for future UI, currently WIP)

## Installing
Clone this repository
```sh
git clone --recursive https://github.com/hmstr7/plane-os-redux.git 
cd plane-os-redux/
```
Build the `main` executable using `cmake`:
```sh
mkdir build && cd build/
cmake .. && make
```

## Usage
Currently, the software can only be ran on a very specific hardware (OrangePi or any SBC running Linux, PCA9685 connected using I2C, Flysky ia10b receiver's ibus/sbus output connected through UART, MPU6500 connected through I2C).

You have to map the device names, input and output channels in [hardware.json](config/hardware.json).

It's recommended to launch the program in a separate session, so no SIGHUP is sent when internet connection is lost with the ground. To do this you can use `tmux`
```sh
tmux new -s <your session name>
```
Run the program from the build directory with `sudo` privileges (required for multiple libraries):
```sh
sudo ./main
```
To see available options use
```
./main --help
```
After launching the program, you can detach the `tmux` session using `Ctrl+B` and then `D`

## Documentation
This project uses Doxygen.
You can generate the documentation using
```sh
doxygen Doxyfile
```

## Credits
See [CREDITS](CREDITS.md)
