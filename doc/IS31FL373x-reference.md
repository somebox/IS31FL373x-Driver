# **Programmer's Guide: IS31FL3733 & IS31FL3737B Matrix Drivers**

This guide provides a software-centric reference for controlling the IS31FL3733 (12x16) and IS31FL3737B (12x12) LED matrix drivers via the I2C protocol.

## **1.0 I2C Communication Protocol**

The primary interface for control is a 1MHz I2C-compatible bus. All operations, from initialization to setting individual LED brightness, are performed through register writes and reads.

**1.1. Slave Address**

The I2C slave address is 7 bits long. The final address is determined by the hardware connection of the ADDR pin(s).

* **IS31FL3733 (Two ADDR Pins):**
  * Fixed bits: `101` (A7:A5)
  * Address is `101` + `A4 A3` (ADDR2 pin) + `A2 A1` (ADDR1 pin).
  * This allows for up to 16 unique addresses on a single bus.
  * **Example:** ADDR1=GND, ADDR2=GND -> A4:A1 = `0000`. Slave Address = `0b1010000` = **`0x50`**.

* **IS31FL3737B (One ADDR Pin):**
  * Fixed bits: `101` (A7:A5)
  * Address is `101` + `A4 A3 A2 A1` (ADDR pin).
  * This allows for 4 unique addresses on a single bus.
  * **Example:** ADDR=GND -> A4:A1 = `0000`. Slave Address = `0b1010000` = **`0x50`**.

**1.2. Fundamental Control Registers**

Two special registers, located outside the main paged memory, govern access to all other registers.

* **Command Register Write Lock (`0xFE`):** This register protects the Command Register from accidental writes.
  * `0xC5`: Unlocks the Command Register for a **single** write operation.
  * After writing to the Command Register, this register automatically resets to `0x00` (locked).
* **Command Register (`0xFD`):** This register acts as a page selector. Writing a value to this register determines which of the four memory pages is currently accessible.
  * `0x00`: Selects Page 0 (LED Control).
  * `0x01`: Selects Page 1 (PWM).
  * `0x02`: Selects Page 2 (Auto Breath Mode).
  * `0x03`: Selects Page 3 (Function).

**A standard I2C write sequence to any paged register is:**
1. START
2. Slave Address + W
3. Write Register Address `0xFE`
4. Write Data `0xC5` (Unlock)
5. STOP
6. START
7. Slave Address + W
8. Write Register Address `0xFD`
9. Write Data `0x00` - `0x03` (Select Page)
10. STOP
11. START
12. Slave Address + W
13. Write Register Address within the selected page (e.g., `0x24` for a PWM value).
14. Write Data.
15. STOP

---

## **2.0 Core Concepts and Register Map**

The device memory is organized into four distinct pages. A programmer must select the correct page before accessing its registers.

**2.1. Page 3: Function Registers (Address `0x03`)**

This page contains all global configuration and control registers. It should be configured first during initialization.

* **Configuration Register (`0x00`):**
  * `D7:D6` **SYNC**: Configures multi-chip synchronization. `01`=Master, `10`=Slave, `00/11`=Disabled.
  * `D5:D3` **PFS** (IS31FL3737B only): Selects PWM frequency (Default `000`=8.4kHz).
  * `D2` **OSD**: Open/Short Detection. Write `1` to trigger a one-shot detection cycle.
  * `D1` **B_EN**: Auto Breath Mode Enable. `1` enables the ABM engine for assigned LEDs.
  * `D0` **SSD**: Software Shutdown. `0`=Shutdown, `1`=Normal Operation (Default).

* **Global Current Control Register (`0x01`):**
  * `D7:D0` **GCC**: A 256-step value (`0x00`-`0xFF`) that sets the maximum output current for all CS pins, in conjunction with the external resistor. This controls the overall display brightness.

* **Auto Breath Control Registers (`0x02` - `0x0D`):**
  * These 12 registers define the timing parameters for the three independent Auto Breath Modes (ABM-1, ABM-2, ABM-3).
  * Each ABM has registers for rise time (T1), hold time (T2), fall time (T3), off time (T4), and loop control.

* **Time Update Register (`0x0E`):**
  * Writing `0x00` to this register latches any new values written to the ABM timing registers (`0x02`-`0x0D`) into the ABM engine. This must be done after updating ABM parameters.

* **Reset Register (`0x11`, Read-Only):**
  * Reading from this register resets all registers on the device to their power-on default values.

**2.2. Page 1: PWM Registers (Address `0x01`)**

This page contains the 8-bit brightness value for every individual LED.

