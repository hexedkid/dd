
void recalcuateState() {

  rawAmbientLight = sensorAlsPs.getAlsIrData(); // get current light level
  if (rawAmbientLight == 0xFFFF) { // probably absent sensor. Or Overload.
    if (sensorAlsPs.getFailureMode() == 0xFF ) {
      rawAmbientLight = MAX_LIGHT_LEVEL / 2; // probably absent sensor
    } else {
      rawAmbientLight = MAX_LIGHT_LEVEL; // probably overload
    }
  }
  sensorAlsPs.clearFailure();
  ambientLight = aver(map((rawAmbientLight >= MAX_LIGHT_LEVEL) ? MAX_LIGHT_LEVEL : rawAmbientLight, 250, MAX_LIGHT_LEVEL, 10, 255)); // усреднение


  timer500OddCycle = !timer500OddCycle; // флаг мигания, как для точек так и для цифр в режиме установки времени
  if ( timer500OddCycle ) {
    recalculateTime(); // время пересчитываем раз в секунду

    // в режиме часов и установки времени - мигать точкой
    if (workMode == CLOCK || (workMode == SETUP && (setupSubMode == S_HOUR || setupSubMode == S_MIN || setupSubMode == S_SEC))) {
      dotsOnOff(ON, ON);
    }
    // мигание для выбранных индиктаторов
    for (pos = 0; pos < 3; pos++) {
      if ( blinkPosition == pos || blinkPosition == ALL_SCREEN_BLINK || (blinkPosition == LAST_4_BLINK && ( pos == 0 || pos == 1 ) ) ) { // если для какой-то пары индикаторов включено мигание
        indicatorOnOff(&pos, indicatorOff); // выключить эту пару
      }
    }

  } else { // в нечетном цикле - все индикаторы в ON и точки погасить
    for (pos = 0; pos < 4; pos++) {
      indicatorOnOff(&pos, indicatorOn);
    }
    if (workMode == CLOCK || (workMode == SETUP && (setupSubMode == S_HOUR || setupSubMode == S_MIN || setupSubMode == S_SEC))) dotsOnOff(OFF, OFF);
  }
}
