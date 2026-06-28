# Development of 3D Printing Filament Extrusion Machine Using PET Bottles
---

## Authors

| Name | Roles |
|------|-------|
| Kim I. Alvendia | Researcher · Project Manager |
| Philip Joshua F. Amista | Researcher · Mechanical Engineer · Leader · Code Reviewer |
| Mars Laurenz C. Manginsay | Researcher · Hardware Engineer |
| James Louie R. Rivera | Researcher · Technical Writer |
| Jamil N. Tupas | Researcher · Embedded Software Engineer |


---

## Abstract

This study focuses on the development of a 3D printing filament extrusion machine designed to utilize polyethylene terephthalate (PET) bottles as a 3D printer filament source. The machine integrates a custom-designed extrusion mechanism, PID temperature control, and semi-automation using an **Arduino Nano** and a motorized stripping module to produce filament from recycled PET bottles.

The prototype has five sections: **extrusion**, **stripping**, **controller**, **spooling**, and **case**. Multiple PET bottle types were tested across extrusion time, strip length, and filament quality criteria. The optimal extrusion temperature range is **220–240°C**. Respondents evaluated the prototype as *"Excellent"* with an overall mean of **x̄ = 4.91 / 5.00**.

**Keywords:** 3D printing · filament extrusion · PET bottles · recycling · polyethylene terephthalate · Arduino Nano · plastic waste management · temperature control · PID

---

## System Architecture

The machine consists of five integrated sections:

| Section | Function |
|---------|----------|
| **Stripping Section** | Motorized blade cuts PET bottle into uniform continuous strips (3–6 mm wide) |
| **Extrusion Section** | Hotend melts PET strips and pushes molten material through nozzle to form filament (~1.75 mm) |
| **Controller Section** | Arduino Nano manages PID temperature loop, stepper speed, and user interface |
| **Spooling Section** | Tension-controlled spool collects output filament; prevents tangling |
| **Case Section** | Structural enclosure housing electronics and mechanical components |

### Block Diagrams

| Diagram | File |
|---------|------|
| Extrusion Section Wiring | [Extruder_Wire_Diagram.png](Documentation/Extruder_Wire_Diagram.png) |
| Stripping Section Wiring | [Stripper_Wire_Diagram.png](Documentation/Stripper_Wire_Diagram.png) |

---

## Hardware Components

### Extrusion / Controller Section

| Component | Details |
|-----------|---------|
| Microcontroller | Arduino Nano (ATmega328P) |
| Display | OLED SSD1309 128×64 (I²C) |
| Temperature Sensor | NTC Thermistor |
| Heater Control | MOSFET Module (D4184 / BAS027) |
| Motor (Extrusion) | NEMA 17 Stepper Motor |
| Motor Driver | A4988 / DRV8825 |
| User Input | Rotary Encoder with Push Switch |
| Power Supply | 12V AC/DC Adapter |
| Voltage Regulation | DC-DC Buck Converter (12V → 5V) |
| Filtering | Electrolytic Capacitors + Resistors |

### Stripping Section

| Component | Details |
|-----------|---------|
| Motor | NEMA 17 Stepper Motor |
| Motor Driver | A4988 / DRV8825 |
| User Input | Rotary Encoder |
| Power | 12V supply (shared bus) |

---

## Peripheral Usage

| Peripheral | Interface | Pin(s) | Role in System |
|------------|-----------|--------|----------------|
| NTC Thermistor | Analog | `A2` | Reads hotend temperature for PID feedback loop |
| MOSFET Module (D4184 / BAS027) | PWM | `D9` | Drives heater element; duty cycle set by PID output |
| NEMA 17 Stepper — Extrusion | Step/Dir | `D2`, `D4`, `D5` | Pushes PET strip through the heated nozzle |
| NEMA 17 Stepper — Stripping | Step/Dir | A4988/DRV8825 board | Motorized blade cuts PET bottle into uniform strips |
| A4988 / DRV8825 Driver | Step/Dir/EN | `D2`/`D4`/`D5` | Translates step pulses to stepper phase current |
| OLED SSD1309 128×64 | I²C | `SDA` / `SCL` | Displays live temperature, PWM duty, and setpoint |
| Rotary Encoder + Switch | Digital IN | `D6`, `D7`, `D8` | User input: adjust setpoint, speed, and toggle ON/OFF |
| DC-DC Buck Converter (12V → 5V) | Power | — | Supplies regulated 5 V to Arduino Nano and logic |
| 12 V AC/DC Adapter | Power | — | Main power rail for heater and stepper motors |

---

## Firmware

### File

```
main.ino
```

### Libraries Required

| Library | Purpose |
|---------|---------|
| `PID_v1` by Brett Beauregard | PID temperature control |
| `thermistor` | NTC thermistor analog-to-temperature conversion |
| `U8g2` (U8x8lib) | OLED display driver (SSD1309) |

Install via Arduino IDE **Library Manager** (`Sketch > Include Library > Manage Libraries`).

### Pin Configuration

