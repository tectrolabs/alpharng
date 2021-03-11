#!/bin/sh

# A shell script that serves as a named pipe server for sharing random bytes produced by an AlphaRNG device on Linux based systems.
# Last updated on 10/Feb/2021
# Configure with 'crontab -e' like the following:
# @reboot /usr/local/bin/run-alrng-pserver.sh >> /var/log/run-alrng-pserver.log 2>&1

# Named pipe used.   
PIPEFILE=/tmp/alpharng

APPDIR=/usr/local/bin

# The application that populates the named pipe with random numbers downloaded from an AlphaRNG device 
APPNAME=alrng

# Application name and command line arguments. Add option '-c none' to disable security and increase performance.
APPCMD="$APPDIR/$APPNAME -e -o $PIPEFILE "

if [ ! -e "$PIPEFILE" ]; then
 mkfifo $PIPEFILE
fi 

#
# You can tune up the following access rights to ensure the security requirements
#
chmod a+r $PIPEFILE
retVal=$?
 if [ ! $retVal -eq 0 ]
 then
  echo "Cannot set read permission on $PIPEFILE"
  exit 1
 fi

chmod u+w $PIPEFILE
retVal=$?
 if [ ! $retVal -eq 0 ]
 then
  echo "Cannot set write permission on $PIPEFILE"
  exit 1
 fi

if [ ! -e "$APPDIR/$APPNAME" ]; then
 echo "$APPDIR/$APPNAME is not installed. Did you run 'make install' ?"
 exit 1
fi

if [ ! -x "$APPDIR/$APPNAME" ]
then
 echo "File '$APPDIR/$APPNAME' is not executable"
 exit 1
fi
CURRENTDATE=`date +"%Y-%m-%d %T"`

echo "$CURRENTDATE Start looping $APPCMD" 

while :
do
 $APPCMD
 retVal=$?
 if [ ! $retVal -eq 0 ]
 then
   if [ ! $retVal -eq 141 ]
   then
    sleep 15
   fi
 fi
done
