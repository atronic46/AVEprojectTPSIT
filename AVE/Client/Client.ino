#include <EEPROM.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
//
//      CLIENT
//
#include "Sensor.h"

#define     Monitor
//#define     MonitorTim
//#define     MonitorT
//#define     MonitorH
//#define     MonitorHR
//#define     PlotterHR
//#define     PlotterI

#define     LedR  25
#define     LedV  26

#define     RomB  1024        // EEPRom Byte used to store the WiFi Net known in JSON format (max as jsDocRx)

char        comRxB[RomB];
int         comInd;

// mqtt Broker
#define     dly   250

//const char* mqttServer = "broker.mqttdashboard.com";
const char* mqttServer = "test.mosquitto.org";
const int   mqttPort = 1883;
const char* mqttUser = "";
const char* mqttPassword = "";

WiFiClient  espClient;
PubSubClient client(espClient);

char        mqttClient[18];   // MAC Address: 'xx:xx:xx:xx:xx:xx' + end string (18)
char        mqttTopic[22];    // Subscribe to: 'ave_xx:xx:xx:xx:xx:xx' + end string (22)
bool        mqttOk = false;

char        readSensor = 0;   // Sensor will be read
char        readTopic[22];    // Topic to send the reading

// Json documents
StaticJsonDocument<256>   jsDocTx;
StaticJsonDocument<1024>  jsDocRx;

// Sensors on device
s_sensor    sensor[maxSen];
int         numSen;

// Time
bool        timIni;
byte        timSS = 0;
byte        timMM = 0;
byte        timHH = 0;

// Timer (software)
int         timMil = 0;
int         timSec = 0;

//----------------------------------------------------//
//      setup: Device setup                           //
//----------------------------------------------------//

void setup()
{
  pinMode(LedR, OUTPUT);
  pinMode(LedV, OUTPUT);

  Serial.begin(115200);
  
#ifdef Monitor
  // Print physical Address
  Serial.println();
  Serial.print("MAC address: ");
  Serial.println(WiFi.macAddress());
#endif

  // Start by connecting to a WiFi network
  WiFiConnect();

#ifdef Monitor
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
#endif

  // Set mqtt server, port, receiving service routine and buffer size (default 256 byte)
  client.setServer(mqttServer, mqttPort);
  client.setCallback(Callback);
  client.setBufferSize(512);

  // Connect with MAC Address: 'xx:xx:xx:xx:xx:xx'
  WiFi.macAddress().toCharArray(mqttClient, sizeof(mqttClient));
  mqttConnect();

  // Subscribe to: 'ave_xx:xx:xx:xx:xx:xx'
  ("ave_" + WiFi.macAddress()).toCharArray(mqttTopic, sizeof(mqttTopic));
  client.subscribe(mqttTopic);

#ifdef Monitor
  Serial.print("Subscribe to: ");
  Serial.println(mqttTopic);
#endif

  // Send to server the clientId and topic (connect with MAC Address)
  JsonObject  jsObj0 = jsDocTx.to<JsonObject>();
  jsObj0["msgType"] = "Boot";
  JsonObject  jsObj1 = jsDocTx.createNestedObject("msgData");
  jsObj1["client"] = mqttClient;
  jsObj1["topic"] = mqttTopic;

  mqttPublish("ave_Server");
}

//----------------------------------------------------//
//      CallBack: Called when receive message         //
//----------------------------------------------------//

void Callback(char* topic, byte* payload, unsigned int length)
{
  char        jsMsg[length + 1];
  for (int ind = 0; ind < length; ind++)
  {
    jsMsg[ind] = (char)payload[ind];
    jsMsg[ind+1] = '\0';
  }

#ifdef Monitor
  Serial.println("--------------------------------");
  Serial.print("Topic: ");
  Serial.println(topic);
  Serial.print("Message: ");
  Serial.println(jsMsg);
#endif

  deserializeJson(jsDocRx, jsMsg);
  JsonObject  jsObj0 = jsDocRx.as<JsonObject>();
  JsonObject  jsObj1 = jsObj0["msgData"];

#ifdef Monitor
  Serial.print("Message type: ");
  Serial.println((const char*)jsObj0["msgType"]);
  Serial.print("Message data: ");
  serializeJson(jsObj1, Serial);
  Serial.println();
#endif

  // Decode json message
  if(jsObj0["msgType"] == "Boot")
  {
       // Unsubscribe to topic-MAC Address and disconnect from mqtt broker
      client.unsubscribe((const char*)topic);
      client.disconnect();

#ifdef Monitor      
      Serial.print("Unsubscribe to: ");
      Serial.println(topic);
#endif

      MessageBoot(jsObj1);
  }
  else if (jsObj0["msgType"] == "Device")
    MessageDevice(jsObj1);
  else if (jsObj0["msgType"] == "Config")
    MessageConfig(jsObj1);
  else if (jsObj0["msgType"] == "Range")
    MessageRange(jsObj1);
  else if (jsObj0["msgType"] == "Time")
    MessageTime(jsObj1);
  else if (jsObj0["msgType"] == "Sensor")
    MessageSensor(jsObj1);
}

