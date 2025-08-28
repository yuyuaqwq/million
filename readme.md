# 环境

```
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE="[vcpkg-root]/scripts/buildsystems/vcpkg.cmake"
```

## Protobuf
- 确保是动态链接，否则ProtoCodec无法正常工作
