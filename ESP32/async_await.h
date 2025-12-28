// =========================================================================================== INFO

// ESP32 Async Await Library (Header File, C version)
// Author: dimakomplekt
// Description: Non-blocking delay implementation using pure C for embedded use
//
// !!! NOTE / IMPORTANT !!! NOTE / IMPORTANT !!! NOTE / IMPORTANT !!! NOTE / IMPORTANT !!!
//
// Passing invalid data into any library functions will trigger execution halt (abort).
// Users must validate time values and context structures before calling the functions,
// using appropriate checks in their own code to prevent undesired behavior or program termination.
//
// With this library you can't use standart delays! To pause execution, use library's await command.
// 
// !!! NOTE / IMPORTANT !!! NOTE / IMPORTANT !!! NOTE / IMPORTANT !!! NOTE / IMPORTANT !!!
//
// The using example could be find in the end of the C-file

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
    TIME_UNIT_S,
    TIME_UNIT_MAX
    
} time_unit_t;


// Struct: async_await_ctx
// Purpose: Stores async await timer values and links to the timers linked list (for ordinary await status check
// and start_ticks summing with not async pause value)
typedef struct async_await_ctx
{

    bool initialization_status;    // Internal flag: is the timer set and registered

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


/*
* Function: async_await_init
*
* Purpose: Automatically creates and initializes an async await context data on the first call.
*
* \param *current_await_ctx: Pointer to async_await_ctx structure to initialize.
* \param time_value: Await duration value (numeric).
* \param time_unit: Await duration time unit (NS, US, MS, S) by the time_unit_t enum.
*
* \returns
*
* true  -> context successfully initialized;
*
* false -> initialization failed (invalid arguments or internal error)
*
*/
bool async_await_init(async_await_ctx *current_await_ctx, uint32_t time_value, time_unit_t time_unit);


/*
* Function: async_await
*
* Purpose: Checks if the delay time has elapsed and switch the end_flag in the current context structure.
*
* \param *current_await_ctx: Pointer to the one of async_await_ctx structure (pass by &).
* \param time_value: Await duration value (numeric).
* \param time_unit: Await duration time unit (NS, US, MS, S) by the time_unit_t enum.
* \param reboot: If true, the await timer will restart with new time_value and time_unit.
*
* \returns
*
* true  -> delay is over;
* 
* false -> still waiting
*
*/
bool async_await(async_await_ctx *current_await_ctx, uint32_t time_value, time_unit_t time_unit, bool reboot);


/*
* Function: await
*
* Purpose: Non-blocking replacement for delay().
*          Updates all registered async timers using a global time base.
*
* \param time_value: Await duration value (numeric).
* \param time_unit: Await duration time unit (NS, US, MS, S) by the time_unit_t enum.
*
* \returns
*
* void
*
*/
void await(uint64_t time_value, time_unit_t time_unit);


/*
* Function: reboot_await
*
* Purpose: Forces restart of an async await timer.
*          Resets internal timing state and applies new duration parameters.
*
* \param *current_await_ctx: Pointer to async_await_ctx structure to reboot.
* \param time_value: New await duration value (numeric).
* \param time_unit: New await duration time unit (NS, US, MS, S) by the time_unit_t enum.
*
* \returns
*
* void
*
*/
void reboot_await(async_await_ctx *current_await_ctx, uint32_t time_value, time_unit_t time_unit);


/*
* Function: end_await
*
* Purpose: Forces async_await to finish immediately, by the flags
*          (exploitation_status, end_flag) reset in the context structure.             
*          On the next call, start_ticks will be reinitialized.
*
* \param *current_await_ctx: Pointer to async_await_ctx structure.
*
* \returns
*
* void
*
*/
void end_await(async_await_ctx *current_await_ctx);



// =========================================================================================== API


#endif // async_await_H