//----------------------------------------------------//
//      MessageBoot: decode message Boot              //
//----------------------------------------------------//

void MessageBoot(JsonObject jsObj)
{
  String((const char*)jsObj["client"]).toCharArray(mqttClient, sizeof(mqttClient));
  String((const char*)jsObj["topic"]).toCharArray(mqttTopic, sizeof(mqttTopic));

  // Connect with new mqttClient
  mqttConnect();

  // Subscribe to new mqttTopic
  client.subscribe(mqttTopic);

#ifdef Monitor   
  Serial.print("Subscribe to: ");
  Serial.println(mqttTopic);
#endif

  // Now is ready for communicating (over mqtt)
  mqttOk = true;
  numSen = 0;
  timIni = false;

  // Send to server the client identifier and sensor number (to be config)
  JsonObject  jsObj0 = jsDocTx.to<JsonObject>();
  jsObj0["msgType"] = "Device";
  JsonObject  jsObj1 = jsDocTx.createNestedObject("msgData");
  jsObj1["client"] = mqttClient;
  jsObj1["sensors"] = numSen;

  mqttPublish("ave_Server");
}

//----------------------------------------------------//
//      MessageDevice: decode message Device          //
//----------------------------------------------------//

void MessageDevice(JsonObject jsObj)
{
  numSen = jsObj["sensors"];

  // Delete all sensors
  for (int sns = 0; sns < numSen; sns++)
  {
    sensor[sns].cfg.enable = false;
    sensor[sns].cfg.sensor = 0;
  }

  CompleteConfig();
}

//----------------------------------------------------//
//      MessageConfig: decode message Config sensor   //
//----------------------------------------------------//

void MessageConfig(JsonObject jsObj)
{
  int         sns = (int)jsObj["number"];

  // Configure the sensor (only first time all the parameters: config - range)
  if (jsObj.containsKey("sensor"))
  {
    if (jsObj["sensor"] == "Temperature")     sensor[sns].cfg.sensor = 'T';
    else if (jsObj["sensor"] == "Humidity")   sensor[sns].cfg.sensor = 'H';
    else if (jsObj["sensor"] == "Heart Rate") sensor[sns].cfg.sensor = 'R';
    else if (jsObj["sensor"] == "Nothing")    sensor[sns].cfg.sensor = ' ';
  }
  if (jsObj.containsKey("input"))   { sensor[sns].cfg.input = (byte)jsObj["input"]; 
                         adcAttachPin(sensor[sns].cfg.input); }
  if (jsObj.containsKey("inhibitor")) sensor[sns].cfg.inhibitor = (byte)((short)jsObj["inhibitor"]);
  if (jsObj.containsKey("function"))  sensor[sns].cfg.function = String((const char*)jsObj["function"]).charAt(0);
  if (jsObj.containsKey("average"))   sensor[sns].cfg.average = (byte)jsObj["average"];
  if (jsObj.containsKey("sample"))    sensor[sns].cfg.sample = (short)jsObj["sample"];
  if (jsObj.containsKey("pointMin"))  sensor[sns].cfg.pointMin = (short)jsObj["pointMin"];
  if (jsObj.containsKey("pointMax"))  sensor[sns].cfg.pointMax = (short)jsObj["pointMax"];
  if (jsObj.containsKey("valueMin"))  sensor[sns].cfg.valueMin = (short)jsObj["valueMin"];
  if (jsObj.containsKey("valueMax"))  sensor[sns].cfg.valueMax = (short)jsObj["valueMax"];
  if (jsObj.containsKey("period"))    sensor[sns].cfg.period = (short)jsObj["period"];
  if (jsObj.containsKey("decimals"))  sensor[sns].cfg.decimals = (byte)jsObj["decimals"];

#ifdef Monitor
  Serial.print("sensor: ");     Serial.println(sensor[sns].cfg.sensor);
  Serial.print("input: ");      Serial.println(sensor[sns].cfg.input);
  Serial.print("inhibitor: ");  Serial.println(sensor[sns].cfg.inhibitor);
  Serial.print("function: ");   Serial.println(sensor[sns].cfg.function);
  Serial.print("average: ");    Serial.println(sensor[sns].cfg.average);
  Serial.print("sample: ");     Serial.println(sensor[sns].cfg.sample);
  Serial.print("pointMin: ");   Serial.println(sensor[sns].cfg.pointMin);
  Serial.print("pointMax: ");   Serial.println(sensor[sns].cfg.pointMax);
  Serial.print("valueMin: ");   Serial.println(sensor[sns].cfg.valueMin);
  Serial.print("valueMax: ");   Serial.println(sensor[sns].cfg.valueMax);
  Serial.print("period: ");     Serial.println(sensor[sns].cfg.period);
  Serial.print("decimals: ");   Serial.println(sensor[sns].cfg.decimals);
#endif

  // Reset all data for this sensor
  sensor[sns].value = 0;
  sensor[sns].range = ' ';
  sensor[sns].lvl = LOW;
  sensor[sns].sec = 0;
  sensor[sns].old = 0;
  for (int jnd = 0; jnd < numVal; jnd++)
    sensor[sns].val[jnd] = 0;
  sensor[sns].sum = 0;
  sensor[sns].smp = 0;
  sensor[sns].ind = 0;

  // Send to server the client identifier and sensor number to configure range (new config -> new range)
  JsonObject  jsObj0 = jsDocTx.to<JsonObject>();
  jsObj0["msgType"] = "Range";
  JsonObject  jsObj1 = jsDocTx.createNestedObject("msgData");
  jsObj1["client"] = mqttClient;
  jsObj1["number"] = sns;

  mqttPublish("ave_Server");
}

