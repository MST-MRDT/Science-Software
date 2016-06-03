#include <EasyTransfer.h>

#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <EthernetClient.h>
#include <EthernetServer.h>
#include <Servo.h>

#include "RoveBoard.h"
#include "RoveEthernet.h"
#include "RoveComm.h"

//////////////////////////////////
// ===== CONFIG VARIABLES ===== //
//////////////////////////////////

static const int TOTAL_SENSORS  = 7;

//////////////////////////////////
//          Board Pins          //
//////////////////////////////////

static const int FUNNEL_SERVO   = PE_3;
static const int LASER_PIN      = PF_3;
static const int MAIN_POWER     = PL_3;
static const int DYNA_POWER     = PL_2;

//////////////////////////////////
//       RoveComm DataID        //
//////////////////////////////////

static const int SCI_CMD_ID      = 1808;
static const int DRILL_CMD_ID    = 866;
static const int CAROUSEL_CMD_ID = 1809;

//////////////////////////////////
// RoveComm received messages   //
//////////////////////////////////

static const byte T1_ENABLE     = 1;
static const byte T1_DISABLE    = 2;
static const byte T2_ENABLE     = 3; 
static const byte T2_DISABLE    = 4;
static const byte T3_ENABLE     = 5;
static const byte T3_DISABLE    = 6;
static const byte T4_ENABLE     = 7;
static const byte T4_DISABLE    = 8;

static const byte M1_ENABLE     = 9;
static const byte M1_DISABLE    = 10; 
static const byte M2_ENABLE     = 11;
static const byte M2_DISABLE    = 12;
static const byte M3_ENABLE     = 13;
static const byte M3_DISABLE    = 14;
static const byte M4_ENABLE     = 15;
static const byte M4_DISABLE    = 16;

static const byte CCD_REQ       = 17;
static const byte LASER_ENABLE  = 18;
static const byte LASER_DISABLE = 19;
static const byte FUNNEL_OPEN   = 20;
static const byte FUNNEL_CLOSE  = 21;

static const byte CS_ENABLE     = 22;
static const byte CS_DISABLE    = 23;

static const byte DRILL_STOP    = 0;
static const byte DRILL_FWD     = 1;
static const byte DRILL_REV     = 2;

//////////////////////////////////
//     DrillBoard Commands      //
//////////////////////////////////

static const int T1_ON           = 3;
static const int T2_ON           = 4;
static const int T3_ON           = 5;
static const int T4_ON           = 6;

static const int M1_ON           = 7;
static const int M2_ON           = 8;
static const int M3_ON           = 9;
static const int M4_ON           = 10;

//////////////////////////////////
//    Device States Variables   //
//////////////////////////////////

bool t1_on = false;
bool t2_on = false;
bool t3_on = false;
bool t4_on = false;
bool m1_on = false;
bool m2_on = false;
bool m3_on = false;
bool m4_on = false;
bool cs_on = false;

uint32_t ccd_serial_timeout = 0;
bool ccd_tcp_req = false;

//////////////////////////////////
//    EasyTransfer Protocol     //
//////////////////////////////////

struct recieve_drill_data 
{ 
  int t1_data;
  int t2_data;
  int t3_data;
  int t4_data;
  int m1_data;
  int m2_data;
  int m3_data;
  int m4_data;
  int drill_current;
};

struct send_drill_data 
{ 
  uint16_t drill_cmd;
};

struct recieve_drill_data receive_telem;
struct send_drill_data    send_command;

EasyTransfer FromDrillBoard, ToDrillBoard;

const int CCD_IMAGE_SIZE     = 3648;
const int CCD_SERIAL_TIMEOUT = 3000;

struct ccd_command 
{ 
  uint16_t ccd_id;
  byte request_flag;
};

struct ccd_image 
{ 
  uint8_t ccd_image[CCD_IMAGE_SIZE]  = { 0 };;
  byte recieved_flag; //i before e, except after c
};



struct ccd_command  send_ccd_command;
struct ccd_image    recieve_ccd_image;

