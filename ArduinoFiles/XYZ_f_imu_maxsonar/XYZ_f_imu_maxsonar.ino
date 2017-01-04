
/*
 * TODO:    
*/
/* Board layout:
         +----------+
         |         *| RST   PITCH  ROLL  HEADING
     ADR |*        *| SCL
     INT |*        *| SDA     ^            /->
     PS1 |*        *| GND     |            |
     PS0 |*        *| 3VO     Y    Z-->    \-X
         |         *| VIN
         +----------+
  */
/*
 *  Uses the imu to gather orientation and motion flow camera to gather location along with the processing demo
 */

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>
#include <ADNS3080.h>
#include <myMaxSonar.h>
#include <DroneCom.h>

bool FirstRunBNO = false; // reset droll, dpitch, dyaw, values
bool FirstRunADNS = false; // reset d*_cam values
bool FirstRunL = false; // reset d*_cm, d*_in values


double ct = 0.00;
double pt = 0.00;
double dt = 0.00;

double roll = 0.00, pitch = 0.00, yaw = 0.00;
double droll = 0.00, dpitch = 0.00, dyaw = 0.00;
double proll = 0.00, ppitch = 0.00, pyaw = 0.00;
double x_cm = 0.00, y_cm = 0.00;    // 

double x_in = 0.00, y_in = 0.00;
double ix_in = 0.00, iy_in = 0.00;

// for motion flow cam
int32_t x_cam = 0.00;
int32_t y_cam = 0.00;
double x_cam_comp = 0.00;
double y_cam_comp = 0.00;
int8_t dx_cam = 0.00;
int8_t dy_cam = 0.00;
int8_t quality = 0.00;

/* Set the delay between fresh samples */
#define BNO055_SAMPLERATE_DELAY_MS (100)
Adafruit_BNO055 bno = Adafruit_BNO055(55);

myMaxSonar sonar;


/*
 *  FULL RUNNING LOOP TO BE CALLED IN VOID LOOP()
 *  Calibration of variables function called in void setup()
 */
void fullRun();
void partRun();

/*
 *  BNO055 IMU functions
 */
void displaySensorDetails(void);
void printOrientation(sensors_event_t &event);

/*
 *  ADNS3080 object
 */

myADNS3080 moFlow;// lol


void setup()
{                   
                                  
  Serial.begin(115200);
  Serial.println("Orientation Sensor Test"); Serial.println("");
  /******************************************************************************
   * THE IMU SENSOR SETUP
   *****************************************************************************/
  if(!bno.begin())
  {
    Serial.print("Ooops, no BNO055 detected ... Check your wiring or I2C ADDR!");
    while(1);
  }
  delay(1000);
  sensor_t sensor;
  bno.getSensor(&sensor);
  displaySensorDetails();
  /******************************************************************************
   * THE OPTICAL FLOW SENSOR SETUP
   *****************************************************************************/
  moFlow.setup();
  
  /******************************************************************************
   * THE MaxSonar SENSOR SETUP
   *****************************************************************************/


  /*
   * Run the function once to make sure it starts at zero position
   */
   Serial.println("Calibrating initical conditions");
   partRun();
   Serial.println("Initial conditions set");
  
  
}

void loop()
{
  fullRun();
}
void partRun()
{
  sensors_event_t event;
  bno.getEvent(&event);
  
  // calculating change in time since last loop
  pt = ct;
  ct = millis();
  dt = ct-pt;

  // calculating change in angles since last loop
  proll = roll;
  ppitch = pitch;
  pyaw = yaw;
  roll = event.orientation.z;
  pitch = event.orientation.y;
  yaw = event.orientation.x;
  droll = roll - proll;
  dpitch = pitch - ppitch;
  dyaw = yaw - pyaw;

  // updating  S by updating altitude and angles from imu
  sonar.updateLocation(roll, pitch, droll, dpitch);

  // updataing motion from location
  moFlow.updateLocation();
  
  x_cam = moFlow.getY() * sonar.alt_cm * moFlow.conv_factor;
  y_cam = moFlow.getX() * sonar.alt_cm * moFlow.conv_factor;
  dx_cam = moFlow.getDY() * sonar.alt_cm * moFlow.conv_factor;
  dy_cam = moFlow.getDX() * sonar.alt_cm * moFlow.conv_factor;
  
  //x_cam_comp = x_cam + ix_cm;
  //x_cam_comp = x_cam + ix_cm;
  //x_cam += dx_cam;

  x_cam = 0.00;
  y_cam = 0.00;
  x_cam_comp = 0.00;
  y_cam_comp = 0.00;
  dx_cam = 0.00;
  dy_cam = 0.00;
  sonar.ix_cm = 0.00;
  sonar.iy_cm = 0.00;
 
  droll = 0.00;
  dpitch = 0.00;
  dyaw = 0.00;
  
}
void fullRun()
{
  /* Get a new sensor event */
  sensors_event_t event;
  bno.getEvent(&event);
  
  // calculating change in time since last loop
  pt = ct;
  ct = millis();
  dt = ct-pt;

  // calculating change in angles since last loop
  proll = roll;
  ppitch = pitch;
  pyaw = yaw;
  roll = event.orientation.z;
  pitch = event.orientation.y;
  yaw = event.orientation.x;
  droll = roll - proll;
  dpitch = pitch - ppitch;
  dyaw = yaw - pyaw;

  // updating  S by updating altitude and angles from imu
  sonar.updateLocation(roll, pitch, droll, dpitch);

  // updataing motion from location
  moFlow.updateLocation();
  
  // this will set the integrated x and y from motionflow cam
  x_cam = (moFlow.getX()*sonar.alt_cm)*moFlow.conv_factor;//*moFlow.conv_factor*alt_cm;
  y_cam = (moFlow.getY()*sonar.alt_cm)*moFlow.conv_factor;

  // this will set the dif X and Y from motionflow cam
  dx_cam = (moFlow.getDX()*sonar.alt_cm)*moFlow.conv_factor;//*moFlow.conv_factor*alt_cm;
  dy_cam = (moFlow.getDY()*sonar.alt_cm)*moFlow.conv_factor;
  //x_cam_comp += (dx_cam + dx_cm);//

  // see if there are any changes on the cam
  if(abs(dx_cam) >= 0)
  {
    x_cam_comp = x_cam - sonar.ix_cm;
  }
  else
  {
    x_cam_comp = x_cam;
  }

  if(abs(dy_cam) >= 0)
  {
    y_cam_comp = y_cam + sonar.iy_cm;
  }
  else
  {
    y_cam_comp = y_cam;
  }
  //x_cam += dx_cam;
  // can be part to sonar class
  x_in = cm2in(sonar.x_cm);
  y_in = cm2in(sonar.y_cm);
  ix_in = cm2in(sonar.ix_cm);
  iy_in = cm2in(sonar.iy_cm);
  
  // iX/Y come from IMU and sonar 
  // X/Y_cam are retrieved from camera
  // X/Y_cam_comp uses X/Y_cam and iX/Y
  
  // output drone location
  printLocation(moFlow.getSurfaceQuality(), x_cm, y_cm, sonar.alt_cm);
  // ouput drone orientation
  printOrientation(roll, pitch, yaw);
}

/*
 * Sonar Functions
 */






