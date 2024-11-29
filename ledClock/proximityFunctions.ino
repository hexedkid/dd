void readProximity(uint16_t *proximity, bool *proximityContinuing) {

  uint16_t rawProximity = sensorAlsPs.getPsData();
  if ( ! (sensorAlsPs.getFailureMode() & 128)) {
    *proximity = rawProximity;
  } else {
    *proximity = 250; // error reading proximity or absent sensor
  }
  sensorAlsPs.clearFailure();

  if ( *proximity <= PROXIMITY_TRESHOLD ) {
    *proximityContinuing = false; // прочитать текущее состояние proximity (для анализа новое/продолжающееся срабатывание)
  } else {
    if ( ! *proximityContinuing ) {
      // Serial.println(*proximity);
      increaseMode();
      chirp();
    }
    *proximityContinuing = true;
  }
}
