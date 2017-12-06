import sys

f = open('sample.txt', 'w')
for count in range(0,5043):
	ch = 'a'
	for i in range(0,26):
		newch = chr(ord(ch) + i)
		f.write(newch)
f.close()
