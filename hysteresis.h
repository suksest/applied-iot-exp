#pragma once

// Ambang hysteresis kontrol relay (derajat Celsius)
const float TEMP_ON  = 30.0;  // relay ON jika suhu >= 30 °C
const float TEMP_OFF = 27.0;  // relay OFF jika suhu <= 27 °C

// Menentukan status relay berikutnya berdasarkan suhu dan status saat ini.
// Di antara 27 °C dan 30 °C status sebelumnya dipertahankan (hysteresis).
inline bool nextRelayState(float temperature, bool currentState) {
  if (temperature >= TEMP_ON)  return true;
  if (temperature <= TEMP_OFF) return false;
  return currentState;
}
