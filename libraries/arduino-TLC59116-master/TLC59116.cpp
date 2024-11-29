#include <Arduino.h>
#include <Arduino.h>

#include "TLC59116.h"

const unsigned char TLC59116::Power_Up_Register_Values[TLC59116_Unmanaged::Control_Register_Max + 1] PROGMEM = {
  TLC59116_Unmanaged::MODE1_OSC_mask | TLC59116_Unmanaged::MODE1_ALLCALL_mask,
  0, // mode2
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // pwm0..15
  0xff, // grppwm
  0, // grpfreq
  0,0,0,0, // ledout0..3
  (TLC59116_Unmanaged::SUBADR1 << 1), (TLC59116_Unmanaged::SUBADR2 << 1), (TLC59116_Unmanaged::SUBADR3 << 1),
  (TLC59116_Unmanaged::AllCall_Addr << 1),
  TLC59116_Unmanaged::IREF_CM_mask | TLC59116_Unmanaged::IREF_HC_mask | ( TLC59116_Unmanaged::IREF_CC_mask & 0xff),
  0,0 // eflag1..eflag2
  };

void TLC59116::reset_happened() {
  this->reset_shadow_registers();
  }

//
// TLC59116Manager
//

void TLC59116Manager::init() {
  // Only once. We are mean.
  if (this->reset_actions & Already) { 
    return; 
    }
  this->reset_actions |= Already;

  if (reset_actions & WireInit) i2cbus.begin();
  // don't know how to set other WIRE interfaces
  if (&i2cbus == &Wire && init_frequency != 0) 
    {
    TWBR = ((F_CPU / init_frequency) - 16) / 2; // AFTER wire.begin
    }

  scan();
  if (reset_actions & Reset) reset(); // does enable

  }

int TLC59116Manager::scan() {
  // this code lifted & adapted from Nick Gammon (written 20th April 2011)
  // http://www.gammon.com.au/forum/?id=10896&reply=6#reply6
  // Thanks Nick!

  // FIXME: no rescan supported, no add/remove supported

  byte debug_tried = 0;

  this->device_ct = 0;
  for (byte addr = TLC59116_Unmanaged::Base_Addr; addr <= TLC59116_Unmanaged::Max_Addr; addr++) {
    debug_tried++;
    // If we hang here, then you did not do a Wire.begin();, or the frequency is too high, or communications is borked

    // yup, just "ping"
    i2cbus.beginTransmission(addr);
    int stat = i2cbus.endTransmission(); // the trick is: 0 means "something there"

    if (stat == 0) {
      if (addr == TLC59116_Unmanaged::AllCall_Addr) { continue; }
      if (addr == TLC59116_Unmanaged::Reset_Addr) { continue; }

      this->devices[ this->device_ct++ ] = new TLC59116(i2cbus,addr, *this);

      ::delay(10);  // maybe unneeded?
      } // end of good response

    } // end of for loop

  return this->device_ct;
  }

int TLC59116Manager::reset() {
  i2cbus.beginTransmission(TLC59116_Unmanaged::Reset_Addr);
    // You might think you could:
    // i2cbus.write((const byte*)&TLC59116_Unmanaged::Reset_Bytes, size_t(TLC59116_Unmanaged::Reset_Bytes));
    i2cbus.write( TLC59116_Unmanaged::Reset_Bytes >> 8 ); 
    i2cbus.write( TLC59116_Unmanaged::Reset_Bytes & 0xFF); 
  int rez = TLC59116_Unmanaged::_end_trans(i2cbus);

  if (rez)  { 
    return rez;
    }

//  broadcast().reset_happened();
  for (byte i=0; i< 6; i++) { devices[i]->reset_happened(); }
//  if (this->reset_actions & EnableOutputs) broadcast().enable_outputs();
  return 0;
  }

