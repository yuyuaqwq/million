set PACKAGE=Desc

cd table
..\tool\tabtoy -mode=v3 -index=Index.xlsx -package=%PACKAGE% -proto_out=../desc/desc.proto
..\tool\tabtoy -mode=v3 -index=Index.xlsx -package=%PACKAGE% -pbbin_out=../../descgen/descdata.pbb
::..\tool\tabtoy -mode=v3 -index=Index.xlsx -package=%PACKAGE% -pbbin_dir=../../descgen/

cd ..
set OUTPUT_DIR=..\protogen
mkdir %OUTPUT_DIR% 2>nul
for /d %%D in (*) do (
    protoc --cpp_out=%OUTPUT_DIR% .\%%D\*.proto
)

pause