EasyTransfer FromCCDBoard, ToCCDBoard;

//////////////////////////////////

float dataRead;
uint16_t dataID = 0;
size_t size = 0;
byte receivedMsg[1];

//Dynamixel Carousel;
Servo Funnel;

EthernetServer CCDServer(11001);

void setup()
{
  roveComm_Begin(192, 168, 1, 135); 
  CCDServer.begin();
  
  pinMode(MAIN_POWER, OUTPUT);
  digitalWrite(MAIN_POWER, HIGH);
  
  pinMode(DYNA_POWER, OUTPUT);
  digitalWrite(DYNA_POWER, HIGH);


  Serial.begin(9600);
  
  pinMode(LASER_PIN, OUTPUT);
  
  Funnel.attach(FUNNEL_SERVO);
  
  Serial6.begin(9600); 
  Serial7.begin(115200); 
  
  FromDrillBoard.begin(details(receive_telem), &Serial6);  
  ToDrillBoard.begin(details(send_command), &Serial6);
  
  FromCCDBoard.begin(details(recieve_ccd_image), &Serial7);  
  ToCCDBoard.begin(details(send_ccd_command), &Serial7);
}//end setup

void loop()
{
   // Get command from base station
   roveComm_GetMsg(&dataID, &size, receivedMsg);
   
   if(dataID == SCI_CMD_ID)
   {
     //////////////////////////  
     // enable devices block //
     //////////////////////////
     
     switch(receivedMsg[0])
     {
       case T1_ENABLE:
         t1_on = true;
         break;
       case T1_DISABLE:
         t1_on = false;
         break;
       case T2_ENABLE:
         t2_on = true;
         break;
       case T2_DISABLE:
         t2_on = false;
         break;
       case T3_ENABLE:
         t3_on = true;
         break;
       case T3_DISABLE:
         t3_on = false;
         break;
       case T4_ENABLE:
         t4_on = true;
         break;
       case T4_DISABLE:
         t4_on = false;
         break;
       
       case M1_ENABLE:
         m1_on = true;
         break; 
       case M1_DISABLE:
         m1_on = false;
         break;
       case M2_ENABLE:
         m2_on = true;
         break;
       case M2_DISABLE:
         m2_on = false;
         break;
       // M3 will not be equipped. Line filled by oscillating crystal.
       case M3_ENABLE:
         m3_on = true;
         break;
       case M4_ENABLE:
         m4_on = true;
         break;
       case M4_DISABLE:
         m4_on = false;
         break;

       case CS_ENABLE:
         cs_on = true;
         break;
       case CS_DISABLE:
         cs_on = false;
         break;
         
       case LASER_ENABLE:
         digitalWrite(LASER_PIN, HIGH);
         break;
       case LASER_DISABLE:
         digitalWrite(LASER_PIN, LOW);
         break;
       case FUNNEL_OPEN:
         Funnel.write(180);
         break;
       case FUNNEL_CLOSE:
         Funnel.write(37);
         break;
     }//end switch
   }//end if
   
   if(dataID == DRILL_CMD_ID) 
   {
     switch(receivedMsg[0])
     {
       case DRILL_FWD:
         send_command.drill_cmd = DRILL_FWD;
         ToDrillBoard.sendData();
         break;
       case DRILL_STOP:
         send_command.drill_cmd = DRILL_STOP;
         ToDrillBoard.sendData();
         break;
       case DRILL_REV:
         send_command.drill_cmd = DRILL_REV;
         ToDrillBoard.sendData();
         break;
     }//end switch
   }//end if
     
   if(dataID == CAROUSEL_CMD_ID)
   {
     uint16_t position = *(uint8_t*)(receivedMsg);
     if (position == 5)
       position = 4;
       
     
     //DynamixelRotateJoint(Carousel, position * 204);
     setRegister2(1,0x1e,position*204);//set goal position
   }
  /* 
   if(dataID == CCD_REQ)
   {
     send_ccd_command.ccd_id = dataID;
     send_ccd_command.request_flag = true;
     
     recieve_ccd_image.recieved_flag = false;
     ToCCDBoard.sendData();
     
     ccd_serial_timeout = millis() + CCD_SERIAL_TIMEOUT;
     while(!recieve_ccd_image.recieved_flag && (millis() < ccd_serial_timeout) )
     {
       FromCCDBoard.receiveData();
     }//end fnctn 
   }//end if
  */ 
   /////////////////////
   // CCD-Data Server //
   /////////////////////
   EthernetClient client = CCDServer.available();
   if(client)
   {
     CCDServer.write(recieve_ccd_image.ccd_image, CCD_IMAGE_SIZE);
     client.stop();
   }
   
   /////////////////////////////////////
   // Send sensor data to BaseStation //
   /////////////////////////////////////
   Serial.println("trying to receive_telem.t1_data: ");
   if(FromDrillBoard.receiveData())
   {
     
     Serial.print("receive_telem.t1_data: "); Serial.println(receive_telem.t1_data);
     Serial.print("receive_telem.t2_data: "); Serial.println(receive_telem.t2_data);
     Serial.print("receive_telem.t3_data: "); Serial.println(receive_telem.t3_data);
     Serial.print("receive_telem.t4_data: "); Serial.println(receive_telem.t4_data);
     Serial.print("receive_telem.m1_data: "); Serial.println(receive_telem.m1_data);
     Serial.print("receive_telem.m2_data: "); Serial.println(receive_telem.m2_data);
     Serial.print("receive_telem.m3_data: "); Serial.println(receive_telem.m3_data);
     Serial.print("receive_telem.m4_data: "); Serial.println(receive_telem.m4_data);
  /*   
     if(t1_on)
       roveComm_SendMsg(0x720, sizeof(receive_telem.t1_data), &receive_telem.t1_data);
     if(t2_on)
       roveComm_SendMsg(0x721, sizeof(receive_telem.t2_data), &receive_telem.t2_data);
     if(t3_on)
       roveComm_SendMsg(0x722, sizeof(receive_telem.t3_data), &receive_telem.t3_data);
     if(t4_on)
       roveComm_SendMsg(0x723, sizeof(receive_telem.t4_data), &receive_telem.t4_data);
     if(m1_on)  
       roveComm_SendMsg(0x728, sizeof(receive_telem.m1_data), &receive_telem.m1_data);
     if(m2_on)  
       roveComm_SendMsg(0x729, sizeof(receive_telem.m2_data), &receive_telem.m2_data);
     if(m3_on) 
       roveComm_SendMsg(0x72A, sizeof(receive_telem.m3_data), &receive_telem.m3_data);
     if(m4_on) 
       roveComm_SendMsg(0x72B, sizeof(receive_telem.m4_data), &receive_telem.m4_data);
   */    
   }//end if
}//end loop
  
  
  
  
  
  
  
  
  
  
  
  
  

//sets a 2-byte register value
void setRegister(unsigned char ID, unsigned char ins, unsigned char data){
    unsigned char Checksum = (~(ID + 4 + 3 + ins + data))&0xFF;
    Serial7.write(0xff);                
    Serial7.write(0xff);
    Serial7.write(ID);
    Serial7.write(4);//length
    Serial7.write(0x03);//write
    Serial7.write(ins);
    Serial7.write(data);
    Serial7.write(Checksum);
    delay(4);        
}

//sets a 2-byte register value
void setRegister2(unsigned char ID, unsigned char ins, int data){
    unsigned char dataH = data >> 8;  
    unsigned char dataL = data;
    unsigned char Checksum = (~(ID + 5 + 3 + ins + dataH + dataL))&0xFF;
    Serial7.write(0xff);                
    Serial7.write(0xff);
    Serial7.write(ID);
    Serial7.write(5);//length
    Serial7.write(0x03);//write
    Serial7.write(ins);
    Serial7.write(dataL);
    Serial7.write(dataH);
    Serial7.write(Checksum);
    delay(4);              
}



  