//----------------------------------------------------//
//      MessageRange: decode message Range sensor     //
//----------------------------------------------------//

void MessageRange(JsonObject jsObj)
{
  int         sns = (int)jsObj["number"];

  // Configure the sensor Range
  if (jsObj.containsKey("wrnMin"))    sensor[sns].cfg.wrnMin = (short)jsObj["wrnMin"];
  if (jsObj.containsKey("wrnMax"))    sensor[sns].cfg.wrnMax = (short)jsObj["wrnMax"];
  if (jsObj.containsKey("dngMin"))    sensor[sns].cfg.dngMin = (short)jsObj["dngMin"];
  if (jsObj.containsKey("dngMax"))    sensor[sns].cfg.dngMax = (short)jsObj["dngMax"];
  if (jsObj.containsKey("intNrm"))    sensor[sns].cfg.intNrm = (short)jsObj["intNrm"];
  if (jsObj.containsKey("intWrn"))    sensor[sns].cfg.intWrn = (short)jsObj["intWrn"];
  if (jsObj.containsKey("intDng"))    sensor[sns].cfg.intDng = (short)jsObj["intDng"];
  if (jsObj.containsKey("actWrn"))    sensor[sns].cfg.actWrn = (byte)jsObj["actWrn"];
  if (jsObj.containsKey("actDng"))    sensor[sns].cfg.actDng = (byte)jsObj["actDng"];

#ifdef Monitor
  Serial.print("wrnMin: ");     Serial.println(sensor[sns].cfg.wrnMin);
  Serial.print("wrnMax: ");     Serial.println(sensor[sns].cfg.wrnMax);
  Serial.print("dngMin: ");     Serial.println(sensor[sns].cfg.dngMin);
  Serial.print("dngMax: ");     Serial.println(sensor[sns].cfg.dngMax);
  Serial.print("intNrm: ");     Serial.println(sensor[sns].cfg.intNrm);
  Serial.print("intWrn: ");     Serial.println(sensor[sns].cfg.intWrn);
  Serial.print("intDng: ");     Serial.println(sensor[sns].cfg.intDng);
  Serial.print("actWrn: ");     Serial.println(sensor[sns].cfg.actWrn);
  Serial.print("actDng: ");     Serial.println(sensor[sns].cfg.actDng);
#endif

  CompleteConfig();
}

//----------------------------------------------------//
//      CompleteConfig: require config to server      //
//----------------------------------------------------//

