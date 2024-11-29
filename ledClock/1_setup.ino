
void setup() {

  // Serial.begin(115200);

  /* Инициализация сенсора ALS-PS и смена ему адреса, т.к. его дефолтный 0х60 занят */
  Wire.begin();
  sensorAlsPs.init();
  sensorAlsPs.setI2CAddress(0x29);

  /* init all TLC59116 */
  tlcmanager.init();
  static byte tlc;
  for (tlc = 0; tlc < 6; tlc++) {
    tlcmanager[(int)tlc].allcall_address_disable();
    tlcmanager[(int)tlc].enable_outputs(true, false);
    tlcmanager[(int)tlc].pwm((byte)0, (byte)15, (byte)0);
  }

  /* для вывода цифр времени, при 6-ке вывод цифр произойдет без "прокрутки" по вертикали
     при переходе в режим вывода "цифр" выставлять переменную в 6,
     при переходе в режим "бегущей строки" - выставлять в "ноль" */
  rollPointer = 6;

  // настраиваем сенсор ALS-PS
  sensorAlsPs.disableAllInterrupts();
  sensorAlsPs.disableHighSignalVisRange();
  /* choices: PS_TYPE, ALS_TYPE, PSALS_TYPE, ALSUV_TYPE, PSALSUV_TYPE || FORCE, AUTO, PAUSE */
  // sensorAlsPs.enableMeasurements(0b00000011, 0b00001100);
  sensorAlsPs.enableMeasurements(SI1145MeasureType::PSALS_TYPE, SI1145MeasureMode::AUTO1);
  /* choose gain value: 0, 1, 2, 3, 4, 5, 6, 7 */
  //sensorAlsPs.setAlsVisAdcGain(7);
  sensorAlsPs.setAlsIrAdcGain(6);
  /* choose gain value: 0, 1, 2, 3, 4, 5 */
  sensorAlsPs.setPsAdcGain(3);
  sensorAlsPs.selectPsDiode(SMALL_DIODE);
  sensorAlsPs.enableHighResolutionVis();
  sensorAlsPs.disableHighResolutionPs();
  sensorAlsPs.disableHighSignalPsRange();

  /* инициализация модуля RTC */
  rtc.begin();
  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  recalculateTime();                                // синхронизироваться с RTC
  updateDateString();                               // текущую дату в строку для показа
  pinMode(DOT1, OUTPUT);
  pinMode(DOT2, OUTPUT);

  pinMode(BUZZERPIN, OUTPUT);

  /* инициализация модулей датчика температуры
     и фиксация разрешени/запрещения режима в зависимости от наличия датчика */
  DeviceAddress deviceAddress;                      // адреса датчиков
  for (byte i = 0; i < 2; i++) {
    tSensor[i].setOneWire(&ds18x20[i]);
    tSensor[i].begin();
    enabledModes >>= 1;       // сдвиг на 1 разряд вправо
    if (tSensor[i].getAddress(deviceAddress, 0)) {
      enabledModes += 0b10000000;                   // разрешить режим если датчик найден
      tSensor[i].setResolution(deviceAddress, 11);  // 9 - неточно, 12 - очень долго
      tSensor[i].setWaitForConversion(false);       // не ожидать пока т-ра посчитается датчиком, т.к. вся программа на это время останавливается
    }
  }
  /* начиная с режима SETUP и далее - биты разрешения неиспользуемые. Всего может быть до 8 режимов (часы, календарь, температура-1, -2, и т.д.) */
  enabledModes >>= ( 8 - SETUP );

  /* вычитываем установки из eeprom */
  if (eeprom_read_byte((uint8_t *)INIT_ADDR) != INIT_KEY) { // первый запуск
    eeprom_update_byte((uint8_t *)INIT_ADDR, INIT_KEY);
    eeprom_update_byte((uint8_t *)BEEP_MODE_CELL, beepMode);
    eeprom_update_byte((uint8_t *)AUTOCHANGE_MODE_CELL, autoChangeModes);
  } else {                                                  // не первый запуск - вычитаем сохраненные установки
    beepMode = eeprom_read_byte((uint8_t *)BEEP_MODE_CELL);
    autoChangeModes = eeprom_read_byte((uint8_t *)AUTOCHANGE_MODE_CELL);
  }

  /* кнопки определить */
  analogButtons.add(butt1);
  analogButtons.add(butt2);
  analogButtons.add(butt3);
}
