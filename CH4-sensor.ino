/////////////
// Arduino //
/////////////
const bool DEBUG = false; // debug output (calibration phase) is not compatible with external monitoring system ('./monitor.sh').

const int CALIBRATION_PIN = 13; // Turn on during calibration
const int MQ_PIN = A0; // analog input
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
const double RL = 1.777; // K Ohms measured between A0 and GND (polymeter)
double CLEAN_AIR_RATIO = 4.3; // RS_air/R0, horizontal black line at graph: you must guarantee 'clean air' during setup() calibration
double R0 = 11.82; // estimated (will be calculated on calibration)
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
double voltage(int samples = DEFAULT_SAMPLES, int samplesIntervalMs = DEFAULT_SAMPLES_INTERVAL_MS) {
  double avg = 0.0;
  for (int k = 0; k < samples; k ++) {
    avg += analogRead(MQ_PIN);
    delay(samplesIntervalMs);
  }

  double result = (avg/samples) * VOLT_RESOLUTION / ADC_RESOLUTION;

  return result;
}

// Get R0 on using clean-air as graph reference
double calibrate() {
  double sensor_volt = voltage(50, 20); //Define variable for sensor voltage
  double RS_air; // Define variable for sensor resistance
  double R0; // Define variable for resulting R0

  RS_air = ((VOLT_RESOLUTION * RL) / sensor_volt) - RL; // Calculate RS in fresh air
  R0 = RS_air / CLEAN_AIR_RATIO; // Calculate R0

  return R0;
}

// Get CH4 concentratin (ppm)
int ppm(double sensor_volt) {
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
    Serial.println("\n==============================================================");
    Serial.println("METANE CONCENTRATION MEASUREMENT (CH4 ppm) BY MEAN MQ-4 SENSOR");
    Serial.println("==============================================================\n");

    Serial.print("Estimated R0 (K Ohms) = ");
    Serial.println(R0);
  }

  pinMode(CALIBRATION_PIN, OUTPUT);
  digitalWrite(CALIBRATION_PIN, HIGH); // light up
  if (DEBUG) Serial.print("Calibrating ... ");
  R0 = calibrate();
  if (DEBUG) Serial.println("done !");
  digitalWrite(CALIBRATION_PIN, LOW); // light down

  if (DEBUG) {
    Serial.println("Start measuring ...");
    Serial.print("Calibrated R0 (K Ohms) = ");
    Serial.println(R0);
  }

  delay(2000);
}

//////////
// MAIN //
//////////
void loop() {

  double sensor_volt = voltage();

  // Sensor VS (V) = <sensor_volt>
  // RS_gas (K Ohms) = <RS_gas>
  Serial.println(ppm(sensor_volt));

  delay(500-DEFAULT_MEASURE_LAPSE); // half second in total
}

