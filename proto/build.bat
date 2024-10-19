mkdir ..\protogen

protoc --cpp_out=../protogen ./comm/*.proto
protoc --cpp_out=../protogen ./ss/*.proto
protoc --cpp_out=../protogen ./cs/*.proto
protoc --cpp_out=../protogen ./db/*.proto
pause