cd dist
copy ..\_win32\release\dagaa.exe
copy ..\_win32\release\dagaa.map
kkrunchy --best --out dagaa_k.exe dagaa.exe
rem kkrunchy_k7 --best --out dagaa_k7.exe dagaa.exe
cd ..
