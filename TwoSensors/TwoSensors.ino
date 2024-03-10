///////////////////////////////////////////////////////////////////////////////////////////////////////////
// VARIATION TO MEASURE TWO SENSORS, one inside the biomethane reactor, another outside (clean air)      //
//                                                                                                       //
// 1. One MQ-4 sensor (1) will be put inside the biometane reactor                                       //
// 2- One MQ-4 sensor (2) will be put outside. Useful to detect gas leaks from the near reactor          //
//                                                                                                       //
// Load resistances have been measured with polimeter (keep the adjustment stable) for my 2 sensors      //
//                                                                                                       //
// 1. RL_1 = 1.777 kOhms; // K Ohms (measured with polymeter between A0 and GND: 1.773 (without cables)) //
// 1. RL_2 = 1.915 kOhms; // K Ohms (measured with polymeter between A1 and GND: 1.913 (without cables)) //
//                                                                                                       //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////
// Arduino //
/////////////
const bool DEBUG = false; // debug output (calibration phase) is not compatible with external monitoring system ('./monitor.sh').

const int CALIBRATION_PIN = 13; // Turn on during calibration
const int MQ_PIN_1 = A0; // analog input for sensor inside the reactor
const int MQ_PIN_2 = A1; // analog input for sensor outside the reactor
const double VOLT_RESOLUTION = 5.0; // if 3.3V, use 3.3
const int ADC_RESOLUTION = 1023; // for 10-bit analog to digital converter: 0..2^10-1 = 0..1023
//const int SERIAL_BAUD=115200;
const int SERIAL_BAUD=9600;

// How to interpret ADC measure:
// ADC = analogRead(MQ_PIN); // 0..1023
// Sensor voltage = 5*ADC/1023
//
// MQ-4 idea is to use the voltage divider formula:
//
//  RS     V - VS
// ---- = ---------- => RS = V*RL/VS - RL
//  RL       VS
//
// RS: sensor resistance
// VS: sensor voltage
// RL: load resistance
// V: volt resolution (usually 5.0V)

///////////////////////////////////////////////////////////////
// DATA SHEET SPECIFICATIONS, SENSIBILITY GRAPH and FORMULAS //
///////////////////////////////////////////////////////////////
const double RL_1 = 1.777 ; // K Ohms
const double RL_2 = 1.915 ; // K Ohms
double CLEAN_AIR_RATIO = 4.3; // RS_air/R0, horizontal black line at graph: you must guarantee 'clean air' during setup() calibration
double R0_1 = 11.82; // estimated (will be calculated on calibrations)
double R0_2 = R0_1; // same
const int MQ4_PPM_MAX_RANGE = 10000;

// Graph shows RS/R0 against ppm
// Graph is in logarithm scale: log y = m * log x + b
//
// y=1;    x=1000 ppm  => 0 = 3m + b
// y=0.43; x=10000 ppm => −0,366531544 = 4m + b
//
// So:
const double m = -0.366531544;
const double b = 1.099594632;

///////////////
// FUNCTIONS //
///////////////

const int DEFAULT_SAMPLES = 5;
const int DEFAULT_SAMPLES_INTERVAL_MS = 10;
const int DEFAULT_MEASURE_LAPSE = DEFAULT_SAMPLES * DEFAULT_SAMPLES_INTERVAL_MS;

// Get average voltage
double voltage(int pin, int samples = DEFAULT_SAMPLES, int samplesIntervalMs = DEFAULT_SAMPLES_INTERVAL_MS) {
  double avg = 0.0;
  for (int k = 0; k < samples; k ++) {
    avg += analogRead(pin);
    delay(samplesIntervalMs);
  }

  double result = (avg/samples) * VOLT_RESOLUTION / ADC_RESOLUTION;

  return result;
}

// Get R0 on using clean-air as graph reference
double calibrate(int pin, double RL) {
  double sensor_volt = voltage(pin, 50, 20); //Define variable for sensor voltage
  double RS_air; // Define variable for sensor resistance
  double R0; // Define variable for resulting R0

  RS_air = ((VOLT_RESOLUTION * RL) / sensor_volt) - RL; // Calculate RS in fresh air
  R0 = RS_air / CLEAN_AIR_RATIO; // Calculate R0

  return R0;
}

// Get CH4 concentratin (ppm)
int ppm(double sensor_volt, double RL, double R0) {
  double RS_gas; //Define variable for sensor resistance
  double ratio; //Define variable for ratio

  RS_gas = ((VOLT_RESOLUTION * RL) / sensor_volt) - RL; //Get value of RS in a gas
  ratio = RS_gas / R0;   // Get ratio for CH4

  double ppm_log = (log10(ratio) - b) / m; //Get ppm value in linear scale according to the the ratio value
  double ppm = pow(10, ppm_log); //Convert ppm value to log scale

  // Limit range to maximum by specification:
  int result = min(floor(ppm), MQ4_PPM_MAX_RANGE);

  return result;
}

////////////////////
// INITIALIZATION //
////////////////////
void setup() {
  Serial.begin(SERIAL_BAUD); // hay que ajustarlo en el monitor tambén, o se verá basura
  delay(1000);

  if (DEBUG) {
    Serial.println("\n===============================================================");
    Serial.println("METANE CONCENTRATION MEASUREMENT (CH4 ppm) BY MEAN MQ-4 SENSORS");
    Serial.println("Sensor 1: located inside the biomethane reactor");
    Serial.println("Sensor 2: located outside the biomethane reactor");
    Serial.println("===============================================================\n");

    Serial.print("Estimated R0_1 (K Ohms) = ");
    Serial.println(R0_1);
    Serial.print("Estimated R0_2 (K Ohms) = ");
    Serial.println(R0_2);
  }

  pinMode(CALIBRATION_PIN, OUTPUT);
  digitalWrite(CALIBRATION_PIN, HIGH); // light up
  if (DEBUG) Serial.print("Calibrating ... ");
  R0_1 = calibrate(MQ_PIN_1, RL_1);
  R0_2 = calibrate(MQ_PIN_2, RL_2);
  if (DEBUG) Serial.println("done !");
  digitalWrite(CALIBRATION_PIN, LOW); // light down

  if (DEBUG) {
    Serial.println("Start measuring ...");
    Serial.print("Calibrated R0_1 (K Ohms) = ");
    Serial.println(R0_1);
    Serial.print("Calibrated R0_2 (K Ohms) = ");
    Serial.println(R0_2);
  }

  delay(2000);
}

//////////
// MAIN //
//////////
void loop() {
  double sensor_volt_1 = voltage(MQ_PIN_1);
  delay(50);
  double sensor_volt_2 = voltage(MQ_PIN_2);

  // Sensor VS (V) = <sensor_volt>
  // RS_gas (K Ohms) = <RS_gas>
  Serial.print(ppm(sensor_volt_1, RL_1, R0_1));
  Serial.print(",");
  Serial.println(ppm(sensor_volt_2, RL_2, R0_2));

  delay(450-/* two sensors */ 2*DEFAULT_MEASURE_LAPSE); // half second in total
}

