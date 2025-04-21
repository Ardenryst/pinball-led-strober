# Firmware

Firmware is written in C and has no external dependecies.

**Info** This software is very minimal and likely to change in the future. But hey, 

## Building

While this is NOT an Arduino project, the easiest way to get all the necessary development tools, is in fact to use what comes bundled with Arduino. If you already have Arduino installed, follow these steps, otherwise I'd recommend using `arduino-cli` instead.

### Setup using Arduino

So you're already using Arduino? Then you can use that. But first you may have to install the package `ATTinyCore` via the board manager. This may require to add the line `http://drazzy.com/package_drazzy.com_index.json` to "Additional Boards Manager URLs". **Nope, not needed!**

### Setup using arduino-cli

 > **This is nonsense! Headers and advdude config is part of arduino distribution already! But I think this still need to be installed as cli comes without any cores, correct?**

This option saves a lot of the bloat that comes with a standard Arduino installation.

1. Check the documentation for the prefered way to install `arduino-cli` on your platform. Then enter the terminal.

2. Add packe source to index: `arduino-cli config add board_manager.additional_urls http://drazzy.com/package_drazzy.com_index.json`

3. Install core: `arduino-cli core install ATTinyCore:avr`

4. `arduino-cli config dump` to check the directories you have to look for the toolchain nex

### Configure build system

The firmware is build via `make`. To tell it where your compiler and other tools are, an `Makefile.in` file is required. You can use copy the existing `Makefile.in.template` and adjust it to your needs.


### Compile

Run `make` from the firmware root directory to build the software.


### Set fuses

To prime chip the fuse bits have to be flashed first. This configures hardware functions like clock speed etc. of the MCU. Use `make burn_fuses` for this. This has only to be done once since uploading firmware file does not touch the fuse settings.


### Upload firmware

Run `make program` to flash the firmware. This will upload the build to the target. The execution should start imeadiately once upload is completed.

`make && make program` will rebuild the project and, if successfull, then upload it. This is convinient during development.


## Changing animation

The animation can be customized using constants in the `main.c` file. The following settings are particularly relevant:

* `ANIMATION_DURATION`: Total duration of the animation in milliseconds
* `INITIAL_FLASH_DURATION`: Duration of the initial light flash in milliseconds
* `INITIAL_FLASH_MULTIPLIER`: Brightness multiplier for the initial flash (relative to MAX_BRIGHTNESS)
* `MIN_ANIMATION_RUN_TIME`: Minimum runtime of the animation before it can be restarted
* `BASE_BRIGHTNESS`: Base brightness of the LED
* `BRIGHTNESS_VARIANCE`: Range of brightness variation
* `MAX_BRIGHTNESS`: Maximum brightness during power surges
* `FLICKER_PROBABILITY`: Probability of power surge effects (1-100)
* `BLACKOUT_PROBABILITY`: Probability of complete blackout

After any changes, the code must be recompiled and uploaded.