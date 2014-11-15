package com.virtualapplications.codegentestsuite;

public class NativeInterop
{
	 static 
	 {
		 System.loadLibrary("CodeGenTestSuite");
	 }

	 public static native void start();
}
