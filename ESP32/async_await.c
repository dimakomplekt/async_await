// =========================================================================================== INFO

// ESP32 Async Await Library (main File, C version)
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
// The using example could be find in the end of this file

// =========================================================================================== INFO


// =========================================================================================== IMPORT

// Header import
#include "async_await.h"

#include <assert.h>
#include <stdio.h>

// ESP32 standart libs import
#include "esp_rom_sys.h"

#include "esp_timer.h" // esp_timer_get_time()
#include "soc/rtc.h" // esp_cpu_get_cycle_count()
#include "esp_cpu.h"

// =========================================================================================== IMPORT


// ===========================================================================================  CRUSH

// By stdlib

__attribute__((weak))
void async_await_fatal(const char *msg)
{
    printf("[async_await] FATAL: %s\n", msg);
    abort();
}

// =========================================================================================== CRUSH


// =========================================================================================== DEFINES

// NO OS / OS

// For FreeRTOS
#ifdef USE_FREERTOS
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
// For ordinary ESP32 workflow
#else
    #include <time.h>
    #include <stdint.h>
#endif


// CPU FREQUENCY SELECTION SECTION

// CPU main frequency for the nanoseconds ticks compare (showed in terminal, 240 for ESP32, 160
// for ESP32C3)
#define CPU_FREQ_MHZ 240

// =========================================================================================== DEFINES


// =========================================================================================== VARIABLES


// Global variable head of the linked list which uses for ordinary await status check
// and start_ticks summing with not async pause value)
static async_await_ctx* registry_head = NULL;

// =========================================================================================== VARIABLES


// =========================================================================================== INNER FUNCTIONS DECLARATION

// Function: async_await_register
// Purpose: Adds external timer context into global linked list to track across blocking delays
void async_await_register(async_await_ctx *current_await_ctx);

// Function: get_monotonic_time
// Purpose: Count and return ticks value, translated to the general purpose timer ticks,
// by the selected time value and unit
uint64_t get_monotonic_time(time_unit_t time_unit);


// Function: get_timer_ticks_value
// Purpose: Translate the user selected time value to the ticks value
uint64_t get_timer_ticks_value(uint32_t time_value, time_unit_t time_unit);


// Function: get_timer_ticks_value
// Ticks convert from unit to unit
uint64_t convert_time_between_units(uint64_t value, time_unit_t from, time_unit_t to);


// Function: reboot_this_timer
// Purpose: Check if we need to reboot timer due to new time settings and reboot flag
bool reboot_by_new_data(async_await_ctx *current_await_ctx, uint32_t time_value, time_unit_t time_unit, bool reboot);


// =========================================================================================== INNER FUNCTIONS DECLARATION


// =========================================================================================== FUNCTIONS DEFINITION


// =========================================================================================== EXTERN

