set PACKAGE=desc

cd table
..\tool\tabtoy -mode=v3 -index=Index.xlsx -package=%PACKAGE% -proto_out=../../proto/desc/desc.proto
::..\tool\tabtoy -mode=v3 -index=Index.xlsx -package=%PACKAGE% -pbbin_out=../../descgen/descdata.pbb
..\tool\tabtoy -mode=v3 -index=Index.xlsx -package=%PACKAGE% -pbbin_dir=../../descgen/

pause