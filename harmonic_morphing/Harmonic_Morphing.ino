

//////////////// CONFIG //////////////////
const uint8_t PIN_EMG  = A0;
const uint8_t MIDI_CH  = 1;
const uint16_t FS_HZ   = 1000;   // muestreo
const float    ALPHA   = 0.12f;  // EMA envolvente
const uint8_t  MA_LEN  = 4;      // media móvil corta
const uint16_t DECIDE_MS = 250;  // ritmo de decisión (evita parpadeo)
const uint16_t DWELL_MS  = 140;  // permanencia mínima por estado
const uint32_t NOTE_TIMEOUT_MS = 3500; // seguridad anti-colgado

// Umbrales sobre valor normalizado (0..1)
const float ENTER_THRESH = 0.18f;   // entrar al “mundo musical”
const float EXIT_THRESH  = 0.10f;   // salir a silencio (histeresis)
                                     // (EXIT < ENTER)

const uint8_t VEL_MIN = 70;
const uint8_t VEL_MAX = 127;

// Tonalidad (C4) y progresión (I–vi–IV–V)
int BASE_NOTE = 60; // C4
const uint8_t PROG_LEN = 4;
const int8_t PROG[PROG_LEN][3] = {
  { 0, 4,  7},   // I  (C E G)
  { 9, 12, 16},  // vi (A C E)
  { 5, 9,  12},  // IV (F A C)
  { 7, 11, 14}   // V  (G B D)
};

/////////////// ESTADO /////////////////
int offsetADC = 512;
float envEMA = 0.0f;
int   ring[MA_LEN]; uint8_t ri=0; long ringSum=0;

int ENV_MIN = 12, ENV_MAX = 280;  // se calibran
uint32_t tLastDec=0, tLastChange=0, tLastNoteOn=0;
uint8_t currentIdx = 255; // 255 = sin estado (silencio)
uint8_t active[4]; uint8_t actN=0;

////////////// MIDI HELPERS ////////////
inline void send3(byte s, byte d1, byte d2){ Serial.write(s); Serial.write(d1); Serial.write(d2); }
inline void noteOn(byte ch, byte n, byte v){ send3(0x90 | ((ch-1)&0x0F), n, v); }
inline void noteOff(byte ch, byte n){ send3(0x80 | ((ch-1)&0x0F), n, 0); }
void clearAll(){ for(uint8_t i=0;i<actN;i++) noteOff(MIDI_CH, active[i]); actN=0; currentIdx=255; }
bool inActive(byte n){ for(uint8_t i=0;i<actN;i++) if(active[i]==n) return true; return false; }
void removeActive(byte n){ for(uint8_t i=0;i<actN;i++) if(active[i]==n){ active[i]=active[--actN]; return; } }

////////////// CALIBRACIÓN /////////////
void calibrate(){
  // 1) Offset (reposo 1.5 s)
  long acc=0; int n=0; unsigned long t0=millis();
  while(millis()-t0 < 1500){ acc += analogRead(PIN_EMG); n++; delay(1); }
  if(n>0) offsetADC = acc/n;

  // 2) Ruido base (EMA 0.9 s)
  float tmp=0; t0=millis();
  while(millis()-t0 < 900){
    int d=analogRead(PIN_EMG)-offsetADC; if(d<0) d=-d;
    tmp += ALPHA*(d - tmp);
    delay(1);
  }
  int noise = (int)tmp;

  // 3) Máximo durante 2.5 s (hacer 2-3 contracciones fuertes)
  int peak = noise + 60;  // valor mínimo razonable
  t0=millis();
  while(millis()-t0 < 2500){
    int d=analogRead(PIN_EMG)-offsetADC; if(d<0) d=-d;
    tmp += ALPHA*(d - tmp);
    if((int)tmp > peak) peak = (int)tmp;
    delay(1);
  }

  // 4) Fijar rangos con margen
  ENV_MIN = noise + 5;                      // un poco sobre ruido
  ENV_MAX = max(ENV_MIN + 180, peak - 20);  // asegura rango útil

  // reset estado
  envEMA = 0; ringSum=0; actN=0; currentIdx=255;
  for(uint8_t i=0;i<MA_LEN;i++) ring[i]=0;
}

