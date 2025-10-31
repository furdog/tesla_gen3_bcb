# Tesla GEN3 Single Phase Module Controller
![image](media/module.jpg)
- **[Fig. 1]:** *Top view of single phase module within Tesla charger (BCB)*

This page is available in [DOXYGEN](https://furdog.github.io/tesla_gen3_bcb/) format
### WIP
The project is actively work in progress and not ready for any usage yet.

### Useful Resources 💡
* [Project that was used as inspiration](https://github.com/damienmaguire/Tesla-Charger) - Damien's software for controlling Tesla charger
* [Openinverter page of the charger](https://openinverter.org/wiki/Tesla_Model_S/X_GEN3_Charger) - Gives broad description of the module  
* [Alternative Tesla charger software](https://github.com/jsphuebner/stm32-teslacharger) - Another piece of work, similar to Damien's software

## Project overview
The `tg3spmc` library provides a **hardware-agnostic** logic layer for controlling a **Tesla GEN3 Single Phase Module** (tg3spmc), which is an internal component of the Tesla GEN3 Battery Controller Board (BCB).

This library implements the module's state machine, handles the encoding and decoding of **CAN 2.0 frames**, and manages the module's control signals (power on, charge enable).

**Key Features:**
  * **Hardware agnostic:** Absolute ZERO hardware-dependend code (except the examples)
  * **Zero dependency:** No dependencies has been used except standart library
  * **MISRA-C compliant:** Integration providen by cppcheck (100% compliance achieved for core library)
  * **Single header:** Makes integration with other projects super simple and seamless
  * **Pure C:** Specifically ANSI(C89) standard, featuring linux kernel style formatting
  * **Automated tests:** Automatically tests main features (not TDD though, but aimed to be in the future)
  * **Documentation:** It is not really well made yet, but it's on the priority!
  * **Designed by rule of 10:** No recursion, dynamic memory allocations, callbacks, etc
  * **Deterministic:** Designed with constant time execution in mind
  * **GitHub actions:** Automated checks and doxygen generation

**Pitfalls:**
  * **Reverse engineering:** This is not an official firmware :(
  * **WIP:** Actively work in progress (not for production) 
  * **Not certified:** Use it at own risk

## Specific module used in the project
| Description | Code |
| :--- | :--- |
| **PCB Design code** | FAB 1034376-00-D REV-01 |
| **Specific part code (QR)** | p1045483-00-d:REV03:S0317AS0001706 |

![image](media/code.jpg)
- **[Fig. 2]:** *Module used in this project*

## About module
There are generally 3 or 2 similar modules within single BCB.
We've just extracted other module(s). I strongly advice not to do that if you plan to use
board outside of BCB. Because it's simply not designed to be outside... You'll have a big
trouble with cleaning thermal resin mess and designing a hull or a cooling system for it.
just don't do it that way. You have been warned.

---

### Components
The module contains a lot of various components that will be listed here upon research.
Currently only most significant components are present. There are really more
stuff to discover that may to drop some shades onto the architecture 
of the module, but unfortunately i am not the expert in the field of electronics,
so any external help is highly appreciated.

### Logic and Control
| Component | Part Number/ID | Location/Notes *(see **Fig. 1**)*|
| :--- | :--- | :--- |
| **MCU** | TMS320 F28035PAG0 | The big one in center of the module |
| **CPLD** | ALTERA 5M160ZE64A5N | Smaller piece, right from the MCU |

It's likely that MCU used for general complex tasks and communication while
CPLD is used for time-critical peripheral control.

I'd like to document and discover signals for every pin,
but thats gonna take some time and wish (which i do not have at the moment).

### Power Components
![image](media/fuse.jpg)
- **[Fig. 3]:** *There are two fuses at the AC input side (Fig. 1 rightmost side of the module)*

### Peripherals and Pinout
Our custom control hardware will use the 8 pin interface *(see **Fig. 4**)* to talk with the module.

The 8 pin interface is located at the leftmost side of the board *(see **Fig. 1**)*.

>matches *(**Fig. 4**)* [1 2 3 4 5 6 7 8]

| Pin | Value | Description |
| :---: | :--- | :--- |
| **1** | 12V-in | Supply pin leftmost on each module |
| **2** | GND | Ground |
| **3** | PWR-in | 3.3v signal (HIGH will power the charger) |
| **4** | CHG-in | 3.3v signal (HIGH will enable charge mode) |
| **5** | CANL | CAN2.0 bus |
| **6** | CANH | CAN2.0 bus |
| **7** | ? | Unknown |
| **8** | 5V-in | Looks like it's a reference voltage for CAN transciever (needs research) |

>Do not forget to add a **120Ω terminating resistor** between the **CANH/CANL** pins.

**⚠️ IMPORTANT NOTE:** you must disconnect original control board from the module.
Either desolder it, or cut. We do not need it, since we will use our own.
The original control board is connected to all 3 or 2 modules. You can use all of them.
![image](media/pinout.jpg)
- **[Fig. 4]:** *8 pin interface*

## Module control hardware
For implementing controller hardware i have used custom CAN filter board.
It's not documented here, but i'll give a short description.
Based on that, you'll be able to design your own, custom board:
- 12v->5v DCDC converter
- two TJA-1030 (or TJA-1050) CAN2.0 transcievers (dual CAN)
- Mount for ESP32-C6 board which has two native CAN2.0(TWAI) controllers
- ESP32-C6 board

I don't recomend to use specifically `Super Mini` board, because it has
serious PCB flaws.

For testing purposes i only use single built-in CAN2.0(TWAI) controller,
And two GPIO2 ports to provide PWR and CHG signals.

![image](media/can_filter.jpg)
- **[Fig. 5]:** *CAN2.0 filter*

More description and documentation about examples use will come soon.

## Licensing
This repository contains code that is the original creation of furdog and licensed under MIT License.
See the LICENSE file for details.
This applies to the current version of the software.
As the sole author, the project owner (furdog) reserves the right to change
the license for future versions.

While the project is informed by general engineering knowledge and publicly available ideas,
no copyrighted material or explicit source code from other authors has been incorporated.
The content is asserted as original.

## Contributions
To maintain the flexibility to change the project's license in the future,
contributions are **NOT** being **ACCEPTED** at this time.
Though any suggestions, tips and insights are welcome.

## Project Discussions
To keep things organized and easy to follow, I’d really appreciate it if all questions, ideas, and issue reports stayed right here on this GitHub repository.

Having everything in one place makes it much easier for everyone to find information, track progress, and avoid confusion from scattered discussions elsewhere.

So if you’d like to chat about the code, suggest improvements, or report a bug - please open an Issue right here on GitHub.
Thanks for helping keep things tidy and transparent!

## AI content
This repository's core, is written and verified manually by the author (furdog).
However, certain non-critical elements, such as:
- comments
- documentation
- build scripts
- github actions

may contain content generated by Generative AI Systems.
