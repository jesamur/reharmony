# ReHarmony 

ReHarmony is an open biofeedback system that converts **muscle electrical activity (EMG)** into **immediate sensory feedback**.

The project has two main modes:

- 🎵 **Musical Mode (Harmonic Morphing)**  
  Muscle intensity controls chords, notes, or MIDI parameters inside a DAW.

- 🎮 **Game Mode (EMG Button)**  
  Muscle contraction acts as a virtual button (keyboard key or mouse click), compatible with games like Geometry Dash or any software that responds to a single input.

The core idea is simple:  
**Your body movement produces instant feedback.**  
This can be used for motivation, rehabilitation exercises, interactive art, or experimentation.

---

# 🌍 Project Philosophy

This repository is not a closed academic assignment.  
It is designed so **anyone in the world can build it without unnecessary complexity**.

ReHarmony:

- Does not require a complex discrete analog front-end.
- Works with commercially available EMG modules.
- Uses free software tools.
- Is fully modifiable.

If you have an Arduino and an EMG module, you can make it work.

---

# 🔬 How It Works (Simple Explanation)

When a muscle contracts:

1. Muscle fibers generate small electrical potential differences (microvolts).
2. The EMG module amplifies and filters that signal.
3. The Arduino reads the analog output.
4. The software interprets it as:
   - Musical intensity (MIDI)
   - A button press (game mode)

In short:
Muscle → EMG Module → Arduino → Music or Game

---

# 🧰 Required Hardware

## Main Components

- Arduino UNO / Leonardo / Pro Micro
- Integrated EMG module:
  - Myoware
  - AD8232
  - Generic EMG sensor board (like the one shown)
- 3 ECG electrodes (Ag/AgCl)
- Snap cables or jumper wires
- USB cable

⚠️ You do NOT need a discrete AD8226-based analog front-end.  
These modules already include amplification and filtering internally.

---

# 🔌 Basic Wiring

EMG MODULE
VCC → 5V (Arduino)
GND → GND (Arduino)
OUT → A0 (Arduino)
Electrodes:
IN+ → muscle
IN− → muscle (2–3 cm from IN+)
REF → nearby bony area

Important:
- Module GND and Arduino GND must be shared.
- Clean, dry skin improves signal quality.
- Keep cables short to reduce noise.

---

# 💻 Required Software

## For Musical Mode (Windows)

- loopMIDI (create virtual MIDI port)
- EA serial MIDI-bridge  
  https://github.com/ezequielabregu/EA-serialmidi-bridge
- FL Studio or any MIDI-compatible DAW

Alternatives:
- Hairless MIDI-Serial
- MIDIUSB (for Leonardo / Pro Micro)
- IAC Driver (macOS)
- a2jmidid / JACK (Linux)

---

# 📂 Repository Structure

reharmony/
├── firmware/
│ 
├── software/
│ 
│
└── README.md

---

# 🚀 Setup Guide

## 1️⃣ Upload Firmware

1. Open Arduino IDE.
2. Load `arduino_emg_harmonic_morphing_calib.ino`.
3. Select your board.
4. Upload the sketch.

---

## 2️⃣ Configure MIDI (Musical Mode)

1. Open loopMIDI.
2. Create a port called: `EMG-MIDI`.
3. Open EA serial MIDI-bridge:
   - Select your Arduino COM port.
   - Set baud rate (e.g., 115200).
   - Set MIDI OUT to `EMG-MIDI`.
4. In FL Studio:
   - Options → MIDI Settings
   - Enable EMG-MIDI as Input.

Done.

---

# 🎵 Musical Mode

The calibration firmware:

1. Detects resting level.
2. Detects maximum contraction.
3. Normalizes the signal.
4. Divides intensity into zones.
5. Changes chords based on intensity.

Stronger contraction means:
- Higher MIDI velocity.
- Harmonic changes.
- More energetic sound.

This creates direct auditory biofeedback.

---

# 🎮 Game Mode

Open in Processing:


How it works:

- If signal exceeds threshold → simulate SPACE key or mouse click.
- Compatible with one-button games.
- Adjust sensitivity using + / - keys.
- Toggle between keyboard and mouse with "t".

Can be used for:
- Geometry Dash
- Flappy-style games
- Interactive training exercises

---

# ⚙️ Adjustable Parameters (Firmware)

You can modify:

- Activation threshold
- Decision timing
- Response curve
- Harmonic progression
- MIDI velocity range

This allows adaptation for:
- Different muscles
- Different strength levels
- Different training goals

---

# 🛠 Troubleshooting

### No sound
- Verify MIDI port is enabled.
- Check EA bridge output.
- Confirm correct baud rate.

### Activates by itself
- Recalibrate by sending "c" via Serial Monitor.
- Check electrode placement.
- Increase threshold.

### Weak signal
- Improve electrode contact.
- Reposition electrodes.
- Use larger muscle (biceps, forearm).

---

# 🔐 Safety Notice

- Do not connect the body directly to external power sources.
- Use commercially available EMG modules.
- Prefer battery power for analog stages if possible.
- Do not use with implanted medical devices without professional supervision.

---

# 🎨 Possible Extensions

- Control modular synthesizers.
- Use two muscles for dual control.
- Add real-time signal visualization.
- Integrate with Unity.
- Build accessible musical instruments.

---

# 📜 License

Software: MIT  
Documentation: CC BY-SA 4.0  

You are free to use, modify, and improve this project.

---

# 🤝 Contributions

If you improve the firmware, add support for another EMG module, or adapt the system to another platform, feel free to fork or submit a pull request.

---

# ✨ Final Note

ReHarmony is not just about converting EMG into MIDI.

It is about using the body's electrical signal as a bridge between biology and digital experience.

Turning contraction into sound.  
Turning effort into interaction.

If you build it, share it.
