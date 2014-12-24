#!/bin/bash
set -e
adb install -r ./bin/CodeGenTestSuite-debug.apk
$ANDROID_NDK_ROOT/ndk-gdb --nowait --start
