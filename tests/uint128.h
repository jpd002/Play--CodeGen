#pragma once

struct uint128
{
	union
	{
		struct
		{
			uint32 nV[4];
		};
		struct
		{
			uint32 nV0;
			uint32 nV1;
			uint32 nV2;
			uint32 nV3;
		};
		struct
		{
			uint64 nD0;
			uint64 nD1;
		};
	};
};