TLC59116& TLC59116::enable_outputs(bool yes, bool with_delay) {
  if (yes) {
    modify_control_register(MODE1_Register,MODE1_OSC_mask, 0x00); // bits off is osc on
    if (with_delay) delayMicroseconds(500); // would be nice to be smarter about when to do this
    }
  else {
    modify_control_register(MODE1_Register,MODE1_OSC_mask, MODE1_OSC_mask); // bits on is osc off
    }
  return *this;
  }

void TLC59116::modify_control_register(byte register_num, byte value) {
  if (shadow_registers[register_num] != value) {
      shadow_registers[register_num] = value;
      control_register(register_num, value);
      }
  }

void TLC59116::modify_control_register(byte register_num, byte mask, byte bits) {
  byte new_value = set_with_mask(shadow_registers[register_num], mask, bits);
  if (register_num < PWM0_Register || register_num > 0x17) {
    }
  modify_control_register(register_num, new_value);
  }

TLC59116& TLC59116::set_outputs(word pattern, word which) {
  // Only change bits marked in which: to bits in pattern

  // We'll make the desired ledoutx register set
  byte new_ledx[4];
  // need initial value for later comparison
  memcpy(new_ledx, &(this->shadow_registers[LEDOUT0_Register]), 4);

  // count through LED nums, starting from max (backwards is easier)

  for(byte ledx_i=15; ; ledx_i--) {
    if (0x8000 & which) {
      new_ledx[ledx_i / 4] = LEDx_set_mode(
        new_ledx[ledx_i / 4], 
        ledx_i, 
        (pattern & 0x8000) ? LEDOUT_DigitalOn : LEDOUT_DigitalOff
        );
      }
    pattern <<= 1;
    which <<= 1;

    if (ledx_i==0) break; // can't detect < 0 on an unsigned!
    }

  update_registers(new_ledx, LEDOUT0_Register, LEDOUTx_Register(15));
  return *this;
  }

TLC59116& TLC59116::set_outputs(byte led_num_start, byte led_num_end, const byte brightness[] /*[ct]*/ ) {
  // We are going to start with current shadow values, mark changes against them, 
  //  then write the smallest range of registers.
  // We are going to write LEDOUTx and PWMx registers together to prevent flicker.
  // This becomes less efficient if the only changed PWMx is at a large 'x',
  // And if a LEDOUTx needs to be changed to PWM,
  // And/Or if the changed PWMx is sparse.
  // But, pretty efficient if the number of changes (from first change to last) is more than 1/2 the range.

  byte ct = led_num_end - led_num_start + 1;

  // need current values to mark changes against
  // FIXME: We could try to minimize the register_count: to start with the min(PWMx), but math
  byte register_count = /* r0... */ LEDOUTx_Register(Channels-1) + 1;
  byte want[register_count]; // I know that TLC59116 is: veryfew...PWMx..LEDOUTx. so waste a few at head
  memcpy(want, this->shadow_registers, register_count);

  // (for ledoutx, Might be able to do build it with <<bitmask, and then set_with_mask the whole thing)
  for(byte ledi=led_num_start; ledi < led_num_start + ct; ledi++) {
    // ledout settings
    byte wrapped_i = ledi % 16; // %16 wraps
    byte out_r = LEDOUTx_Register(wrapped_i);
    want[out_r] = LEDx_set_mode(want[out_r], wrapped_i, LEDOUT_PWM); 
    
    // PWM
    want[ PWMx_Register(wrapped_i) ] = brightness[ledi - led_num_start];
    }

  update_registers(&want[PWM0_Register], PWM0_Register, LEDOUTx_Register(Channels-1));
  return *this;
  }

