
/*Cấu trúc câu lệnh SMS hoặc Serial
  *101# Kiểm tra tài khoản
  *100*xxxxxxxxxxxxxxx# Nạp tiền vào tài khoản
  *TL TMK Tìm mật khẩu
  *TL DMK Đặt lại mật khẩu
  *TL NHO 1/0 Nhớ/không nhớ trạng thái của rơ-le
  *TL TTRLMD TCRL MO/TAT|RL1 MO/TAT RL2 MO/TAT RL3 MO/TAT RL4 MO/TAT Đặt trạng thái mặc định khi thiết bị khởi động
  *TL RESET Khởi động lại thiết bị
  *TL DKGS xxxxxxxx Đặt thời gian không giám sát xxxxxxxx là giờ, phút bắt đầu và giờ phút kết thúc
  *TL TKGS Lấy thông tin thời gian không giám sát
  *DKRL MK TCRL MO/TAT|RL1 MO/TAT RL2 MO/TAT RL3 MO/TAT RL4 MO/TAT Điều khiển rơ-le
  *DKRL MK TT Lấy tình trạng hiện tại rơ-le
 */
 
#include "SIM900.h"
#include <SoftwareSerial.h>
#include "sms.h"
#include <call.h>
#include <EEPROM.h>
#include <Wire.h>
#include "ds3231.h"

#define DECT1 4
#define DECT2 5
#define PUM 7
#define LEDCALLIN 6
#define addrMaster  0
#define addrPassword  21
#define addrMemory  40
#define addrNoDetect 90
//#define addrFeedback  42
#define RL "RL"
#define ON "MO"
#define OFF "TAT"
#define SPC " "
#define ALL "TC"
#define STA "TT:"
#define PASS "MK:"
#define BUFF_MAX 162

unsigned long mm, prev, interval = 5000;

SMSGSM sms;
CallGSM call;
byte statusRL[4]={0,0,0,0};
byte addrSRL[4]={50,60,70,80};
byte ARL[4]={A0,A1,A2,A3};
byte PU[4]={9,10,11,12};
byte Memory=0;
//byte Feedback=0;
char number[]="+84xxxxxxxxx";
char numberMaster[]="+84xxxxxxxxx";
char message[BUFF_MAX];
char password[]="123456";
char timenodetect[]="00000000";
char pos;
char *p; 
struct ts t;

void(* resetFunc) (void) = 0;//cài đặt hàm reset

void setup()
{
  Serial.begin(9600);
  for (byte i=0;i<=3;i++)
  {
    pinMode(ARL[i],OUTPUT);  
    pinMode(PU[i],INPUT_PULLUP);
  }
  pinMode(LEDCALLIN,OUTPUT);
  pinMode(DECT1,INPUT);
  pinMode(DECT2,INPUT);
  pinMode(PUM,INPUT_PULLUP);
  if (digitalRead(PUM)==LOW)
  {
    eeprom_write_string(addrMaster,'\0');
    delay(5);
    eeprom_write_string(addrPassword,"123456");
    delay(5);
    eeprom_write_string(addrNoDetect,"00000000");
    delay(5);
    eeprom_write_string(addrMemory, "0");
    delay(5);
    //eeprom_write_string(addrFeedback, "0");
    //delay(5);
    for (byte i=0;i<=3;i++)
    {
      eeprom_write_string(addrSRL[i],"0");
      delay(5);
    }
  }
  eeprom_read_string(addrMaster,numberMaster,20);
  eeprom_read_string(addrPassword,password,8);
  eeprom_read_string(addrNoDetect,timenodetect,9);
  //Serial.print("SDT master: ");
  Serial.println(numberMaster);
  //Serial.print("Mat khau: ");
  Serial.println(password);
  char tmp[2];
  //eeprom_read_string(addrFeedback, tmp,2);
  //Feedback=atoi(tmp);
  //Serial.println(Feedback);
  eeprom_read_string(addrMemory, tmp,2);
  //Serial.print("Ghi nho tinh trang: ");
  Memory=atoi(tmp);
  Serial.println(Memory);
  for (byte i=0; i<=3;i++)
  {
    tmp[0]=0;
    eeprom_read_string(addrSRL[i], tmp,2);
    statusRL[i]=atoi(tmp);
    digitalWrite(ARL[i],statusRL[i]);
    delay(5);
  }
  //digitalWrite(13,LOW);
  digitalWrite(LEDCALLIN,LOW);
  //kiểm tra nếu kết nối thành công thì hiển thị lên cửa sổ serial mornitoring
  Wire.begin();
  DS3231_init(DS3231_INTCN);
  memset(message, 0, BUFF_MAX);
  if (gsm.begin(2400))
  {
    call.SetDTMF(1);
    //call.Call(numberMaster);
    Serial.println(F("Tinh trang = San Sang"));
    //if (Feedback) sms.SendSMS(number,"SIM900A san sang");
  }
  else {
    Serial.println(F("Tinh trang = Chua san sang"));
    //if (Feedback) sms.SendSMS(number,"SIM900A chua san sang");
  }
  delay(500);
  XuLyTinhTrangRole();
}

