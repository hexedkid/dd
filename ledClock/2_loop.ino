void loop() {

  if ( isRollFinished ) {
    prevMode = workMode;
    workMode = nextMode;
  }


  switch (workMode) {
    case CLOCK:
      showClock(ON); // с "прокруткой"
      break;
    case CALENDAR:
      showDate();
      break;
    case T_INT:
    case T_EXT:
      calculateTemperature(&workMode);
      break;
    case SETUP:
      doSetup();
      break;
  }

  if ( Timer500ms.isReady() ) {
    recalcuateState();
  }

  /* авто-смена режимов, если autoChangeModes установлено в true
     возврат в режим часов по другому таймеру */
  if ( (workMode != SETUP) && autoChangeModes && TimerAutoModeChange.isReady() ) {
    do {
      if (++autoChangeNextMode >= SETUP) autoChangeNextMode = CALENDAR;
    } while ( ! isModeEnabled(autoChangeNextMode) );
    setCurrentMode(autoChangeNextMode);
  }

  /* сбросить в режим часов, если не в сетапе, или прокрутили один раз дату */
  if ( ((workMode != SETUP) && TimerReturnToClockMode.isReady()) || ((workMode == CALENDAR) && isRollFinished) ) {
    nextMode = CLOCK;
  }

  if ( workMode != SETUP && Timer100ms.isReady() ) {
    readProximity(&proximity, &proximityContinuing);
  }

  if ( workMode == SETUP && TimerFinishSetup.isReady() ) {
    outOfSetup();
  }

  analogButtons.check();

}
