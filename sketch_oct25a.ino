//Hello this is gonna be my first time trying to write some original code. Wish me luck

#include <ESP_I2S.h>

// I2S Audio

#define I2S_DIN 13
#define I2S_BCLK 12
#define I2S_LRC 14

// IO controls

#define MODE_LED 23
#define MODE_BTN 22
#define RANGE_UP_BTN 36
#define VOLUME_INPUT 32
#define INTERNAL_LED 2

// piano keys

#define KEY1NP 39
#define KEY2 19
#define KEY3 21
#define KEY4 18
#define KEY5NP 34

#define KEY6NP 35
#define KEY7 33
#define KEY8 25
#define KEY9 17
#define KEY10 26

#define KEY11 5
#define KEY12 16
#define KEY13 4
#define KEY14 15
#define KEY15 27

// key states
#define KEY_IDLE 0
#define KEY_PRESSED 1
#define KEY_RELEASED 2

// number of keys
#define KEYS_PHYSICAL 15
#define KEYS_TOTAL 15+12

int keyPressed[KEYS_PHYSICAL] = {0};
int keyState[KEYS_TOTAL] = {0};
float keyPhase[KEYS_TOTAL] = {0};
float keyAmplitude[KEYS_TOTAL] = {0};

int octaveUpKeyState = 0;

// key frequencie
int keyFrequencies[KEYS_TOTAL] = {
    // Physical keys
    262,  // C4
    277,  // C#
    294,  // D4
    311,  // D#
    330,  // E4
    349,  // F4
    370,  // F#
    392,  // G4
    415,  // G#
    440,  // A4
    466,  // A#
    494,  // B4
    523,  // C5
    554,  // C#
    587,  // D5

    // keys with octave up
    622,  // D#
    659,  // E5
    698,  // F5
    740,  // F#
    784,  // G5
    831,  // G#
    880,  // A5
    932,  // A#
    988,  // B5
    1046, // C6
    1109, // C#
    1175  // D6
};

// audio
I2SClass i2s;
volatile int amplitude = 0;
const int SAMPLE_RATE = 11025;

// play
#define SINE_WAVE 1
#define SQUARE 2
#define SAW_TOOTH 3
#define TRIANGLE 4

volatile int audioMode = SINE_WAVE;

void setup() {
    Serial.begin(115200);

    // wait
    while (!Serial) {
        ;
    }

    setupPins();
    setupI2S();

    Serial.println("App Started");
}

bool setupI2S() {
    i2s.setPins(I2S_BCLK, I2S_LRC, I2S_DIN);

    // start I2S
    if (!i2s.begin(I2S_MODE_STD, SAMPLE_RATE,
                   I2S_DATA_BIT_WIDTH_16BIT,
                   I2S_SLOT_MODE_MONO)) {
        return false;
    }

    return true;
}

void setupPins() {
    // LEDs
    pinMode(MODE_LED, OUTPUT);
    digitalWrite(MODE_LED, 0);

    pinMode(INTERNAL_LED, OUTPUT);
    digitalWrite(INTERNAL_LED, 0);

    // mode button
    pinMode(MODE_BTN, INPUT_PULLUP);

    // range up button
    pinMode(RANGE_UP_BTN, INPUT);

    // Volume
    int potValue = analogRead(VOLUME_INPUT);

    // piano keys
    int allKeys[] = {
        /*KEY1NP,*/ KEY2, KEY3, KEY4, /*KEY5NP*/
        /*KEY6NP,*/ KEY7, KEY8, KEY9, KEY10,
        KEY11, KEY12, KEY13, KEY14, KEY15
    };

    for (int i = 0; i < sizeof(allKeys) / sizeof(allKeys[0]); i++) {
        pinMode(allKeys[i], INPUT_PULLUP);
    }

    pinMode(KEY1NP, INPUT);
    pinMode(KEY5NP, INPUT);
    pinMode(KEY6NP, INPUT);
}

void updateKeys() {
    // update piano keys
    int keys[KEYS_PHYSICAL] = {
        KEY1NP, KEY2, KEY3, KEY4, KEY5NP,
        KEY6NP, KEY7, KEY8, KEY9, KEY10,
        KEY11, KEY12, KEY13, KEY14, KEY15
    };

    for (int i = 0; i < KEYS_PHYSICAL; i++) {
        int isPressed = 0;

        if (digitalRead(keys[i]) == 0) {
            isPressed = 1;
        }

        // check if the key was pressed
        if (isPressed == 1 && keyPressed[i] == 0) {
            onKeyDown(i);
            keyPressed[i] = 1;
        }

        // check if the key was released
        if (isPressed == 0 && keyPressed[i] == 1) {
            onKeyUp(i);
            keyPressed[i] = 0;
        }
    }

    // update octave up
    int octavePressed = 0;

    if (digitalRead(RANGE_UP_BTN) == 0) {
        octavePressed = 1;
    }

    if (octavePressed != octaveUpKeyState) {
        if (octavePressed == 1) {
            onOctavePressDown();
        } else {
            onOctavePressUp();
        }

        octaveUpKeyState = octavePressed;
    }
}

