# Developer's Guide to the IS31FL37xx LED Driver Family

This guide provides a comprehensive software development reference for the Lumissil IS31FL3733, IS31FL3737, and IS31FL3737B I2C matrix LED drivers. It focuses on the key differences, control flow, and best practices for writing a robust and adaptable driver.

## 1. Key Differences & Chip Identification

While sharing a core architecture, these chips have critical differences that a driver must handle.

#### 1.1. At a Glance Comparison

| Feature | IS31FL3733 | IS31FL3737 | IS31FL3737B |
| :--- | :--- | :--- | :--- |
| **Matrix Size** | **12×16** | 12×12 | 12×12 |
| **Max LEDs** | 192 (64 RGB) | 144 (48 RGB) | 144 (48 RGB) |
| **Package(s)** | QFN-48, eTQFP-48 | QFN-40 | QFN-40 |
| **I2C Address Pins** | **ADDR1, ADDR2** (16 addr) | **ADDR** (4 addr) | **ADDR** (4 addr) |
| **PWM Frequency** | Fixed (~7.8 kHz) | Fixed (~7.8 kHz) | **Selectable (1.05 - 26.7 kHz)** |

#### 1.2. Programmatic Chip Identification

> **CRITICAL:** None of these devices contain a "Device ID" register. It is **not possible** to programmatically query the chip to determine its model. The driver **must** be configured at compile-time or initialization with the specific model it is controlling.

A recommended driver design pattern:

```c
typedef enum {
    MODEL_IS31FL3733,
    MODEL_IS31FL3737,
    MODEL_IS31FL3737B
} is31fl37xx_model_t;

// Driver instance must store the model
struct is31fl37xx_dev {
    is31fl37xx_model_t model;
    // ... other device properties
};
```

## 2. I2C Communication & Control Flow

### 2.1. Slave Addressing

The 7-bit I2C slave address consists of a fixed 3-bit prefix (`0b101`) and a 4-bit suffix set by hardware pin connections.

* **IS31FL3733 (Two Address Pins):**
  * Address: `0b101` | `A4:A3 (ADDR2)` | `A2:A1 (ADDR1)`
  * Provides 16 unique addresses.

    | ADDRx Pin Tied To | Bit Value |
    | :--- | :--- |
    | GND | `00` |
    | SCL | `01` |
    | SDA | `10` |
    | VCC | `11` |

* **IS31FL3737 & IS31FL3737B (Single Address Pin):**
  * Address: `0b101` | `A4:A1 (ADDR)`
  * Provides 4 unique addresses.

    | ADDR Pin Tied To | Bit Value (A4:A1) |
    | :--- | :--- |
    | GND | `0000` |
    | SCL | `0101` |
    | SDA | `1010` |
    | VCC | `1111` |

### 2.2. Core Control Flow: The Page System

All primary registers are organized into four memory pages. Accessing any paged register requires a three-step logical sequence: **Unlock → Select Page → Access Register**.

1. **Unlock:** Write `0xC5` to the **Command Register Write Lock (`0xFE`)**. This allows a *single* subsequent write to the Command Register.
2. **Select Page:** Write the desired page number (`0x00`-`0x03`) to the **Command Register (`0xFD`)**.
3. **Access Register:** Perform standard I2C read/write operations on registers within the now-active page.

This sequence is required for every page switch. Multiple accesses within the same page do not require repeating the sequence.

## 3. Register Map Deep Dive

### Page 3: Function Registers

This page contains global configuration and must be configured on initialization.

* **Configuration Register (`0x00`)**
| Bit(s) | Name | Function & Key Differences |
| :--- | :--- | :--- |
| `D7:D6` | **SYNC** | Multi-chip sync: `01`=Master, `10`=Slave, `00/11`=Off. |
| `D5:D3` | **PFS** | **IS31FL3737B ONLY:** PWM Frequency Select. Reserved on other models. `000`=8.4kHz (Default), `010`=26.7kHz. |
| `D2` | **OSD** | Write `1` to trigger one-shot Open/Short Detection. |
| `D1` | **B_EN** | `1` enables the Auto Breath Mode engine. |
| `D0` | **SSD** | `0`=Software Shutdown, `1`=Normal Operation. **Default is `0` (Shutdown)**. |

* **Global Current Control (GCC) Register (`0x01`)**: An 8-bit value (`0x00`-`0xFF`) setting the max current for all channels. Controls overall brightness.
* **Auto Breath (ABM) Timing Registers (`0x02` - `0x0D`)**: Defines timing for the three ABM profiles.
* **Time Update Register (`0x0E`)**: Writing `0x00` to this register latches new ABM timing values. **This is mandatory after updating ABM timing.**
* **Reset Register (`0x11`, Read-Only)**: Reading from this address resets all device registers to their power-on defaults.

### Page 1: PWM Registers

This page contains the 8-bit (`0x00`-`0xFF`) brightness value for each LED.

| Model | Address Range | Size |
| :--- | :--- | :--- |
| IS31FL3733 | `0x00` - `0xBF` | 192 bytes |
| IS31FL3737 / B | `0x00` - `0x8F` | 144 bytes |

