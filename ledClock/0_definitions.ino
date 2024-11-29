#define DEBUG_MODE 1

#include <Wire.h>
#include <avr/pgmspace.h>
#include <SI1145_WE.h>

#include <GyverTimer.h>

#include "RTClib.h"

#include <OneWire.h>
#include <DallasTemperature.h>

#include <avr/eeprom.h>

#include <AnalogButtons.h>

#include <TLC59116_Unmanaged.h>
#include <TLC59116.h>

/* пины подключения точек индикатора */
#define DOT1 9                              // left dot
#define DOT2 10                             // right dot
#define BUTTONS_PIN A7                      // пин подключения кнопок
#define BUZZERPIN A3                        // пищалка
#define BUFFERLENGTH 37                     // длина буферной строки, для прокрутки

/* On/Off definitions */
#define ON true
#define OFF false

/* для EEPROM */
#define INIT_ADDR 1023                      // номер резервной ячейки
#define INIT_KEY 0x5A                       // ключ первого запуска. 0-254, на выбор

/* DS18B20 */
#define T_INT_PIN 5                         // пин для датчика внутренней температуры
#define T_EXT_PIN 6                         // пин для датчика внешней температуры

/* TLC59116 BEGIN */
TLC59116Manager tlcmanager;
#define zero 0
byte shadowRegisters[6][16];                //
uint16_t bufferLine[BUFFERLENGTH];          // буферная строка, ее полностью либо частично показывавем на экране
uint16_t intermediateBuffer[6];             // промежуточный буфер для значения времени
byte rollPointer = 0;                       // указатель на символ для "прокрутки" строки на экране
bool isRollFinished = false;                // флаг завершения "прокрутки" изображения на экране
/* TLC59116 END */

/* TIMERS BEGIN */
GTimer TimerReturnToClockMode(MS, 5000);   // таймер возврата в режим часов по бездействию
GTimer Timer300ms(MS, 250);                // 250-мс таймер для горизонтальной прокрутки
GTimer Timer71ms(MS, 71);                  // 71-мс таймер для вертикальной прокрутки
GTimer Timer500ms(MS, 500);                // полсекундный, основной счет времени и всех изменений
GTimer Timer100ms(MS, 100);                // опрос датчика приближения
GTimer TimerFinishSetup(MS, 30000);        // таймер возврата из режима сетапа по бездействию
GTimer TimerAutoModeChange(MS, 25000);     // таймер переключения авто-смены режимов
GTimer TimerSilence(MS);                   // таймер для периода "тишины" часового бипера, во избежание повторов при рассинхроне времени с RTC
/* TIMERS END */

/* SI-1145 BEGIN */
SI1145_WE sensorAlsPs = SI1145_WE();
uint8_t ambientLight = 0;                   // усредненные и нормализованные показания освещенности
uint16_t rawAmbientLight = 0;               // мгновенные данные по освещенности с датчика
uint16_t proximity = 0;                     // значение параметра "приближение"
bool proximityContinuing = false;           // новое или продолжающееся срабатывание по приближению
#define PROXIMITY_TRESHOLD 770              // порог превышения приближенности, при котором происходит срабатывание
#define NUM_AVER 10                         // размер выборки для усреднения(из скольки усредняем)
#define MAX_LIGHT_LEVEL 4500                // уровень освещенности, при достижении которого яркость выставлятся на макс уровень
/* SI-1145 END */


/* DS3231 BEGIN */
RTC_DS3231 rtc;
#define SYNC_PERIOD 10                      // раз в столько минут синхронизироваться c RTC
uint8_t timeHour;
uint8_t timeMinutes;
uint8_t timeSeconds = 59;
uint8_t dateDay;                            // это день месяца
uint8_t dateMonth;                          // месяц
uint8_t weekDay;                            // день недели
uint16_t dateYear;
byte minsCount = SYNC_PERIOD;               // счетчик минут для пересинхронизации с RTC
bool timer500OddCycle = false;              // флаг мигания, как для точек так и для цифр в режиме установки времени
bool timeChanged = true;                    // признак "время изменилось"
/* DS3231 END */


/* modes */
#define CLOCK 0
#define CALENDAR 1
#define T_INT 2                             // internal temperature
#define T_EXT 3                             // external temperature
#define SETUP 4                             // режим установки
uint8_t workMode = 0;                       // текущий режим работы
uint8_t nextMode = 0;                       // следующий режим, если происходит переключение
uint8_t prevMode = 0;                       // предыдущий режим работы, для проверки сменился он или таким и был
uint8_t enabledModes = 0b11000000;          // битовый флаг разрешенных режимов. Изначально разрешены часы и календарь, после сдвигов это будут два младших бита

int8_t autoChangeNextMode = CLOCK;
// int8_t autoChangeMaxMode;
#define BEEP_NONE 0                         // ежечасный сигнал отключен
#define BEEP_ALWAYS 1                       // ежечасный сигнал включен всегда
#define BEEP_DAYTIME 2                      // ежечасный сигнал включен с 9 до 22 часов
byte beepMode = BEEP_DAYTIME;               // ежечасный сигнал. 0 - молчать, 1 - каждый час, 2 - только с 9 до 22 часов
bool autoChangeModes = ON;                  // автоматическая смена режимов - регулярный показ даты, температуры и т.д.
#define BEEP_MODE_CELL 0                    // ячейка eeprom для хранения флага режима пищалки
#define AUTOCHANGE_MODE_CELL 1              // ячейка eeprom для хранения флага автосмены режмимов

/* мигание разрядов */
/* 0, 1, 2 - мигать соотвествующим разрядом индикатора (от секунд к часам) */
#define ALL_SCREEN_BLINK 3                  // если надо мигать всеми индикаторами 
#define LAST_4_BLINK 4                      // мигать 4 последними позициями
#define NO_BLINK 5                          // не мигать
byte blinkPosition = NO_BLINK;              // Каким "разрядом" мигать. 0-сек 1-мин 2-час 3-все индикаторы, 4-разряды 0 и 1, 5-не мингать
byte pos;                                   // позиция на индикаторе, для циклов по гашению/включению индикторов

/* кнопки */
AnalogButtons analogButtons(BUTTONS_PIN, INPUT);
Button butt1 = Button(254, &butt1Click);    // кнопка "вверх"
Button butt2 = Button(339, &butt2Click);    // кнопка "вниз"
Button butt3 = Button(509, &butt3Click, &butt3Hold, 400, 500); // кнопка "setup"

/* термометры */
OneWire ds18x20[] = { T_INT_PIN, T_EXT_PIN };
DallasTemperature tSensor[2];
// DeviceAddress deviceAddress[2];          // адреса датчиков

bool needToReadTemperature;                 // флаг готовности конверсии температуры
float temperature;                          // в эту переменную вычитывать температуру с датчиков

bool enableBeep = true;                     // разрешен ли звук в данный момент времени
#define BEEP_PAUSE 10000                    // миллисекунд тишины после часового сигнала; во избежание повторного звука если синхронизация с RTC сдвинула время

/* для режима setup */
#define S_YEAR 0
#define S_MON 1
#define S_DAY 2
#define S_HOUR 3
#define S_MIN 4
#define S_SEC 5
#define S_HOURLY_BEEPER 6
#define S_AUTOCHANGE 7

byte setupSubMode = S_AUTOCHANGE;
