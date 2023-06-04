#include <Keypad.h>                                                                 // Required for the keypad.
#include <LiquidCrystal_I2C.h>                                                      // Required for the lcd.

// =============================================================================
// Varibales
// =============================================================================

LiquidCrystal_I2C lcd(0x27, 16, 2);

const uint8_t KEYPAD_ROWS = 4;
const uint8_t KEYPAD_COLS = 3;
const uint8_t rowPins[KEYPAD_ROWS] = { 5, 4, 3, 2 };                                // ! Change the pins.
const uint8_t colPins[KEYPAD_COLS] = { A3, A2, A1 };                                // ! Change the pins.
const char keys[KEYPAD_ROWS][KEYPAD_COLS] = {
    { '1', '2', '3' },
    { '4', '5', '6' },
    { '7', '8', '9' },
    { '.', '0', '=' }
};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, KEYPAD_ROWS, KEYPAD_COLS);

#define MOTOR_ACTIVE_LENGTH_IN_MILLIS   5000
#define DEFAULT_SPEED_DELAY_IN_MILLIS   500
#define UPDATE_SPEED_DELAY_IN_MILLIS    10000
#define INTRO_BLINK_DELAY_IN_MILLIS     500
#define SHOP_BLINK_DELAY_IN_MILLIS      5000
#define ENEMY_SPRITE_COUNT              5
#define ENEMY_AMOUNT                    10
#define PLAYER_CHAR                     0
#define INTRO                           0
#define PLAYING                         1
#define END_PLAYING                     2
#define SHOP                            3

uint8_t currentState = INTRO;
uint8_t fullDistanceInCm = 5;                                                       // ! Change this to be right
uint8_t usingSensorIndex = 0;
uint16_t score = 0;

u32 updateTimeDelayInMillis = 500;
u32 currentTimeInMillis = 0;

const uint8_t PLAYER[8] = { 0x00, 0x00, 0x1C, 0x1F, 0x1F, 0x1C, 0x00, 0x00 };
const uint8_t ENEMY_SPRITE[ENEMY_SPRITE_COUNT][8] = { 
    { 0x03, 0x0F, 0x07, 0x1F, 0x1F, 0x07, 0x0F, 0x03 }, 
    { 0x04, 0x0C, 0x1E, 0x1F, 0x1F, 0x1E, 0x0C, 0x04 }, 
    { 0x07, 0x0E, 0x0F, 0x0F, 0x0F, 0x0F, 0x0E, 0x07 },
    { 0x07, 0x1E, 0x0C, 0x1F, 0x1F, 0x0C, 0x1E, 0x07 },
    { 0x0E, 0x1C, 0x0E, 0x19, 0x19, 0x0E, 0x1C, 0x0E }
};
const uint8_t ENEMY_CHAR[ENEMY_SPRITE_COUNT] = { 1, 2, 3 ,4, 5};
const uint8_t echo[3] = { 25, 26, 27 };                                             // ! Change the pins.
const uint8_t trig[3] = { 24, 23, 22 };                                             // ! Change the pins.
const uint8_t motor[3] = { 44, 0, 0 };                                              // ! Change the pins.

struct Enemy {
int8_t sprite;
int8_t x;
bool y;
};
Enemy enemies[ENEMY_AMOUNT];

const unsigned short itemCost[3] = { 5, 5, 10 };
const char itemName[3][16] = { "Item1", "Item2", "Item3" };

bool isItemAvalible[3] = { true, true, true };
bool isNeededToUpdateLcd = false;
bool isShowingText = true;
bool shopTextI = false;                                                             // This needed
bool playerY = 1;                                                                   // Y level of player 0 top 1 bot.


// =============================================================================
// Helping functions
// =============================================================================

void WriteAt(uint8_t _x, bool _y, uint8_t _char) {                                  // Write a character on the LCD at a specific position.
    lcd.setCursor(_x, _y);
    lcd.write(_char);
}

void PrintAt(uint8_t _x, bool _y, const byte* _str) {                               // Prints text to the LCD at a specific position.
    lcd.setCursor(_x, _y);
    lcd.printstr(_str);
}

