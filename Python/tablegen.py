#!/usr/bin/python

#import os
import sys
import math
from PIL import Image

columns = 8        # Number of columns in formatted output
size    = 128      # Pixels across minor axis of display
radius  = size / 2 # 1/2 display size

# Write hex digit (with some formatting for C array) to stdout
def outputHex(n, digits):
	global columns, column, counter, limit
	column += 1                        # Increment column number
	if column >= columns:              # If max column exceeded...
		sys.stdout.write("\n  ")   # end current line, start new one
		column = 0                 # Reset column number
	sys.stdout.write("{0:#0{1}X}".format(n, digits + 2))
	counter += 1                       # Increment element index
	if counter < limit:                # If NOT last element in list...
		sys.stdout.write(",")      # add column between elements
		if column < (columns - 1): # and space if not last column
			sys.stdout.write(" ")
	else:
		print(" };");              # Cap off table


# OPEN AND VALIDATE IMAGE FILE ---------------------------------------------

try:    filename = sys.argv[1]
except: filename = "colormap.png"
im = Image.open(filename)
if (im.size[0] > 512) or (im.size[1] > 128):
	print "Image can't exceed 512 pixels wide or 128 pixels tall"
	exit(1)
im     = im.convert("RGB")
pixels = im.load()


# GENERATE IMAGE MAP -------------------------------------------------------

# Initialize outputHex() global counters:
counter = 0                       # Index of next element to generate
column  = columns                 # Current column number in output
limit   = im.size[0] * im.size[1] # Total # of elements in generated list

print "#define IMG_WIDTH  " + str(im.size[0])
print "#define IMG_HEIGHT " + str(im.size[1])
print

sys.stdout.write("const uint16_t img[IMG_HEIGHT][IMG_WIDTH] = {")

for y in range(im.size[1]):
	for x in range(im.size[0]):
		p = pixels[x, y] # Pixel data (tuple)
		outputHex(((p[0] & 0b11111000) << 8) | # Convert 24-bit RGB
		          ((p[1] & 0b11111100) << 3) | # to 16-bit value w/
		          ( p[2] >> 3), 4)             # 5/6/5-bit packing


# GENERATE POLAR COORDINATE TABLE ------------------------------------------

# One element per screen pixel, 16 bits per element -- high 9 bits are
# angle relative to center point (fixed point, 0-511) low 7 bits are
# distance from circle perimeter (fixed point, 0-127, pixels outsize circle
# are set to 127).

counter = 0           # Reset outputHex() counters again for new table
column  = columns
limit   = size * size

sys.stdout.write("\nconst uint16_t polar[%s][%s] = {" % (size, size))

for y in range(size):
	dy = y - radius + 0.5
	for x in range(size):
		dx       = x - radius + 0.5
		distance = math.sqrt(dx * dx + dy * dy)
		if(distance >= radius): # Outside circle
			outputHex(127, 4) # angle = 0, dist = 127
		else:
			angle     = math.atan2(dy, dx)  # -pi to +pi
			angle    += math.pi             # 0.0 to 2pi
			angle    /= (math.pi * 2.0)     # 0.0 to <1.0
			a         = int(angle * 512.0)  # 0 to 511
			distance /= radius              # 0.0 to <1.0
			distance *= 128.0               # 0.0 to <128.0
			d         = 127 - int(distance) # 127 to 0
			outputHex((a << 7) | d, 4)