bool async_await(async_await_ctx *current_await_ctx, uint32_t time_value, time_unit_t time_unit, bool reboot)
{
    // Function with bool return for the async delay operation (by general purpose timer (GPT) ticks comparing) - uses with flow, like:
    // if (async_await(timer_1)) {...
    // Checks whether the specified async timer has completed its delay interval
    // Returns true if delay elapsed, false otherwise

    assert(current_await_ctx && "async_await: NULL context pointer");

    /* Optional runtime guard for release builds */
    if (!current_await_ctx) {
        assert(0 && "async_await: NULL context pointer");
        async_await_fatal("NULL context pointer");
    }

    // Initialization with data error handling by the first call or reboot request
    if (!current_await_ctx->initialization_status || reboot_by_new_data(current_await_ctx, time_value, time_unit, reboot))
    {
        if (!async_await_init(current_await_ctx, time_value, time_unit))
        {
            printf("[async_await] FATAL: async_await_init failed\n");
            assert(0 && "async_await_init failed during async_await");
            async_await_fatal("Wrong async_await data");
        }
    }

    // Values update without reboot
    if (!reboot)
    {
        uint64_t current_ticks_value = get_timer_ticks_value(time_value, time_unit);

        if (current_ticks_value != current_await_ctx->time_value || time_unit != current_await_ctx->time_unit)
        {
            current_await_ctx->time_value = current_ticks_value;
            current_await_ctx->time_unit = time_unit;
        }
    }

    // Current ticks inside the current function call by GPT
    uint64_t current_time = get_monotonic_time(time_unit);
    // Current variable inside the current function call for elapsed time value storage
    uint64_t elapsed_time;

    // If timer called 1st time  
    if (!current_await_ctx->exploitation_status)
    {
        // Write start_ticks value for comparation operations
        current_await_ctx->start_ticks = current_time;
        // Set the initialization status as "true"
        current_await_ctx->exploitation_status = true;
        // Exit from function call
        current_await_ctx->end_flag = false;
    }
    // If timer called 2nd, 3rd... time
    else
    {
        // Comparing with overflow control
        if (current_time >= current_await_ctx->start_ticks) elapsed_time = current_time - current_await_ctx->start_ticks;
        // Overflow control
        else elapsed_time = (UINT64_MAX - current_await_ctx->start_ticks + current_time + 1);

        // Action for timer ending  
        if (elapsed_time >= current_await_ctx->time_value)
        {
            // Initialization status reset
            current_await_ctx->start_ticks = get_monotonic_time(time_unit);   // Momentary
            current_await_ctx->exploitation_status = false;  // Reset

            current_await_ctx->end_flag = true;

            // Return the true
            return current_await_ctx->end_flag;
        }
    }
    // Return false if there is some time left
    return current_await_ctx->end_flag;
}


// Ordinary delay function, which don't kill the async_await timers workflow, by current_ticks summation
// with delay value after delay
void await(uint64_t time_value, time_unit_t time_unit)
{
    // Error handling 

    assert(time_value > 0 && "await: time_value must be > 0");
    assert(time_unit >= TIME_UNIT_NS && time_unit < TIME_UNIT_MAX);

    if (time_value == 0) async_await_fatal("await: time_value must be > 0");

    if (time_unit < TIME_UNIT_NS || time_unit >= TIME_UNIT_MAX) async_await_fatal("await: invalid time_unit");


    // Delay starting time
    uint64_t start = get_monotonic_time(time_unit);

    // Delay for FreeRTOS using
    #ifdef USE_FREERTOS
    vTaskDelay(get_timer_ticks_value(time_value, time_unit) / 1000); // Only ms
    // Ordinary delay
    #else
    // MAX time for esp_rom_delay_us is 4000000 (4 sec)
    // This code chunks delay for the several steps and repeats it until the end of delay time
    
    const uint32_t max_step_ms = 4000;
    uint64_t ticks = get_timer_ticks_value(time_value, time_unit);
    uint64_t remaining_us = convert_time_between_units(ticks, time_unit, TIME_UNIT_US); // convert to microseconds

    while (remaining_us > 0)
    {
        uint32_t chunk = (remaining_us > max_step_ms * 1000ULL) ? (max_step_ms * 1000) : (uint32_t)remaining_us;
        esp_rom_delay_us(chunk);
        remaining_us -= chunk;
    }
    #endif

    // Delay ending time
    uint64_t end = get_monotonic_time(time_unit);

    // Elapsed time value without overflow
    uint64_t elapsed = (end >= start) ? (end - start) : (UINT64_MAX - start + end + 1);


    // Cycle for the active async awaits (which were registrated inside the linked list) start_ticks value change
    async_await_ctx* current = registry_head; // 1st list element

    while (current)
    {
        // If async await initialized
        if (current->exploitation_status)
        {
            uint64_t corrected_elapsed = convert_time_between_units(elapsed, time_unit, current->time_unit);
            current->start_ticks += corrected_elapsed;
        }

        current = current->next; // next list element
    }
}

