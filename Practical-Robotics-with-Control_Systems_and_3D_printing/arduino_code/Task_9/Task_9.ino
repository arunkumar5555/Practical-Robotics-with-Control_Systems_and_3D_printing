
//  http://www-personal.umich.edu/~johannb/position.htm book link

#include <WiFi.h>
#include <WiFiUdp.h>
#include <PID_v1.h>
const char*  ssid = "LuqmanZGeh";
const char*  password = "qwertyuiop";
const char*  udpAddress = "192.168.43.255";
unsigned int localPort_ = 9999;
WiFiUDP udp;
IPAddress ipServer(192, 168, 43, 225); 
IPAddress ipClient(192, 168, 43, 226);
IPAddress Subnet(255, 255, 255, 0);


// motor pins
#define enc_RA  5
#define enc_RB  18
#define enc_LA  17
#define enc_LB  16
#define motorRa 27
#define motorRb 14
#define motorLa 25
#define motorLb 26
#define Rpwm    12
#define Lpwm    33

//Fixed values
#define WHEEL_DIAMETER 0.067
#define PULSES_PER_REVOLUTION 757.0
#define AXLE_LENGTH  0.129
#define PI 3.1416
int RIGHT_BASE_SPEED =140  ;
int LEFT_BASE_SPEED= 140 ;

// New Variables
double Setpoint, angle_to_goal, distance_to_goal,des_x,des_y;
float goal_x             = 1;
float goal_y             = 1;
double kp = 10; 
double ki = 0;
double kd = 0; 
double Output;
float error;
bool goal_reached;
// Old Variables
float disp_r_wheel       = 0;                          // X Y co ordinate variables
float disp_l_wheel       = 0;
int count_R_prev         = 0;
int count_L_prev         = 0;             
float x                  = 0;
float y                  = 0;
double theta              = 0;
float meter_per_ticks    = PI * WHEEL_DIAMETER / PULSES_PER_REVOLUTION;
float orientation_angle , disp_body;
int count_R              = 0;                          //For Encoders
int count_L              = 0;  
const int channel_R      = 0;                          //PWM setup
const int channel_L      = 1;   
char packetBuffer[255];                                        //UDP variables
char packet[8];
PID GTG_PID(&theta, &Output, &angle_to_goal, kp, ki, kd, DIRECT);
void setup()
{ 
  Serial.begin(115200);
  setup_motors();                                      
  wifi_def();                                         // Begin the udp communication
  GTG_PID.SetMode(AUTOMATIC);
  GTG_PID.SetOutputLimits(-200,+200);
  Start_Robot();
} 


void loop()
{   
  Udp_Packet_Check();
  if(!goal_reached){
  calculate_traveling();
  Go_to_Goal_Calculations();         
  Update_Motors_Speeds();
  send_xy_over_Wifi();                               
  }else Serial.println("Reached Goal");
}

void send_xy_over_Wifi(){
  udp.beginPacket(ipServer, localPort_);
  dtostrf(x, 5,2, packet); 
  udp.write(packet[0]); //uint8_t
  udp.write(packet[1]);
  udp.write(packet[2]);
  udp.write(packet[3]);
  udp.write(packet[4]);
  udp.printf(" / ");
  dtostrf(y, 5,2, packet);
  udp.write(packet[0]);
  udp.write(packet[1]);
  udp.write(packet[2]);
  udp.write(packet[3]);
  udp.write(packet[4]);

  
  udp.endPacket();
 
}

void calculate_traveling(){
      
    count_L_prev = count_L;
    count_R_prev = count_R;
    count_L      = 0;
    count_R      = 0;
    disp_l_wheel = (float)count_L_prev * meter_per_ticks;              // geting distance in meters each wheel has traveled
    disp_r_wheel = (float)count_R_prev * meter_per_ticks;
    
    if (count_L_prev == count_R_prev)
    {                                                                  // The Straight line condition -> book reference Where am i ?
      x += disp_l_wheel * cos(theta);
      y += disp_l_wheel * sin(theta);
    }
    else                                                               // for circular arc equations change
    { orientation_angle = (disp_r_wheel - disp_l_wheel)/AXLE_LENGTH;
      disp_body   = (disp_r_wheel + disp_l_wheel) / 2.0;
      x += (disp_body/orientation_angle) * (sin(orientation_angle + theta) - sin(theta));
      y -= (disp_body/orientation_angle) * (cos(orientation_angle + theta) - cos(theta));
      theta += orientation_angle;

       //theta=constrain(theta,-PI,PI); -> it will result into wrong results
       while(theta > PI)
         theta -= (2.0*PI);
       while(theta < -PI) 
         theta += (2.0*PI); 
    }
} 

