#!/bin/sh
# usage: sh sheet.sh <name> then lines "level x z yaw pitch [eyeY]" on stdin -> out/audit/<name>.png
cd "$(dirname "$0")/../../build"   # binaries and frames live in build/
name=$1; k=0; rm -f out/audit/_s_*.bmp
while read L x z yaw pitch ey; do
  [ -z "$L" ] && continue
  ./limen_shots.exe at ${SEED:-1337} 1 $L $x $z $yaw $pitch out/audit/_s_$k.bmp $ey > /dev/null 2>&1
  k=$((k+1))
done
python - "$name" <<'PY'
import sys, glob
from PIL import Image
fs=sorted(glob.glob('out/audit/_s_*.bmp'), key=lambda f:int(f.split('_s_')[1][:-4]))
n=len(fs); rows=(n+1)//2
sheet=Image.new('RGB',(960*2,540*rows))
for k,f in enumerate(fs):
    sheet.paste(Image.open(f).convert('RGB').resize((960,540)),((k%2)*960,(k//2)*540))
sheet.save('out/audit/%s.png'%sys.argv[1])
PY
