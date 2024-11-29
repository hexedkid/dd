void recalculateTime() {
  timeChanged = true;

  if (TimerSilence.isReady()) enableBeep = true;

  timeSeconds++;
  if (timeSeconds > 59) {
    timeSeconds = 0;
    timeMinutes++;

    if (++minsCount >= SYNC_PERIOD && workMode != SETUP) {            // каждые XX мин
      minsCount = 0;
      resyncWithRTC();
    }

  }
  if (timeMinutes > 59) {
    timeMinutes = 0;
    timeHour++;
    if ( enableBeep && ( (beepMode == BEEP_ALWAYS ) || ( (beepMode == BEEP_DAYTIME ) && (timeHour >= 9) && (timeHour <= 22) ) ) ) {
      chirp();
      enableBeep = false;
      TimerSilence.setTimeout(BEEP_PAUSE);
    }
    if (timeHour > 23) {
      timeHour = 0;
      if ( workMode != SETUP) resyncWithRTC(); // новый день - надо вычитать новую дату из RTC
    }
  }
  updateTimeString();
}

void resyncWithRTC() {
  DateTime now = rtc.now();       // синхронизация с RTC
  timeHour = now.hour();
  timeMinutes = now.minute();
  timeSeconds = now.second();
  weekDay = now.dayOfTheWeek();
  dateDay = now.day();
  dateMonth = now.month();
  dateYear = now.year();

}

void updateDateString() {
  // обновить строку новым месяцем
  memcpy_P(&bufferLine[8], month[dateMonth - 1], 16); // array of 8 uint16_t -> 16 bytes
  // и новым днем
  memcpy_P(&bufferLine[8 + monLength[dateMonth - 1] + 1], wDay[weekDay], 22); // array of 8 uint16_t -> 16 bytes
  bufferLine[5] = pgm_read_word(&(digit[dateDay % 100 / 10])); // раскладываем день в две цифры и отправляем в общую строку
  bufferLine[6] = pgm_read_word(&(digit[dateDay % 10]));
}

void updateTimeString() {
  // разложим на отдельные цифры
  intermediateBuffer[5] = pgm_read_word(&(digit[timeSeconds % 10]));
  intermediateBuffer[4] = pgm_read_word(&(digit[(timeSeconds / 10) % 10]));
  intermediateBuffer[3] = pgm_read_word(&(digit[timeMinutes % 10]));
  intermediateBuffer[2] = pgm_read_word(&(digit[(timeMinutes / 10) % 10]));
  intermediateBuffer[1] = pgm_read_word(&(digit[timeHour % 10]));
  intermediateBuffer[0] = pgm_read_word(&(digit[(timeHour / 10) % 10]));
}

//Функция возвращает количество дней в месяце любого года с учетом високосных лет
byte dayOfmonth(byte Month, int Year) {
  Month += !Month - Month / 13; //если будет запрос на 0 или 13 месяц то будем считать для 1го и 12го
  return !(Year % 4) * !(Month - 2) + 28 + ((0x3bbeecc >> (Month * 2)) & 3);
}
