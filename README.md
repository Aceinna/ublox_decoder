# u-blox GNSS receiver data decoder
#### Support M8, F9 series data

- M8L
- F9P, F9K

The decoder is modify from the rtklib, please refer to original rtklib code for more details.

The output includes
1) the raw GNSS measurements (wk, ws, sys, prn, code, carrier phase, doppler, snr, lli)
2) imu raw data at 10Hz (wk, ws, fx, fy, fz, wx, wy, wz), wxyz => deg/s, fxyz, => m/s/s
3) GPS+IMU integrated solution from internal INS engine

#### Definitions

Define solution type:
  Status 1: SPP solution.
  Status 2: DGNSS solution.
  Status 3: Dead reckoning only.
  Status 4: Carrier phase range solution with fixed ambiguities.
  Status 5: Carrier phase range solution with floating ambiguities.
  Status 6: GNSS + dead reckoning combined solution.
