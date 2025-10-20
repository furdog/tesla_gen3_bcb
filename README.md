# Tesla GEN3 Single Phase Module Controller

## Overview

The `tg3spmc` library provides a **hardware-agnostic** logic layer for controlling a **Tesla GEN3 Single Phase Module** (tg3spmc), which is an internal component of the Tesla GEN3 Battery Controller Board (BCB).

This library implements the module's state machine, handles the encoding and decoding of **CAN 2.0 frames**, and manages the module's control signals (power on, charge enable).

**Key Features:**

  * **Logical Control:** Implements the core state machine required to safely power on and enable charging.
  * **Hardware Abstraction:** The core logic is completely separate from the actual hardware interface (pins, CAN bus drivers).
  * **CAN Protocol Handling:** Manages the timing and content for sending (Tx) and receiving (Rx) the specific CAN messages required by the Tesla module.
  * **Multi-Module Support:** Designed to handle multiple modules (up to 3) within a single charger system.

## WIP
This project is actively work in progress and not ready for any usage yet.

Please check https://github.com/damienmaguire/Tesla-Charger .
This project is primarily based on Damien's codebase and knowledge.
