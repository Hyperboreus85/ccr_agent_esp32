# TODO

1. Set up PlatformIO project structure (src/, include/, data/).
2. Implement configuration/constants and data models for samples/events.
3. Build modular components:
   - WifiManager
   - TimeSync
   - VoltageSampler (ADC1 + RMS)
   - EventDetector + ring buffer
   - BatchUploader + StorageQueue (LittleFS)
4. Add minimal serial CLI for calibration gain/offset and assisted calibration output.
5. Wire everything in `main.cpp` with logging and backoff retries.
6. Document wiring, safety, calibration, and build/flash steps in README.
