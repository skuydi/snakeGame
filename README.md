# Arduino Snake Game – Enhanced Matrix Edition

This project is an **Arduino-powered Snake Game** that runs on a 10x10 NeoPixel LED matrix with MAX7219 displays for score and time.  
It is a modified version of [this project](https://elektro.turanis.de/html/prj099/index.html),  
offering an improved gaming experience, new controls, effects, and visual feedback.

## Features

- **10x10 NeoPixel Matrix**: More space for the snake and apples!
- **Zigzag Matrix Layout**: Optimized for certain LED matrices (code easily adapted for other arrangements).
- **Turbo & Boost Modes**: Temporary game speedups using dedicated buttons.
- **Timer Per Apple**: Each apple must be collected within a time limit, shown on the display.
- **MAX7219 Display**: Real-time display of score, remaining time, and high score with fun scrolling effects.
- **Potentiometer Control**: Adjust LED and display brightness on the fly.
- **Animated Effects**: Fireworks, spirals, scrolling text, and more!
- **Sound Feedback**: Buzzer sounds for fireworks and game events.
- **High Score Memory**: Challenge yourself to beat your best score.
- **User-Friendly Controls**: Separate buttons for up, down, left, right, boost, and turbo.

## Hardware Requirements

- **Arduino UNO** (or compatible)
- **10x10 NeoPixel (WS2812) Matrix** (or LED strip configured as 10x10, zigzag wiring)
- **MAX7219 4-in-1 Display Module** (for score/time)
- **6 Push Buttons**:
    - Up (Pin 2)
    - Down (Pin 3)
    - Left (Pin 4)
    - Right (Pin 5)
    - Turbo (Pin 7)
    - Boost (Pin 8)
- **Potentiometer** (brightness adjustment, connected to A0)
- **Buzzer** (Pin 9)
- **External 5V power supply** (recommended for NeoPixel matrix)
- Jumper wires, breadboard or PCB

## Wiring (Example)

| Function    | Arduino Pin | Notes                       |
|-------------|-------------|-----------------------------|
| NeoPixel    | 6           | Data IN                     |
| MAX7219 CS  | 10          |                             |
| MAX7219 DIN | 11 (MOSI)   | SPI                         |
| MAX7219 CLK | 12 (SCK)    | SPI                         |
| Up Button   | 2           | Pullup                      |
| Down Button | 3           | Pullup                      |
| Left Button | 4           | Pullup                      |
| Right Button| 5           | Pullup                      |
| Turbo Btn   | 7           | Pullup                      |
| Boost Btn   | 8           | Pullup                      |
| Potentiometer| A0         |                             |
| Buzzer      | 9           |                             |

See comments in code for additional details.

## Libraries Needed

- [Adafruit NeoPixel](https://github.com/adafruit/Adafruit_NeoPixel)
- [Adafruit GFX](https://github.com/adafruit/Adafruit-GFX-Library)
- [Max72xxPanel](https://github.com/markruys/arduino-Max72xxPanel) (or compatible)
- SPI (usually included with Arduino)

Install via the Arduino Library Manager.

## How to Play

1. **Start Game:** Press LEFT while on the game over screen to restart.
2. **Control Snake:** Use UP, DOWN, LEFT, RIGHT buttons to move.
3. **Collect Apples:** Move the snake to the apple (green pixel) before time runs out (shown on the MAX7219).
4. **Boost/Turbo:** Hold BOOST or TURBO buttons to temporarily speed up gameplay.
5. **Potentiometer:** Adjust brightness of both LED matrix and MAX7219 displays.
6. **Game Over:** The game ends if the snake hits itself or the wall, or if time runs out before eating the next apple.
7. **High Score:** Beat your best score to see a fireworks animation!

## Animations & Fun

- Several unique LED animations are used at startup, game over, and when setting a new high score.
- The buzzer plays sounds for fireworks, game events, etc.

## Customization

- Change matrix size (`X_MAX`, `Y_MAX`) for different boards.
- Adjust color, speed, and scoring in code to your liking.
- Easily switch between zigzag and standard LED matrix wiring (see comments).

## Acknowledgments

- Original concept: [elektro.turanis.de Snake Game](https://elektro.turanis.de/html/prj099/index.html)
- Heavily modified, extended, and commented by [Your Name or GitHub Username].

## License

This project is open source. See the `LICENSE` file for details.

---

**Have fun and happy coding!**

