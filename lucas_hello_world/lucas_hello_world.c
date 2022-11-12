#include <stdio.h>
#include <furi.h>
#include <furi_hal.h>
#include <gui/gui.h>
#include <input/input.h>
#include <notification/notification_messages.h>

typedef enum {
    EventTypeTick,
    EventTypeInput,
} EventType;

typedef struct {
    EventType type;
    InputEvent input;
} HelloWorldEvent;

static void draw_callback(Canvas* canvas, void* ctx) {
    UNUSED(ctx);

    char buffer[64];
    FuriHalRtcDateTime curr_dt;
    furi_hal_rtc_get_datetime(&curr_dt);
    uint32_t current_timestamp = furi_hal_rtc_datetime_to_timestamp(&curr_dt);

    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 0, 10, "Olar Mundo!");

    canvas_set_font(canvas, FontSecondary);
    snprintf(
        buffer,
        sizeof(buffer),
        "%02ld:%02ld:%02ld",
        ((uint32_t)current_timestamp % (60 * 60 * 24)) / (60 * 60),
        ((uint32_t)current_timestamp % (60 * 60)) / 60,
        (uint32_t)current_timestamp % 60);
    canvas_draw_str(canvas, 0, 20, buffer);
}

static void input_callback(InputEvent* input_event, void* ctx) {
    // Check if the context is not null
    furi_assert(ctx);
    FuriMessageQueue* event_queue = ctx;

    HelloWorldEvent event = {.type = EventTypeInput, .input = *input_event};
    furi_message_queue_put(event_queue, &event, FuriWaitForever);
}

static void timer_callback(FuriMessageQueue* event_queue) {
    // Check if the context is not null
    furi_assert(event_queue);

    HelloWorldEvent event = {.type = EventTypeTick};
    furi_message_queue_put(event_queue, &event, 0);
}

int32_t hello_world_app(void* p) {
    UNUSED(p);

    // Current event of custom type HelloWorldEvent
    HelloWorldEvent event;
    // Event queue for 8 elements of size HelloWorldEvent
    FuriMessageQueue* event_queue = furi_message_queue_alloc(8, sizeof(HelloWorldEvent));

    // Create a new view port
    ViewPort* view_port = view_port_alloc();
    // Create a draw callback, no context
    // view_port_draw_callback_set(view_port, draw_callback, NULL);
    // Create a callback for keystrokes, pass as context
    // our message queue to push these events into it
    view_port_input_callback_set(view_port, input_callback, event_queue);

    // Create a GUI application
    Gui* gui = furi_record_open(RECORD_GUI);
    // Connect view port to GUI in full screen mode
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);

    // Create a periodic timer with a callback, where as
    // context will be passed to our event queue
    FuriTimer* timer = furi_timer_alloc(timer_callback, FuriTimerTypePeriodic, event_queue);
    // Start timer
    furi_timer_start(timer, 1000);

    // Enable notifications
    NotificationApp* notifications = furi_record_open(RECORD_NOTIFICATION);
    notification_message(notifications, &sequence_display_backlight_enforce_on);

    // Infinite event queue processing loop
    while(1) {
        // Select an event from the queue into the event variable (wait indefinitely if the queue is empty)
        // and check that we managed to do it
        furi_check(furi_message_queue_get(event_queue, &event, FuriWaitForever) == FuriStatusOk);

        // Our event is a button click
        if(event.type == EventTypeInput) {
            // If the "back" button is pressed, then we exit the loop, and therefore the application
            if(event.input.key == InputKeyBack) {
                break;
            }
            // Our event is a fired timer
        } else if(event.type == EventTypeTick) {
            // Create a draw callback, no context
            view_port_draw_callback_set(view_port, draw_callback, NULL);

            // Send notification of blinking blue LED
            notification_message(notifications, &sequence_blink_white_100);
            // notification_message(notifications, &sequence_single_vibro);
        }
    }

    notification_message(notifications, &sequence_display_backlight_enforce_auto);

    // Clear the timer
    furi_timer_free(timer);

    // Special cleanup of memory occupied by the queue
    furi_message_queue_free(event_queue);

    // Clean up the created objects associated with the interface
    gui_remove_view_port(gui, view_port);
    view_port_free(view_port);
    furi_record_close(RECORD_GUI);

    // Clear notifications
    furi_record_close(RECORD_NOTIFICATION);

    return 0;
}
