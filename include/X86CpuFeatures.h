#pragma once

class CX86CpuFeatures
{
public:
	bool hasSsse3 = false;
	bool hasSse41 = false;
	bool hasAvx = false;
	bool hasAvx2 = false;

	static CX86CpuFeatures AutoDetect();
};