* **Address Range:**
  * IS31FL3733: `0x00` - `0xBF` (192 bytes)
  * IS31FL3737B: `0x00` - `0x8F` (144 bytes)
* **Function:** Each byte (`0x00`-`0xFF`) corresponds to one LED and sets its PWM duty cycle (0-255 steps).
* **Mapping (CSx, SWy) to Register Address:**
  * `Address = (SWy - 1) * 16 + (CSx - 1)`
  * **Example (IS31FL3733):** The LED at `CS5`, `SW3` is controlled by PWM register at address `(3 - 1) * 16 + (5 - 1) = 2 * 16 + 4 = 36 = 0x24`.

**2.3. Page 0: LED Control Registers (Address `0x00`)**

This page controls the on/off state and stores the open/short fault status for each LED.

* **LED On/Off Registers (`0x00` - `0x17`):**
  * **Function:** These are write-only registers. Each bit corresponds to one LED. `1`=On, `0`=Off. An LED must be in the 'On' state for its PWM value to be displayed.
  * **Mapping:** The mapping is bitwise. E.g., for SW1, register `0x00` controls CS1-CS8 and `0x01` controls CS9-CS16.

* **LED Open Registers (`0x18` - `0x2F`):**
  * **Function:** Read-only registers. After an open/short detection cycle, a `1` in a bit position indicates that the corresponding LED is detected as an open circuit.

* **LED Short Registers (`0x30` - `0x47`):**
  * **Function:** Read-only registers. A `1` indicates a short circuit.

**2.4. Page 2: Auto Breath Mode Registers (Address `0x02`)**

This page is used to assign an operating mode to each individual LED.

* **Address Range:** Same as PWM registers (`0x00-0xBF` or `0x00-0x8F`).
* **Function:** Each byte in this page controls one LED. The lower two bits determine its behavior.
  * `D1:D0` **ABMS**:
    * `00`: PWM control mode (uses value from Page 1).
    * `01`: Auto Breath Mode 1 (ABM-1).
    * `10`: Auto Breath Mode 2 (ABM-2).
    * `11`: Auto Breath Mode 3 (ABM-3).

---

## **3.0 Key Operational Procedures**

**3.1. Initialization Sequence**
1. Power on the device.
2. Send an I2C command to read the **Reset Register** (`0x11` on Page 3) to ensure a known state.
3. Select **Page 3** (Function).
4. Write `0x01` to the **Configuration Register** (`0x00`). This sets `SSD=1` to enable normal operation. (Modify other bits like SYNC or PFS as needed).
5. Write a value (e.g., `0x80`) to the **Global Current Control Register** (`0x01`) to set overall brightness.
6. Select **Page 0** (LED Control).
7. Write `0xFF` to all **LED On/Off registers** (`0x00`-`0x17`) to enable all LEDs.
8. Select **Page 1** (PWM).
9. Write `0x00` to all **PWM registers** to turn all LEDs off initially. The device is now ready.

**3.2. Setting Individual LED Brightness**
1. Ensure the device is initialized and in normal operation.
2. Select **Page 1** (PWM).
3. Calculate the register address for the target LED at `(CSx, SWy)`.
4. Write the desired 8-bit PWM value (`0x00`-`0xFF`) to that address.

**3.3. Using Auto-Breath Mode (ABM)**
1. Select **Page 3** (Function).
2. Configure the timing parameters for the desired ABM (e.g., ABM-1) by writing to registers `0x02`-`0x05`.
3. Write `0x00` to the **Time Update Register** (`0x0E`) to apply the new timing.
4. Select **Page 2** (Auto Breath Mode).
5. For each LED you want to use this effect on, write the appropriate mode value (`0x01` for ABM-1) to its register in this page.
6. Select **Page 3** (Function).
7. Read the **Configuration Register** (`0x00`), set the `B_EN` bit (D1) to `1`, and write the value back. The breathing effect will now start.

**3.4. Performing Open/Short Detection**
1. Ensure all LEDs to be tested are enabled via the On/Off registers (Page 0) and have a non-zero PWM value (Page 1).
2. Select **Page 3** (Function).
3. Set the **Global Current Control** (`0x01`) to a low value (e.g., `0x01`) to ensure a valid test current.
4. Read the **Configuration Register** (`0x00`), set the `OSD` bit (D2) to `1`, and write it back. This starts the detection.
5. Wait at least two full scan cycles (~3.3ms for IS31FL3733).
6. Select **Page 0** (LED Control).
7. Read the **LED Open Registers** (`0x18`-`0x2F`) and **LED Short Registers** (`0x30`-`0x47`) to get the fault status for each LED.

---

