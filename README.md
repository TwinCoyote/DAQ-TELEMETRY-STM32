# DAQ Telemetry

A minimal data acquisition (DAQ) system for an STM32 microcontroller: it samples an analog signal at a fixed rate using a timer and sends each reading over UART in a human-readable format.

```
ADC: 2048 | Voltage: 1.65 V
ADC: 2051 | Voltage: 1.65 V
ADC: 3120 | Voltage: 2.51 V
```

## How it works

1. **TIM** generates an interrupt at **10 Hz**; the ISR only increments a counter (`sample_count`).
2. The **main loop** compares `sample_count` with `processed_count`. For each pending tick it:
   - Starts a conversion on **ADC1 channel 0** (12-bit, polling) with `ADC_Read()`.
   - Converts the raw value to a voltage: `V = raw / 4095 * 3.3`.
   - Sends the text line over **USART2** (115200 8N1).

Keeping the ISR minimal and doing the work in the main loop avoids blocking interrupts during conversion and transmission. If transmission falls behind, pending ticks are processed later instead of being lost.

## Hardware

| Signal      | Pin     | Notes                                   |
|-------------|---------|-----------------------------------------|
| ADC1_IN0    | PA0     | Analog input 0 – 3.3 V                  |
| USART2_TX   | PA2     | Telemetry (ST-LINK VCP on Nucleo)       |
| USART2_RX   | PA3     |                                         |
| SWDIO/SWCLK | PA13/14 | Debugging                               |

There are **two firmware variants** in this repository:

| Variant                 | Folder    | MCU            | Use                              |
|-------------------------|-----------|----------------|----------------------------------|
| Real hardware           | root      | STM32F401RET6  | Flashed to the physical board    |
| **Wokwi** simulation    | `wokwi/`  | STM32C031C6    | Tested without hardware          |

## Hardware variant (STM32F401RE)

- **STM32CubeIDE** project (configuration in `DAQ_Telemetry_New.ioc`).
- Clock: HSI 16 MHz, no PLL. TIM2: prescaler 1599, period 999 → 10 Hz.
- To build and flash: open the folder as an existing project in STM32CubeIDE, build (`Debug`) and run with the ST-LINK. To see the output, open the ST-LINK serial port at 115200 baud.

## Wokwi simulation

Wokwi does not simulate the STM32F4 family, so `wokwi/` contains a port of the same firmware to the **Nucleo-C031C6** (`board-st-nucleo-c031c6`), which does support ADC, USART and timers.

### Differences from the F401 version

| Aspect               | F401 (hardware)         | C031C6 (Wokwi)                            |
|----------------------|-------------------------|-------------------------------------------|
| HAL                  | STM32F4xx               | STM32C0xx                                 |
| Clock                | HSI 16 MHz              | HSI 48 MHz                                |
| Sampling timer       | TIM2                    | TIM3 (the C031 has no TIM2); still 10 Hz  |
| USART2 AF            | AF7                     | AF1                                       |
| Voltage in the text  | `float` + `%.2f`        | Integer arithmetic (identical output); the C031C6 has only 32 KB of flash |
| Signal on PA0        | External source         | Virtual potentiometer                     |

The application logic (timer → counter → ADC read → UART) is the same.

### Requirements

- `arm-none-eabi-gcc` and `make`. If you use STM32CubeIDE you already have the compiler in `plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.*/tools/bin`.
- One of these ways to run Wokwi:
  - The **Wokwi for VS Code** extension (requires a free Wokwi license/sign-in), or
  - **wokwi-cli** with the `WOKWI_CLI_TOKEN` variable (token at <https://wokwi.com/dashboard/ci>).

### Build

```bash
cd wokwi
make
# if the compiler is not in your PATH:
make GCC_PATH="C:/ST/STM32CubeIDE_1.19.0/STM32CubeIDE/plugins/<gnu-tools-for-stm32...>/tools/bin"
```

This produces `wokwi/build/DAQ_Telemetry_Wokwi.elf` and `.hex`, which are the files referenced by `wokwi/wokwi.toml`.

### Run

**VS Code:** open the `wokwi/` folder, run `make` and use *Wokwi: Start Simulator* (F1). Move the potentiometer and watch the serial monitor.

**Command line:**

```bash
cd wokwi
export WOKWI_CLI_TOKEN=<your_token>
wokwi-cli . --timeout 5000 --expect-text "Voltage"
```

### Circuit (`wokwi/diagram.json`)

| Potentiometer | Nucleo-C031C6 |
|---------------|---------------|
| SIG           | PA0           |
| VCC           | 3V3           |
| GND           | GND           |

The Wokwi serial monitor is connected to PA2 (TX) / PA3 (RX) of USART2.

## Structure

```
.
├── Core/                  # F401 application (main.c, ISR, MSP)
├── Drivers/               # STM32F4xx HAL + CMSIS
├── DAQ_Telemetry_New.ioc  # STM32CubeMX configuration
├── STM32F401RETX_*.ld     # F401 linker scripts
└── wokwi/                 # Port for Wokwi simulation
    ├── Core/              # C031C6 main.c, ISR, MSP
    ├── Drivers/           # STM32C0xx HAL + C0 CMSIS device
    ├── Makefile           # Build with arm-none-eabi-gcc
    ├── wokwi.toml         # Simulator configuration
    └── diagram.json       # Circuit (Nucleo-C031C6 + potentiometer)
```

The CMSIS core headers (`core_cm0plus.h`, etc.) are reused from `Drivers/CMSIS/Include` in the root.

## License

The HAL and CMSIS drivers belong to STMicroelectronics / Arm and keep their own licenses (see the `LICENSE` files in `Drivers/`).
