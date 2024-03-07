# Arduino Serial graph monitor

# Important note:
#   To make this script work properly, run arduino 'CH4-sensor.ino' program with:
#      const bool EXTERNAL_PYTHON_MONITOR = true;

import serial
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from collections import deque
import datetime
import signal
import os

baudRate = 9600 # maybe 115200
arduinoDevice = '/dev/ttyACM0'
arduinoSerialData = serial.Serial(arduinoDevice, baudRate)

fig = plt.figure()
ax = fig.add_subplot(1,1,1)

max_length = 1000 # restrict animation to set CPU usage limit ('None' to remove limits)
xdata = deque(maxlen=max_length)
ydata = deque(maxlen=max_length)

# SIGUSR1 to reset data
def signal_handler(sig, frame):
    print(f'SIGUSR1 signal ({sig}) captured (pid {os.getpid()})')
    xdata.clear()
    ydata.clear()

signal.signal(signal.SIGUSR1, signal_handler)

try:
  logfile = open('serial.log', 'w')

  while True:
    if (arduinoSerialData.inWaiting() > 0):
      def animate(i,xdata,ydata):
        data = arduinoSerialData.readline().decode().strip()

        try: # avoid first fail
          data = float(data)
        except:
          data = 0

        xdata.append(i)
        ydata.append(data)

        ax.clear()
        ax.plot(xdata,ydata)

        register = "[{}] {} ppm CH4 (sample {}) | Plot size {}".format(datetime.datetime.now(), int(data), int(i), len(xdata))
        logfile.write(register + "\n")
        print (register)

      ani = animation.FuncAnimation(fig,animate, fargs=(xdata,ydata), cache_frame_data=False)
      plt.show()

except KeyboardInterrupt:
  print("\nExiting !")

finally:
  logfile.close()

