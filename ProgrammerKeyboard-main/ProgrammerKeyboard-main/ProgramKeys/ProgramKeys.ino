#include <Arduino.h>
#include "T-Keyboard-S3-Pro_Drive.h"
#include "pin_config.h"
#include "Arduino_GFX_Library.h"
#include "USB.h"
#include "USBHIDKeyboard.h"
#include "USBHIDConsumerControl.h"
#include "Arduino_DriveBus_Library.h"
#include "AGENCY.h"
#include "AGENCY12.h"
#include "AGENCY9.h"

#define KEY_HID_R 'r'
#define IIC_MAIN_DEVICE_ADDRESS 0x01 // 主设备的IIC地址，默认为0x01
#define IIC_LCD_CS_DEVICE_DELAY 20   // 选定屏幕等待时间

int pressed[4]={0};
int press[4]={0};
unsigned short bck= 0x00A5;

unsigned short dotsCol[4]={0xFAEA,0xFDE5,0x2647,0xBA3A};
String lbls[4]={"For","If","Else","While"};
String lbls2[4]={"(int i=0; i<8; i++)","If(i==0)","else{}","( n<=16 )"};
String lbls3[4]={"VOLOS","PROJECTS","YOUTUBE","CHANNEL"};


// string that will be send by pressing  keys
String firstKey="for(int i=0;i<8;i++) {}";
String secondKey="if(n==8) {}";
String thirdKey="if(n==8) {} else {}";
String forthKey="while(n<8){}";

enum KNOB_State
{
    KNOB_NULL,
    KNOB_INCREMENT,
    KNOB_DECREMENT,
};

int8_t KNOB_Data = 0;
bool KNOB_Trigger_Flag = false;
uint8_t KNOB_State_Flag = KNOB_State::KNOB_NULL;

//  0B000000[KNOB_DATA_A][KNOB_DATA_B]
uint8_t KNOB_Previous_Logical = 0B00000000;
uint8_t IIC_Master_Receive_Data;

size_t IIC_Bus_CycleTime = 0;
size_t KNOB_CycleTime = 0;

uint8_t KEY1_Lock = 0;
uint8_t KEY2_Lock = 0;
uint8_t KEY3_Lock = 0;
uint8_t KEY4_Lock = 0;
uint8_t KEY5_Lock = 0;

std::vector<unsigned char> IIC_Device_ID_Registry_Scan;
std::vector<unsigned char> vector_temp;
std::shared_ptr<Arduino_IIC_DriveBus> IIC_Bus =
std::make_shared<Arduino_HWIIC>(IIC_SDA, IIC_SCL, &Wire);
std::vector<T_Keyboard_S3_Pro_Device_KEY> KEY_Trigger;
USBHIDKeyboard Keyboard;
USBHIDConsumerControl ConsumerControl;

/*LCD*/
bool IIC_Device_ID_State = false;
std::vector<unsigned char> IIC_Device_ID_Scan;
std::vector<unsigned char> IIC_Device_ID_Registry;
Arduino_DataBus *bus = new Arduino_HWSPI(
    LCD_DC /* DC */, -1 /* CS */, LCD_SCLK /* SCK */, LCD_MOSI /* MOSI */, -1 /* MISO */);

Arduino_GFX *gfx = new Arduino_GC9107(
    bus, -1 /* RST */, 0 /* rotation */, true /* IPS */,
    LCD_WIDTH /* width */, LCD_HEIGHT /* height */,
    2 /* col offset 1 */, 1 /* row offset 1 */, 0 /* col_offset2 */, 0 /* row_offset2 */);

Arduino_Canvas *sprite = new Arduino_Canvas(128, 128, gfx); // Width: 100, Height: 50