void loop()

{
  //Xử lý thông tin từ Serial
  
  byte availableBytes=Serial.available();
  message[0]=0;
  number[0]=0;
  
  if (availableBytes > 0 ) {
    if (availableBytes>160) availableBytes=160;
    for(byte i=0; i<availableBytes; i++)
    {
      message[i]=Serial.read();
      message[i+1]=0;
    }
    strcpy(number,numberMaster);
    Serial.flush();
    strupr(message);
    Serial.println(message);
  }
  else  {
    //Xử lý tin nhắn
    //giá trị trả về là vị trí của tin nhắn chưa đọc đầu tiên. Nếu ko có tin nhắn mới đến thì giá trị trả về 0
    pos=sms.IsSMSPresent(SMS_UNREAD);
    if((int)pos>0&&(int)pos<=20){
      char SMS_time[18];
      sms.GetSMS((int)pos,number,SMS_time,message,160);
      //Serial.println(number);
      //Serial.println(SMS_time);
      //Serial.println(message);
      sms.DeleteAllSMS();
    }
  }
  //Xử lý thông tin nhận được từ tin nhắn hoặc Serial
  if (strlen(message)!=0) {
    XuLyThongTinTuSMSHoacSerial();
  }
  else {
    //Xử lý thời gian thực
    DS3231_get(&t);
  
    //Xử lý phát hiện chuyển động
    XuLyPhatHienChuyenDong();  
  }
  
  //Xử lý nút nhấn
  XuLyNutNhan();
    
  //Xử lý cuộc gọi đến
  XuLyCuocGoiDen();

  //Xu lý nhiệt độ cao
  XuLyNhietDo();
}
void XuLyNhietDo(void)
{
  if (DS3231_get_treg()>50){
    sms.SendSMS(numberMaster,"Canh Bao: Nhiet do da hon 50*C");
  }
}
void InKetQua(byte vitri, boolean chuyendong)
{
  char buff[40];
  if (chuyendong)
  {
    Serial.print(F("Vi tri: "));
    snprintf(buff, 40, "%d luc: %02d:%02d:%02d %02d/%02d/%d ", vitri, t.hour, t.min, t.sec, t.mday,
             t.mon, t.year);
  }
  else
  {
    snprintf(buff, 40, "%02d/%02d/%d %02d:%02d:%02d ", t.mday,
             t.mon, t.year, t.hour, t.min, t.sec);
  }
  Serial.print(buff);
  //Serial.print(F("Nhiet do: "));
  Serial.print(DS3231_get_treg(), 2);
  buff[0]=176;
  buff[1]='C';
  buff[2]=0;
  Serial.println(buff);
  //Serial.println(F("C"));
  //parse_cmd("C",1);
}
void XuLyPhatHienChuyenDong(void)
{
  unsigned long time2long=t.hour * 60 + t.min;
  unsigned long time2longb=inp2toi(timenodetect,0) * 60 + inp2toi(timenodetect,2);
  unsigned long time2longs=inp2toi(timenodetect,4) * 60 + inp2toi(timenodetect,6);
  unsigned long now = millis();
  boolean bdetect = false;
 
  if (digitalRead(DECT1)==HIGH)  {
    InKetQua(1,true);
    bdetect=true;
  }
  if (digitalRead(DECT2)==HIGH) {
    InKetQua(2,true);
    bdetect=true;
  }
  //Serial.print(t.wday);
  if (time2long>=time2longb && time2long<=time2longs && t.wday !=1) {
    if ((now - prev > interval) ) {
      InKetQua(0,false);
      prev = now;
    }
  } else if (bdetect && (now - mm > 20000)) {
      Serial.println(F("Bao dong..."));
      call.Call(numberMaster);
      mm=now;
  } else  if ((now - prev > interval) ) {
        InKetQua(0,false);
        prev = now;
  }
}

