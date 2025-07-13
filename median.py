# Inspired by: https://www.geeksforgeeks.org/python/spatial-filters-averaging-filter-and-median-filter-in-image-processing/

# "Median" Spatial Domain Filtering, which for 1-bit images becomes quantity filter.

import sys
import cv2
import numpy as np

if len(sys.argv) != 5:
	print("Pixel rank ('median') filter. Virtual background is Black.")
	print("Usage: median in.png out.png radius rank")
	exit(-1)

# Read the image.
# If not 1-bit image, only Classic routine will work, uncomment it below.
imgIn = cv2.imread(sys.argv[1], 0)

if imgIn is None:
	exit(-1)

# Geometry of sliding window
radius = int(sys.argv[3])
rank = int(sys.argv[4])
size = radius * 2 + 1
area = size * size		# Like for 5x5 area, radius is 2

if rank >= area:
	print("Rank", rank, "exceeds area", area, ".")
	exit(-1)

# Obtain the number of rows and columns
# of the image
w, h = imgIn.shape

# Traverse the image. For every nXn area,
# find the "median" of the pixels and
# replace the center pixel by the "median"
imgOut = np.zeros([w, h])

s = 0

# Classic O(N^2)
# for Y in range(0, h):
# 	for X in range(0, w):
# 		s = 0
# 		for dy in range(-radius, radius+1):
# 			for dx in range(-radius, radius+1):
# 				x = X + dx
# 				y = Y + dy
# 				if (x >= 0) and (x <= w-1) and (y >= 0) and (y <= h-1):
# 					if imgIn[x, y] > 0:
# 						s = s + 1
#
# 		imgOut[X, Y] = (s > rank) * 255

# Smart O(2*N) only for 1-bit images
# for Y in range(0, h):
# 	for X in range(0 - radius, w + radius):
# 		x = X + radius
# 		if x <= w-1:
# 			for y in range(max(0, Y-radius), min(h, Y+radius+1)):
# 				if imgIn[x, y] > 0:
# 					s = s + 1
#
# 		if (X >= 0) and (X <= w-1):
# 			imgOut[X, Y] = (s > rank) * 255
#
# 		x = X - radius
# 		if x >= 0:
# 			for y in range(max(0, Y-radius), min(h, Y+radius+1)):
# 				if imgIn[x, y] > 0:
# 					s = s - 1

# Smarter O(2*1) only for 1-bit images
imgTemp = np.zeros([w, h])
for Y in range(0, h):
	for X in range(0 - radius, w + radius):
		x = X + radius
		if x <= w-1:
			if imgIn[x, Y] > 0:
				s = s + 1

		if (X >= 0) and (X <= w-1):
			imgTemp[X, Y] = s

		x = X - radius
		if x >= 0:
			if imgIn[x, Y] > 0:
				s = s - 1

for X in range(0, w):
	for Y in range(0 - radius, h + radius):
		y = Y + radius
		if y <= h-1:
			s = s + imgTemp[X, y]

		if (Y >= 0) and (Y <= h-1):
			imgOut[X, Y] = (s > rank) * 255

		y = Y - radius
		if y >= 0:
			s = s - imgTemp[X, y]


if s != 0:
	print("Coding error, insufficient coffee. s =", s)
	exit(-1)

imgOut = imgOut.astype(np.uint8)
cv2.imwrite(sys.argv[2], imgOut, [cv2.IMWRITE_PNG_BILEVEL, 1])

exit(0)
