iar_build.exe "img/katakoi_imgcn"

rename sec5\RESR RESR_bak
rename sec5\RTFC RTFC_bak
rename sec5\RESR.new RESR
rename sec5\RTFC.new RTFC

call #pack.bat

rename sec5\RESR RESR.new
rename sec5\RTFC RTFC.new
rename sec5\RESR_bak RESR
rename sec5\RTFC_bak RTFC