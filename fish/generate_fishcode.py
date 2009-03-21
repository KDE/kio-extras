#1 is generate_fishcode.py
#2 is md5sum
#3 is the output file
#4 are the parameters for cut 
import sys
import re
import hashlib

fp = open(sys.argv[1], "rb")
r = fp.read()
fp.close()

m = hashlib.md5()
m.update(r)
sm = m.hexdigest()

fp = open(sys.argv[1], "r")
r = fp.read()
fp.close()

fp = open(sys.argv[3], "wb")
fp.write('#define CHECKSUM "%s"\n' % sm)
fp.write('static const char *fishCode(\n')

r = re.sub(r'\\', r'\\\\', r)
r = re.sub(r'"', r'\\"', r)
p = re.compile('^[ 	]*', re.M)
r = p.sub('"', r)
p = re.compile(r'^"# .*\n*', re.M)
r = p.sub('', r)
p = re.compile('[ 	]*$', re.M)
r = p.sub(r'\\n"', r)
p = re.compile(r'^"\\n"\n?', re.M)
r = p.sub('', r)
r = re.sub(r'{CHECKSUM}', sm, r, 1)
fp.write(r + ");\n")
fp.close()
