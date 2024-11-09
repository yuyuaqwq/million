<#
# Copyright (C) 2023-present ichenq@outlook.com. All rights reserved.
#>

$CURRENT_DIR = Get-Location
$DATASHEET_DIR = Resolve-Path "./datasheet"
$OUT_SRC_DIR = Join-Path $CURRENT_DIR  "../descgen/src"
$OUT_DATA_DIR = Join-Path $CURRENT_DIR  "./outres"
$TARGET_HEADER_PATH = Join-Path $OUT_SRC_DIR "Config.h"
$TARGET_CPP_PATH = Join-Path $OUT_SRC_DIR "Config.cpp"
$NEW_INCLUDE_PATH = "../million/descgen/include/descgen/Config.h"

$Env:PYTHONPATH = $CURRENT_DIR

Function Generate {
    python $CURRENT_DIR/tabugen/__main__.py --file_asset=$DATASHEET_DIR --cpp_out=$OUT_SRC_DIR/Config --package=config --gen_csv_parse --source_file_encoding=utf_8_sig --out_data_path=$OUT_DATA_DIR --out_data_format=csv
    python $CURRENT_DIR/tabugen/__main__.py --file_asset=$DATASHEET_DIR --out_data_path=$OUT_DATA_DIR --out_data_format=json --json_indent
}

Function ModifyAndMoveHeader {
    if (Test-Path $TARGET_HEADER_PATH) {
        # 读取文件内容
        $fileContent = Get-Content -Path $TARGET_HEADER_PATH -Raw

        # 替换 #include "rapidcsv.h" 为 #include <descgen/rapidcsv.h>
        $fileContent = $fileContent -replace '#include "rapidcsv.h"', '#include <descgen/rapidcsv.h>'

        # 将修改后的内容写回文件
        Set-Content -Path $TARGET_HEADER_PATH -Value $fileContent -Encoding utf8

        # 移动文件到新的路径
        $destinationDir = Split-Path -Path $NEW_INCLUDE_PATH
        if (!(Test-Path $destinationDir)) {
            New-Item -ItemType Directory -Path $destinationDir -Force
        }
        Move-Item -Path $TARGET_HEADER_PATH -Destination $NEW_INCLUDE_PATH -Force
    }
    else {
        Write-Output "目标文件 $TARGET_HEADER_PATH 不存在，无法进行替换和移动。"
    }
}

Function ModifyConfigCpp {
    if (Test-Path $TARGET_CPP_PATH) {
        # 读取文件内容
        $fileContent = Get-Content -Path $TARGET_CPP_PATH -Raw

        # 替换 #include "Config.h" 和 #include "Conv.h" 为新的路径
        $fileContent = $fileContent -replace '#include "Config.h"', '#include <descgen/Config.h>'
        $fileContent = $fileContent -replace '#include "Conv.h"', '#include <descgen/Conv.h>'

        # 将修改后的内容写回文件
        Set-Content -Path $TARGET_CPP_PATH -Value $fileContent -Encoding utf8
    }
    else {
        Write-Output "目标文件 $TARGET_CPP_PATH 不存在，无法进行替换。"
    }
}

Generate
ModifyAndMoveHeader
ModifyConfigCpp

pause
