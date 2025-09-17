#!/bin/sh
### Rozhuk Ivan 2025
### motp_openconnect.sh
### Script for auto mOTP auth with openconnect


# Configure variables.
## mOTP
MOTP_SECRET=''
MOTP_PIN=''
MOTP_TZ='+0500'
## openconnect
USER=''
PASSWORD=''
AUTHGROUP=''
SERVER=''
PROTOCOL=''
#EXTRA_PARAMS='--base-mtu=1200 --queue-len=4096'


# Waiting for zero second.
echo -n 'Waiting for zero second...'
while true; do
	_SECOND=`date +'%S' | tail -c2 | tr -cd '[:print:]'`
	[ "${_SECOND}" -eq '0' ] && break
	sleep 1
	echo -n '.'
done
echo 'done!'

# Get mOTP code.
OTP_KEY=`motp -secret "${MOTP_SECRET}" -pin "${MOTP_PIN}" -tz "${MOTP_TZ}"`

# Try to connect.
printf '%s\n%s\n' "${PASSWORD}" "${OTP_KEY}" | openconnect -v --protocol="${PROTOCOL}" --user="${USER}" --authgroup="${AUTHGROUP}" --server="${SERVER}" ${EXTRA_PARAMS} --passwd-on-stdin

