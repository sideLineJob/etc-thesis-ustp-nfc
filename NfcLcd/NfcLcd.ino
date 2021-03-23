#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <EEPROM.h>

#define RST_PIN 9
#define SS_PIN 10
#define BUZZER_PIN 3

// uncomment DEBUG for debuggging mode
//#define DEBUG

MFRC522 mfrc522(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Default Products Container
struct Products {
  String prodName;
  String prodId;
  String date;
  boolean active;
};

const int PRODUCTS_ARRAY_SIZE = 2;
struct Products prodList[PRODUCTS_ARRAY_SIZE];

// control variables
int indexFound = false;
int indexValue = 0;

void setup() {
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();

  #ifdef DEBUG
    Serial.println(F("Initialising..."));
  #endif
  
  // LCD
  lcd.init();
  lcd.backlight();

  // buzzer
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  lcdPrint(
    "Initialising...", 0,
    "-- please wait --", 0
  );

  initializeDefaultProducts();

  #ifdef DEBUG
    // Display Product List
    for (int i = 0; i < PRODUCTS_ARRAY_SIZE; i++) {
      Serial.println();
      Serial.print(prodList[i].prodName); Serial.print(" | ");
      Serial.print(prodList[i].prodId); Serial.print(" | ");
      Serial.print(prodList[i].date); Serial.print(" | ");
      Serial.print(prodList[i].active);
    }
  #endif

  delay(1000);

  int prodLeft = getProductLeft();
  lcdPrint(
     "Prod Left: " + String(prodLeft), 0,
    " - Scan Card", 0
  );

  #ifdef DEBUG
    Serial.println();
    Serial.println("\nWelcome!");
    Serial.println("\n -- Tap Card --");
  #endif
  
  delay(1000);

  // init buzzer buzz
  digitalWrite(BUZZER_PIN, HIGH);
  delay(100);
  digitalWrite(BUZZER_PIN, LOW);
  delay(50);
  digitalWrite(BUZZER_PIN, HIGH);
  delay(100);
  digitalWrite(BUZZER_PIN, LOW);

  // Excel
//  Serial.println("CLEARDATA");
  Serial.println("LABEL,DATE,TIME,NAME,PROD DATE ADDED,ID,TYPE,PRODUCT LEFT");
}

void loop() {
  String hex = nfcScannerListener();

  if (hex != "") {
    checkProductId(hex);
    int prodLeft = getProductLeft();

    lcdPrint(
      "Prod Left: " + String(prodLeft), 0,
      " - Scan Card", 0
    );

    if (indexFound) {
      Serial.print("DATA,DATE,TIME,");
      Serial.print(prodList[indexValue].prodName);
      Serial.print(",");
      Serial.print(prodList[indexValue].date);
      Serial.print(",");
      Serial.print(prodList[indexValue].prodId);
      Serial.print(",");
      Serial.print(prodList[indexValue].active ? "IN" : "OUT");
      Serial.print(",");
      Serial.println(prodLeft);
      indexFound = false;
    }
  }
}

String nfcScannerListener() {
  MFRC522::MIFARE_Key key;

  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

  MFRC522::StatusCode status;

  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return "";
  }

  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return "";
  }

//  Serial.println(F("**Card Detected:**"));
//  mfrc522.PICC_DumpDetailsToSerial(&(mfrc522.uid));
//  Serial.println(F("\n**End Reading**\n"));

  digitalWrite(BUZZER_PIN, HIGH);
  delay(100);
  digitalWrite(BUZZER_PIN, LOW);

  #ifdef DEBUG
    Serial.print(F("Product ID: "));
    String hex = getHex(mfrc522.uid.uidByte, mfrc522.uid.size);
    Serial.println(hex);
  #else
    String hex = getHex(mfrc522.uid.uidByte, mfrc522.uid.size);
  #endif

  delay(500);

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();

  return hex;
}

String getHex(byte *buffer, byte bufferSize) {
  String hexData = "";
  for (byte i = 0; i < bufferSize; i++) {
    hexData += String(buffer[i], HEX);
    hexData += i >= (bufferSize-1) ? "" : "-";
  }

  return hexData;
}

void checkProductId(String id) {
  boolean found = false;
  
  for (int i = 0; i < PRODUCTS_ARRAY_SIZE; i++) {

    if (id == prodList[i].prodId) {
      byte value = EEPROM.read(i);

      switch(value) {
        case 1:
          lcdPrint("ID: " + id, 0, "Type: OUT", 0);
          EEPROM.write(i, 0);
          prodList[i].active = false;
          found = true;

          #ifdef DEBUG
            Serial.println("ID: " + id + " | Type: OUT" );
          #endif

          indexValue = i;
          indexFound = true;
              
          break;
        case 0:
          lcdPrint("ID: " + id, 0, "Type: IN", 0);
          EEPROM.write(i, 1);
          prodList[i].active = true;
          found = true;

          #ifdef DEBUG
            Serial.println("ID: " + id + " | Type: IN" );
          #endif

          indexValue = i;
          indexFound = true;
        
          break;
        default:
          lcdPrint("ID: " + id, 0, "Type: OUT", 0);
          EEPROM.write(i, 0);
          prodList[i].active = false;
          found = true;

          #ifdef DEBUG
            Serial.println("ID: " + id + " | Type: OUT" );
          #endif

          indexValue = i;
          indexFound = true;
          
      }

      break;
    }
  }

  if (!found) {
    lcdPrint("Product Not", 2, "Found!", 5);
    
    #ifdef DEBUG
      Serial.println("Product Not Found!" );
    #endif
  }

  delay(2000);
}

int getProductLeft() {
  int prodLeft = 0;
  
  for (int i = 0; i < PRODUCTS_ARRAY_SIZE; i++) {
    boolean active = prodList[i].active;

    if (active) {
      prodLeft += 1;
    }
  }

  return prodLeft;
}

void lcdPrint(String line1, int cursorNum1, String line2, int cursorNum2) {
  lcd.clear();
  lcd.setCursor(cursorNum1, 0);
  lcd.print(line1);
  lcd.setCursor(cursorNum2, 1);
  lcd.print(line2);
}

void initializeDefaultProducts() {
  // Products 1
  prodList[0].prodName   = "Product 1";
  prodList[0].prodId     = "79-12-ad-99";
  prodList[0].date       = "03/21/2021";
  prodList[0].active = EEPROM.read(0) == 0 ? false : true;
  // Product 2
  prodList[1].prodName   = "Product 2";
  prodList[1].prodId     = "c9-fe-8e-6e";
  prodList[1].date       = "03/24/2021";
  prodList[1].active = EEPROM.read(1) == 0 ? false : true;
}
