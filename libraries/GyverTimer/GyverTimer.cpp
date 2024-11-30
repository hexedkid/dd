#include <Arduino.h>
#include "GyverTimer.h"

// конструктор
GTimer::GTimer(timerType type, uint32_t interval) {
	setInterval(interval);		// запуск в режимі Інтервалу
	_type = type;				// встановлення типу
}

// запуск в режимі інтервала
void GTimer::setInterval(uint32_t interval) {
	if (interval != 0) {				// захист від ввода 0
		_interval = interval;			// встановлення
		_mode = TIMER_INTERVAL;			// режим "інтервал"
		start();						// скидання и запуск	
	} else stop();						// зупинка, якщо час == 0
}

// запуск в режимі таймаута
void GTimer::setTimeout(uint32_t timeout) {
	setInterval(timeout);		// дати інтервал і запустити
	_mode = TIMER_TIMEOUT;		// режим "таймаут"
}

// зупинити рахунок
void GTimer::stop() {
	_state = false;
	_resumeBuffer = ( (_type) ? millis() : micros() ) - _timer;		// запомнить "час зупинки"
}

// продовжити рахунок
void GTimer::resume() {
	start();
	_timer -= _resumeBuffer;	// відновили час зупинки
}

// перезапустити рахунок
void GTimer::start() {
	_state = true;
	reset();
}

// скидання періода
void GTimer::reset() {
	_timer = (_type) ? millis() : micros();
}

// стан
boolean GTimer::isEnabled() {
	return _state;
}

// перевірка таймера v2.0 (дотримання інтервалів, захист від пропуску та переповнення)
boolean GTimer::isReady() {	
	if (!_state) return false;							// якщо таймер зупинено
	uint32_t thisTime = (_type) ? millis() : micros();	// поточний час
	if (thisTime - _timer >= _interval) {				// Перевірка часу
		if (_mode) {						// якщо режим інтервалів
			do {
				_timer += _interval;
				if (_timer < _interval) break;			//  Захист від переповнення uint32_t
			} while (_timer < thisTime - _interval);	// Захист від пропуску кроку			
		} else {							// якщо режим таймауту
			_state = false;					// Зупинити таймер
		}
		return true;
	} else {
		return false;
	}
}

// змінити режим власноруч
void GTimer::setMode(boolean mode) {
	_mode = mode;
}

// ================== УСТАРЕЛО (но працює, для сумісності) ===================

// ====== millis ======
GTimer_ms::GTimer_ms() {}

GTimer_ms::GTimer_ms(uint32_t interval) {
	_interval = (interval == 0) ? 1 : interval;    // захист від введення 0
	reset();
}

void GTimer_ms::setInterval(uint32_t interval) {
	_interval = (interval == 0) ? 1 : interval;    // захист від введення 0
	GTimer_ms::reset();
	_state = true;
	_mode = true;
}
void GTimer_ms::setTimeout(uint32_t interval) {
	setInterval(interval);
	_mode = false;
}
void GTimer_ms::setMode(uint8_t mode) {
	_mode = mode;
}
void GTimer_ms::start() {
	_state = true;
}
void GTimer_ms::stop() {
	_state = false;
}
boolean GTimer_ms::isReady() {
	if (!_state) return false;
	uint32_t thisMls = millis();
	if (thisMls - _timer >= _interval) {
		if (_mode) {
			do {
				_timer += _interval;
				if (_timer < _interval) break;          // переповнення uint32_t
			} while (_timer < thisMls - _interval);  // захист від пропускання кроку      
		}
		else {
			_state = false;
		}
		return true;
	}
	else {
		return false;
	}
}

void GTimer_ms::reset() {
	_timer = millis();
}

// ====== micros ======
GTimer_us::GTimer_us() {}

GTimer_us::GTimer_us(uint32_t interval) {
	_interval = (interval == 0) ? 1 : interval;    // захист від введення 0
	reset();
}

void GTimer_us::setInterval(uint32_t interval) {
	_interval = (interval == 0) ? 1 : interval;    // захист від введення 0
	GTimer_us::reset();
	_state = true;
	_mode = true;
}
void GTimer_us::setTimeout(uint32_t interval) {
	setInterval(interval);
	_mode = false;
}
void GTimer_us::setMode(uint8_t mode) {
	_mode = mode;
}
void GTimer_us::start() {
	_state = true;
}
void GTimer_us::stop() {
	_state = false;
}
boolean GTimer_us::isReady() {
	if (!_state) return false;
	uint32_t thisUs = micros();
	if (thisUs - _timer >= _interval) {
		if (_mode) {
			do {
				_timer += _interval;
				if (_timer < _interval) break;          // переповнення uint32_t
			} while (_timer < thisUs - _interval);  // захист від пропускання кроку      
		}
		else {
			_state = false;
		}
		return true;
	}
	else {
		return false;
	}
}

void GTimer_us::reset() {
	_timer = micros();
}