void CompleteConfig()
{
  int         sns = 0;

  while (sns < numSen && sensor[sns].cfg.sensor != 0)
    sns++;

  if (sns < numSen)
  {
    // Send to server the client identifier and sensor number not yet configured (config and range)
    JsonObject  jsObj0 = jsDocTx.to<JsonObject>();
    jsObj0["msgType"] = "Config";
    JsonObject  jsObj1 = jsDocTx.createNestedObject("msgData");
    jsObj1["client"] = mqttClient;
    jsObj1["number"] = sns;

    mqttPublish("ave_Server");
  }
  else
  {
    if (!timIni)
    {
      // Request hour of the day
      JsonObject  jsObj0 = jsDocTx.to<JsonObject>();
      jsObj0["msgType"] = "Time";
      JsonObject  jsObj1 = jsDocTx.createNestedObject("msgData");
      jsObj1["client"] = mqttClient;
      jsObj1["hh"] = 0;
      jsObj1["mm"] = 0;
      jsObj1["ss"] = 0;

      mqttPublish("ave_Server");
    }
  }
}

//----------------------------------------------------//
//      MessageTime: decode message Time              //
//----------------------------------------------------//

void MessageTime(JsonObject jsObj)
{
  timHH = jsObj["hh"];
  timMM = jsObj["mm"];
  timSS = jsObj["ss"];

  if (!timIni)
    timSec = 0;

  timIni = true;

  // Config complete, enable all sensors
  for (int sns = 0; sns < numSen; sns++)
    sensor[sns].cfg.enable = true;
}

//----------------------------------------------------//
//      MessageSensor: decode message Sensor read     //
//----------------------------------------------------//

void MessageSensor(JsonObject jsObj)
{
  String((const char*)jsObj["topic"]).toCharArray(readTopic, sizeof(readTopic));
  
  if (jsObj["sensor"] == "Temperature")
    readSensor = 'T';
  else if (jsObj["sensor"] == "Humidity")
    readSensor = 'H';
  else if (jsObj["sensor"] == "Heart Rate")
    readSensor = 'R';
}

//----------------------------------------------------//
//      mqttConnect: connects ESP32 to mqtt Broker    //
//----------------------------------------------------//

void mqttConnect()
{
  while (!client.connected())
  {
#ifdef Monitor       
    Serial.println("");
    Serial.print("Try connecting to MQTT: ");
    Serial.println(mqttServer);
    Serial.print("port: ");
    Serial.println(mqttPort);
    Serial.print("to user: ");
    Serial.println(mqttUser);
    Serial.print("as client: ");
    Serial.println(mqttClient);
    Serial.println("");
#endif

    if (client.connect(mqttClient, mqttUser, mqttPassword))
    {
#ifdef Monitor         
      Serial.println("Connected");
#endif      
    }
    else
    {
#ifdef Monitor         
      Serial.print("failed with state: ");
      Serial.println(client.state());
#endif      
      delay(2000);
    }
  }
}

//----------------------------------------------------//
//      mqttPublish: Publish jsDocTx                  //
//----------------------------------------------------//

void mqttPublish(char *tpc)
{
  size_t      len = measureJson(jsDocTx) + 1;
  char        jsMsg[len];
  serializeJson(jsDocTx, jsMsg, len);

#ifdef Monitor
  Serial.print("Send: ");
  Serial.println(jsMsg);
#endif

  while (!client.publish(tpc, jsMsg));
}

//----------------------------------------------------//
//      loop: Main loop                               //
//----------------------------------------------------//