void KNOB_Logical_Scan_Loop(void)
{
    uint8_t KNOB_Logical_Scan = 0B00000000;

    if (digitalRead(KNOB_DATA_A) == 1)
    {
        KNOB_Logical_Scan |= 0B00000010;
    }
    else
    {
        KNOB_Logical_Scan &= 0B11111101;
    }

    if (digitalRead(KNOB_DATA_B) == 1)
    {
        KNOB_Logical_Scan |= 0B00000001;
    }
    else
    {
        KNOB_Logical_Scan &= 0B11111110;
    }

    if (KNOB_Previous_Logical != KNOB_Logical_Scan)
    {
        if (KNOB_Logical_Scan == 0B00000000 || KNOB_Logical_Scan == 0B00000011)
        {
            KNOB_Previous_Logical = KNOB_Logical_Scan;
            KNOB_Trigger_Flag = true;
        }
        else
        {
            if (KNOB_Logical_Scan == 0B00000010)
            {
                switch (KNOB_Previous_Logical)
                {
                case 0B00000000:
                    KNOB_State_Flag = KNOB_State::KNOB_INCREMENT;
                    break;
                case 0B00000011:
                    KNOB_State_Flag = KNOB_State::KNOB_DECREMENT;
                    break;

                default:
                    break;
                }
            }
            if (KNOB_Logical_Scan == 0B00000001)
            {
                switch (KNOB_Previous_Logical)
                {
                case 0B00000000:
                    KNOB_State_Flag = KNOB_State::KNOB_DECREMENT;
                    break;
                case 0B00000011:
                    KNOB_State_Flag = KNOB_State::KNOB_INCREMENT;
                    break;

                default:
                    break;
                }
            }
        }
        // delay(10);
    }
}

void IIC_KEY_Read_Loop(void)
{
    if (IIC_Device_ID_Registry_Scan.size() > 0)
    {
        
        for (int i = 0; i < IIC_Device_ID_Registry_Scan.size(); i++)
        {
            IIC_Bus->IIC_ReadC8_Data(IIC_Device_ID_Registry_Scan[i], T_KEYBOARD_S3_PRO_RD_KEY_TRIGGER,
                                     &IIC_Master_Receive_Data, 1);

            T_Keyboard_S3_Pro_Device_KEY key_trigger_temp;
            key_trigger_temp.ID = IIC_Device_ID_Registry_Scan[i];
            key_trigger_temp.Trigger_Data = IIC_Master_Receive_Data;

            KEY_Trigger.push_back(key_trigger_temp);
        }
    }
}

void IIC_KEY_Trigger_Loop(void)
{
    if (KEY_Trigger.size() > 0)
    {
        switch (KEY_Trigger[0].ID)
        {
        case 0x01:
            if (((KEY_Trigger[0].Trigger_Data & 0B00010000) >> 4) == 1)
            {
                if (KEY1_Lock == 0)
                {
                   pressed[0]++;
                   press[0]++;
                   if(press[0]==8) press[0]=0;
                   Iocn_Show(vector_temp,0);
                   
                  Keyboard.sendString(firstKey.c_str());
                  
                }
                KEY1_Lock = 1;
            }
            else
            {
                KEY1_Lock = 0; // loop auto release
            }

            if (((KEY_Trigger[0].Trigger_Data & 0B00001000) >> 3) == 1)
            {
                if (KEY2_Lock == 0)
                {
                   pressed[1]++;
                   press[1]++;
                   if(press[1]==8) press[1]=0;
                   Iocn_Show(vector_temp,1);
                   Keyboard.sendString(secondKey.c_str());
                }
                KEY2_Lock = 1;
            }
            else
            {
                KEY2_Lock = 0;
            }

            if (((KEY_Trigger[0].Trigger_Data & 0B00000100) >> 2) == 1)
            {
                if (KEY3_Lock == 0)
                {
                  pressed[2]++;
                  press[2]++;
                   if(press[2]==8) press[2]=0;
                  Iocn_Show(vector_temp,2);
                  Keyboard.sendString(thirdKey.c_str());
                }

                KEY3_Lock = 1;
            }
            else
            {
                KEY3_Lock = 0;
            }

            if (((KEY_Trigger[0].Trigger_Data & 0B00000010) >> 1) == 1)
            {
                if (KEY4_Lock == 0)
                {
                  pressed[3]++;
                  press[3]++;
                   if(press[3]==8) press[3]=0;
                  Iocn_Show(vector_temp,3);
                   Keyboard.sendString(forthKey.c_str());
                }
                KEY4_Lock = 1;
            }
            else
            {
                KEY4_Lock = 0;
            }

            if ((KEY_Trigger[0].Trigger_Data & 0B00000001) == 1)
            {
                if (KEY5_Lock == 0)
                {
                  //Keyboard.sendString("zadnji");
                }
            }

            break;

        default:
            Keyboard.release(KEY1_Lock);
            Keyboard.release(KEY2_Lock);
            Keyboard.release(KEY3_Lock);
            Keyboard.release(KEY4_Lock);
            break;
        }
        KEY_Trigger.erase(KEY_Trigger.begin());
    }
}