TLC59116& TLC59116::group_pwm(word bit_pattern, byte brightness) {
  // Doing them together to minimize freq/mode being out of sync
  byte register_count = LEDOUTx_Register(Channels-1) + 1;
  byte want[register_count]; // 0..MODE2_Register...GRPPWM_Register,GRPFREQ_Register,LEDOUTx...; wasting 1

  // not touching PWM registers
  memcpy( &want[PWM0_Register], &shadow_registers[PWM0_Register],  GRPPWM_Register - PWM0_Register);
  // start with extant LEDOUTx values
  memcpy(&want[LEDOUT0_Register], &shadow_registers[LEDOUT0_Register], LEDOUTx_Register(Channels-1) - LEDOUT0_Register+1);

  want[MODE2_Register] = set_with_mask(shadow_registers[MODE2_Register], MODE2_DMBLNK, 0);
  want[GRPPWM_Register] = brightness;
  want[GRPFREQ_Register] = 0; // not actuall "don't care", it's ~dynamic_range
  LEDx_set_mode( &want[LEDOUT0_Register], LEDOUT_GRPPWM, bit_pattern);

  // do it
  update_registers(&want[MODE2_Register], MODE1_Register, register_count-1);
  return *this;
  }

TLC59116& TLC59116::group_blink(word bit_pattern, int blink_delay, int on_ratio) {
  // Doing them together to minimize freq/mode being out of sync
  byte register_count = LEDOUTx_Register(Channels-1) + 1;
  byte want[register_count]; // 0..MODE2_Register...GRPPWM_Register,GRPFREQ_Register,LEDOUTx...; wasting 1

  // not touching PWM registers
  memcpy( &want[PWM0_Register], &shadow_registers[PWM0_Register],  GRPPWM_Register - PWM0_Register);
  // start with extant LEDOUTx values
  memcpy(&want[LEDOUT0_Register], &shadow_registers[LEDOUT0_Register], LEDOUTx_Register(Channels-1) - LEDOUT0_Register+1);

  // Hypoth: DMBLNK resets the grpfreq timer. Nope.
  // want[MODE2_Register] = set_with_mask(shadow_registers[MODE2_Register], MODE2_DMBLNK, ~MODE2_DMBLNK);
  // want[GRPPWM_Register] = 0xFF;
  // want[GRPFREQ_Register] = 0;
  // LEDx_set_mode( &want[LEDOUT0_Register], LEDOUT_PWM, bit_pattern);
  // update_registers(&want[MODE1_Register], MODE1_Register, 1);

  // Hypoth: OSC resets the grpfreq timer. Nope.
  // want[MODE1_Register] = set_with_mask(shadow_registers[MODE1_Register], MODE1_OSC_mask, MODE1_OSC_mask);
  // update_registers(&want[MODE1_Register], MODE1_Register, 1);
  // delayMicroseconds(501);

  // Only Reset resetsthe grpfreq timer.

  want[MODE2_Register] = set_with_mask(shadow_registers[MODE2_Register], MODE2_DMBLNK, MODE2_DMBLNK);
  want[GRPPWM_Register] = on_ratio;
  want[GRPFREQ_Register] = blink_delay;
  LEDx_set_mode( &want[LEDOUT0_Register], LEDOUT_GRPPWM, bit_pattern);

  // do it
  update_registers(&want[MODE2_Register], MODE2_Register, register_count-1);
  return *this;
  }

void TLC59116::update_registers(const byte want[] /* [start..end] */, byte start_r, byte end_r) {
  // Update only the registers that need it
  // 'want' has to be (shadow_registers & new_value)
  // want[0] is register_value[start_r], i.e. a subset of the full register set
  // Does wire.begin, write,...,end
  // (_i = index of a led 0..15, _r is register_number)
  // now find the changed ones
  //  best case is: no writes
  //  2nd best case is some subrange

  const byte *want_fullset = want - start_r; // now want[i] matches shadow_register[i]

  // First change...
  bool has_change_first = false;
  byte change_first_r; // a register num
  for (change_first_r = start_r; change_first_r <= end_r; change_first_r++) {
    if (want_fullset[change_first_r] != shadow_registers[change_first_r]) {
      has_change_first = true; // found it
      break;
      }
    }

  // Write the data if any changed

  if (!has_change_first) { // might be "none changed"

    }
  else {
    // Find last change
    byte change_last_r; // a register num
    for (change_last_r = end_r; change_last_r >= change_first_r; change_last_r--) {
      if (want_fullset[change_last_r] != shadow_registers[change_last_r]) {
        break; // found it
        }
      }

    // We have a first..last, so write them
    _begin_trans(Auto_All, change_first_r);
      // TLC59116Warn("  ");
      i2cbus.write(&want_fullset[change_first_r], change_last_r-change_first_r+1);
    _end_trans();
    // update shadow
    memcpy(&shadow_registers[change_first_r], &want_fullset[change_first_r], change_last_r-change_first_r+1);
    // FIXME: propagate shadow
    }
  }

