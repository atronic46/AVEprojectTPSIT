#include <Arduino.h>

#define     numAmb 2          // Sensor number on Ambient
#define     numMon 3          // Sensor number on Monitor
#define     maxSen 3          // Max client sensor number

#define     numVal 64         // Array dimension
#define     maxAvg 32         // Max average value (be careful: max is numVal / 2)

#define     actMsg 0b00000001 // Action when change range: Send a message

typedef struct
{
  bool        enable;         // Enable (true / false)
  char        sensor;         // Sensor name for message ('T' = Temperature / 'H' = Humidity / 'R' = Heart Rate)
  byte        input;          // Esp32 Analog pin
  byte        inhibitor;      // Inhibitor sensor (number)
  char        function;       // Function ('L' = Linear / 'F' = Frequency)
  byte        average;        // Average (sample number for linear / impulse number for frequency)
  short       sample;         // Sample time (ms)
  short       pointMin;       // ADC point min for Linear / Lower threshold for Frequency
  short       pointMax;       // ADC point max  "    "    / Upper threshold  "      "
  short       valueMin;       // Value min for Linear / Min period for Frequency
  short       valueMax;       // Value max  "    "    / Max period  "      "
  short       period;         // Reference Period for frequency
  byte        decimals;       // Decimal point resolution
  short       wrnMin;         // Low limit warning
  short       wrnMax;         // High limit warning
  short       dngMin;         // Low limit danger
  short       dngMax;         // High limit danger
  short       intNrm;         // Interval acquisition (normal)
  short       intWrn;         // Interval acquisition in warning
  short       intDng;         // Interval acquisition in danger
  byte        actWrn;         // Action in warning
  byte        actDng;         // Action in danger
} s_cfg;

typedef struct
{
  short       value;          // Measure sensor value  
  char        range;          // Range (' ' = undefined / 'N' = Normal / 'W' = Warning / 'D' = Danger) 
  byte        lvl;            // Previous level for Frequency
  int         sec;            // Previous second that send measure
  int         old;            // Old time sample
  int         val[numVal];    // Array of value for every sample (circular buffer)
  int         sum;            // Sum for average
  byte        smp;            // Samples executed
  byte        ind;            // Current array index
  union
  {
    s_cfg       cfg;          // Configuration (share same memory location whit cfgBuf)
    byte        cfgByt[sizeof(s_cfg)];   
  };
} s_sensor;