void KNOB_Trigger_Loop(void)
{
    KNOB_Logical_Scan_Loop();

    if (KNOB_Trigger_Flag == true)
    {
        KNOB_Trigger_Flag = false;

        switch (KNOB_State_Flag)
        {
        case KNOB_State::KNOB_INCREMENT:
            KNOB_Data++;
            Serial.printf("\nKNOB_Data: %d\n", KNOB_Data);
            ConsumerControl.press(CONSUMER_CONTROL_VOLUME_INCREMENT);
            ConsumerControl.release();

            delay(5);
            break;
        case KNOB_State::KNOB_DECREMENT:
            KNOB_Data--;
            Serial.printf("\nKNOB_Data: %d\n", KNOB_Data);
            ConsumerControl.press(CONSUMER_CONTROL_VOLUME_DECREMENT);
            ConsumerControl.release();
            delay(5);
            break;

        default:
            break;
        }
    }
    else
    {
        ConsumerControl.release();
    }
}

void Task1(void *pvParameters)
{
   
    while (1)
    {
        IIC_Bus->IIC_Device_7Bit_Scan(&IIC_Device_ID_Registry_Scan);
        if (millis() > IIC_Bus_CycleTime)
        {
            if (IIC_Bus->IIC_Device_7Bit_Scan(&IIC_Device_ID_Scan) == true)
            {
                if (IIC_Device_ID_Scan.size() != IIC_Device_ID_Registry.size())
                {
                    IIC_Device_ID_State = true;
                }
            }
            
            IIC_Bus_CycleTime = millis() + 10; // 10ms
        }
        delay(1); 
    }
}

void Select_Screen_All(std::vector<unsigned char> device_id, bool select)
{
    if (select == true)
    {
        for (int i = 0; i < device_id.size(); i++)
        {
            IIC_Bus->IIC_WriteC8D8(device_id[i],
                                   T_KEYBOARD_S3_PRO_WR_LCD_CS, 0B00011111); // 选定全部屏幕
            delay(IIC_LCD_CS_DEVICE_DELAY);
        }
    }
    else
    {
        for (int i = 0; i < device_id.size(); i++)
        {
            IIC_Bus->IIC_WriteC8D8(device_id[i],
                                   T_KEYBOARD_S3_PRO_WR_LCD_CS, 0B00000000); // 取消选定屏幕
            delay(IIC_LCD_CS_DEVICE_DELAY);
        }
    }
}