////////////// SETUP ///////////////////
void setup(){
  Serial.begin(115200);
  analogReference(DEFAULT);
  pinMode(PIN_EMG, INPUT);
  calibrate();
}

////////////// LOOP ////////////////////
void loop(){
  // Recalibración por Serial ('c')
  if(Serial.available()){
    char c = Serial.read();
    if(c=='c' || c=='C'){ clearAll(); calibrate(); }
  }

  // Muestreo estable a 1 kHz
  static uint32_t tS=micros();
  uint32_t nu=micros(); if(nu - tS < (1000000UL/FS_HZ)) return; tS += (1000000UL/FS_HZ);

  // Lectura -> rectificación -> EMA -> media móvil
  int raw = analogRead(PIN_EMG);
  int d = raw - offsetADC; if(d<0) d=-d;
  envEMA += ALPHA * (d - envEMA);

  ringSum -= ring[ri];
  ring[ri]  = (int)envEMA;
  ringSum += ring[ri];
  ri++; if(ri>=MA_LEN) ri=0;

  int env = (int)(ringSum / MA_LEN);

  // Decidir cada DECIDE_MS (ritmo armónico)
  uint32_t now=millis();
  if(now - tLastDec < DECIDE_MS) return;
  tLastDec = now;

  // Normalización 0..1 con curva perceptual
  if(env < ENV_MIN) env = ENV_MIN;
  if(env > ENV_MAX) env = ENV_MAX;
  float m = (float)(env - ENV_MIN) / (float)(ENV_MAX - ENV_MIN);
  if(m<0) m=0; if(m>1) m=1;
  float mc = pow(m, 0.7f); // más sensible en bajas intensidades

  // Estado 0 = silencio, 1..PROG_LEN = acordes
  uint8_t targetIdx = 0;
  if(mc > ENTER_THRESH){ // dentro del “mundo musical”
    uint8_t band = (uint8_t)(mc * PROG_LEN);
    if(band >= PROG_LEN) band = PROG_LEN-1;
    targetIdx = band + 1; // 1..PROG_LEN
  } else if(mc < EXIT_THRESH){
    targetIdx = 0;        // silencio al bajar del exit
  } else {
    targetIdx = currentIdx; // zona muerta entre enter/exit
  }

  // Dwell para evitar ping-pong
  if(currentIdx==255){ currentIdx = targetIdx; tLastChange=now; }
  if(targetIdx != currentIdx && (now - tLastChange) < DWELL_MS) return;

  // Transición
  if(targetIdx == 0){
    // Silencio: apaga todo
    if(actN>0){ clearAll(); }
    currentIdx = 0; tLastChange=now;
  } else {
    // Construye acorde objetivo (triada)
    const int8_t* triad = PROG[targetIdx-1];

    // Apaga notas que no están
    for(uint8_t i=0;i<actN;i++){
      bool keep=false;
      for(uint8_t j=0;j<3;j++){
        byte cand = BASE_NOTE + triad[j];
        if((active[i]%12) == (cand%12)){ keep=true; break; }
      }
      if(!keep){ noteOff(MIDI_CH, active[i]); removeActive(active[i]); i--; }
    }

    // Enciende las que falten (voicing cercano)
    byte ref = (actN>0)? active[0] : (BASE_NOTE + triad[0]);
    for(uint8_t j=0;j<3;j++){
      byte nn = BASE_NOTE + triad[j];
      while(nn < ref-7) nn += 12;
      while(nn > ref+9) nn -= 12;
      if(!inActive(nn)){
        int vel = map(env, ENV_MIN, ENV_MAX, VEL_MIN, VEL_MAX);
        if(vel<VEL_MIN) vel=VEL_MIN; if(vel>127) vel=127;
        noteOn(MIDI_CH, nn, vel); active[actN++] = nn; tLastNoteOn=now;
      }
    }

    currentIdx = targetIdx; tLastChange=now;
  }

  // Seguridad anti nota colgada
  if(actN>0 && (now - tLastNoteOn) > NOTE_TIMEOUT_MS){
    clearAll();
  }
}
