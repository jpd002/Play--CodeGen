#!/bin/bash
set -e

case "`uname`" in
	CYGWIN* | MINGW*)
		$$ANDROID_SDK_ROOT/tools/android.bat update project -p .
		$ANDROID_NDK_ROOT/ndk-build NDK_DEBUG=1
		$ANT_HOME/bin/ant debug
    ;;
  Darwin* | Linux*)
		$$ANDROID_SDK_ROOT/tools/android update project -p .
		$ANDROID_NDK_ROOT/ndk-build NDK_DEBUG=1
		$ANT_HOME/bin/ant debug
    ;;
esac
