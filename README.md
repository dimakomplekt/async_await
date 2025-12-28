# async_await

A lightweight **async/await-style timing library** for embedded systems **without an RTOS**  
(ESP32, STM32, Arduino).

It provides a simple and clean API for creating non-blocking delays using hardware timer ticks.  
No tasks, no threads, no RTOS — just fast and minimalistic async timing.

---

## ✨ Features

- Minimalistic async/await-like API  
- Fully non-blocking delays  
- Simple context initialization  
- Does not interfere with other async timers  
- Works purely on hardware tick comparisons  
- Eliminates boilerplate `if (millis() - prev >= interval)` code  
- Portable design intended for ESP32 / STM32 / Arduino

---

## 🚀 Basic Idea

Instead of writing 10+ lines of repetitive timing logic and managing variables manually,  
you can simply do:

```c
// Just declare the context once:
async_await_ctx delay_1;

// And use the async-await whenever needed inside your loop.
if (async_await(&delay_1, 1200, TIME_UNIT_MS, false))
{
    // Your logic
}
```

With this library you can't use standart delays! If you want to pause execution, use library's await command:

```c
await(4000, TIME_UNIT_MS);
```

📌 How It Works

* Each delay uses a small context (timestamp, state, duration)
* The function internally compares hardware timer ticks
* Returns a boolean or state flag when the delay is fulfilled
* Multiple async awaits can run in parallel
* Works without blocking the main loop
* No RTOS or OS-like scheduler required

This allows you to structure asynchronous logic without delay(), threads, or complicated state machines.


⚡ USING EXAMPLES - AT THE END OF THE C-FILES


🛠 Future Plans:

    1. The long-term vision includes three dedicated versions, each fully tested and adapted to:

      ✔ ESP32 (ESP-IDF)
      ✔ STM32 (HAL/LL) - With the selection of the hardware timers
      ✔ Arduino Framework - Same, as ESP32, but works with the OOP syntax.


    2. Planned features:

      * Optional callback on await completion
      * Extended utilities for async sequences
      * Improved portability layer


  🫵 You are welcome to help us make this library better!
