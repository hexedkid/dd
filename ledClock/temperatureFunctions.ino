
void calculateTemperature(uint8_t *mode) {
  byte sensorNumber = *mode - 2; // 2 - внутренняя т-ра, 3 - внешняя т-ра. Отнимая два, получим индекс датчика
  // при переходе в режим показа т-ры - запросить ее на датчике
  if (isModeChanged()) {
    temperature = -127;
    tSensor[sensorNumber].requestTemperatures();
    needToReadTemperature = true;
  }
  // прочитать единожды по готовности, далее до конца показа в этом режиме датчик не дергать
  if (needToReadTemperature  && tSensor[sensorNumber].isConversionComplete()) {
    needToReadTemperature = false;
    temperature = tSensor[sensorNumber].getTempCByIndex(0); //здесь чтение т-ры внутр
    // temperature = tSensor[sensorNumber].getTempC(deviceAddress[sensorNumber]);
    int tShow = abs(temperature) * 10;
    bufferLine[0] = (*mode == T_INT) ? tmprInt : tmprExt; // in/out значок
    if ( tShow == 1270 ) { // -127 при отсутствии датчика. Здесь - модуль от умноженного на 10 (выше), чтобы не сравнивать float
      for (byte pos = 1; pos < 5; pos++) {
        bufferLine[pos] = 0;
      }
      dotsOnOff(OFF, OFF);
    } else {
      byte highDigit = tShow / 100 % 10;
      if (highDigit == 0) {
        bufferLine[2] = ( temperature >= 0) ? plusSign : minusSign;
        bufferLine[1] = 0;
      } else {
        bufferLine[1] = ( temperature >= 0) ? plusSign : minusSign;
        bufferLine[2] = pgm_read_word(&(digit[highDigit]));
      }
      bufferLine[3] = pgm_read_word(&(digit[tShow / 10 % 10]));
      bufferLine[4] = pgm_read_word(&(digit[tShow % 10]));
      dotsOnOff(OFF, ON);
    }
    bufferLine[5] = degreeSign;

    Roll (&rollPointer, 0, &isRollFinished);
  }
}
