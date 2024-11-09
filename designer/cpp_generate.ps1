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
        # ��ȡ�ļ�����
        $fileContent = Get-Content -Path $TARGET_HEADER_PATH -Raw

        # �滻 #include "rapidcsv.h" Ϊ #include <descgen/rapidcsv.h>
        $fileContent = $fileContent -replace '#include "rapidcsv.h"', '#include <descgen/rapidcsv.h>'

        # ���޸ĺ������д���ļ�
        Set-Content -Path $TARGET_HEADER_PATH -Value $fileContent -Encoding utf8

        # �ƶ��ļ����µ�·��
        $destinationDir = Split-Path -Path $NEW_INCLUDE_PATH
        if (!(Test-Path $destinationDir)) {
            New-Item -ItemType Directory -Path $destinationDir -Force
        }
        Move-Item -Path $TARGET_HEADER_PATH -Destination $NEW_INCLUDE_PATH -Force
    }
    else {
        Write-Output "Ŀ���ļ� $TARGET_HEADER_PATH �����ڣ��޷������滻���ƶ���"
    }
}

Function ModifyConfigCpp {
    if (Test-Path $TARGET_CPP_PATH) {
        # ��ȡ�ļ�����
        $fileContent = Get-Content -Path $TARGET_CPP_PATH -Raw

        # �滻 #include "Config.h" �� #include "Conv.h" Ϊ�µ�·��
        $fileContent = $fileContent -replace '#include "Config.h"', '#include <descgen/Config.h>'
        $fileContent = $fileContent -replace '#include "Conv.h"', '#include <descgen/Conv.h>'

        # ���޸ĺ������д���ļ�
        Set-Content -Path $TARGET_CPP_PATH -Value $fileContent -Encoding utf8
    }
    else {
        Write-Output "Ŀ���ļ� $TARGET_CPP_PATH �����ڣ��޷������滻��"
    }
}

Generate
ModifyAndMoveHeader
ModifyConfigCpp

pause
