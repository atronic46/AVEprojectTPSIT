#include <EEPROM.h>
#include <WiFi.h>
#include <ArduinoJson.h>

#define     Monitor
#define     MonitorTim

#define     RomB  1024        // EEPRom Byte used to store the WiFi Net known in JSON format (max as jsDocRx)

char        comRxB[RomB];
int         comInd;

// Json documents
StaticJsonDocument<256>   jsDocTx;
StaticJsonDocument<1024>  jsDocRx;

// Time
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
}

//----------------------------------------------------//
//      loop: Main loop                               //
//----------------------------------------------------//

void loop()
{
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

  //const char* def = "{\"numNet\":3,\"wifiNet\":[{\"SSId\":\"IIS\",\"Password\":\"\"},{\"SSId\":\"SAILWEB_WIFI\",\"Password\":\"17320641\"},{\"SSId\":\"HUAWEI P9 lite\",\"Password\":\"hjklvbnm\"}]}";
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

        ind = 0;
        rxc = '\0';
      }
      
      if (rxc == 'R')
        ESP.restart();
    }
  } while (rxc != 'D');
}