void XuLyThongTinTuSMSHoacSerial(void)
{
  if (strlen(message)==0) return;
  if (strlen(numberMaster)>6)
  {
    strupr(message);
    //Xử lý thông tin nhận được
      
    //Nạp tiền
    p=strstr(message,"*100*");
    if (p){
      XuLySMSNapTien();
    }
    else
    {
      //Kiểm tra tài khoản
      p=strstr(message,"*101#");
      if(p){
        XuLySMSKTTaiKhoan(); 
      }
      else
      {
        //Xử lý đóng ngắt role
        p=strstr(message,"*DKRL");
        if (p) {
          XuLySMSDongMoRoLe();
        }  
        else
        {
          //Xử lý tin nhắn cấu hình
          p=strstr(message,"*TL");
          if (p)
          {
            XuLySMSThietLap();
          }
          else
          {
            //Serial.println(F("Khong dung cu phap"));
            //if (Feedback) sms.SendSMS(number,"Khong dung cu phap");
            parse_cmd(message,strlen(message));
          }
        }
      }
    }
  }
  else
  {
    Serial.println(F("Chua thiet lap so dien thoai chu. Vui long thiet lap SDT chu..."));
    //if (Feedback) sms.SendSMS(number,"Chua thiet lap so dien thoai chu. Vui long thiet lap SDT chu...");
  }
  message[0]=0;
}

void XuLySMSThietLap(void)
{
  char cmd[20];
  char *p_char; 
  char *p_char1;
  //Serial.print("DT tin nhan: ");
  //Serial.println(number);
  //Serial.print("DT chu: ");
  //Serial.println(numberMaster);
  p_char=numberMaster;
  p_char1=p_char+1;
  strcpy(cmd, (char *)(p_char1));
  p=strstr(number,cmd);
  if (p)
  {
    p=strstr(message,"NHO");
    if (p)
    {
      p=strstr(message,"NHO 1");
      if (p)
      {
        eeprom_write_string(addrMemory, "1");
        delay(5);
      }
      else
      {
        p=strstr(message,"NHO 0");
        if (p)
        {
          eeprom_write_string(addrMemory, "0");
          delay(5);
        }
        else
        {
          Serial.print(F("NHO: Sai cu phap"));
          //if (Feedback) sms.SendSMS(number,"Sai cu phap");
        }
      }  
    }
    else {
      p=strstr(message,"TMK");
      if (p){
        if (p){
          Serial.print(PASS);
          Serial.print(SPC);
          Serial.println(password);
          //sms.SendSMS(number,password);
        }
      }
      else{
        p=strstr(message,"DMK");
        //Serial.println(p);
        if (p) {
          if (strlen(message)<13) {
            Serial.println(F("Sai cu phap"));
            //if (Feedback) sms.SendSMS(number,"Sai cu phap");
          }
          else {
            p_char = strstr((char *)(message),"DMK");
            p_char1 = p_char+4; 
            p_char = p_char1+6;
            *p_char=0;
            strcpy(password, (char *)(p_char1));
            eeprom_write_string(addrPassword,password);
            delay(5);
            Serial.print(F("Mat khau da duoc thay doi. Mat khau moi la: "));
            Serial.println(password);
            //if (Feedback) sms.SendSMS(number,"Mat khau da duoc thay doi.");
          }
        }
        else{
          p=strstr(message,"RESET");
          //Serial.println(p);
          if (p) {
            resetFunc();
          }
          else{
            p=strstr(message,"TTRLMD");
            if (p) {
              DatTinhTrangMacDinh();
            }
            else {
              p=strstr(message,"TKGS");
              if (p) {
                Serial.print(F("Khoang thoi gian khong giam sat la: "));
                Serial.println(timenodetect);
              }
              else {
                p=strstr(message,"DKGS");
                if (p) {
                  
                  if (strlen(message)<17) {
                    Serial.println(F("Sai cu phap"));
                    //if (Feedback) sms.SendSMS(number,"Sai cu phap");
                  }
                  else {
                    p_char = strstr((char *)(message),"DKGS");
                    p_char1 = p_char+5; 
                    p_char = p_char1+8;
                    *p_char=0;
                    strcpy(timenodetect, (char *)(p_char1));
                    eeprom_write_string(addrNoDetect,timenodetect);
                    delay(5);
                    Serial.print(F("Khoang thoi gian khong giam sat la: "));
                    Serial.println(timenodetect);
                    //if (Feedback) sms.SendSMS(number,"Mat khau da duoc thay doi.");
                  }
                }
                else
                {
                  Serial.println(F("Sai cu phap"));
                  //if (Feedback) sms.SendSMS(number,"Sai cu phap");
                }
              }
            }
          }
        }
      }
    }
  }
  else
  {
    Serial.print(F("Ban khong co quyen thiet lap"));
    //if (Feedback) sms.SendSMS(number,"Ban khong co quyen thiet lap");
  }
}

