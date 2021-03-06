/****************************************引脚定义****************************************/
//SG90舵机
#define Servo_PWM1 9    // SG90橙色线 ←→ Arduino 9
#define Servo_PWM2 5    // SG90橙色线 ←→ Arduino 5

//L298N直流电机驱动器
#define DC_Pin1 11   // L298N IN1 ←→ Arduino ~11(T/C2),左前
#define DC_Pin2 10   // L298N IN2 ←→ Arduino  10      左后
#define DC_Pin3 3   // L298N IN3 ←→ Arduino  ~3 (T/C2)右前
#define DC_Pin4 2   // L298N IN4 ←→ Arduino   2       右后

//红外循迹
#define IR0 A0  // 红外对管0 OUT ←→ A0
#define IR1 A1  // 红外对管1 OUT ←→ A1
#define IR2 A2  // 红外对管2 OUT ←→ A2
#define IR3 A3  // 红外对管3 OUT ←→ A3

//HC SR04超声波测距模块
#define SR_Trig 7       // HC SR04 Trig ←→ Arduino 7
#define SR_Echo 6       // HC SR04 Echo ←→ Arduino 6
/****************************************库函数和公共变量定义****************************************/

#include <Servo.h> // 引用舵机库
Servo SG901;  // 动态调用舵机类
Servo SG902;  

const int IR_TH = 800;  // 红外循迹模块阈值，大于此值视为高电平，小于此值视为低电平 -------------------- 待调参数
int IR_Value[10] = {0};  // 定义 红外循迹模块读取值
int IR_STA = 0;         // 定义 红外循迹状态指示符
double SR_Distance = 0;    // 定义 超声波测量距离
int Servo_Angle = 0;  // 定义 舵机角度位置

/****************************************红外循迹(IR)相关函数****************************************/

void IR_Init()  //红外循迹模块初始化
{
  pinMode(IR0,INPUT);
  pinMode(IR1,INPUT);
  pinMode(IR2,INPUT);
  pinMode(IR3,INPUT);
}

int IR_Read()
{
  int State = 0;
  IR_Value[0] =  analogRead(IR0);
  IR_Value[1] =  analogRead(IR1);
  IR_Value[2] =  analogRead(IR2);
  IR_Value[3] =  analogRead(IR3);
  for (int i = 0; i < 4; i++)
  {
    if (IR_Value[i] > IR_TH) State += 1 << i;
  }
  return State;
}


/****************************************超声波测距(SR)相关函数****************************************/

void SR_Init()//超声波初始化函数
{
  pinMode(SR_Trig, OUTPUT); // Trig ←→ Output
  pinMode(SR_Echo, INPUT);  // Echo ←→ Input
}