| Pin | Signal | Function |
|-----|--------|----------|
| `A2` | Analog IN | Thermistor temperature read |
| `D9` | PWM OUT | Heater MOSFET control |
| `D2` | Digital OUT | Stepper STEP signal |
| `D4` | Digital OUT | Stepper DIR signal |
| `D5` | Digital OUT | Stepper ENABLE (active LOW) |
| `D6` | Digital IN | Rotary Encoder CLK (A) |
| `D7` | Digital IN | Rotary Encoder DT (B) |
| `D8` | Digital IN | Rotary Encoder Switch |
| `SDA / SCL` | I²C | OLED display |

### PID Tuning Parameters

| Parameter | Value |
|-----------|-------|
| `Kp` (Proportional) | 80 |
| `Ki` (Integral) | 12.3 |
| `Kd` (Derivative) | 40 |
| Min Setpoint | 220°C |
| Max Setpoint | 250°C |
| Setpoint Step | 1°C |
| Stepper Min Interval | 2900 µs/step |
| Stepper Max Interval | 3500 µs/step |
| Temperature Update | every 1000 ms |

---

## Flashing the Firmware

1. Install [Arduino IDE](https://www.arduino.cc/en/software)
2. Install the three libraries listed above via Library Manager
3. Open `main.ino`
4. Select:
   - **Board:** Arduino Nano
   - **Processor:** ATmega328P (Old Bootloader) — if upload fails, try the non-old-bootloader variant
   - **Port:** your COM/ttyUSB port
5. Click **Upload**

---

## Operation Guide

| Action | Effect |
|--------|--------|
| Power on | OLED shows intro: *"3D FILAMENT EXTRUSION / Press Encoder To Start"* |
| **Short press** encoder | Toggle system ON / OFF |
| **Rotate** encoder (normal mode) | Adjust temperature setpoint ± 1°C (range: 220–250°C) |
| **Long press** encoder (≥ 3 s) | Enter / exit stepper speed adjustment menu |
| **Rotate** encoder (stepper menu) | Adjust step interval 2900–3500 µs/step |

**Automatic behaviour:**
- Stepper motor starts only when temperature ≥ 220°C
- Below 220°C: stepper stops; heating continues until setpoint is reached
- OLED updates every second showing live temperature, PWM duty, and setpoint

---

## Test Results

### Strip and Filament Length per Bottle

| Bottle | Strip Length (m) | Filament Length (m) | Notes |
|--------|-----------------|---------------------|-------|
| Bottle 1 | 1.9 – 2.3 | 1.9 – 2.3 | Consistent output |
| Bottle 2 | 1.24 | — | No filament produced |
| Bottle 3 | 1.23 – 1.24 | 1.23 – 1.24 | Good quality |
| Bottle 4 | 2.9 – 4.5 | 3.9 – 4.1 | Best yield |
| Bottle 5 | 1.23 – 1.25 | 1.23 – 1.26 | Consistent |

**Optimal temperature:** 220–240°C  
**Overall evaluation:** x̄ = **4.91 / 5.00** (Excellent)

### Evaluation Criteria (TUP Instrument)

| Criterion | Rating |
|-----------|--------|
| Functionality | Excellent |
| Aesthetics | Excellent |
| Workability | Excellent |
| Durability | Excellent |
| Economy | Excellent |
| Safety | Excellent |

---

## Project Photos

| Photo | Description |
|-------|-------------|
| [Actual Extruder](Documentation/Actial_photo_of_extruder.jpg) | Full prototype assembly |
| [Stripping Section](Documentation/Actual_photo_of_stripper.jpg) | Motorized PET bottle stripper |
| [Print Output](Documentation/print_output_using_our_filament.jpg) | 3D print using produced filament |
| [Thesis Defense 1](Documentation/Thesis_defended_1.jpg) | Thesis defense presentation |
| [Thesis Defense 2](Documentation/Thesis_defended_2.jpg) | Thesis defense presentation |
| [Chapter Overview](Documentation/Chapter-1-2-3.jpg) | Chapter 1–3 summary image |

---

## Repository Structure

```
Thesis-repo/
├── main.ino                         # Arduino firmware
├── Documentation/
│   ├── FINAL_THESIS.pdf             # Full thesis document
│   ├── Extruder_Wire_Diagram.png    # Extrusion section wiring
│   ├── Stripper_Wire_Diagram.png    # Stripping section wiring
│   ├── Actial_photo_of_extruder.jpg
│   ├── Actual_photo_of_stripper.jpg
│   ├── print_output_using_our_filament.jpg
│   ├── Thesis_defended_1.jpg
│   ├── Thesis_defended_2.jpg
│   └── [additional documentation photos]
└── README.md
```

---

## Significance

This project demonstrates that **PET plastic waste** — one of the Philippines' most prevalent pollution sources — can be reclaimed as **functional 3D printing filament**, contributing to:

- Reduction of marine plastic pollution
- Circular economy in plastic waste management
- Low-cost, accessible 3D printing materials
- Open-source sustainable manufacturing practices

---

## License

This project was developed for academic purposes in partial fulfillment of the requirements for the degree **Bachelor of Engineering Technology major in Electronics Engineering Technology** at the Technological University of the Philippines.

© 2024 Alvendia · Amista · Manginsay · Rivera · Tupas. All rights reserved.
# THESIS-PETG