void XuLySMSKTTaiKhoan(void)
{
  char resp[162];
  char cmd[8];
  resp[0]=0;
  cmd[0]=0;
  LayLenhUSSD(message,cmd,7);
  ChayLenhUSSD(cmd,resp,160);
  Serial.println(resp);
  sms.SendSMS(number,resp);
}

void XuLySMSNapTien(void)
{
  char resp[162];
  char cmd[22];
  resp[0]=0;
  cmd[0]=0;
  LayLenhUSSD(message,cmd,21);
  ChayLenhUSSD(cmd,resp,160);
  Serial.println(resp);
  sms.SendSMS(number,resp);
}

void DatTinhTrangMacDinh(void)
{
  char cmd[10];
  char tmp[2];
  cmd[0]=0;
  strcat(cmd,ALL);
  strcat(cmd,RL);
  strcat(cmd,SPC);
  strcat(cmd,ON);
  p=strstr(message,cmd);  //Mở tất cả role
  if (p) {
    for (byte i=0;i<=3;i++) {
      eeprom_write_string(addrSRL[i],"1");
      delay(5);
    }
  }
  else {
    cmd[0]=0;
    strcat(cmd,ALL);
    strcat(cmd,RL);
    strcat(cmd,SPC);
    strcat(cmd,OFF);
    p=strstr(message,cmd); //Tắt tất cả role
    if (p) {
      for (byte i=0;i<=3;i++) {
        eeprom_write_string(addrSRL[i],"0");
        delay(5);
      }
    }
    else {
      for (byte i=0;i<=3;i++) {
        cmd[0]=0;
        itoa(i+1,tmp,10);
        strcat(cmd,RL);
        strcat(cmd, tmp);
        strcat(cmd,SPC);
        strcat(cmd,ON);
        p=strstr(message,cmd);  //Mở từng role
        if(p) {
          eeprom_write_string(addrSRL[i],"1");
        }
        else {
          eeprom_write_string(addrSRL[i],"0");
        }
        delay(5);
      }
    }
  } 
}

void XuLySMSDongMoRoLe(void)
{
  p=strstr(message,password);
  if (p)
  {
    char cmd[10];
    bool bSendSMS;
    bSendSMS=false;
    char tmp[2];
    p=strstr(message,"TT"); //Lấy thông tin
    if(p)
    {
      bSendSMS=true;
    }
    else {
      cmd[0]=0;
      strcat(cmd,ALL);
      strcat(cmd,RL);
      strcat(cmd,SPC);
      strcat(cmd,ON);
      p=strstr(message,cmd);  //Mở tất cả role
      if (p) {
        for (byte i=0;i<=3;i++) {
          digitalWrite(ARL[i],HIGH);
          statusRL[i]=HIGH;
          bSendSMS=true;
          if (Memory){
            eeprom_write_string(addrSRL[i],"1");
            delay(5);
          }
        }
      }
      else {
        cmd[0]=0;
        strcat(cmd,ALL);
        strcat(cmd,RL);
        strcat(cmd,SPC);
        strcat(cmd,OFF);
        p=strstr(message,cmd); //Tắt tất cả role
        if (p) {
          for (byte i=0;i<=3;i++) {
            digitalWrite(ARL[i],LOW);
            statusRL[i]=LOW;
            bSendSMS=true;
            if (Memory) {
              eeprom_write_string(addrSRL[i],"0");
              delay(5);
            }
          }
        }
        else {
          for (byte i=0;i<=3;i++)
          {
            cmd[0]=0;
            itoa(i+1,tmp,10);
            strcat(cmd,RL);
            strcat(cmd, tmp);
            strcat(cmd,SPC);
            strcat(cmd,ON);
            p=strstr(message,cmd);  //Mở từng role
            if(p)
            {
              digitalWrite(ARL[i],HIGH);
              statusRL[i]=HIGH;
              bSendSMS=true;
              if (Memory){
                eeprom_write_string(addrSRL[i],"1");
                delay(5);
              }
            }
            else
            {
              cmd[0]=0;
              itoa(i+1,tmp,10);
              strcat(cmd,RL);
              strcat(cmd, tmp);
              strcat(cmd,SPC);
              strcat(cmd,OFF);
              p=strstr(message,cmd); //Tắt từng role
              if(p)
              {
                digitalWrite(ARL[i],LOW);
                statusRL[i]=LOW;
                bSendSMS=true;
                if (Memory)
                {
                  eeprom_write_string(addrSRL[i],"0");
                  delay(5);
                }
              } //Het 
            }
          }
        }
      } 
    }
    
    if (bSendSMS) 
    {
      XuLyTinhTrangRole();
    }
  }
  else
    Serial.println(F("Sai mat khau"));
}