TLC59116& TLC59116::describe_shadow() {
  Serial.print(Device);Serial.print(F(" 0x"));Serial.print(address(),HEX);
  Serial.print(F(" on bus "));Serial.print((unsigned long)&i2cbus, HEX);Serial.println();
  Serial.println(F("Shadow Registers"));
  TLC59116_Unmanaged::describe_registers(shadow_registers); 
  return *this; 
  }

TLC59116& TLC59116::set_address(const byte address[/* sub1,sub2,sub3,all */], byte enable_mask /* MODE1_ALLCALL_mask | MODE1_SUB1_mask... */) {
  // does much:
  // sets the adresses, if != 0
  // enables/disables if corresponding address !=0
  byte want_mode1 = shadow_registers[MODE1_Register];
  byte want_addresses[4];
  memcpy(want_addresses, &shadow_registers[SUBADR1_Register], 4);

  for(byte i=0; i<4; i++) {
    if (address[i] != 0) {
      // actual address
      if (address[i]==Reset_Addr) {
        continue;
        }

      want_addresses[i] = address[i];
      if (i==3) {
        // enable'ment
        set_with_mask(&want_mode1, MODE1_ALLCALL_mask, enable_mask);
        }
      else {
        // enable'ment
        set_with_mask(&want_mode1, MODE1_SUB1_mask >> (i), enable_mask);
        }
      }
    }
  update_registers( &want_mode1, MODE1_Register, MODE1_Register);
  update_registers( want_addresses, SUBADR1_Register, AllCall_Addr_Register);
  return *this;
  }


TLC59116& TLC59116::SUBADR_address(byte which, byte address, bool enable) {
  byte want_address[4] = {0,0,0,0};
  if (which==0 || which > 3) { return *this;}
  want_address[which-1] = address << 1;
  return set_address(want_address, enable ? MODE1_SUB1_mask >> (which-1) : 0);
  }

TLC59116& TLC59116::allcall_address(byte address, bool enable) {
  byte want_address[4] = {0,0,0,address << 1};
  return set_address(want_address, enable ? MODE1_ALLCALL_mask : 0);
  }

TLC59116& TLC59116::allcall_address_enable() {
  byte want_mode1 = shadow_registers[MODE1_Register];
  set_with_mask(&want_mode1, MODE1_ALLCALL_mask, MODE1_ALLCALL_mask);
  update_registers( &want_mode1, MODE1_Register, MODE1_Register);
  }

TLC59116& TLC59116::allcall_address_disable() {
  byte want_mode1 = shadow_registers[MODE1_Register];
  set_with_mask(&want_mode1, MODE1_ALLCALL_mask, 0);
  update_registers( &want_mode1, MODE1_Register, MODE1_Register);
  }

TLC59116& TLC59116::resync_shadow_registers() {
  byte rez = fetch_registers(0, Control_Register_Max+1, shadow_registers);
  return *this;
  }

TLC59116& TLC59116::set_milliamps(byte ma, int Rext) {
  byte iref_value = best_iref(ma, Rext);
  modify_control_register(IREF_Register,iref_value);
  return *this;
  }

//
// Broadcast
//

TLC59116::Broadcast& TLC59116::Broadcast::enable_outputs(bool yes, bool with_delay) {
  TLC59116::enable_outputs(yes, with_delay);
  propagate_register(MODE1_Register);
  return *this;
  }

void TLC59116::Broadcast::propagate_register(byte register_num) {
  byte my_value = shadow_registers[register_num];
  for (byte i=0; i<= manager.device_ct; i++) { manager.devices[i]->shadow_registers[register_num]=my_value; }
  }