void loop()
{
  int         sns;
  int         val;
  int         tim;
  int         dif;
  int         avg;
  int         smp;
  int         ind;
  int         lst;

  client.loop();

  tim = millis();

  //
  // For each sensor on device check if eanable, then calculate value
  //
  for (sns = 0; sns < numSen; sns++)
    if (sensor[sns].cfg.enable)
    {
      switch (sensor[sns].cfg.function)
      {
        // Sensor function Frequency
        case 'F':
          if (tim - sensor[sns].old > sensor[sns].cfg.sample)
          {
            // First time old = current time in millisecond
            if (sensor[sns].old == 0)
              sensor[sns].old = tim;

            val = analogRead(sensor[sns].cfg.input);

#ifdef PlotterHR
            Serial.println(val);
#endif

            // Detect a rising edge _/
            if (val > sensor[sns].cfg.pointMax && sensor[sns].lvl == LOW)
            {
              ind = sensor[sns].ind;
              smp = sensor[sns].smp;
              avg = sensor[sns].cfg.average;

              // Calculate how much time spend from last edge (difference = time between two impuls)
              dif = tim - sensor[sns].val[ind];

#ifdef MonitorHR
              Serial.print(val);
              Serial.print("\t");
              Serial.print(dif);
#endif

              // Check if difference is valid (not too short and not too long)
              if (dif > sensor[sns].cfg.valueMin)
              {
                if (dif < sensor[sns].cfg.valueMax)
                {
                  // Save the measure in array
                  sensor[sns].val[ind] = dif;

                  // Samples starts from 0 to avg * 2 (fisrt quarter and last quarter are discharged)
                  if (smp < avg * 2)
                  {
                    smp++;
                    sensor[sns].smp = smp;
                  }

                  // At least one sample valid
                  if (smp > avg)
                  {
                    int         valTim[smp];
                    int         jnd = 0;
                    int         knd;

                    // The array index before the first sample to use for average (circular array)
                    lst = (ind - smp + numVal) % numVal;
                    do
                    {
                      lst = (lst + 1) % numVal;
                      dif = sensor[sns].val[lst];

                      // Insert each sample in a sorted array
                      knd = jnd++;
                      while (knd > 0 && dif < valTim[knd - 1])
                      {
                        valTim[knd] = valTim[knd - 1];
                        knd--;
                      }
                      valTim[knd] = dif;

                    } while (lst != ind);

#ifdef MonitorHR
                    Serial.print("\t|");
                    for (jnd = 0; jnd < smp; jnd++)
                    {
                      Serial.print("\t");
                      Serial.print(valTim[jnd]);
                    }
                    Serial.print("\t|");
#endif

                    // First and last index of valid sample (first and last quarter of array are discharged)
                    jnd = avg / 2;
                    lst = smp - avg / 2 - 1;

#ifdef MonitorHR
                    Serial.print("\t scarto: ");
                    Serial.print(valTim[lst] - valTim[jnd]);
#endif

                    // Check if difference between the longer and the shorter sample is not too large 
                    if (valTim[lst] - valTim[jnd] < (sensor[sns].cfg.valueMax - sensor[sns].cfg.valueMin) * (smp - avg) / 100)
                    {
                      dif = 0;
                      for (; jnd <= lst; jnd++)
                        dif += valTim[jnd];

                      // Frequncy = Reference period / (Summary of samples / number of samples)
                      sensor[sns].value = (sensor[sns].cfg.period * 1000 * (smp - avg)) / dif;

#ifdef MonitorHR
                      Serial.print("\t value: ");
                      Serial.print(sensor[sns].value);
#endif
                    }
                  }

                  // Next array index (circular array)
                  ind = (ind + 1) % numVal;
                  sensor[sns].ind = ind;
                }
                // Save this time for calculate difference next edge or
                // update if dif > max but not if dif < min (a bounce)
                sensor[sns].val[ind] = tim;
              }
#ifdef MonitorHR
              Serial.println(".");
#endif

              sensor[sns].lvl = HIGH;
            }
            else if (val < sensor[sns].cfg.pointMin)
            {
              sensor[sns].lvl = LOW;
            }

            // Is a long time from last beats, set bpm to 0 and reset sample number 
            if ((tim - sensor[sns].val[sensor[sns].ind]) / 1000 > sensor[sns].cfg.period / 4)
            {
              sensor[sns].value = 0;
              sensor[sns].smp = 0;
            }

            digitalWrite(LedR, sensor[sns].lvl);

            // Set next sample time
            sensor[sns].old += sensor[sns].cfg.sample;
          }
          break;

        // Sensor function Linear
        case 'L':
          if (tim - sensor[sns].old > sensor[sns].cfg.sample)
          {
            // First time old = current time in millisecond
            if (sensor[sns].old == 0)
              sensor[sns].old = tim;

            val = analogRead(sensor[sns].cfg.input);

#ifdef MonitorT
            if (sns == 0)
              Serial.print(val);
#endif

#ifdef MonitorH
            if (sns == 1)
              Serial.print(val);
#endif

#ifdef PlotterI
            if (sns == 2)
              Serial.println(val);
#endif

            ind = sensor[sns].ind;
            smp = sensor[sns].smp;
            avg = sensor[sns].cfg.average;

            // Save the adc point in array
            sensor[sns].val[ind] = val;

            // Add the last to the summmary
            sensor[sns].sum += val;
            
            // Samples starts from 0 to avg
            if (smp < avg)
            {
              smp++;
              sensor[sns].smp = smp;
            }
            else
            {
              // If sample equals to number used for average, subtract the older sample from summary
              lst = (ind - smp + numVal) % numVal;
              sensor[sns].sum -= sensor[sns].val[lst];
            }

            // Linear translation from average point to value
            sensor[sns].value = map(sensor[sns].sum / smp, sensor[sns].cfg.pointMin, sensor[sns].cfg.pointMax, sensor[sns].cfg.valueMin,sensor[sns].cfg.valueMax);

#ifdef MonitorT
            if (sns == 0)
              Serial.print("\t => T: ");
              Serial.println(sensor[sns].value);
#endif

#ifdef MonitorH
            if (sns == 1)
              Serial.print("\t => H: ");
              Serial.println(sensor[sns].value);
#endif

            // Next array index (circular array)
            sensor[sns].ind = (ind + 1) % numVal;

            // Set next sample time
            sensor[sns].old += sensor[sns].cfg.sample;
          }
          break;
      }
    }

  //
  // Check if a second has passed
  //
  if (millis() - timMil > 1000)
  {
    // Every second
    timMil = timMil + 1000;

    timSec++;

    // Update time
    if (++timSS == 60)
    {
      timSS = 0;
      if (++timMM == 60)
      {
        timMM = 0;
        if (++timHH == 24)
          timHH = 0;
      }
    }

#ifdef MonitorTim
    Serial.print("Time: ");
    Serial.print(timHH);
    Serial.print(":");
    Serial.print(timMM);
    Serial.print(":");
    Serial.println(timSS);
#endif

    if (mqttOk)
    {
      // For each sensor on device check if eanable
      for (sns = 0; sns < numSen; sns++)
        if (sensor[sns].cfg.enable)
        {
          bool        inh = false;
          bool        rng = false;
          bool        acq = false;

          // Only if sensor has a inhibitor
          if (sensor[sns].cfg.inhibitor != (byte)-1)
            inh = (sensor[sensor[sns].cfg.inhibitor].value > 0);

          // This is inhibited
          if (inh)
          {
            if (timSec > 1)
            {
              // Send message End Working
              JsonObject  jsObj0 = jsDocTx.to<JsonObject>();
              jsObj0["msgType"] = "Status";
              JsonObject  jsObj1 = jsDocTx.createNestedObject("msgData");
              jsObj1["client"] = mqttClient;
              jsObj1["work"] = "End";
              jsObj1["hh"] = timHH;
              jsObj1["mm"] = timMM;
              jsObj1["ss"] = timSS;

              mqttPublish("ave_Server");
            }

            sensor[sns].sec = 0;
            timSec = 0;
          }
          else
          {
            // Danger range?
            if (sensor[sns].value < sensor[sns].cfg.dngMin || sensor[sns].value > sensor[sns].cfg.dngMax)
            {
              // First time in danger?
              if (sensor[sns].range != 'D')
              {
                if (sensor[sns].cfg.actDng && actMsg)
                  rng = true;

                sensor[sns].range = 'D';
              }

              // Is interval time spent to send a measure acquired (in danger range)?
              if (sensor[sns].cfg.intDng > 0 && timSec - sensor[sns].sec >= sensor[sns].cfg.intDng)
              {
                acq = true;

                sensor[sns].sec = timSec;
              }
            }
            // Warning range?
            else if (sensor[sns].value < sensor[sns].cfg.wrnMin || sensor[sns].value > sensor[sns].cfg.wrnMax)
            {
              // First time in warning?
              if (sensor[sns].range != 'W')
              {
                if (sensor[sns].cfg.actWrn && actMsg)
                  rng = true;

                sensor[sns].range = 'W';
              }

              // Is interval time spent to send a measure acquired (in warning range)?
              if (sensor[sns].cfg.intWrn > 0 && timSec - sensor[sns].sec >= sensor[sns].cfg.intWrn)
              {
                acq = true;

                sensor[sns].sec = timSec;
              }
            }
            // Normal range!
            else
            {
              // From which range
              switch (sensor[sns].range)
              {
                case 'D':
                  if (sensor[sns].cfg.actDng && actMsg)
                    rng = true;
                    break;

                case 'W':
                  if (sensor[sns].cfg.actWrn && actMsg)
                    rng = true;
                    break;
              }
              sensor[sns].range = 'N';
                
              // Is interval time spent to send a measure acquired (in warning range)?
              if (sensor[sns].cfg.intNrm > 0 && timSec - sensor[sns].sec >= sensor[sns].cfg.intNrm)
              {
                acq = true;

                sensor[sns].sec = timSec;
              }
            }

            // Sensor change range or Interval time: send a data message (sensor, range, value, time)
            if (rng || acq || readSensor == sensor[sns].cfg.sensor)
            {
              JsonObject  jsObj0 = jsDocTx.to<JsonObject>();
              jsObj0["msgType"] = "Sensor";
              JsonObject  jsObj1 = jsDocTx.createNestedObject("msgData");
              jsObj1["client"] = mqttClient;
              switch (sensor[sns].cfg.sensor)
              {
                case 'T':
                  jsObj1["sensor"] = "Temperature";
                  break;

                case 'H':
                  jsObj1["sensor"] = "Humidity";
                  break;

                case 'R':
                  jsObj1["sensor"] = "Heart Rate";
                  break;
              }
              switch (sensor[sns].range)
              {
                case 'N':
                  jsObj1["range"] = "Normal";
                  break;

                case 'W':
                  jsObj1["range"] = "Warning";
                  break;

                case 'D':
                  jsObj1["range"] = "Danger";
                  break;
              }
              switch (sensor[sns].cfg.decimals)
              {
                case 0:
                  jsObj1["value"] = sensor[sns].value;
                  break;

                case 1:
                  jsObj1["value"] = (float)sensor[sns].value / 10;
                  break;

                case 2:
                  jsObj1["value"] = (float)sensor[sns].value / 100;
                  break;
              }
              jsObj1["hh"] = timHH;
              jsObj1["mm"] = timMM;
              jsObj1["ss"] = timSS;

              if (rng || acq)
                mqttPublish("ave_Server");

              if (readSensor == sensor[sns].cfg.sensor)
              {
                mqttPublish(readTopic);
                readSensor = 0;
              }
            }
          }
        }

      // If any sensor is inhibited timSec is reset
      if (timSec == 1)
      {
        // Send message Begin Working
        JsonObject  jsObj0 = jsDocTx.to<JsonObject>();
        jsObj0["msgType"] = "Status";
        JsonObject  jsObj1 = jsDocTx.createNestedObject("msgData");
        jsObj1["client"] = mqttClient;
        jsObj1["work"] = "Begin";
        jsObj1["hh"] = timHH;
        jsObj1["mm"] = timMM;
        jsObj1["ss"] = timSS;

        mqttPublish("ave_Server");
      }
    }
  }
}

