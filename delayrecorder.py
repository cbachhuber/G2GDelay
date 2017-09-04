import serial
import serial.tools.list_ports
import sys
import time
#import pyperclip
import csv

errormsg = "Call this function using \n\n'python recorder.py [number of measurements] [path to savefile]'\n\nwhere the parameters are optional. E.g. 'python delayrecorder.py 50 stats.csv' takes 50 G2G delay measurements and writes them to file 'stats.csv'.\nBy default (no parameters given, i.e. 'python delayrecorder.py'), the script records 100 samples and does not create a csv file."

args = sys.argv
if len(args)==1:
	iterations = 100
elif len(args) ==2:
	try:
		iterations = int(args[1])
	except:
		print errormsg
		quit()
elif len(args) == 3:
	try:
		iterations = int(args[1])
		savefile = args[2]
	except:
		print errormsg
		sys.exit()
else:
	print errormsg
	sys.exit()

ports = list(serial.tools.list_ports.comports())
found = 0
for p in ports:
	if "Arduino" in p[1]:
		print "Found Arduino at " + p[0]
		ser = serial.Serial(p[0], 115200,timeout=5)
		found = 1

if found==0:
	print "Did not find Arduino on any serial port. Is it connected?"
	quit()

timeout = time.time() + 0.01
while True:
	a = ser.readline()
	if time.time() > timeout:
		break

G2Gdelays = []

i=0
overallrounds = 0
initmessage = 0

try:
	while (i < iterations):
		overallrounds = overallrounds + 1
		a = ser.readline()
		if "." in a:
			initmessage = 1
			i = i + 1
			a = a.replace("\n","")
			a = a.replace("\r","")
			G2Gdelays.append(float(a))
			print "G2G Delay: " + a + " ms"
		else:
			if overallrounds > 0 and initmessage==1:
				print "Did not receive msmt data from the Arduino for another 5 seconds. Is the phototransistor still sensing the LED?"
			else:
				print "Did not receive msmt data from the Arduino for 5 seconds. \n Is the phototransistor sensing the LED on the screen? \n Is the correct side of the PT pointing towards the screen (the flat side with the knob on it)? \n Is the screen brightness high enough (max recommended)?"
				initmessage = 1
except KeyboardInterrupt:
	pass

# Post-processing
time.sleep(.1)
print "\n"
print "Min/Mean/Max G2G Delay: " + '%.2f' % (min(G2Gdelays)) + "/" + '%.2f' % (sum(G2Gdelays)/float(len(G2Gdelays))) + "/" + '%.2f' % (max(G2Gdelays)) + " ms"

#Copying to clipboard
#pyperclip.copy('%.2f' % (min(G2Gdelays)) + "/" + '%.2f' % (sum(G2Gdelays)/float(len(G2Gdelays))) + "/" + '%.2f' % (max(G2Gdelays)) + " ms")
#spam=pyperclip.paste()

#Saving to csv file
if 'savefile' in globals():
	with open(savefile, 'wb') as f:
		writer = csv.writer(f, delimiter = ',')
		writer.writerow(G2Gdelays)
