#pragma once

struct Overworld{
	char padding1[64];					
	double XPos;						//+64
	double YPos;						//+72
	double unkUnused_height;			//+80
	double unkUnused_width;				//+88
	double unkUnused_speedX;			//+96
	double unkUnused_speedY;			//+104
	short unknown1;						//+112
	short currentWalkingFrame;			//+114		0 / 1 = Walking frame down | 2 / 3 Walking frame right | 4 / 5 Walking frame left | 6 / 7 Walking frame up
	short currentWalkingFrameTimer;		//+116		0-7 is won't change the frame, if higher or equal to 8 then currentRunningFrame will be swaped
	short currentWalkingDirection;		//+118		0 = Not Walking (Idle) | 1 = Walking Up | 2 = Walking Left | 3 = Walking Down | 4 = Walking Right
	short currentWalkingTimer;			//+120		0-32	will increment by 2 each frame, if reached 32 then the target tile has been reached and movement will be stopped
	short isCurrentlyWalking;			//+122		TRUE / FALSE	0 / -1
	wchar_t* currentLevelTitle;			//+124
};

namespace SMBXOverworld{
	Overworld* get();
}