void XuLyNutNhan(void)
{
  boolean bSendSMS=false;
  for (int i=0;i<=3;i++)
  {
    if (digitalRead(PU[i])==LOW)
    {
      //Serial.print(statusRL[i]);
      statusRL[i] = !statusRL[i];
      digitalWrite(ARL[i],statusRL[i]);
      bSendSMS=true;
      while (digitalRead(PU[i])==LOW){}
      if (Memory)
      {
        if (statusRL[i])
          eeprom_write_string(addrSRL[i],"1");
        else
          eeprom_write_string(addrSRL[i],"0");
        delay(5);
      }
    } 
  }
  if (bSendSMS) {
    XuLyTinhTrangRole();
  }
}

void XuLyTinhTrangRole(void)
{
  char resp[50];
  char tmp[2];
  resp[0]=0;
  strcat(resp,STA);
  strcat(resp,SPC);
  for (int i=0;i<=3;i++)
  {
    itoa(i+1,tmp,10);
    strcat(resp,RL);
    strcat(resp, tmp);
    strcat(resp,":");
    strcat(resp,SPC);
    if (statusRL[i])
      strcat(resp,ON);
    else
      strcat(resp,OFF);
    if (i==3)
      strcat(resp,".");
    else {
      strcat(resp,",");
      strcat(resp,SPC);
    }
  }
  //sms.SendSMS(number,resp);
  Serial.println(resp);
}
void XuLyCuocGoiDen(void)
{
  if ((call.CallStatusWithAuth(number,0,0)==CALL_INCOM_VOICE_AUTH) || (call.CallStatusWithAuth(number,0,0)==CALL_INCOM_VOICE_NOT_AUTH))  {
    digitalWrite(LEDCALLIN,HIGH);
    if (digitalRead(PUM)==LOW) {
      call.HangUp();  
      eeprom_write_string(addrMaster,number);
    }
      //else call.PickUp();
  }
  else {
    digitalWrite(LEDCALLIN,LOW);
    /*while ((call.CallStatus()==CALL_ACTIVE_VOICE)) {
      Serial.print("DTMF: ");
      char iNum=call.DetDTMF();
      Serial.println(iNum);
    }*/
  }
}

void ChayLenhUSSD(char *cmdUSSD,char *txtUSSD,byte max_USSD_len) 
{
  char *p_char; 
  char *p_char1;
  gsm.SimpleWrite(F("AT+CUSD=1,\""));
  gsm.SimpleWrite(cmdUSSD);  
  gsm.SimpleWriteln("\"");
  gsm.WaitResp(5000, 100, "+CUSD");
  gsm.WaitResp(4000, 100, "+CUSD");
  if (gsm.IsStringReceived("+CUSD")) {
    p_char = strchr((char *)(gsm.comm_buf),',');
    p_char1 = p_char+2; 
    p_char = strchr((char *)(p_char1),'"');
    if (p_char != NULL) {
      *p_char = 0; // end of string
      //strcpy(txtUSSD, (char *)(p_char1));
      byte len = strlen(p_char1);
      if (len < max_USSD_len) {
        strcpy(txtUSSD, (char *)(p_char1));
      }
      else {
        memcpy(txtUSSD, (char *)(p_char1), (max_USSD_len-1));
        txtUSSD[max_USSD_len] = 0; // finish string
      }
    }
  }
}