void Iocn_Show(std::vector<unsigned char> device_id, int chosen)
{
    Select_Screen_All(device_id, true);
    
    //variable chosen dictate which display will be updated if chosen =0 first display will be updated
    // if chosen =2 second one will be updated, if chosen == 5 , all display will be updated

    if(chosen==5)
    gfx->fillScreen(BLACK);


    //UPDATING ONLY FIRST DISPLAY
    if(chosen==0 || chosen==5){
    for (int i = 0; i < device_id.size(); i++)
    {
        IIC_Bus->IIC_WriteC8D8(device_id[i],
                               T_KEYBOARD_S3_PRO_WR_LCD_CS, 0B00010000); // 选定屏幕1 LCD_1
        delay(IIC_LCD_CS_DEVICE_DELAY);
    }
  sprite->fillScreen(bck);

  for(int i=0;i<3;i++)
  sprite->fillCircle(18+(i*12),9,4,dotsCol[i]);

  sprite->setFont(&AGENCYR30pt7b);
  sprite->setTextColor(WHITE);
  sprite->setCursor(12, 92);
  sprite->println(lbls[0]);

  sprite->setFont(&AGENCYR12pt7b);
  sprite->setTextColor(0xA61C);
  sprite->setCursor(12, 38);
  sprite->println("PRESSED: ");

  sprite->setTextColor(ORANGE);
  sprite->setCursor(86, 38);
  sprite->println(String(pressed[0]));

  sprite->setFont(&AGENCYR10pt7b);
  sprite->setTextColor(0x445A);
  sprite->setCursor(12, 116);
  sprite->println(lbls2[0]);

  sprite->setFont(NULL);
  sprite->setCursor(58, 6);
  sprite->setTextColor(0x122D);
  sprite->println(lbls3[0]);

  for(int i=0;i<8;i++)
  if(i<=press[0])
  sprite->fillRect(118, 94-(i*6), 6, 3, dotsCol[0]);
  else
  sprite->fillRect(118, 94-(i*6), 6, 3, 0x122D);

  sprite->flush();
  delay(5);}

    //UPDATING SECOND DISPLAY
    if(chosen==1 || chosen==5){
    for (int i = 0; i < device_id.size(); i++)
    {
        IIC_Bus->IIC_WriteC8D8(device_id[i],
                               T_KEYBOARD_S3_PRO_WR_LCD_CS, 0B00001000); // 选定屏幕2 LCD_2
        delay(IIC_LCD_CS_DEVICE_DELAY);
    }
      sprite->fillScreen(bck);

  for(int i=0;i<3;i++)
  sprite->fillCircle(18+(i*12),9,4,dotsCol[i]);

  sprite->setFont(&AGENCYR30pt7b);
  sprite->setTextColor(WHITE);
  sprite->setCursor(12, 92);
  sprite->println(lbls[1]);

  sprite->setFont(&AGENCYR12pt7b);
  sprite->setTextColor(0xA61C);
  sprite->setCursor(12, 38);
  sprite->println("PRESSED: ");

  sprite->setTextColor(ORANGE);
  sprite->setCursor(86, 38);
  sprite->println(String(pressed[1]));

  sprite->setFont(&AGENCYR10pt7b);
  sprite->setTextColor(0x445A);
  sprite->setCursor(12, 116);
  sprite->println(lbls2[1]);

  sprite->setFont(NULL);
  sprite->setCursor(58, 6);
  sprite->setTextColor(0x122D);
  sprite->println(lbls3[1]);

  for(int i=0;i<8;i++)
  if(i<=press[1])
  sprite->fillRect(118, 94-(i*6), 6, 3, dotsCol[1]);
  else
  sprite->fillRect(118, 94-(i*6), 6, 3, 0x122D);

 
  sprite->flush();
    delay(5);}

    //UPDATING THIRD DISPLAY
    if(chosen==2 || chosen==5){
    for (int i = 0; i < device_id.size(); i++)
    {
        IIC_Bus->IIC_WriteC8D8(device_id[i],
        T_KEYBOARD_S3_PRO_WR_LCD_CS, 0B00000100); // 选定屏幕3 LCD_3
        delay(IIC_LCD_CS_DEVICE_DELAY);
    }
  sprite->fillScreen(bck);

  for(int i=0;i<3;i++)
  sprite->fillCircle(18+(i*12),9,4,dotsCol[i]);

  sprite->setFont(&AGENCYR30pt7b);
  sprite->setTextColor(WHITE);
  sprite->setCursor(12, 92);
  sprite->println(lbls[2]);

  sprite->setFont(&AGENCYR12pt7b);
  sprite->setTextColor(0xA61C);
  sprite->setCursor(12, 38);
  sprite->println("PRESSED: ");

  sprite->setTextColor(ORANGE);
  sprite->setCursor(86, 38);
  sprite->println(String(pressed[2]));

  sprite->setFont(&AGENCYR10pt7b);
  sprite->setTextColor(0x445A);
  sprite->setCursor(12, 116);
  sprite->println(lbls2[2]);

  sprite->setFont(NULL);
  sprite->setCursor(58, 6);
  sprite->setTextColor(0x122D);
  sprite->println(lbls3[2]);

  for(int i=0;i<8;i++)
  if(i<=press[2])
  sprite->fillRect(118, 94-(i*6), 6, 3, dotsCol[2]);
  else
  sprite->fillRect(118, 94-(i*6), 6, 3, 0x122D);

 
  sprite->flush();
    delay(5);}

    //UPDATING LAST DISPLAY
    if(chosen==3 || chosen==5){
    for (int i = 0; i < device_id.size(); i++)
    {
        IIC_Bus->IIC_WriteC8D8(device_id[i],
                               T_KEYBOARD_S3_PRO_WR_LCD_CS, 0B00000010); // 选定屏幕4 LCD_4
        delay(IIC_LCD_CS_DEVICE_DELAY);
    }
    
  sprite->fillScreen(bck);

  for(int i=0;i<3;i++)
  sprite->fillCircle(18+(i*12),9,4,dotsCol[i]);

  sprite->setFont(&AGENCYR30pt7b);
  sprite->setTextColor(WHITE);
  sprite->setCursor(12, 92);
  sprite->println(lbls[3]);

  sprite->setFont(&AGENCYR12pt7b);
  sprite->setTextColor(0xA61C);
  sprite->setCursor(12, 38);
  sprite->println("PRESSED: ");

  sprite->setTextColor(ORANGE);
  sprite->setCursor(86, 38);
  sprite->println(String(pressed[3]));

  sprite->setFont(&AGENCYR10pt7b);
  sprite->setTextColor(0x445A);
  sprite->setCursor(12, 116);
  sprite->println(lbls2[3]);

  sprite->setFont(NULL);
  sprite->setCursor(58, 6);
  sprite->setTextColor(0x122D);
  sprite->println(lbls3[3]);

  for(int i=0;i<8;i++)
  if(i<=press[3])
  sprite->fillRect(118, 94-(i*6), 6, 3, dotsCol[3]);
  else
  sprite->fillRect(118, 94-(i*6), 6, 3, 0x122D);

 
    sprite->flush();
    delay(5);}
    Select_Screen_All(device_id, false);
}