bool async_await_init(async_await_ctx *current_await_ctx, uint32_t time_value, time_unit_t time_unit)
{
    // Error handling
    if (time_value == 0) {
        printf("[async_await_init] Error: time_value must be greater than 0\n");
        return false;
    }

    if (time_unit < TIME_UNIT_NS || time_unit >= TIME_UNIT_MAX)
    {
        printf("[async_await_init] Error: invalid time_unit value\n");
        return false;
    }

    bool saved_end_flag = current_await_ctx->end_flag;

    // Default context values with initialization
    *current_await_ctx = async_await_ctx_default();

    current_await_ctx->end_flag = saved_end_flag;

    // time_value value as user defined
    current_await_ctx->time_value = get_timer_ticks_value(time_value, time_unit);
    current_await_ctx->time_unit = time_unit;


    // Linked list fill with current context pointer by function-helper
    async_await_register(current_await_ctx);

    // Timer initialization confirming
    current_await_ctx->initialization_status = true;
    current_await_ctx->exploitation_status = false;

    return true;
}

// Force restart of async_await or await timer (no checks, always restarts)
void reboot_await(async_await_ctx *current_await_ctx, uint32_t time_value, time_unit_t time_unit)
{
    if (!current_await_ctx) return;

    current_await_ctx->time_value = get_timer_ticks_value(time_value, time_unit);
    current_await_ctx->time_unit = time_unit;

    current_await_ctx->start_ticks = get_monotonic_time(time_unit);

    current_await_ctx->exploitation_status = true;
    current_await_ctx->end_flag  = false;
}

// Force the async_await to reinitialize start_ticks with next call await
void end_await(async_await_ctx *current_await_ctx)
{
    current_await_ctx->initialization_status = false;
    current_await_ctx->exploitation_status  = false;
    current_await_ctx->end_flag  = false;
}

// =========================================================================================== EXTERN


// =========================================================================================== INNER

// Adds a timer context to the global linked list if it's not already registered
void async_await_register(async_await_ctx* current_await_ctx)
{
    // If the current_await_ctx is already registrated
    async_await_ctx* current = registry_head;
    
    while (current)
    {
        if (current == current_await_ctx) return; //  it is head now - return
        current = current->next; // Next current_await_ctx
    }

    // Put current_await_ctx to the the linked list startpoint
    current_await_ctx->next = registry_head;
    registry_head = current_await_ctx;
}


// Ticks value getter function
uint64_t get_monotonic_time(time_unit_t time_unit) {

    static uint32_t last_timer_ns = 0;
    static uint64_t overflow_count_ns = 0;


    // For nanoseconds
    if (time_unit == TIME_UNIT_NS)
    {
        // Get the current clock from 240/160 MHZ GPT
        uint32_t current_clock_ns = esp_cpu_get_cycle_count();

        // Overflow defence
        if (current_clock_ns < last_timer_ns) overflow_count_ns++;

        last_timer_ns = current_clock_ns;

        // Total cycles count return
        uint64_t total_ns_ticks = ((uint64_t)overflow_count_ns << 32) | current_clock_ns;
        return total_ns_ticks; // Ticks from 240/160 MHz GPT
    }
    else
    {
        uint64_t current_clock_us = esp_timer_get_time();

        // Overflow defence is not important - overflow after 500000 years
        return current_clock_us;
    }
}


// Ticks to time unit translator
uint64_t get_timer_ticks_value(uint32_t time_value, time_unit_t time_unit) {

    switch (time_unit)
    {
        // Ticks by GPU for nanoseconds compare
        // 1 tick = ticks = time_value [ns] * CPU_FREQ_MHZ / 1000
        case TIME_UNIT_NS: return ((uint64_t)time_value * CPU_FREQ_MHZ) / 1000ULL;

        // 1 tick = 1 us
        case TIME_UNIT_US: return (uint64_t)time_value;

        // 1 tick = 1000 us
        case TIME_UNIT_MS: return (uint64_t)time_value * 1000ULL;

        // 1 tick = 1000000 us
        case TIME_UNIT_S: return (uint64_t)time_value * 1000000ULL;
        
        // Default - ms
        default: return (uint64_t)time_value * 1000ULL;
    }
}