void Go_to_Goal_Calculations(){
  des_x = goal_x - x;
  des_y = goal_y - y;
  angle_to_goal = atan2(des_y, des_x);
  distance_to_goal  = sqrt(pow(des_x, 2) + pow(des_y, 2)); // distance to goal 
  error = angle_to_goal - theta;
  Serial.print("Desired Remaining X Y   -----> ");Serial.print(des_x);Serial.print(" |||  ");Serial.println(des_y);
//  Serial.print("Angle/Distance  to goal -----> ");Serial.print(angle_to_goal);Serial.print(" |||  ");Serial.println(distance_to_goal);
     
}
  
void Update_Motors_Speeds() {
//      if(error < 0.3 ){ // Slow Down the Robot 
//        RIGHT_BASE_SPEED=100;
//        LEFT_BASE_SPEED=100;
//        
          if(des_x < 0.03 && des_y < 0.03){ // stopping condition
      Stop_Robot();
      goal_reached=true;
    
  }

 GTG_PID.Compute();
  int  rightMotorSpeed = RIGHT_BASE_SPEED + Output ;
  int   leftMotorSpeed = LEFT_BASE_SPEED -  Output  ;
   Serial.print("OUTPUT PID            -----> ");Serial.println(Output);
  // Serial.print("error ---> ");Serial.println(error);
  
  rightMotorSpeed = constrain(rightMotorSpeed, 90, 200);
  leftMotorSpeed  = constrain(leftMotorSpeed, 90, 200);
//  Serial.print(leftMotorSpeed);Serial.print(" / ");Serial.println(rightMotorSpeed);

  ledcWrite(channel_R, rightMotorSpeed);  
  ledcWrite(channel_L, leftMotorSpeed);




}
void reset(){goal_reached=false;  x=0;y=0;
}

void Udp_Packet_Check(){
  if(udp.parsePacket() ){                               // If we recieve any packet
    reset();
    udp.read(packetBuffer, 255);
    Stop_Robot();
    char * strtokIndx;
    strtokIndx = strtok(packetBuffer,",");
    kp = atof(strtokIndx); 
    strtokIndx = strtok(NULL, ",");
    ki = atof(strtokIndx);
    strtokIndx = strtok(NULL, ",");
    kd= atof(strtokIndx);
    strtokIndx = strtok(NULL, ",");
    goal_x = atof(strtokIndx);
    strtokIndx = strtok(NULL, ",");
    goal_y= atof(strtokIndx);
    
    Serial.print("\n\n\nKP / Ki / KD =");Serial.print(kp);Serial.print("/");Serial.print(ki);Serial.print(" / ");Serial.println(kd);
    Serial.print("New goal -> X // Y ");Serial.print(goal_x);Serial.print(" // ");Serial.print(goal_y);
    Serial.println("\n\n\nGET Ready !!");
    
    delay (3000);
    Start_Robot();                     
    
  
  }
}

// Encoders-Interrupt callback functions
void Update_encR(){
   if (digitalRead(enc_RA) == digitalRead(enc_RB)) count_R--;
    else count_R++;  
}

void Update_encL(){
 if (digitalRead(enc_LA) == digitalRead(enc_LB)) count_L--;
  else count_L++; 
}

// all motor setup is done in one function call
void setup_motors(){
  // pwm setup variables
  const int freq = 5000;
  const int res = 8;

  // direction for motor pinout defination
  pinMode(motorLa, OUTPUT);
  pinMode(motorLb, OUTPUT);
  pinMode(motorRa, OUTPUT);
  pinMode(motorRb, OUTPUT);
  // encoder pinout defination
  pinMode(enc_RA,INPUT);
  pinMode(enc_RB,INPUT);
  pinMode(enc_LA,INPUT);
  pinMode(enc_LB,INPUT);
  // Interrupt connection to gpio pins and defining interrupt case
  attachInterrupt(digitalPinToInterrupt(enc_RA),Update_encR,CHANGE);
  attachInterrupt(digitalPinToInterrupt(enc_LA),Update_encL,CHANGE);
  // Pwm functionality setup
  ledcSetup(channel_R ,freq , res);
  ledcSetup(channel_L ,freq , res);
  ledcAttachPin(Rpwm,channel_R);
  ledcAttachPin(Lpwm,channel_L);

}


void Stop_Robot(){
  digitalWrite(motorLa,LOW);
  digitalWrite(motorRa,LOW);
  digitalWrite(motorLb,LOW);
  digitalWrite(motorRb,LOW);
  ledcWrite(channel_R , 0);                             
  ledcWrite(channel_L , 0);
}

void Start_Robot(){
  digitalWrite(motorLa,HIGH);
  digitalWrite(motorRa,HIGH);
  digitalWrite(motorLb,LOW);
  digitalWrite(motorRb,LOW);
}

void wifi_def(){
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {          // Exit only when connected
    delay(500);
    Serial.print(".");}
  
  Serial.print("\nConnected to ");Serial.println(ssid);
  Serial.print("IP address: ");Serial.println(WiFi.localIP());
  delay(3000);
  udp.begin(localPort_);                            // Begin the udp communication
}
