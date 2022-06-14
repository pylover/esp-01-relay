#! /usr/bin/env bash

source testing.sh

#ADDR=192.168.1.158
#NAME="dev.fota"
#ADDR=192.168.1.162
NAME="dev.fota6"
ADDR=192.168.1.162
#NAME="dev.fota8"

assert-eq "UNS resolve" ${ADDR} `uns resolve --short ${NAME}`
assert-eq "Simple GET" \
  'Index' \
  `uns http get ${NAME}/demo`

PRV=`pip3 freeze | grep 'requests==' | cut -d'=' -f3`
assert-eq "HTTP Headers" "\
200 OK HTTP/1.1
Server: esp8266-HTTPd/2.0.0
Connection: keep-alive
Content-Length: 0
Host: ${ADDR}
User-Agent: python-requests/${PRV}
Accept-Encoding: gzip, deflate
Accept: */* -H'foo: bar'" \
  "`uns http --include-headers echo ${NAME}/demo/headers` -H'foo: bar'"


assert-eq "Querystring" \
  "foo=bar baz=qux " \
  "$(uns http echo "${NAME}/demo/queries?foo=bar&baz=qux")"

assert-eq "Querystring Empty field" \
  "foo= baz=qux quux= " \
  "$(uns http echo "${NAME}/demo/queries?foo=&baz=qux&quux=")"


assert-eq "URL encoded form" \
  "foo=bar baz=qux " \
  "$(uns http ECHO ${NAME}/demo/urlencodedforms foo=bar baz=qux)"

assert-eq "URL encoded form empty field" \
  "foo=bar baz=qux quux= " \
  "$(uns http ECHO ${NAME}/demo/urlencodedforms foo=bar baz=qux quux=)"

assert-eq "Streaming" \
  "$(printf 'Foo\r\nBar\r\nBaz\r\nQux\r\n')" \
  "$(uns http DOWNLOAD ${NAME}/demo)"

# Multipart, single field
tmp=$(tempfile)
echo -n "bar" > ${tmp}
assert-eq "Small multipart form, single field" \
  "foo=bar " \
  "$(uns http echo ${NAME}/demo/multipartforms @foo=${tmp})"
rm ${tmp}

# Multipart, multiple fields
tmpbar=$(tempfile)
tmpbaz=$(tempfile)
echo -n "bar" > ${tmpbar}
echo -n "baz" > ${tmpbaz}
assert-eq "Small multipart form, multipart fields" \
  "foo=bar qux=baz " \
  "$(uns http echo ${NAME}/demo/multipartforms @foo=${tmpbar} @qux=${tmpbaz})"
rm ${tmpbar} ${tmpbaz}

# Multipart, streaming
tmp1=$(tempfile)
tmp2=$(tempfile)
dd if=/dev/urandom bs=1 count=100000 of=${tmp1} >> /dev/null 2>&1

uns http upload ${NAME}/demo/multipartstreams @foo=${tmp1} > /dev/null & \
uns http -b download ${NAME}/demo/multipartstreams > ${tmp2}

assert-eq "Multipart streaming" "$(cat $tmp1 | md5)" "$(cat $tmp2 | md5)"
if [ "$?" != 0 ]; then
  ylw "Source file: ${tmp1}" 
  ylw "Target file: ${tmp2}" 
  exit 1
else
  rm ${tmp1}
  rm ${tmp2}
fi

assert-eq "SSL client test" \
  'ESP8266 FOTA SSL' \
  `uns http test ${NAME}/tlsclient`


echo -e "$LK"
make map6user2 >> /dev/null
echo -e "$C"
binsize user/.output/eagle/debug/lib/libuser.a
binsize httpd/.output/eagle/debug/lib/libhttpd.a
binsize uns/.output/eagle/debug/lib/libuns.a 
echo ' ------------------------------------'
binsize bin/upgrade/user2.4096.new.6.bin
