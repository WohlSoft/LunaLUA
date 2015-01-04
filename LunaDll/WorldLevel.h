#pragma once

#pragma pack(push, 4)
struct WorldLevel{
	double XPos;						//+0
	double YPos;						//+8
	double unkUnused_height;			//+16
	double unkUnused_width;				//+24
	double unkUnused_speedX;			//+32
	double unkUnused_speedY;			//+40
	char padding1[20];					//+48
	wchar_t* levelTitle;				//+68
	char padding2[28];					//+72
};
#pragma pack(pop)

namespace SMBXLevel{
	WorldLevel* get(int index);
}