double SR_Read()//超声波模块测距函数
{
  unsigned long Duration; // 定义脉持续时间Duration(单位: us)
  double Distance;           // 定义距离Distance(单位: mm)

  //发送高电平脉冲，启动测量
  digitalWrite(SR_Trig, LOW);
  delayMicroseconds(2);
  digitalWrite(SR_Trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(SR_Trig, LOW);

  // 测量超声波往返时间和计算距离
  Duration = pulseIn(SR_Echo, HIGH);  // 测量超声波往返时间(单位: us)
  Distance = (double)Duration * 171 / 10000; // 计算距离(单位: cm)
  Serial.print("Distance:");
  Serial.println(Distance);
  return Distance;
}

/****************************************舵机控制(Servo)相关函数****************************************/
void Servo_Init()   // 舵机初始化
{
  SG901.attach(Servo_PWM1);  // 舵机信号的IO口 = Servo_PWM1
  SG901.write(90);
  SG902.attach(Servo_PWM2);  // 舵机信号的IO口 = Servo_PWM2
  SG902.write(90);
}

/****************************************直流电机(DCMotor)相关函数****************************************/
void DCMotor_Init()//直流电机初始化
{
  //规定IO口工作模式
  pinMode(DC_Pin1, OUTPUT);
  pinMode(DC_Pin2, OUTPUT);
  pinMode(DC_Pin3, OUTPUT);
  pinMode(DC_Pin4, OUTPUT);

  //预置高电平，使电机保持不动。
  digitalWrite(DC_Pin1, HIGH);
  digitalWrite(DC_Pin2, HIGH);
  digitalWrite(DC_Pin3, HIGH);
  digitalWrite(DC_Pin4, HIGH);
}

void DCMotor_Move(int d1, int d2) //电机运动函数，d1、d2范围-255~255，负号代表反转
{
  if (d1 >= 0) //电机1 PWM调速
  {
    analogWrite(DC_Pin1, d1);
    digitalWrite(DC_Pin2, LOW);
  }
  else
  {
    analogWrite(DC_Pin1, 255 + d1);
    digitalWrite(DC_Pin2, HIGH);
  }

  if (d2 >= 0) // 电机2 PWM调速
  {
    analogWrite(DC_Pin3, d2);
    digitalWrite(DC_Pin4, LOW);
  }
  else
  {
    analogWrite(DC_Pin3, 255 + d2);
    digitalWrite(DC_Pin4, HIGH);
  }
}

void setup()
{
  Serial.begin(9600);
  IR_Init();           // 红外测距模块初始化
  SR_Init();           // 超声波模块初始化
  Servo_Init();        // 舵机初始化
  DCMotor_Init();      // 直流电机初始化
}

void line_tracking(){
    int IR_total=IR_Read();
    Serial.println(IR_total);
    switch (IR_total){
       case 0: { DCMotor_Move(255,255); break; }   //0000
       case 1: { DCMotor_Move(120,255); break; }   //0001
       case 3: { DCMotor_Move(120,255); break; }   //0011
       case 6: { DCMotor_Move(255,255); break; }   //1001
       case 7: { DCMotor_Move(120,255); break; }   //0111
       case 8: { DCMotor_Move(255,120); break; }   //1000
       case 12: { DCMotor_Move(255,255); break; }  //1100
       case 14: { DCMotor_Move(255,120); break; }  //1110
       default:
          break;
    }
}

void BTControl()
{
  uint8_t dir;
  while(1)
  {
    dir=Serial.read();
    
    while(dir=0x03)  //forward
    {
      DCMotor_Move(255,255);
      
      if(Serial.read()==0x02)
      {
        DCMotor_Move(0,0);
        break;
      }
    }
    
    while(dir==0x09)  //right
    {
      DCMotor_Move(255,-255);
      if(Serial.read()==0x08)
      {
        DCMotor_Move(0,0);
        break;
      }
    }
    
    while(dir==0x07) //left
    {
      DCMotor_Move(-255,255);
      if(Serial.read()==0x06)
      {
        DCMotor_Move(0,0);
        break;
      }
    }
    
    while(dir==0x05)  //back
    {
      DCMotor_Move(-255,-255);
      if(Serial.read()==0x04)
      {
        DCMotor_Move(0,0);
        break;
      }
    }

    if(dir==0x0F)
    {
      break;
    }
  }
}
double amplitudeRestrict()
{
  double dis=0;
  dis=SR_Read();
  while(dis>1000.0)
  {
    dis=SR_Read();
  }
  if(dis<=1000.0)
  {
    return dis;
  }
}
/*void turn90(bool LR)
{
  switch(LR)
  {
    case 1 :
      DCMotor_Move();//Right
      delay(); //delay一个适当时间
      break;
    case 0 :
      DCMotor_Move();//Left
      delay();
      break;
  }
}
bool servoRotate()
{
  double disL=0, disR=0;
  //舵机左转90
  disL=amplitudeRestrict();
  //舵机转180到右侧
  disR=amplitudeRestrict();
  //舵机归位
  return disR > disL ? 1 : 0 ;
}

void Avoidance()
{
  double di=0;
  di=amplitudeRestrict();
  bool LorR=0;
  if(di<20.0)
  {
    DCMotor_Move(0,0);
    LorR=servoRotate();
    turn90(LorR);
  }
}*/
void loop()
{
   // static uint8_t i='a';
   
   // i=Serial.read();

     // a=Serial.read();
    /*  while(1)
      {
        //循迹
        line_tracking();
        if(Serial.read()==0x01)
          break;
      }
      */
        
        //遥控
        //BTControl();
        
      
        //迷宫
      // Avoidance();
       
        double dis;
        dis=SR_Read();
        Serial.println(dis);
        delay(100);
        DCMotor_Move(0,0);
   
    
}

