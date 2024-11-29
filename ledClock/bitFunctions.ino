static uint16_t set_with_mask(uint16_t was, uint16_t mask, uint16_t new_bits) { // only set the bits marked by mask
  uint16_t to_change = mask & new_bits;                                         // sanity
  uint16_t unchanged = ~mask & was;                                             // the bits of interest are 0 here
  uint16_t new_value = unchanged | to_change;
  return new_value;
}
static void set_with_mask(uint16_t *was, uint16_t mask, uint16_t new_bits) {
  *was = set_with_mask(*was, mask, new_bits);
}
/* устанавливает биты new_bits согласно маске mask прямо в was; то что в 'was' не попало в маску - остается неизменным */

bool isModeChanged() {
  return (workMode != prevMode);
};


void increaseMode() {
  do {
    if ( ++nextMode >= SETUP) nextMode = CLOCK;         // SETUP не выбирается переистыванием, доступны к перелистыванию режимы находящиеся ниже
  } while ( ! isModeEnabled(nextMode) );
  setCurrentMode(nextMode);
}

void decreaseMode() {
  do {
    if ( --nextMode == UINT8_MAX) nextMode = SETUP - 1; // листание "вниз", проверка на переход через 0
  } while ( ! isModeEnabled(nextMode) );
  setCurrentMode(nextMode);
}

void setCurrentMode(uint8_t newMode) {
  nextMode = newMode;
  memset(bufferLine, 0, sizeof(bufferLine));            // очистить cтроку вывода
  isRollFinished = true;
  rollPointer = 0;
  TimerReturnToClockMode.reset();
  TimerAutoModeChange.reset();
}


bool isModeEnabled(uint8_t mode) {
  return ((enabledModes >> mode) & 1);
}

// быстрый analogWrite
void setPWM(uint8_t pin, uint16_t duty) {
  //  if (duty == 0) setPin(pin, LOW);
  if (duty == 0) duty = 1;
  else {
    switch (pin) {
      case 5:
        bitSet(TCCR0A, COM0B1);
        OCR0B = duty;
        break;
      case 6:
        bitSet(TCCR0A, COM0A1);
        OCR0A = duty;
        break;
      case 10:
        bitSet(TCCR1A, COM1B1);
        OCR1B = duty;
        break;
      case 9:
        bitSet(TCCR1A, COM1A1);
        OCR1A = duty;
        break;
      case 3:
        bitSet(TCCR2A, COM2B1);
        OCR2B = duty;
        break;
      case 11:
        bitSet(TCCR2A, COM2A1);
        OCR2A = duty;
        break;
      default:
        break;
    }
  }
}