// Timer reboot by new data function
bool reboot_by_new_data(async_await_ctx *current_await_ctx, uint32_t time_value, time_unit_t time_unit, bool reboot)
{
    // Always false without reboot
    if (!reboot) return false;

    uint64_t new_ticks = get_timer_ticks_value(time_value, time_unit);

    // True if parameters changed
    return (new_ticks != current_await_ctx->time_value || time_unit != current_await_ctx->time_unit);
}

// Unit converter function-helper
uint64_t convert_time_between_units(uint64_t value, time_unit_t from, time_unit_t to)
{
    // Translation to ns as a base
    uint64_t value_ns;

    switch (from)
    {
        case TIME_UNIT_NS: value_ns = value; break;
        case TIME_UNIT_US: value_ns = value * 1000ULL; break;
        case TIME_UNIT_MS: value_ns = value * 1000000ULL; break;
        case TIME_UNIT_S:  value_ns = value * 1000000000ULL; break;
        default:           value_ns = value * 1000000ULL; break; // По умолчанию — миллисекунды
    }

    // Convert to the goal unit 
    switch (to)
    {
        case TIME_UNIT_NS: return value_ns;
        case TIME_UNIT_US: return value_ns / 1000ULL;
        case TIME_UNIT_MS: return value_ns / 1000000ULL;
        case TIME_UNIT_S:  return value_ns / 1000000000ULL;
        default:           return value_ns / 1000000ULL;
    }
}

// =========================================================================================== INNER


// =========================================================================================== API FUNCTIONS DEFINITION


// =========================================================================================== USING EXAMPLES SECTION


// =========================================================================================== async_await 2 diodes blinking

/*
// INCLUDES:
#include <stdio.h>
#include "driver/gpio.h"

#include "esp_timer.h"
#include "esp_rom_sys.h"


#include <my_libs/async_await/async_await.h>

#define USE_FREERTOS 0 // 0 for bare metal or 1 for RTOS

#if USE_FREERTOS
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
#endif
// INCLUDES END


// DEFINES:
#define LED_1 GPIO_NUM_9
#define LED_2 GPIO_NUM_10

#define RTOS_LOOP_DELAY_MS 1
// DEFINES END.


// STRUCTS:

// STRUCTS END.


// CONSTANTS:

// CONSTANTS END.


// VARIABLES:
async_await_ctx delay_1;
async_await_ctx delay_2;
async_await_ctx delay_3;
async_await_ctx delay_4;
// VARIABLES END.


// FUNCTIONS DECLARATIONS:
void initialization();
void main_loop(void *pvParameter);
// FUNCTIONS DECLARATIONS END


// SETUP:
void initialization()
{
    gpio_set_direction(LED_1, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_2, GPIO_MODE_OUTPUT);

    delay_1.end_flag = false;
    delay_2.end_flag = true;

    delay_3.end_flag = false;
    delay_4.end_flag = true;
}
// SETUP END


// MAIN LOOP:
void main_loop(void *pvParameter)
{
    while (1)
    {
        // LOOP:
        
        if (delay_2.end_flag && async_await(&delay_1, 100, TIME_UNIT_MS, false))
        {
            gpio_set_level(VCC_BD, 0); 

            delay_1.end_flag = true;
            delay_2.end_flag = false;
        }

        if (delay_1.end_flag && async_await(&delay_2, 300, TIME_UNIT_MS, false))
        {
            gpio_set_level(VCC_BD, 1);

            delay_2.end_flag = true;
            delay_1.end_flag = false;
        }   

        if (delay_4.end_flag && async_await(&delay_3, 100, TIME_UNIT_MS, false))
        {
            gpio_set_level(VCC_BD, 0); 

            delay_3.end_flag = true;
            delay_4.end_flag = false;
        }

        if (delay_3.end_flag && async_await(&delay_4, 300, TIME_UNIT_MS, false))
        {
            gpio_set_level(VCC_BD, 3);

            delay_4.end_flag = true;
            delay_3.end_flag = false;
        }   
        // LOOP END

        // Delay for correct loop
        #if USE_FREERTOS
            vTaskDelay(pdMS_TO_TICKS(RTOS_LOOP_DELAY_MS));
        #else
            await(1, TIME_UNIT_US); //  1 mks delay at the end of the loop for the stability
        #endif
    }
}
// MAIN LOOP END


// MAIN:
void app_main()
{
    // INITIALIZATION:
    initialization();
    // INITIALIZATION END

    // PRE-CYCLE:
    // PRE-CYCLE END

    // CYCLE:
    #if USE_FREERTOS
        xTaskCreate(main_loop, "Main Loop", 2048, NULL, 5, NULL);
    #else
        main_loop(NULL);
    #endif
    // CYCLE END
}
// MAIN END


// FUNCTIONS DEFINITIONS:

// FUNCTIONS DEFINITIONS END

*/