void onKeyDown(int keyIndex) {
    Serial.printf("onKeyDown %d\n", keyIndex + 1);

    digitalWrite(INTERNAL_LED, 1);

    // set the volume
    updateVolume();

    int isOctaveUpPressed = 0;

    if (digitalRead(RANGE_UP_BTN) == 0) {
        isOctaveUpPressed = 1;
    }

    int index = keyIndex;

    if (isOctaveUpPressed == 1) {
        index = keyIndex + 12;
    }

    onVirtualKeyDown(index);
}

void onKeyUp(int keyIndex) {
    Serial.printf("onKeyUp %d\n", keyIndex + 1);

    digitalWrite(INTERNAL_LED, 0);

    // release both possible octave key
    onVirtualKeyUp(keyIndex);
    onVirtualKeyUp(keyIndex + 12);
}

void onVirtualKeyDown(int keyIndex) {
    if (keyState[keyIndex] == KEY_IDLE) {
        keyPhase[keyIndex] = 0;
    }

    keyState[keyIndex] = KEY_PRESSED;
    keyAmplitude[keyIndex] = 1.0f;
}

void onVirtualKeyUp(int keyIndex) {
    if (keyState[keyIndex] == KEY_PRESSED) {
        keyState[keyIndex] = KEY_RELEASED;
    }
}

void onOctavePressDown() {
    Serial.printf("onOctavePressDown\n");

    // move pressed keys up one octave
    for (int i = 0; i < KEYS_PHYSICAL; i++) {
        if (keyPressed[i] == 1) {
            onVirtualKeyUp(i);
            onVirtualKeyDown(i + 12);
        }
    }
}

void onOctavePressUp() {
    Serial.printf("onOctavePressUp\n");

    // move pressed keys back down one octave
    for (int i = 0; i < KEYS_PHYSICAL; i++) {
        if (keyPressed[i] == 1) {
            onVirtualKeyUp(i + 12);
            onVirtualKeyDown(i);
        }
    }
}

void updateVolume() {
    
    amplitude = 4 * analogRead(VOLUME_INPUT);
}

int getNextMode(int mode) {
    if (mode == SINE_WAVE) {
        return SQUARE;
    }

    if (mode == SQUARE) {
        return SAW_TOOTH;
    }

    if (mode == SAW_TOOTH) {
        return TRIANGLE;
    }

    return SINE_WAVE;
}

void updateModeKey() {
    unsigned long currentTime = millis();

    // check the mode button
    static unsigned long lastPressTime = 0;

    if (digitalRead(MODE_BTN) == 0 &&
        currentTime - lastPressTime > 500) {

        Serial.printf("Output mode is %d\n", audioMode);

        audioMode = getNextMode(audioMode);

        Serial.printf("Output mode set to %d\n", audioMode);

        lastPressTime = currentTime;
    }
}
























void generateWaveOutputToneOnly() {
    // generate tone for testing
    static float phase = 0;

    int frequency = 440;
    int amplitude = 10000;

    float value = sin(phase) * amplitude;

    float phaseIncrement = (2.0 * PI * frequency) / SAMPLE_RATE;

    phase = phase + phaseIncrement;

    if (phase >= 2.0 * PI) {
        phase = phase - (2.0 * PI);
    }

    // send the sound to I2S
    int16_t sample = (int16_t)value;

    i2s.write(sample);
    i2s.write(sample >> 8);
}

void generateWaveOutput() {
    float value = 0;

    // go through all the stylphone keys
    for (int i = 0; i < KEYS_TOTAL; i++) {

        if (keyState[i] == KEY_IDLE) {
            continue;
        }

        // get the frequency and sound value
        int frequency = keyFrequencies[i];

        float keyValue = sin(keyPhase[i]);

        // lower sound when key released
        if (keyState[i] == KEY_RELEASED) {

            if (keyAmplitude[i] > 0) {
                keyValue = keyValue * keyAmplitude[i];

                keyAmplitude[i] = keyAmplitude[i] - 0.001;
            } else {
                keyState[i] = KEY_IDLE;
                keyAmplitude[i] = 0;
                keyValue = 0;
            }
        }

        // add the key sound 
        value = value + (keyValue * amplitude);

        // update the phase
        float phaseIncrement = (2.0 * PI * frequency) / SAMPLE_RATE;

        keyPhase[i] = keyPhase[i] + phaseIncrement;

        if (keyPhase[i] >= 2.0 * PI) {
            keyPhase[i] = keyPhase[i] - (2.0 * PI);
        }
    }

    // send the sound to I2S
    int16_t sample = (int16_t)value;

    i2s.write(sample);
    i2s.write(sample >> 8);
}










void showHeartbeatLED() {
    unsigned long currentTimeMs = millis();

    static unsigned long lastTimeMs = 0;

    if (currentTimeMs - lastTimeMs > 2000) {
        digitalWrite(MODE_LED, 0);

        lastTimeMs = currentTimeMs;
    } else {
        if (currentTimeMs - lastTimeMs > 1000) {
            digitalWrite(MODE_LED, 1);
        }
    }
}

void showAudioError() {
    Serial.println("Failed to initialize I2S!");

    while (true) {
        digitalWrite(INTERNAL_LED, 1);
        digitalWrite(MODE_LED, 1);

        delay(200);

        digitalWrite(INTERNAL_LED, 0);
        digitalWrite(MODE_LED, 0);

        delay(200);
    }
}

void loop() {
    // read all the piano keys
    updateKeys();

    // update the heartbeat LED
    showHeartbeatLED();

    // generate the audio
    generateWaveOutput();  
}