//----------------------------------------------------//
//      WiFiConnect: connects ESP32 to WiFi Lan       //
//----------------------------------------------------//

void WiFiConnect()
{
  int         ssidNum;
  int         net = 0;
  int         sec;
  int         ind;
  int         opn;
  
  char        jsMsg[RomB + 1];
  
  EEPROM.begin(RomB);

  ind = 0;
  opn = 0;
  do
  {
    jsMsg[ind] = EEPROM.read(ind);

    switch(jsMsg[ind])
    {
      case '{':
        opn++;
        break;
        
      case '}':
        opn--;
        break;
    }
    
    jsMsg[++ind] = '\0';
  } while (opn > 0 && ind < RomB);

#ifdef Monitor
  Serial.println("--------------------------------");
  Serial.print("EEPRom: ");
  Serial.println(jsMsg);
#endif

  if (opn > 0 || ind <= 2)
  {
    const char* def = "{\"numNet\":0,\"wifiNet\":[]}";
    String(def).toCharArray(jsMsg, strlen(def) + 1);

#ifdef Monitor
  Serial.println("--------------------------------");
  Serial.println("New EEPROM");
#endif

    for (ind = 0; ind < strlen(def) + 1; ind++)
      EEPROM.write(ind, jsMsg[ind]);
    EEPROM.commit();
  }

  //const char* def = "{\"numNet\":1,\"wifiNet\":[{\"SSId\":\"IIS\",\"Password\":\"\"}]}";
  //String(def).toCharArray(jsMsg, strlen(def) + 1);

  deserializeJson(jsDocRx, jsMsg);
  JsonObject  jsObj0 = jsDocRx.as<JsonObject>();
  JsonArray   jsArr0 = jsObj0["wifiNet"];

#ifdef Monitor
  Serial.print("numNet: ");
  Serial.println((byte)jsObj0["numNet"]);
  Serial.print("wifiNet: ");
  serializeJson(jsObj0["wifiNet"], Serial);
  Serial.println();
#endif
  
  // Detect WiFi network
  do
  {
    SerialCheck();
    
    ssidNum = WiFi.scanNetworks();

    if (ssidNum == -1)
    {
#ifdef Monitor
      Serial.println("Couldn't get a wifi connection");
#endif
      delay(2000);
    }
  } while (ssidNum == -1);

#ifdef Monitor
  // Print the list of networks seen
  Serial.print("Number of available networks:");
  Serial.println(ssidNum);

  // Print the network number and name for each network found
  for (net = 0; net < ssidNum; net++)
  {
    Serial.print(net);
    Serial.print(") '");
    Serial.print(WiFi.SSID(net));
    Serial.print("'\tSignal: ");
    Serial.print(WiFi.RSSI(net));
    Serial.print(" dBm");
    Serial.print("\tEncryption: ");
    Serial.println(WiFi.encryptionType(net), HEX);
  }
#endif

  do
  {
    // Try to connect for each network found that known (and retry)
    net = net % ssidNum;

#ifdef Monitor
    Serial.print("Net: ");
    Serial.println(net);
#endif

    sec = 0;
    for (ind = 0; ind < (byte)jsObj0["numNet"]; ind++)
      if (strequ(WiFi.SSID(net), (const char *)jsArr0[ind]["SSId"]))
      {
#ifdef Monitor        
        Serial.print("Try connecting to: ");
        Serial.print(WiFi.SSID(net));
#endif

        WiFi.begin((const char *)jsArr0[ind]["SSId"], (const char *)jsArr0[ind]["Password"]);
        sec = 10;
      }

    net++;
  } while (!WiFiStatus(sec));
}

