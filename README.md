# DAQ Telemetry

Sistema de adquisición de datos (DAQ) mínimo para microcontrolador STM32: muestrea una señal analógica a una frecuencia fija con un timer y envía cada lectura por UART en formato legible.

```
ADC: 2048 | Voltage: 1.65 V
ADC: 2051 | Voltage: 1.65 V
ADC: 3120 | Voltage: 2.51 V
```

## Cómo funciona

1. **TIM** genera una interrupción a **10 Hz**; la ISR solo incrementa un contador (`sample_count`).
2. El **bucle principal** compara `sample_count` con `processed_count`. Por cada tick pendiente:
   - Lanza una conversión del **ADC1 canal 0** (12 bits, por polling) con `ADC_Read()`.
   - Convierte el valor crudo a voltaje: `V = raw / 4095 * 3.3`.
   - Envía la línea de texto por **USART2** (115200 8N1).

Mantener la ISR mínima y hacer el trabajo en el bucle principal evita bloquear interrupciones durante la conversión y la transmisión. Si la transmisión se atrasa, los ticks pendientes se procesan después en vez de perderse.

## Hardware

| Señal      | Pin     | Notas                                  |
|------------|---------|----------------------------------------|
| ADC1_IN0   | PA0     | Entrada analógica 0 – 3.3 V            |
| USART2_TX  | PA2     | Telemetría (VCP del ST-LINK en Nucleo) |
| USART2_RX  | PA3     |                                        |
| SWDIO/SWCLK| PA13/14 | Depuración                             |

Hay **dos variantes** del firmware en este repositorio:

| Variante                | Carpeta   | MCU            | Uso                              |
|-------------------------|-----------|----------------|----------------------------------|
| Hardware real           | raíz      | STM32F401RET6  | Se carga en la placa física      |
| Simulación en **Wokwi** | `wokwi/`  | STM32C031C6    | Se prueba sin hardware           |

## Variante hardware (STM32F401RE)

- Proyecto de **STM32CubeIDE** (configuración en `DAQ_Telemetry_New.ioc`).
- Reloj: HSI 16 MHz sin PLL. TIM2: prescaler 1599, periodo 999 → 10 Hz.
- Para compilar y cargar: abrir la carpeta como proyecto existente en STM32CubeIDE, compilar (`Debug`) y ejecutar con el ST-LINK. Para ver la salida, abrir el puerto serie del ST-LINK a 115200 baud.

## Simulación en Wokwi

Wokwi no simula la familia STM32F4, así que `wokwi/` contiene un port del mismo firmware a la **Nucleo-C031C6** (`board-st-nucleo-c031c6`), que sí soporta ADC, USART y timers.

### Diferencias respecto a la versión F401

| Aspecto            | F401 (hardware)         | C031C6 (Wokwi)                            |
|--------------------|-------------------------|-------------------------------------------|
| HAL                | STM32F4xx               | STM32C0xx                                 |
| Reloj              | HSI 16 MHz              | HSI 48 MHz                                |
| Timer de muestreo  | TIM2                    | TIM3 (el C031 no tiene TIM2); sigue a 10 Hz|
| USART2 AF          | AF7                     | AF1                                       |
| Voltaje en el texto| `float` + `%.2f`        | Aritmética entera (salida idéntica); el C031C6 solo tiene 32 KB de flash |
| Señal en PA0       | Fuente externa          | Potenciómetro virtual                     |

La lógica de la aplicación (timer → contador → lectura ADC → UART) es la misma.

### Requisitos

- `arm-none-eabi-gcc` y `make`. Si usas STM32CubeIDE ya tienes el compilador en `plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.*/tools/bin`.
- Una de estas formas de ejecutar Wokwi:
  - Extensión **Wokwi for VS Code** (requiere licencia/inicio de sesión gratuito de Wokwi), o
  - **wokwi-cli** con la variable `WOKWI_CLI_TOKEN` (token en <https://wokwi.com/dashboard/ci>).

### Compilar

```bash
cd wokwi
make
# si el compilador no está en el PATH:
make GCC_PATH="C:/ST/STM32CubeIDE_1.19.0/STM32CubeIDE/plugins/<gnu-tools-for-stm32...>/tools/bin"
```

Genera `wokwi/build/DAQ_Telemetry_Wokwi.elf` y `.hex`, que son los que referencia `wokwi/wokwi.toml`.

### Ejecutar

**VS Code:** abre la carpeta `wokwi/`, ejecuta `make` y usa *Wokwi: Start Simulator* (F1). Mueve el potenciómetro y observa el monitor serie.

**Línea de comandos:**

```bash
cd wokwi
export WOKWI_CLI_TOKEN=<tu_token>
wokwi-cli . --timeout 5000 --expect-text "Voltage"
```

### Circuito (`wokwi/diagram.json`)

| Potenciómetro | Nucleo-C031C6 |
|---------------|---------------|
| SIG           | PA0           |
| VCC           | 3V3           |
| GND           | GND           |

El monitor serie de Wokwi está conectado a PA2 (TX) / PA3 (RX) de USART2.

## Estructura

```
.
├── Core/                  # Aplicación F401 (main.c, ISR, MSP)
├── Drivers/               # HAL STM32F4xx + CMSIS
├── DAQ_Telemetry_New.ioc  # Configuración de STM32CubeMX
├── STM32F401RETX_*.ld     # Linker scripts F401
└── wokwi/                 # Port para simulación en Wokwi
    ├── Core/              # main.c, ISR, MSP del C031C6
    ├── Drivers/           # HAL STM32C0xx + CMSIS device C0
    ├── Makefile           # Build con arm-none-eabi-gcc
    ├── wokwi.toml         # Configuración del simulador
    └── diagram.json       # Circuito (Nucleo-C031C6 + potenciómetro)
```

Los headers CMSIS core (`core_cm0plus.h`, etc.) se reutilizan de `Drivers/CMSIS/Include` en la raíz.

## Licencia

Los drivers HAL y CMSIS pertenecen a STMicroelectronics / Arm y conservan sus licencias (ver los `LICENSE` en `Drivers/`).