void LCD_Initialization(std::vector<unsigned char> device_id)
{
    Select_Screen_All(device_id, true);
    gfx->begin(4500000);
    gfx->setTextWrap(false);
    gfx->fillScreen(BLACK);
    delay(500);

    Select_Screen_All(device_id, false);
}

void Print_IIC_Info(std::vector<unsigned char> device_id)
{
    Select_Screen_All(device_id, true);
    gfx->fillScreen(WHITE);
    gfx->setCursor(15, 30);
    gfx->setTextColor(PURPLE);
    gfx->printf("IIC Info");
    Select_Screen_All(device_id, false);
}

void setup()
{
    
    pinMode(KNOB_DATA_A, INPUT_PULLUP);
    pinMode(KNOB_DATA_B, INPUT_PULLUP);

    while (IIC_Bus->begin() == false)
    {
        Serial.println("IIC_Bus initialization fail");
        delay(2000);
    }
    Serial.println("IIC_Bus initialization successfully");
    xTaskCreatePinnedToCore(Task1, "Task1", 10000, NULL, 1, NULL, 1);
    delay(100);
    Keyboard.begin();
    ConsumerControl.begin();
    USB.begin();

    /*LCD*/
    pinMode(LCD_RST, OUTPUT);
    digitalWrite(LCD_RST, HIGH);
    delay(100);
    digitalWrite(LCD_RST, LOW);
    delay(100);
    digitalWrite(LCD_RST, HIGH);
    delay(100);
    analogWrite(LCD_BL,150);

  
    while (1)
    {
        bool temp = false;
        if (IIC_Device_ID_State == true)
        {
            delay(100);

            Serial.printf("Find IIC ID: %#X\n", IIC_Device_ID_Scan[0]);

            if (IIC_Device_ID_Scan[0] == IIC_MAIN_DEVICE_ADDRESS) // 只初始化主设备，其他设备稍后测试
            {
                IIC_Device_ID_Registry.push_back(IIC_Device_ID_Scan[0]);

                std::vector<unsigned char> vector_temp;
                vector_temp.push_back(IIC_MAIN_DEVICE_ADDRESS);

                LCD_Initialization(vector_temp);

                temp = true;
            }
        }
        else
        {
            temp = false;
        }

        if (temp == true)
        {
            Serial.println("IIC_Bus select LCD_CS successful");
            break;
        }
        else
        {
            Serial.println("IIC ID not found");
            delay(1000);
        }
    }

    if (IIC_Bus->IIC_ReadC8_Delay_Data(IIC_MAIN_DEVICE_ADDRESS, T_KEYBOARD_S3_PRO_RD_DRIVE_FIRMWARE_VERSION, 20, &IIC_Master_Receive_Data, 1) == true)
    {
        Serial.printf("STM32 dirve firmware version: %#X \n", IIC_Master_Receive_Data);
    }
    sprite->begin();
    vector_temp.push_back(IIC_MAIN_DEVICE_ADDRESS);
    Print_IIC_Info(vector_temp);
    Iocn_Show(vector_temp,5);
}

void loop()
{
    IIC_KEY_Read_Loop();
    IIC_KEY_Trigger_Loop();
    if (millis() > KNOB_CycleTime)
    {
        KNOB_Logical_Scan_Loop();
        KNOB_CycleTime = millis() + 20; // 20ms
    }
    KNOB_Trigger_Loop();
}