// =========================================================================================== async_await 2 diodes blinking


// =========================================================================================== async_await 1 diode delayed blinking

/*
// INCLUDES:
#include <stdio.h>
#include "driver/gpio.h"

#include "esp_timer.h"
#include "esp_rom_sys.h"


#include <my_libs/async_await/async_await.h>

#define USE_FREERTOS 0 // 0 for bare metal or 1 for RTOS

#if USE_FREERTOS
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
#endif
// INCLUDES END


// DEFINES:
#define LED_1 GPIO_NUM_9

#define RTOS_LOOP_DELAY_MS 1
// DEFINES END.


// STRUCTS:

// STRUCTS END.


// CONSTANTS:

// CONSTANTS END.


// VARIABLES:
async_await_ctx delay_1;
async_await_ctx delay_2;

bool one_await_flag = true;
// VARIABLES END.


// FUNCTIONS DECLARATIONS:
void initialization();
void main_loop(void *pvParameter);
// FUNCTIONS DECLARATIONS END


// SETUP:
void initialization()
{
    gpio_set_direction(LED_1, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_1, 1);
}
// SETUP END


// MAIN LOOP:
void main_loop(void *pvParameter)
{
    while (1)
    {
        // LOOP:
        
        if (async_await(&delay_1, 1200, TIME_UNIT_MS, false))
        {
            // Should be drop down after 5000 ms instead of 1200 ms because of await(3800) below
            gpio_set_level(LED_1, 0);
        }


        if (async_await(&delay_2, 200, TIME_UNIT_MS, false) && one_await_flag == true)
        {
            await(4000, TIME_UNIT_MS);
            one_await_flag = false;
        }

        // LOOP END

        // Delay for correct loop
        #if USE_FREERTOS
            vTaskDelay(pdMS_TO_TICKS(RTOS_LOOP_DELAY_MS));
        #else
            // For the big programs 
            await(1, TIME_UNIT_US); 
        #endif
    }
}
// MAIN LOOP END


// MAIN:
void app_main()
{
    // INITIALIZATION:
    initialization();
    // INITIALIZATION END

    // PRE-CYCLE:
    // PRE-CYCLE END

    // CYCLE:
    #if USE_FREERTOS
        xTaskCreate(main_loop, "Main Loop", 2048, NULL, 5, NULL);
    #else
        main_loop(NULL);
    #endif
    // CYCLE END
}
// MAIN END


// FUNCTIONS DEFINITIONS:

// FUNCTIONS DEFINITIONS END

*/

// =========================================================================================== async_await 1 diode delayed blinking


// =========================================================================================== USING EXAMPLES SECTION
