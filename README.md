# ublox_decoder
Decode Ublox M8L data
The decoder is modify from the rtklib, please refer to original rtklib code for more details.
The input data is UBX logged data from UBLOX M8L,
The output includes
1) the raw GNSS measurements (wk, ws, sys, prn, code, carrier phase, doppler, snr, lli)
2) imu raw data at 10Hz (wk, ws, fx, fy, fz, wx, wy, wz), wxyz => deg/s, fxyz, => m/s/s
3) GPS+IMU integrated solution from internal INS engine


add a RTCM decoder for easily use

Define solution type:
  Status 1: SPP solution.
  Status 2: DGNSS solution.
  Status 3: Dead reckoning only.
  Status 4: Carrier phase range solution with fixed ambiguities.
  Status 5: Carrier phase range solution with floating ambiguities.
  Status 6: GNSS + dead reckoning combined solution.