bool IsItemAvalible(uint8_t i) {                                                    // Return true if there item in the belt.
    digitalWrite(trig[i], LOW);
    delayMicroseconds(2);
    digitalWrite(trig[i], HIGH);
    delayMicroseconds(10);
    digitalWrite(trig[i], LOW);
    short duration = pulseIn(echo[i], HIGH);
    uint8_t distanceInCm = duration * 0.034 / 2;
    return distanceInCm <= fullDistanceInCm;
}

void BuyItem(uint8_t item) {
    score -= itemCost[item] * 10;                                                   // Paying for the item.
    lcd.clear();
    lcd.print("Buying item!");
    PrintAt(0, 1, "Please wait.");
    delay(1000);
    lcd.clear();
    lcd.print("Moving the Item!");
    digitalWrite(motor[0], HIGH);                                                   // Start to move the belt.
    delay(MOTOR_ACTIVE_LENGTH_IN_MILLIS);
    digitalWrite(motor[0], LOW);                                                    // Stops the belt.
    lcd.clear();
    lcd.print("Thank You!");
    isItemAvalible[item] = IsItemAvalible(item);                                    // Checking if there is still an a item on the belt.
    EnterShop();
}

void ShowItem(uint8_t item) {                                                       // Showing item if player have enough money and the item is avalible.
    if (isItemAvalible[item] && score / 10 >= itemCost[item]) {
        sprintf(string, "%i.%s|$%i", item, itemName[item], itemCost[item]);
        lcd.print(string);
    }
}

// =============================================================================
// Enter functions
// =============================================================================

void EnterIntro() {
    lcd.clear();
    lcd.print("Builded by Gabi.");
    PrintAt(0, 1, "Coded   by Ron .");
    delay(200);
    for (uint8_t i = 0; i < 15; i++) {
        delay(200);
        lcd.scrollDisplayLeft();
    }
    lcd.clear();
    PrintAt(16, 0, "Presented to Daniel");
    PrintAt(25, 1, "2023");
    for (uint8_t i = 0; i < 35; i++) {
        delay(20);
        lcd.scrollDisplayLeft();
    }
    delay(700);
    lcd.clear();
    PrintAt(3, 0, "The Vanding");
    PrintAt(5, 1, "Machine!");
    delay(200);
    lcd.clear();
    lcd.print("Press any key to");
    PrintAt(5, 1, "<Play!>");
}

void EnterGame() {
    updateTimeDelayInMillis = DEFAULT_SPEED_DELAY_IN_MILLIS;                        // Reset the update time to the default.
    randomSeed(574574574 + currentTimeInMillis);                                    // Applying seed to random function.
    for (size_t i = 0; i < ENEMY_AMOUNT; i++) {                                     // Seting randomly the positon of every enemy.
        enemies[i].x = 16 + random(ENEMY_AMOUNT * 10);                              // Changing the enemy positon.
        enemies[i].y = random(2);
        enemies[i].sprite = ENEMY_CHAR[random(ENEMY_SPRITE_COUNT)];                 // Changing the enemy sprite.
    }
    currentState = PLAYING;
    lcd.clear();
    PrintAt(4, 0, "Entering");
    PrintAt(6, 1, "Game");
    delay(1000);
}

void EnterEndScreen() {
    char string[16];
    lcd.clear();
    PrintAt(4, 0, "You died!");
    sprintf(string, "Score:%d ", score / 10);                                       // Convert the score to a string and store it in 'string' variable.
    PrintAt(0, 1, string);
    delay(3000);
    lcd.clear();
    lcd.print("1->Retray.");
    PrintAt(0, 1, "2->Shop.");
    currentState = END_PLAYING;
}

void EnterShop() {
    lcd.clear();
    PrintAt(4, 0, "Entering");
    PrintAt(6, 1, "Shop");
    delay(1000);
    currentState = SHOP;
}

void UpdateEnemies() {
    if (currentTimeInMillis % updateTimeDelayInMillis != 0)                     // Update the enemies position only every x time.
        return;                      
    score++;                                                                    // Adding score.
    for (size_t c = 0; c < ENEMY_AMOUNT; c++) {                                 // Updating every enemy.
        enemies[c].x--;                                                         // Move enemy left.
        if (enemies[c].x < 0) {                                                 // Check if enemy is not out of the bounds of the Lcd screen.
            enemies[c].x = 16 + random(ENEMY_AMOUNT * 10);                      // Set the enemy position randomly.
            enemies[c].y = random(2);
            enemies[c].sprite = ENEMY_CHAR[random(ENEMY_SPRITE_COUNT)];         // Set the enemy sprite randomly.
        }
        if (enemies[c].x != 0)
        continue;                                                               // Checking if the enemy is in the positon x of the player.
        if (enemies[c].y == playerY) {                                          // Checking if the enemy y is the same as the player y.
            EnterEndScreen();
            return;
        }
    }
}

