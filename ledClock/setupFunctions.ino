void doSetup() {
  switch (setupSubMode) {
    case S_YEAR:
      {
        showSetupYear(&dateYear);
        break;;
      }
    case S_MON:
      {
        showSetupMonth(&dateMonth);
        break;;
      }
    case S_DAY:
      {
        showSetupDay(&dateDay);
        break;;
      }
    case S_HOUR:
    case S_MIN:
    case S_SEC:
      {
        timeChanged = true;
        showSetupTime(S_SEC - setupSubMode);
        break;;
      }
    case S_HOURLY_BEEPER:
    case S_AUTOCHANGE:
      {
        showSetupOther(setupSubMode);
        break;;
      }
  }
}

void outOfSetup() {
  blinkPosition = NO_BLINK;
  rtc.adjust(DateTime(dateYear, dateMonth, dateDay, timeHour, timeMinutes, timeSeconds));
  resyncWithRTC(); // вычитаем заново, т.к. мог смениться день недели
  TimerAutoModeChange.reset();
  if (eeprom_read_byte(BEEP_MODE_CELL) != beepMode) eeprom_update_byte(BEEP_MODE_CELL, beepMode);
  if (eeprom_read_byte((uint8_t *)AUTOCHANGE_MODE_CELL) != autoChangeModes) eeprom_update_byte((uint8_t *)AUTOCHANGE_MODE_CELL, autoChangeModes);
  isRollFinished = true;
  setCurrentMode(CLOCK);
}

void showSetupYear(uint16_t *s_year) {
  blinkPosition = LAST_4_BLINK;
  dotsOnOff(OFF, OFF);
  bufferLine[5] = pgm_read_word(&(digit[ *s_year % 10]));
  bufferLine[4] = pgm_read_word(&(digit[(*s_year / 10) % 10]));
  bufferLine[3] = pgm_read_word(&(digit[(*s_year / 100) % 10]));
  bufferLine[2] = pgm_read_word(&(digit[(*s_year / 1000) % 10]));
  bufferLine[1] = 0;
  bufferLine[0] = LETTER_G;
  outputToScreen(zero, 0xFFFF);
}

void showSetupMonth(uint8_t *s_month) {
  blinkPosition = ALL_SCREEN_BLINK;
  dotsOnOff(OFF, OFF);
  memcpy_P(&bufferLine[0], month[*s_month - 1], 16);
  switch (*s_month) { // смена окнончания(падежа)
    case 1:
    case 4:
    case 11:
      bufferLine[5] = LETTER_X;
      break;
    case 3:
      bufferLine[4] = 0;
      break;
    case 5:
      bufferLine[2] = LETTER_I;
      break;
    case 6:
    case 7:
      bufferLine[3] = LETTER_X;
      break;
  }
  outputToScreen(zero, 0xFFFF);
};

void showSetupDay(uint8_t *s_day) {
  blinkPosition = 0;
  dotsOnOff(OFF, OFF);
  memcpy_P(&bufferLine[0], word_ON_OFF_DAY[2], 8);
  bufferLine[5] = pgm_read_word(&(digit[ *s_day % 10]));
  bufferLine[4] = pgm_read_word(&(digit[(*s_day / 10) % 10]));
  outputToScreen(zero, 0xFFFF);
}

void showSetupTime( uint8_t currentPosition) {
  blinkPosition = currentPosition;
  updateTimeString();
  showClock(OFF); // показать время без прокрутки
}

void showSetupOther(byte otherMode) {
  blinkPosition = LAST_4_BLINK;
  dotsOnOff(ON, OFF);
  memcpy_P(&bufferLine[0], (otherMode == S_HOURLY_BEEPER) ? word_BEEPMODE : word_ROLLMODE, 4); // вывод слова, что устанавливается в этом режиме
  // bufferLine[4] = 0; // пустое место
  memcpy_P(&bufferLine[2], word_ON_OFF_DAY[(otherMode == S_HOURLY_BEEPER) ? beepMode : autoChangeModes], 8); // значение параметра в последней позиции
  outputToScreen(zero, 0xFFFF);
}