---
> **WARNING: Critical Note on Address Mapping for 12x12 Models**
>
> The register map uses a fixed row stride of 16 bytes, regardless of the physical matrix size. The address for an LED at `(CSx, SWy)` is `(SWy-1) * 16 + (CSx-1)`.
>
> On the **IS31FL3737 and IS31FL3737B**, which only have 12 columns (CS1-CS12), the register addresses for columns 13-16 in each row (e.g., `0x0C` to `0x0F` for SW1) are physically non-existent.
>
> Attempting to write to these invalid addresses (e.g., via a 16-byte burst write intended for an IS31FL3733) will corrupt the chip's internal address pointer, causing subsequent data to be written to incorrect locations. This manifests as visual artifacts where controlling one LED unexpectedly affects another.
>
> **Your driver MUST limit all writes to valid column ranges (CS1-CS12) for the 12x12 models.**

---

### IS31FL3737 Hardware Quirk: Register Mapping Gaps

**NOTE:** The IS31FL3737 chip has a quirk in its register mapping that affects columns CS7-CS12 (1-based) or CS6-CS11 (0-based):

- **Columns CS1-CS6**: Map directly to register addresses 0-5 ✓
- **Columns CS7-CS12**: Map to register addresses 8-13 (NOT 6-11) ⚠️

This creates a 2-address gap in the register mapping. The IS31FL3737 skips register addresses 6-7 and 14-15 in each row.

#### Example Register Mapping for IS31FL3737:
```
Row SW1 (0-based row 0):
CS1 -> 0x00, CS2 -> 0x01, ..., CS6 -> 0x05
CS7 -> 0x08, CS8 -> 0x09, ..., CS12 -> 0x0D
(Addresses 0x06, 0x07, 0x0E, 0x0F are skipped)

Row SW2 (0-based row 1):  
CS1 -> 0x10, CS2 -> 0x11, ..., CS6 -> 0x15
CS7 -> 0x18, CS8 -> 0x19, ..., CS12 -> 0x1D
(Addresses 0x16, 0x17, 0x1E, 0x1F are skipped)
```

#### Impact on Driver Implementation:
- Without this remapping, LEDs would light up in unexpected locations
- Users would need to call `drawPixel(9, 0)` to light the LED at CS7, SW1
- **This quirk is unique to IS31FL3737** - the IS31FL3733 (and perhaps IS31FL3737B) do not have this issue

#### Driver Solution:
The IS31FL373x driver automatically handles this quirk in the `IS31FL3737` class by overriding the `coordToIndex()` method to apply the +2 offset for columns 6-11.

---

### Page 0: LED Control Registers

* **LED On/Off Registers**: Each bit enables (`1`) or disables (`0`) an individual LED. An LED's PWM value is only output if its corresponding bit is `1`.
  * **IS31FL3733**: `0x00` - `0x17` (24 bytes)
  * **IS31FL3737 / B**: `0x00` - `0x11` (18 bytes)
* **LED Open/Short Registers (`0x18`-`0x47`)**: Read-only fault status registers populated after an OSD cycle. Address ranges differ by model similar to On/Off registers.

### Page 2: Auto Breath Mode (ABM) Assignment

This page assigns an operating mode to each LED. The address range and mapping mirrors the PWM registers.

* `D1:D0` **ABMS**:
  * `00`: Normal PWM Mode (controlled by Page 1).
  * `01`: Auto Breath Mode 1.
  * `10`: Auto Breath Mode 2.
  * `11`: Auto Breath Mode 3.

## 4. Common Operational Procedures

### 4.1. Initialization Sequence

1. **Reset Chip:** Perform a read from the **Reset Register** (`0x11` in Page 3) to ensure a known default state.
2. **Enable Operation:** Select **Page 3** and write to the **Configuration Register (`0x00`)**, setting `SSD` (D0) to `1`. Configure `SYNC` and `PFS` (IS31FL3737B only) as needed.
3. **Set Global Brightness:** In **Page 3**, write a desired value to the **Global Current Control Register (`0x01`)**.
4. **Enable All LEDs:** Select **Page 0** and write `0xFF` to all **LED On/Off registers** to enable all pixels.
5. **Clear Display:** Select **Page 1** and write `0x00` to all **PWM registers** to ensure the display starts blank.

### 4.2. Updating the Display

1. Select **Page 1** (PWM).
2. To update a single pixel, write its 8-bit PWM value to the calculated register address.
3. To update a full frame efficiently, use I2C burst writes with auto-increment, sending the entire PWM frame buffer. **Ensure the correct number of bytes are sent per row (16 for IS31FL3733, 12 for others).**

### 4.3. Performing Open/Short Detection

1. Ensure LEDs under test are enabled (Page 0 On/Off) with a non-zero PWM value (Page 1).
2. Select **Page 3**. Set **GCC (`0x01`)** to a low value (e.g., `0x01`) for the test.
3. Read the **Configuration Register (`0x00`)**, set the `OSD` bit (D2) to `1`, and write it back.
4. Wait >3.3ms for the detection cycle to complete.
5. Select **Page 0**. Read the **LED Open** and **LED Short** registers to find faulty LEDs.
