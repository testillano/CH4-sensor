# Arduino Serial graph monitor

# Important note:
#   To make this script work properly, run arduino 'TwoSensors.ino' program with:
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
ydata1 = deque(maxlen=max_length)
ydata2 = deque(maxlen=max_length)

# SIGUSR1 to reset data
def signal_handler(sig, frame):
    print(f'SIGUSR1 signal ({sig}) captured (pid {os.getpid()})')
    xdata.clear()
    ydata1.clear()
    ydata2.clear()

signal.signal(signal.SIGUSR1, signal_handler)

try:
  logfile = open('serial.log', 'w')

  while True:
    if (arduinoSerialData.inWaiting() > 0):
      def animate(i,xdata,ydata1,ydata2):
        data = arduinoSerialData.readline().decode().strip()

        try: # avoid first fail
          values = data.split(",")
          value1 = float(values[0])
          value2 = float(values[1])
        except:
          value1 = 0
          value2 = 0

        xdata.append(i)
        ydata1.append(value1)
        ydata2.append(value2)

        ax.clear()
        ax.plot(xdata,ydata1, label='Sensor 1 (inside)', color='orange')
        ax.plot(xdata,ydata2, label='Sensor 2 (outside)', color='green')

        register = "[{}] sensor 1/2: {}/{} ppm CH4 (sample {}) | Plot size {}".format(datetime.datetime.now(), int(value1), int(value2), int(i), len(xdata))
        logfile.write(register + "\n")
        print (register)

      ani = animation.FuncAnimation(fig,animate, fargs=(xdata,ydata1,ydata2), cache_frame_data=False)
      plt.show()

except KeyboardInterrupt:
  print("\nExiting !")

finally:
  logfile.close()