// =============================================================================
// Update functions
// =============================================================================

void UpdateShop() {
    if (currentTimeInMillis % SHOP_BLINK_DELAY_IN_MILLIS == 0) {                    // Blink between two texts.
        lcd.clear();
        shopTextI = !shopTextI;
        char string[16];
        if (shopTextI) {
            ShowItem(0);                                                            // Showing item 1 if player have enough money and the item is avalible.
            ShowItem(1);
        }
        else {
            ShowItem(2);
            PrintAt(0, 1, "4.Play");
        }
    }
    char key = keypad.getKey();                                                     // Getting the input of the player.
    if (key == '1' && isItemAvalible[0] && score / 10 >= itemCost[0])
        BuyItem(1);
    else if (key == '2' && isItemAvalible[1] && score / 10 >= itemCost[1])
        BuyItem(2);
    else if (key == '3' && isItemAvalible[2] && score / 10 >= itemCost[2])
        BuyItem(3);
    else if (key == '4')
        EnterGame();
}

void UpdateIntro() {
    char key = keypad.getKey();
    if (key != NO_KEY)
        EnterGame();
}

void UpdateGameLcd() {
    lcd.clear();
    for (size_t c = 0; c < ENEMY_AMOUNT; c++) {                                     // Update all enemies.
        if (enemies[c].x == -1 || enemies[c].x > 16)                                // Checks if the enemy is out of the Lcd screen bounds than don't draw it.
            continue;
        WriteAt(enemies[c].x, enemies[c].y, enemies[c].sprite);                     // Drawing the enemy.
    }
    WriteAt(0, playerY, PLAYER_CHAR);                                               // Drawing the player.
}

void UpdateEndScreen() {
    char key = keypad.getKey();
    if (key == '1')
        EnterGame();
    else if (key == '2')
        EnterShop();
}

void PlayerInput() {
    char key = keypad.getKey();
    if (key == '2') {                                                               // Move player up.
        if (playerY != 0)
            isNeededToUpdateLcd = true;
        playerY = 0;
    }
    else if (key == '8') {                                                          // Move player down.
        if (playerY != 1)
            isNeededToUpdateLcd = true;
        playerY = 1;
    }
}

void UpdateGame() {
    PlayerInput();
    UpdateEnemies();
    if (currentTimeInMillis % UPDATE_SPEED_DELAY_IN_MILLIS == 0) {                  // Making the enemies positon to update faster every x time.
        updateTimeDelayInMillis -= 1;
        if (updateTimeDelayInMillis < 100)                                          // Clamping the update speed.
            updateTimeDelayInMillis = 100;
    }
    if (currentTimeInMillis % updateTimeDelayInMillis == 0 || isNeededToUpdateLcd) { // Update the lcd screen every x time or when the player moved.
        UpdateGameLcd();
        isNeededToUpdateLcd = false;
    }
}

// =============================================================================
// Main
// =============================================================================

void setup() {
    lcd.begin(16, 2);
    lcd.backlight();
    for (uint8_t i = 0; i < 3; i++) {                                               // Seting up the ultrasonic sensors.
        pinMode(trig[i], OUTPUT);
        pinMode(echo[i], INPUT);
    }
    lcd.createChar(PLAYER_CHAR, PLAYER);                                            // Create the player sprite.
    for (uint8_t i = 0; i < ENEMY_SPRITE_COUNT; i++) {
        lcd.createChar(ENEMY_CHAR[i], ENEMY_SPRITE[i]);                             // Create the enemy sprite.
    }
    EnterIntro();
}

void loop() {                                                                       // Main function of the project.
    currentTimeInMillis = millis();
    switch (currentState) {
    case INTRO:
        UpdateIntro();
        break;
    case PLAYING:
        UpdateGame();
        break;
    case END_PLAYING:
        UpdateEndScreen();
        break;
    case SHOP:
        UpdateShop();
        break;
    }
}
