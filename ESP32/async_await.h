// =========================================================================================== INFO

// ESP32 Async Await Library (Header File, C version)
// Author: dimakomplekt
// Description: Non-blocking delay implementation using pure C for embedded use

// =========================================================================================== INFO


// =========================================================================================== IMPORT

#ifndef async_await_H
#define async_await_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// =========================================================================================== IMPORT


// =========================================================================================== EXT STRUCTS

// Time unit enum structure
// Use it to choose the await time unit
typedef enum {

    TIME_UNIT_NS,
    TIME_UNIT_US,
    TIME_UNIT_MS,
    TIME_UNIT_S
    
} time_unit_t;


// Struct: async_await_ctx
// Purpose: Stores async await timer values and links to the timers linked list (for ordinary await status check
// and start_ticks summing with not async pause value)
typedef struct async_await_ctx
{

    bool initialization_status;    // Internal flag: is the timer setted and registered

    uint32_t time_value;           // Duration in GPT ticks (scaled from user time_unit)
    time_unit_t time_unit;         // Delay duration in milliseconds
    uint64_t start_ticks;          // Tick value when the timer was started

    bool exploitation_status;      // Internal flag: is the timer currently running
    bool end_flag;                 // For sequentially work with other delays

    struct async_await_ctx* next;  // Pointer to next context in global registry (linked list)

} async_await_ctx;


// =========================================================================================== EXT STRUCTS


// =========================================================================================== API


// Default struct initializer macros
static inline async_await_ctx async_await_ctx_default(void) {

    return (async_await_ctx) {

        .initialization_status = false,
        .time_value = 0,
        .time_unit = TIME_UNIT_MS,
        .start_ticks = 0,
        .exploitation_status = false,
        .end_flag = false,
        .next = NULL

    };

}


// Function: get_monotonic_time
// Purpose: Count and return ticks value, translated to the general purpose timer ticks,
// by the selected time value and unit
uint64_t get_monotonic_time(time_unit_t time_unit);


// Function: get_timer_ticks_value
// Purpose: Translate the user selected time value to the ticks value
uint64_t get_timer_ticks_value(uint32_t time_value, time_unit_t time_unit);


// Function: get_timer_ticks_value
// Ticks convertion from unit to unit
uint64_t convert_time_between_units(uint64_t value, time_unit_t from, time_unit_t to);


// Function: reboot_this_timer
// Purpose: Check if we need to reboot timer due to new time settings and reboot flag
bool reboot_by_new_data(async_await_ctx *ctx, uint32_t time_value, time_unit_t time_unit, bool reboot);

// Function: async_await_init
// Purpose: Initializes a new async_await_ctx with given delay time
void async_await_init(async_await_ctx *ctx, uint32_t time_value, time_unit_t time_unit);


// Function: async_await_register
// Purpose: Adds external timer context into global linked list to track across blocking delays
void async_await_register(async_await_ctx *ctx);


// Function: async_await
// Purpose: Checks if the delay time has elapsed
// Returns: true  -> delay is over
//          false -> still waiting
bool async_await(async_await_ctx *ctx, uint32_t time_value, time_unit_t time_unit, bool reboot);


// Function: await
// Purpose: Replaces blocking delay() and updates all registered async timers
void await(uint64_t time_value, time_unit_t time_unit);


// Function: reboot_await
// Purpose: Force restart of async_await or await
void reboot_await(async_await_ctx *ctx, uint32_t time_value, time_unit_t time_unit);


// Function: end_await
// Purpose: Force the async_await to reinitialize start_ticks with next call await
void end_await(async_await_ctx *ctx);



// =========================================================================================== API


#endif // async_await_H
