#! /usr/bin/python2.7

import sys
import struct

if len(sys.argv) <= 2:
    print sys.argv
    raise ValueError("you must give at least two parameter");
else:
    print sys.argv

print bin(123)
print struct.pack('i',123)
with open(sys.argv[1],"wb") as output_file:
    output_file.write(struct.pack('<i',123))
    output_file.write(struct.pack('<i',456))
    output_file.write("haha")
    output_file.write('<i',)
output_file.close()


