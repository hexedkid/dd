void butt1Click() {
  if (workMode != SETUP) {
    increaseMode();
  } else {
    TimerFinishSetup.reset();
    switch (setupSubMode) {
      case S_YEAR:
        {
          if (++dateYear > 2040) dateYear = 2021;
          break;
        }
      case S_MON:
        {
          if (++dateMonth > 12) dateMonth = 1;
          break;
        }
      case S_DAY:
        {
          byte maxDay = dayOfmonth(dateMonth, dateYear);
          if (++dateDay > maxDay) dateDay = 1;
          break;
        }
      case S_HOUR:
        {
          if (++timeHour > 23) timeHour = 0;
          break;
        }
      case S_MIN:
        {
          if (++timeMinutes > 59) timeMinutes = 0;
          break;
        }
      case S_SEC:
        {
          timeSeconds = 0;
          break;
        }
      case S_HOURLY_BEEPER:
        {
          if (++beepMode > BEEP_DAYTIME) beepMode = BEEP_NONE;
          break;
        }
      case S_AUTOCHANGE:
        {
          autoChangeModes = !autoChangeModes;
          break;
        }
    }
  }
}

void butt2Click() {
  if (workMode != SETUP) {
    decreaseMode();
  } else {
    TimerFinishSetup.reset();
    switch (setupSubMode) {
      case S_YEAR:
        {
          if (--dateYear < 2021) dateYear = 2040;
          break;
        }
      case S_MON:
        {
          if (--dateMonth < 1) dateMonth = 12;
          break;
        }
      case S_DAY:
        {
          byte maxDay = dayOfmonth(dateMonth, dateYear);
          if (--dateDay < 1 ) dateDay = maxDay;
          break;
        }
      case S_HOUR:
        {
          if (--timeHour == UINT8_MAX) timeHour = 23;
          break;
        }
      case S_MIN:
        {
          if (--timeMinutes == UINT8_MAX) timeMinutes = 59;
          break;
        }
      case S_SEC:
        {
          timeSeconds = 0;
          break;
        }
      case S_HOURLY_BEEPER:
        {
          if (--beepMode == UINT8_MAX) beepMode = BEEP_DAYTIME;
          break;
        }
      case S_AUTOCHANGE:
        {
          autoChangeModes = !autoChangeModes;
          break;
        }
    }
  }
}

void butt3Click() {
  if ( workMode == SETUP) {
    TimerFinishSetup.reset();
    if (++setupSubMode > S_AUTOCHANGE) setupSubMode = S_YEAR; // пробегаем по всем режимам сетапа
  }
}

void butt3Hold() {
  if ( workMode != SETUP) {
    workMode = SETUP;
    nextMode = SETUP;                                         // чтобы не вываливаться при показе времени
    setupSubMode = S_YEAR;
    TimerFinishSetup.reset();
  } else {
    outOfSetup();
  }
}