bool strequ(String str1, const char* str2)
{
  for (int ind = 0; ind < strlen(str2); ind++)
    if (str1[ind] != str2[ind])
      return false;

  return true;
}

bool WiFiStatus(int sec)
{
  while (WiFi.status() != WL_CONNECTED && sec > 0)
  {
    SerialCheck();
    
    delay(1000);
    sec--;    

#ifdef Monitor    
    Serial.print(".");
#endif
  }
#ifdef Monitor  
  Serial.println();
#endif

  return (sec > 0);
}

//----------------------------------------------------//
//      SerialCheck:                                  //
//----------------------------------------------------//

void serialEventRun()
{
  SerialCheck();
}

void SerialCheck()
{
  while (Serial.available())
  {
    char rxc = (char)Serial.read();

    if (rxc == 'C')
      WiFiConfig();
  }
}


void WiFiConfig()
{
  char        jsMsg[RomB + 1];
  char        rxc;
  int         ind;
  int         opn;
  
  ind = 0;
  opn = 0;
  do
  {
    jsMsg[ind] = EEPROM.read(ind);

    switch(jsMsg[ind])
    {
      case '{':
        opn++;
        break;
        
      case '}':
        opn--;
        break;
    }
    
    jsMsg[++ind] = '\0';
  } while (opn > 0 && ind < RomB);

  Serial.print(jsMsg);

  ind = 0;
  opn = 0;
  do
  {
    if (Serial.available())
    {
      rxc = (char)Serial.read();

      switch(rxc)
      {
        case '{':
          opn++;
          break;
          
        case '}':
          opn--;
          break;
      }

      jsMsg[ind] = rxc;
      jsMsg[++ind] = '\0';

      if (opn > 0)
        rxc = '\0';
        
      if (opn == 0 && ind > 25 && ind < RomB)
      {
        for (int jnd = 0; jnd < ind + 1; jnd++)
          EEPROM.write(jnd, jsMsg[jnd]);
        EEPROM.commit();
/*
        ind = 0;
        opn = 0;
        do
        {
          jsMsg[ind] = EEPROM.read(ind);
      
          switch(jsMsg[ind])
          {
            case '{':
              opn++;
              break;
              
            case '}':
              opn--;
              break;
          }
          
          jsMsg[++ind] = '\0';
        } while (opn > 0 && ind < RomB);
        
        Serial.println("");        
        Serial.print(jsMsg);
*/
        ind = 0;
        rxc = '\0';
      }
      
      if (rxc == 'R')
        ESP.restart();
    }
  } while (rxc != 'D');
}
