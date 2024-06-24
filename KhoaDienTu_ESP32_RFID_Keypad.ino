#include <SPI.h>
#include <MFRC522.h>
#include <Keypad.h>
#include <U8x8lib.h>
#include <Preferences.h>

#define SS_PIN 21
#define RST_PIN 22
#define RELAY_PIN 2
#define OLED_SCL_PIN 15
#define OLED_SDA_PIN 4  
#define U8LOG_WIDTH 16
#define U8LOG_HEIGHT 8

MFRC522 rfid(SS_PIN, RST_PIN); // cac chan cua rfid sck:18 sda:21 MISO:19 RST:22 MOSI:23 
U8X8_SH1106_128X64_NONAME_HW_I2C u8x8(/* reset=*/ U8X8_PIN_NONE, OLED_SCL_PIN, OLED_SDA_PIN);
uint8_t u8log_buffer[U8LOG_WIDTH*U8LOG_HEIGHT];
U8X8LOG u8x8log;
Preferences prePass;
Preferences preRfid;

const byte ROWS = 4; // số hàng của keypad
const byte COLS = 4; // số cột của keypad
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {32, 33, 25, 26}; // chân kết nối hàng của keypad
byte colPins[COLS] = {27, 14, 12, 13}; // chân kết nối cột của keypad

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

String defaultPassword = "123456";
String defaultCard = "F3148CA9";
String newPassword = "";
String enteredPassword = "";
int mode = 0;

bool checkCard(String uid)
{
  if(uid == defaultCard)
  {
    return true;
  }
  return false;
}
void openLock() {
  digitalWrite(RELAY_PIN, HIGH); // Kích hoạt relay để mở khóa
  u8x8log.print("\f");
  u8x8log.print("Unlocked!");
  u8x8log.print("\n");
}

void closeLock() {
  digitalWrite(RELAY_PIN, LOW); // Tắt relay để đóng khóa
  u8x8log.print("\f");
  u8x8log.print("Door locked!");
  u8x8log.print("\n");
  delay(1000);
  u8x8log.print("\f");
}
void setup() {
  Serial.begin(115200);
  SPI.begin();
  rfid.PCD_Init();

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW); // Đảm bảo khóa cửa đang đóng

  // Khởi tạo màn hình OLED
  u8x8.begin();
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8log.begin(u8x8, U8LOG_WIDTH, U8LOG_HEIGHT, u8log_buffer);
  u8x8log.setRedrawMode(1);

  prePass.begin("lock", false);
  preRfid.begin("card", false);

  // Kiểm tra nếu mật khẩu đã được lưu trữ
  if (prePass.isKey("password")) {
    defaultPassword = prePass.getString("password", "123456");
  }
  if (preRfid.isKey("rfid")) {
    defaultCard = preRfid.getString("rfid", "F3148CA9");
  }

  Serial.println("Ready to read cards and accept keypad input.");
}

void loop() {
  if(mode == 0)
  {
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      String uid = "";
      for (byte i = 0; i < rfid.uid.size; i++) 
      {
        uid += String(rfid.uid.uidByte[i], HEX);
      }
      uid.toUpperCase();

      if(checkCard(uid))
      {
        openLock();
        delay(3000); // Mở khóa trong 3 giây
        closeLock();
      }
      else
      {
        u8x8log.print("\f");
        u8x8log.print("Card invalid!");
        u8x8log.print("\n");
        delay(1000);
        u8x8log.print("\f");
      }
      rfid.PICC_HaltA();
      rfid.PCD_StopCrypto1();
    }

    char key = keypad.getKey();
    if (key) {
      if (key == '*') 
      {
        u8x8log.print("Old password!\n");
        enteredPassword = "";
        while(enteredPassword.length() < 6)
        {
          char temp = keypad.getKey();
          if(temp >= 48 && temp <= 57)
          {
            u8x8log.print('*');
            enteredPassword += temp;
            if (enteredPassword.length() == 6) {
              if (enteredPassword == defaultPassword) 
              {
                mode = 1;
                enteredPassword = "";
                return;
              }
              else
              {
                u8x8log.print("\f");
                u8x8log.print("Wrong password!");
                u8x8log.print("\n");
                delay(1000);
                u8x8log.print("\f");
                return;
              }
            }
          }
          else if(temp == 'C')
          {
            u8x8log.print("\f");
            return;
          }
        }
      } 
      else if (key == '#') {
        u8x8log.print("Enter password!\n");
        enteredPassword = "";
        while(enteredPassword.length() < 6)
        {
          char temp = keypad.getKey();
          if(temp >= 48 && temp <= 57)
          {
            u8x8log.print('*');
            enteredPassword += temp;
            if (enteredPassword.length() == 6) {
              if (enteredPassword == defaultPassword) 
              {
                mode = 2;
                enteredPassword = "";
                return;
              }
              else
              {
                u8x8log.print("\f");
                u8x8log.print("Wrong password!");
                u8x8log.print("\n");
                delay(1000);
                u8x8log.print("\f");
                return;
              }
            }
          }
          else if(temp == 'C')
          {
            u8x8log.print("\f");
            return;
          }
        }
      } 
      else if(key>=48 && key<=57)
      {
        u8x8log.print('*');
        enteredPassword += key;
        if (enteredPassword.length() == 6) {
          if (enteredPassword == defaultPassword) {
            openLock();
            delay(3000); // Mở khóa trong 3 giây
            closeLock();
          } else {
            u8x8log.print("\f");
            u8x8log.print("Wrong password!");
            u8x8log.print("\n");
            delay(1000);
            u8x8log.print("\f");
          }
          enteredPassword = "";
        }
      }
    }
  }
  else if(mode == 1)
  {
    u8x8log.print("\f");
    u8x8log.print("New password!\n");
    newPassword = "";
    while(newPassword.length() < 6)
    {
      char key = keypad.getKey();
      if(key>=48 && key<=57)
      {
        u8x8log.print('*');
        newPassword += key;
        if (newPassword.length() == 6) {
          defaultPassword = newPassword;
          prePass.putString("password", defaultPassword);
          u8x8log.print("\f");
          u8x8log.print("Successfully\n");
          delay(1000);
          u8x8log.print("\f");
          newPassword = "";
          mode = 0;
          return;
        }
      }
      else if(key == 'C')
      {
        u8x8log.print("\f");
        mode = 0;
        return;
      }  
    }
  }
  else if(mode == 2)
  {
    u8x8log.print("\f");
    u8x8log.print("New card!\n");
    String temp = "";
    do
    {
      while(!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial())
      {
        char key = keypad.getKey();
        if(key == 'C')
        {
          u8x8log.print("\f");
          mode = 0;
          return;
        }
      }
      String uid = "";
      for (byte i = 0; i < rfid.uid.size; i++) 
      {
        uid += String(rfid.uid.uidByte[i], HEX);
      }
      uid.toUpperCase();
      temp = uid;
      rfid.PICC_HaltA();
      rfid.PCD_StopCrypto1();
    }while(temp == defaultCard);
    defaultCard = temp;
    preRfid.putString("rfid", defaultCard);
    u8x8log.print("\f");
    u8x8log.print("Successfully\n");
    delay(1000);
    u8x8log.print("\f");
    mode = 0;
    return;
  }
}

