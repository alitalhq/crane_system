# Crane Control Software

C++ software for controlled, safe, and automatic crane load lifting and releasing. Supports manual and automatic modes, integrating encoder, load cell, and PWM input for precise operation.

---

## Features

- **Manual Mode:** Direct motor control via serial or controller; encoder limits enforce safety.  
- **Automatic Mode:**  
  - `Load_Take` and `Load_Release` functions guide motor to target position.  
  - Continuous load detection via load cell; process restarts safely if load drops.  

- **Safety:** `STOP` command halts all operations; millis()-based waiting maintains safety checks.  
- **Flight Computer Integration:** Optional altitude data input for synchronized motor control.

---

## Hardware

- Arduino Uno  
- BTS7960 motor driver  
- DC Motor + Pulley  
- Encoder  
- Load Cell + HX711  
- PWM input (CH6 & CH7)  
- Optional flight computer (Pixhawk, Cube, etc.)

---

## Serial Commands

| Command          | Function                        |
|-----------------|---------------------------------|
| `Load_Take X`    | Lift load to target encoder `X` |
| `Load_Release X` | Lower load to target encoder `X`|
| `Manual U`       | Move motor up manually           |
| `Manual D`       | Move motor down manually         |
| `STOP`           | Immediately halt all operations  |

---

## PID Control

- Max speed at distance; decelerates approaching target.  
- Parameters `Kp`, `Ki`, `Kd` must be calibrated.

---

## Operation

1. Encoder reset and load cell calibration at startup.  
2. Manual: motor moves via serial or controller; stops at encoder limits.  
3. Automatic: motor moves to target; load presence monitored.  
4. STOP: halts all operations immediately.

---

## Safety

- Encoder-based position limits  
- Load drop detection  
- STOP command enforcement  
- Continuous safety checks during wait periods

---

## License

Open-source under [MIT License](./LICENSE)
