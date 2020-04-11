import matplotlib.pyplot as plt
import numpy as np
import serial
import time
Fs = 100.0;  # how many points
Ts = 1.0/Fs; # sampling interval
t = np.arange(0, 10, 10 * Ts) # time vector; create Fs samples between 0 and 1.0 sec.
# y = np.arange(0, 1, Ts) # signal vector; create Fs samples
x_value = np.arange(0, 1, Ts)
y_value = np.arange(0, 1, Ts)
z_value = np.arange(0, 1, Ts)
log_value = np.arange(0, 1, Ts)


serdev = '/dev/ttyACM0'
s = serial.Serial(serdev, baudrate = 115200)
for idx in range(0, int(Fs)):
    line = s.readline() # Read an echo string from K66F terminated with '\n'
    # print line
    x_value[idx] = float(line)
for idx in range(0, int(Fs)):
    line = s.readline() # Read an echo string from K66F terminated with '\n'
    # print line
    y_value[idx] = float(line)
for idx in range(0, int(Fs)):
    line = s.readline() # Read an echo string from K66F terminated with '\n'
    # print line
    z_value[idx] = float(line)
for idx in range(0, int(Fs)):
    line = s.readline() # Read an echo string from K66F terminated with '\n'
    # print line
    log_value[idx] = float(line)

fig, ax = plt.subplots(2, 1) # 2 * 1


l1, = ax[0].plot(t, x_value, 'b')
l2, = ax[0].plot(t, y_value, 'r')
l3, = ax[0].plot(t, z_value, 'g')

ax[0].legend([l1, l2, l3], ['x', 'y', 'z'], loc = 'lower left')


ax[0].set_xlabel('Time')
ax[0].set_ylabel('Acc Vector')

ax[1].stem(t, log_value, use_line_collection = True)
plt.ylim(-0.1, 1.1) # set the y-limit from 0 to 1
# ax[1].plot(frq,abs(Y),'r') # plotting the spectrum
ax[1].set_xlabel('Time')
ax[1].set_ylabel('Tilt')
plt.show()
s.close()