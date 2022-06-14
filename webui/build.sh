#! /usr/bin/bash

HERE=`dirname "$(readlink -f "$BASH_SOURCE")"`
OUT="${HERE}/public/build/index.bundle.html"
SDIR="${HERE}/public"

cd $HERE && npm run build


truncate -s0 $OUT

echo -n '<!DOCTYPE html><html lang="en"><head><meta charset="utf-8">' >> $OUT
echo -n '<meta name="viewport" content="width=device-width">' >> $OUT
echo -n '<link rel="icon" type="image/png" ' >> $OUT
echo -n 'href="data:image/png;base64,' >> $OUT
base64 -w0 $SDIR/favicon-32x32.png >> $OUT
echo -n '"><style>' >> $OUT
cat $SDIR/build/bundle.css >> $OUT
echo -n '</style></head><body></body><script>' >> $OUT
head -n1 $SDIR/build/bundle.js | tr -d '\n' >> $OUT
echo -n "</script></html>" >> $OUT

echo -en "Raw:        \t"
ls -l $OUT | cut -d' ' -f 5

echo -en "Gzip:       \t"
gzip -c -9 $OUT > "${OUT}.gz"
ls -l "${OUT}.gz" | cut -d' ' -f 5

echo -en "Deflate:    \t"
zlib-flate -compress < $OUT > "${OUT}.deflate"
ls -l "${OUT}.deflate" | cut -d' ' -f 5


echo -en "Flash image:\t"
size=`ls -l "${OUT}.deflate" | cut -d' ' -f 5`
python3 -c "import struct, sys; sys.stdout.buffer.write(\
  struct.pack('<I', ${size}))" > "${OUT}.bin"
cat "${OUT}.deflate" >> "${OUT}".bin
ls -l "${OUT}.bin" | cut -d' ' -f 5
