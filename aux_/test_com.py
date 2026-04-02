import serial, sys, time
try:
  s = serial.Serial('COM4', 115200, timeout=1)
  s.setDTR(False)
  s.setRTS(True)
  time.sleep(0.1)
  s.setRTS(False)
  print('Waiting... flush.')
  while True:
    data = s.read(1024)
    if data:
      sys.stdout.write(data.decode('utf-8', 'replace'))
      sys.stdout.flush()
except Exception as e:
  print(e)
