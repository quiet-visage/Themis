#include "keyboard.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#define MAX_KEY_PRESSED_QUEUE 16
#define MAX_CHAR_PRESSED_QUEUE 16
#define MAX_KEYBOARD_KEYS 512

char currentKeyState[MAX_KEYBOARD_KEYS];   // Registers current frame
                                           // key state
char previousKeyState[MAX_KEYBOARD_KEYS];  // Registers previous frame
                                           // key state

// NOTE: Since key press logic involves comparing prev vs cur key
// state, we need to handle key repeats specially
char keyRepeatInFrame[MAX_KEYBOARD_KEYS];  // Registers key repeats
                                           // for current frame.

int keyPressedQueue[MAX_KEY_PRESSED_QUEUE];  // Input keys queue
size_t keyPressedQueueCount;                 // Input keys queue count

int charPressedQueue[MAX_CHAR_PRESSED_QUEUE];  // Input characters
                                               // queue (unicode)
size_t charPressedQueueCount;  // Input characters queue count

void key_callback(GLFWwindow *window, int key, int scancode,
                  int action, int mods) {
    (void)window;
    (void)scancode;
    if (key < 0) return;  // Security check, macOS fn key generates -1

    // WARNING: GLFW could return GLFW_REPEAT, we need to consider it
    // as 1 to work properly with our implementation
    // (IsKeyDown/IsKeyUp checks)
    if (action == GLFW_RELEASE)
        currentKeyState[key] = 0;
    else if (action == GLFW_PRESS)
        currentKeyState[key] = 1;
    else if (action == GLFW_REPEAT)
        keyRepeatInFrame[key] = 1;

    // WARNING: Check if CAPS/NUM key modifiers are enabled and force
    // down state for those keys
    if (((key == GLFW_KEY_CAPS_LOCK) &&
         ((mods & GLFW_MOD_CAPS_LOCK) > 0)) ||
        ((key == GLFW_KEY_NUM_LOCK) &&
         ((mods & GLFW_MOD_NUM_LOCK) > 0)))
        currentKeyState[key] = 1;

    // Check if there is space available in the key queue
    if ((keyPressedQueueCount < MAX_KEY_PRESSED_QUEUE) &&
        (action == GLFW_PRESS)) {
        // Add character to the queue
        keyPressedQueue[keyPressedQueueCount] = key;
        keyPressedQueueCount++;
    }
}

void char_callback(GLFWwindow *window, unsigned int key) {
    (void)window;
    if (charPressedQueueCount < MAX_CHAR_PRESSED_QUEUE) {
        // Add character to the queue
        charPressedQueue[charPressedQueueCount] = key;
        charPressedQueueCount++;
    }
}

int get_last_pressed_key(void) {
    if (keyPressedQueueCount < 1) return 0;
    return keyPressedQueue[0];
}

int get_sticky_key(void) {
    int key = get_last_pressed_key();
    if (key) return key;

    for (size_t i = 0; i < MAX_KEYBOARD_KEYS; i += 1)
        if (keyRepeatInFrame[i]) return i;

    return 0;
}

void kb_end_frame(void) {
    keyPressedQueueCount = 0;
    charPressedQueueCount = 0;
    for (int i = 0; i < MAX_KEYBOARD_KEYS; i++) {
        previousKeyState[i] = currentKeyState[i];
        keyRepeatInFrame[i] = 0;
    }
}

int is_key_down(int key) {
    return key > 0 && key < MAX_KEYBOARD_KEYS && currentKeyState[key];
}

int get_char(void) {
    int result = 0;

    if (charPressedQueueCount > 0) {
        result = charPressedQueue[0];
        for (int i = 0; i < (charPressedQueueCount - 1); i++)
            charPressedQueue[i] = charPressedQueue[i + 1];
        charPressedQueue[charPressedQueueCount - 1] = 0;
        charPressedQueueCount--;
    }

    return result;
}

int is_key_pressed(int key) {
    for (size_t i = 0; i < keyPressedQueueCount; i += 1) {
        if (keyPressedQueue[i] == key) return 1;
    }
    return 0;
};

int is_key_sticky(int key) {
    assert(key > 0 && key < MAX_KEYBOARD_KEYS);
    int sticky_key = get_sticky_key();

    if (!sticky_key) return 0;
    return sticky_key == key;
}
