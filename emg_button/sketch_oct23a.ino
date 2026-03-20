//Arduino: // EMG -> Geometry Dash con calibración por 5 ciclos de reposo/contracción (Arduino UNO)
// Incluye: envolvente EMA, histéresis, umbrales adaptativos robustos

const int A_EMG = A0;

// ---------- Parámetros ----------
const int N_REPS = 5;                 // Número de ciclos
const int N_SAMPLES_PER_PHASE = 500;  // Muestras por fase (~1s a 500Hz)
const float ALPHA_ENV = 0.12;
const int REFRACT_MS = 140;
const int MIN_TH_ABS = 50;
const float FRAC_MVC = 0.35;
const float RATIO_OFF = 0.55;
const int NOISE_MULT = 4;

// ---------- Estado ----------
int offset = 512;
float env = 0;
int TH_ON = 150, TH_OFF = 80;
bool active = false;
unsigned long tLast = 0;

enum Phase { CALM,
             SQUEEZE,
             DONE };
Phase phase = CALM;
int repIndex = 0;
int sampleCount = 0;

// Datos acumulados
long restSum = 0;
long restSumSq = 0;
int restN = 0;
int squeezePeak = 0;

// Arrays para acumular múltiples repeticiones
long restSums[N_REPS];
long restSqSums[N_REPS];
int squeezePeaks[N_REPS];

void startCalibration() {
  phase = CALM;
  repIndex = 0;
  sampleCount = 0;
  env = 0;
  for (int i = 0; i < N_REPS; i++) {
    restSums[i] = 0;
    restSqSums[i] = 0;
    squeezePeaks[i] = 0;
  }
  Serial.println("MSG: Calibrando... Reposo cuando se indique, luego apriete máximo cuando se indique.");
}

void finishCalibrationAndSetThresholds() {
  // Promedio de fases de reposo
  long totalRest = 0, totalRestSq = 0;
  for (int i = 0; i < N_REPS; i++) {
    totalRest += restSums[i];
    totalRestSq += restSqSums[i];
  }
  double meanRest = (double)totalRest / (N_REPS * N_SAMPLES_PER_PHASE);
  double varRest = ((double)totalRestSq / (N_REPS * N_SAMPLES_PER_PHASE)) - (meanRest * meanRest);
  if (varRest < 0) varRest = 0;
  double stdRest = sqrt(varRest);
  offset = (int)meanRest;

  // Promedio de contracciones máximas
  int peakTotal = 0;
  for (int i = 0; i < N_REPS; i++) peakTotal += squeezePeaks[i];
  int peakAvg = peakTotal / N_REPS;

  int byGain = (int)((peakAvg - meanRest) * FRAC_MVC);
  int byNoise = (int)(stdRest * NOISE_MULT);
  int add = max(byGain, max(byNoise, MIN_TH_ABS));

  TH_ON = (int)meanRest + add;
  TH_OFF = max(5, (int)(TH_ON * RATIO_OFF));

  Serial.print("#CAL baseline=");
  Serial.print((int)meanRest);
  Serial.print(" noise≈");
  Serial.print((int)stdRest);
  Serial.print(" peakAVG=");
  Serial.print(peakAvg);
  Serial.print(" => TH_ON=");
  Serial.print(TH_ON);
  Serial.print(" TH_OFF=");
  Serial.println(TH_OFF);

  phase = DONE;
}

void setup() {
  Serial.begin(115200);
  delay(200);
  startCalibration();
}

void loop() {
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'C' || c == 'c') startCalibration();
  }

  int raw = analogRead(A_EMG);
  int d = raw - offset;
  if (d < 0) d = -d;
  env += ALPHA_ENV * (d - env);

  if (phase != DONE) {
    if (phase == CALM) {
      restSums[repIndex] += raw;
      restSqSums[repIndex] += (long)raw * raw;
    } else if (phase == SQUEEZE) {
      if (raw > squeezePeaks[repIndex]) squeezePeaks[repIndex] = raw;
    }
    sampleCount++;

    if (sampleCount >= N_SAMPLES_PER_PHASE) {
      sampleCount = 0;
      if (phase == CALM) {
        phase = SQUEEZE;
        Serial.println("MSG: Ahora apriete fuerte el antebrazo.");
      } else {
        repIndex++;
        if (repIndex >= N_REPS) {
          finishCalibrationAndSetThresholds();
        } else {
          phase = CALM;
          Serial.println("MSG: Relaje el brazo.");
        }
      }
    }
    return;
  }

  // Lógica de activación (cuando calibrado)
  unsigned long now = millis();
  bool above = env > (TH_ON - offset);
  bool below = env < (TH_OFF - offset);

  if (!active && above && now - tLast >= REFRACT_MS) {
    active = true;
    tLast = now;
  } else if (active && below) {
    active = false;
  }

  // Telemetría
  int envInt = (int)env;
  Serial.print(envInt);
  Serial.print(',');
  Serial.print(active ? 1 : 0);
  Serial.print(',');
  Serial.print(TH_ON - offset);
  Serial.print(',');
  Serial.println(TH_OFF - offset);

  delay(2);  // ~500 Hz
}