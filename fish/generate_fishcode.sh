#!/bin/sh

# $1 is fish.pl
# $2 is md5sum
# $3 is the output file
# $4 are the parameters for cut 

SUM=`$2 "$1" | cut -d ' ' $4 `
echo '#define CHECKSUM "'$SUM'"' > $3
echo 'static const char *fishCode(' >> $3
sed -e 's/\\/\\\\/g;s/"/\\"/g;s/^[ 	]*/"/;/^"# /d;s/[ 	]*$/\\n"/;/^"\\n"$/d;s/{CHECKSUM}/'$SUM'/;' "$1" >> $3 
echo ');' >> $3

