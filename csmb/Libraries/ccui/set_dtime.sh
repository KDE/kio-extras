#!/bin/sh
#
# set_dtime.sh
#
# this shell is called from kcmclock to set the Date & Time
#
# Six parameters received from kcmclock:
#
# $1 = MM 	Months 	(01..12)
# $2 = DD 	Day 	(01..31)
# $3 = hh 	Hour 	(00..23)
# $4 = mm 	Minute 	(00..59)
# $5 = YYYY 	Year 	(0000..9999)
# $6 = ss 	Second 	(00..59)
#
# Return Values:
# 
#  0 = OK
#  1 = Wrong number of parameters
#  2 = date command failed
#  3 = clock command failed

if [ $# != 6 ]
	then exit 1	#Wrong number of parameter
fi
/bin/date $1$2$3$4$5.$6 >/dev/null 2>/dev/null
if [ $? != 0 ]
	then exit 2	#Wrong syntax or wrong values
fi

# on some system there is "hwclock" instead "clock"
hwclock=`type hwclock`

if [ -f "$hwclock" ]
	then
	# Add flag "-utc" if the CMOS clock is set to Universal Time
	$hwclock --systohc >/dev/null 2>/dev/null
	res=$?
else
	# Add flag "-u" if the CMOS clock is set to Universal Time
	/sbin/clock -w >/dev/null 2>/dev/null
	res=$?
fi

if [ $? != 0 ]
	then exit 3	#Hummmm, bad conditions...
fi
exit 0			#Ok !
