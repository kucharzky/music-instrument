# 🎵 Music Instrument - Motion-Controlled Synthesizer

*A motion-controlled synthesizer using the FRDM-KL05Z development board.*

## 🚀 Project Overview
This project is a simple **music synthesizer** controlled via an **accelerometer** on the **FRDM-KL05Z** microcontroller board. By tilting the board, you can control different parameters of the generated sound, creating a unique motion-based musical experience!

## 🎛 Features
- 🎵 **Motion-controlled sound synthesis** using the onboard accelerometer.
- 🎮 **4x1 keyboard that changes octave and switches between triangle,sin,saw signals**
- 🎵 **Volume changes by slider on board**
- 🎚 **Frequency modulation** by tilting the board.
- 🔊 **ADC-based audio output** through a connected speaker.
- 🛠 **Implemented in C using the Kinetis SDK**.

## 🛠 Hardware Requirements
- **NXP FRDM-KL05Z** development board.
- Onboard **MMA8451Q accelerometer**.
- Speaker (connected to ADC output).
- USB cable for power and programming.

## 📦 Software Requirements
- **Keil uVision**.
- **Kinetis SDK** for peripheral drivers.
- **OpenSDA bootloader** for flashing firmware.

## 🔧 Setup & Compilation
### 1️⃣ Clone the Repository
```sh
git clone https://github.com/kucharzky/music-instrument.git
cd music-instrument
```
### 2️⃣ Open the Project in MCUXpresso
- Import the project into MCUXpresso.
- Ensure the **Kinetis SDK** is installed.

### 3️⃣ Build & Flash
- Compile the project.
- Flash the firmware using OpenSDA.
- Reset the board to start the synthesizer.

## 🎮 How to Use
- Tilt the **FRDM-KL05Z** board to change the frequency.
- The **X-axis** affect different parameters of the generated sound.
- Connect a speaker or buzzer to the ADC output to hear the generated tones.

## 🤝 Contributing
Contributions and improvements are welcome! Feel free to submit pull requests.

---
🎶 **Make music with motion!** 🎶