void LayLenhUSSD(char *strChar, char *strReturn,byte max_Return_len)
{
 char *p_char; 
 char *p_char1;
 p_char = strchr((char *)(strChar),'*');
 p_char1 = p_char; 
 p_char = strchr((char *)(p_char1),'#')+1;
 if (p_char != NULL) {
   *p_char = 0; // end of string
    //strcpy(strReturn, (char *)(p_char1));
    byte len = strlen(p_char1);
    if (len < max_Return_len) {
      strcpy(strReturn, (char *)(p_char1));
    }
    else {
      memcpy(strReturn, (char *)(p_char1), (max_Return_len-1));
      strReturn[max_Return_len] = 0; // finish string
    }
  }
}

void parse_cmd(char *cmd, int cmdsize)
{
    //uint8_t i;
    //uint8_t time[8];
    uint8_t reg_val;
    char buff[BUFF_MAX];

    //snprintf(buff, BUFF_MAX, "cmd was '%s' %d\n", cmd, cmdsize);
    //Serial.print(buff);

    // TssmmhhWDDMMYYYY aka set time
    if (cmd[0] == 84 && cmdsize == 16) {
        struct ts t;
        //T355720619112011
        t.sec = inp2toi(cmd, 1);
        t.min = inp2toi(cmd, 3);
        t.hour = inp2toi(cmd, 5);
        t.wday = cmd[7] - 48;
        t.mday = inp2toi(cmd, 8);
        t.mon = inp2toi(cmd, 10);
        t.year = inp2toi(cmd, 12) * 100 + inp2toi(cmd, 14);
        DS3231_set(t);
        Serial.println(F("OK"));
    /*} else if (cmd[0] == 49 && cmdsize == 1) {  // "1" get alarm 1
        DS3231_get_a1(&buff[0], 59);
        Serial.println(buff);
    } else if (cmd[0] == 50 && cmdsize == 1) {  // "2" get alarm 1
        DS3231_get_a2(&buff[0], 59);
        Serial.println(buff);
    } else if (cmd[0] == 51 && cmdsize == 1) {  // "3" get aging register
        Serial.print(F("aging reg is "));
        Serial.println(DS3231_get_aging(), DEC);
    } else if (cmd[0] == 65 && cmdsize == 9) {  // "A" set alarm 1
        DS3231_set_creg(DS3231_INTCN | DS3231_A1IE);
        //ASSMMHHDD
        for (i = 0; i < 4; i++) {
            time[i] = (cmd[2 * i + 1] - 48) * 10 + cmd[2 * i + 2] - 48; // ss, mm, hh, dd
        }
        uint8_t flags[5] = { 0, 0, 0, 0, 0 };
        DS3231_set_a1(time[0], time[1], time[2], time[3], flags);
        DS3231_get_a1(&buff[0], 59);
        Serial.println(buff);
    } else if (cmd[0] == 66 && cmdsize == 7) {  // "B" Set Alarm 2
        DS3231_set_creg(DS3231_INTCN | DS3231_A2IE);
        //BMMHHDD
        for (i = 0; i < 4; i++) {
            time[i] = (cmd[2 * i + 1] - 48) * 10 + cmd[2 * i + 2] - 48; // mm, hh, dd
        }
        uint8_t flags[5] = { 0, 0, 0, 0 };
        DS3231_set_a2(time[0], time[1], time[2], flags);
        DS3231_get_a2(&buff[0], 59);
        Serial.println(buff);*/
    } else if (cmd[0] == 67 && cmdsize == 1) {  // "C" - get temperature register
        Serial.print(F("Nhiet do: "));
        Serial.println(DS3231_get_treg(), 2);
    /*} else if (cmd[0] == 68 && cmdsize == 1) {  // "D" - reset status register alarm flags
        reg_val = DS3231_get_sreg();
        reg_val &= B11111100;
        DS3231_set_sreg(reg_val);
    } else if (cmd[0] == 70 && cmdsize == 1) {  // "F" - custom fct
        reg_val = DS3231_get_addr(0x5);
        Serial.print(F("orig "));
        Serial.print(reg_val,DEC);
        Serial.print(F("month is "));
        Serial.println(bcdtodec(reg_val & 0x1F),DEC);
    } else if (cmd[0] == 71 && cmdsize == 1) {  // "G" - set aging status register
        DS3231_set_aging(0);
    } else if (cmd[0] == 83 && cmdsize == 1) {  // "S" - get status register
        Serial.print(F("status reg is "));
        Serial.println(DS3231_get_sreg(), DEC);*/
    } else {
        Serial.print(F("Khong dung cu phap "));
        Serial.println(cmd[0]);
        Serial.println(cmd[0], DEC);
    }
}


