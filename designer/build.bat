set PACKAGE=million.table

mkdir ..\tablegen

cd table
..\tool\tabtoy -mode=v3 -index=Index.xlsx -package=%PACKAGE% -proto_out=../../proto/table/table.proto
::..\tool\tabtoy -mode=v3 -index=Index.xlsx -package=%PACKAGE% -pbbin_out=../../tablegen/table.pbb
..\tool\tabtoy -mode=v3 -index=Index.xlsx -package=%PACKAGE% -pbbin_dir=../../tablegen

pause