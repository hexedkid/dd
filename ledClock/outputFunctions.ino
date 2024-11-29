void showClock(bool rollOnOff) {                          // параметр - показывать с "прокруткой" (true) или просто отрисовать (false)
  if ( isRollFinished && timeChanged ) {
    memcpy(&bufferLine[0], &intermediateBuffer[0], 12);   // array of 6 uint16_t -> 12 bytes
    timeChanged = false;
  }
  if ( Timer71ms.isReady() ) {
    Roll (&rollPointer, rollOnOff ? 5 : 0, &isRollFinished);
  }
}

void showDate() {
  for (byte pos = 0; pos < 4; pos++) {
    indicatorOnOff(&pos, indicatorOn);                    // включить все индикаторы, если на каких-то было мигание и они остались отключены
  }
  dotsOnOff(OFF, OFF);
  if (isModeChanged()) updateDateString();                // только если сменилась дата или режим отображения
  if ( Timer300ms.isReady() ) {
    Roll (&rollPointer, 7 + monLength[dateMonth - 1] + 1 + wDayLength[weekDay] + 10, &isRollFinished); // добить строку 5 пробелами
  }
}

void Roll( byte *pointer, byte len, bool *finished) {
  /* прокрутка  бегущей строки либо "сверху вниз"
     направление определяется по значению "длина" (второй параметр):
           если он равен 5 (кол-во линий на экране) - прокрутка вертикальна
           в противном случае горизонтальная */
  if ( len == 5 ) { // вертикальная прокрутка
    outputToScreen(zero, (*pointer == 6) ? 0xFFFF : pgm_read_word(&(row[*pointer]))); // 0 - начальная позиция в выводимой bufferline, второй параметр - паттерн обновляемой "линии"
  } else if ( len == 0 ) {                                                            // без прокрутки, просто отрисовать
    outputToScreen(zero, 0xFFFF);
  } else {                                                                            // горизонтальная прокрутка
    outputToScreen(*pointer, 0xFFFF);                                                 // pointer - начальная позиция в выводимой bufferline, второй параметр - паттерн обновляемой "линии"
  }
  *pointer += 1;
  if (*pointer > (( len == 5 ) ? 6 : len - 6) ) {
    *pointer = 0;
    *finished = true;
  } else {
    *finished = false;
  }
}

void outputToScreen(byte pointer, uint16_t mask) {
  static uint16_t pwmOnOffValue;
  static uint16_t _mask;
  for (byte tlc = 0; tlc < 6; tlc++) {
    pwmOnOffValue = 0;
    _mask = mask;
    for (byte led_num = 15; ; led_num--) {                                            // по каждому леду, от старшего к младшему
      pwmOnOffValue <<= 1;
      if (shadowRegisters[tlc][led_num] > 0) {                                        // по состоянию каждого pwm регистра
        pwmOnOffValue += 1;                                                           // если он не нулевой - выставить в единицу бит в отображении текущего состояния
      }
      if (led_num == 0) break;                                                        // can't detect < 0 on an unsigned!
    }

    /* в pwmOnOffValue - биты в 1 для вкл. ледов */
    set_with_mask(&pwmOnOffValue, (uint16_t)mask, (uint16_t)bufferLine[pointer + tlc]);

    /* пробежать по всем ледам */
    for (byte led_num = 0; led_num <= 15; led_num++) { // по каждому леду, от старшего к младшему
      shadowRegisters[tlc][led_num] = (pwmOnOffValue & 1 != 0) ? 1 : 0;
      if (_mask & 1 != 0) { // only update changing leds
        (pwmOnOffValue & 1 != 0) ? tlcmanager[(int)5 - tlc].pwm(led_num, ambientLight) : tlcmanager[(int)5 - tlc].pwm(led_num, 0); // set led on or off
      }
      pwmOnOffValue >>= 1;
      _mask >>= 1;
    }
  }
}

// функция высчитывающая среднее
int aver(int val) {
  static byte t = 0;
  static int vals[NUM_AVER];
  static int average = 0;
  if (++t >= NUM_AVER) t = 0; // перемотка t
  average -= vals[t];     // вычитаем старое
  average += val;         // прибавляем новое
  vals[t] = val;          // запоминаем в массив
  return (average / NUM_AVER);
}

// короткий звук
void chirp() {
  tone(BUZZERPIN, 3200, 90);
}

// вкл/выкл индикатор в нужном разряде (разряд 0 - индикаторы 0 и 1, разряд 1 - инд. 2 и 3, разр. 2 - инд. 4 и 5)
void indicatorOnOff(byte *part, byte OnOff) {
  for (byte LEDx_register = TLC59116_Unmanaged::LEDOUT0_Register; LEDx_register < TLC59116_Unmanaged::LEDOUT0_Register + 4; LEDx_register++ ) {
    tlcmanager[(int)*part * 2].control_register(LEDx_register, OnOff);
    tlcmanager[(int)*part * 2 + 1].control_register(LEDx_register, OnOff);
  }
}

void dotsOnOff(bool dot1state, bool dot2state) {
  byte dotBrightness = 0xFF - (ambientLight - 1);
  setPWM(DOT1, dot1state ? dotBrightness : dotOff);
  setPWM(DOT2, dot2state ? dotBrightness : dotOff);
}
