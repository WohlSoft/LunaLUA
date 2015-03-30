#include "Blocks.h"

Block* Blocks::Get(int index) {
	if(GM_BLOCKS_PTR == 0 || index < 0 || index > GM_BLOCK_COUNT) {
		return NULL;
	} else {
		return &((GetBase())[index]);
	}
}

bool Blocks::IsPlayerTouchingType(int type, int sought, PlayerMOB* demo) {	
	Block* blocks = Blocks::GetBase();
	Block* block = 0;
	double playerX = demo->momentum.x - 0.20;
	double playerY = demo->momentum.y - 0.20;
	double playerX2 = demo->momentum.x + demo->momentum.width + 0.20;
	double playerY2 = demo->momentum.y + demo->momentum.height + 0.20;

	for(int i = 1; i <= GM_BLOCK_COUNT; i++) {
		if(blocks[i].BlockType == type) {
			block = &blocks[i];

			if(playerX > block->mometum.x + block->mometum.width ||
				playerX2 < block->mometum.x  ||
				playerY > block->mometum.y + block->mometum.height ||
				playerY2 < block->mometum.y)
				continue;

			if(TestCollision(demo, block) == sought)
				return true;
		}
	}
	return false; // no collision
}

// TEST COLLISION (SMBX BUILTIN)
int Blocks::TestCollision(PlayerMOB* pMobPOS, Block* pBlockPOS) {	
	typedef int __stdcall colfunc(void*, void*);
	colfunc* f = (colfunc*)GF_MOB_BLOCK_COL;	
	return f(&pMobPOS->momentum.x, &pBlockPOS->mometum.x);
}

// SET ALL
void Blocks::SetAll(int type1, int type2) {
	Block* blocks = Blocks::GetBase();	
	for(int i = 1; i <= GM_BLOCK_COUNT; i++) {
		if(blocks[i].BlockType == type1) {
			blocks[i].BlockType = type2;
		}
	}
}

// SWAP ALL
void Blocks::SwapAll(int type1, int type2) {
	Block* blocks = Blocks::GetBase();	
	for(int i = 1; i <= GM_BLOCK_COUNT; i++) {
		if(blocks[i].BlockType == type1) {
			blocks[i].BlockType = type2;
		}
		else if(blocks[i].BlockType == type2) {
			blocks[i].BlockType = type1;
		}
	}
}

void Blocks::ShowAll(int type) {
	Block* blocks = Blocks::GetBase();	
	for(int i = 1; i <= GM_BLOCK_COUNT; i++) {
		if(blocks[i].BlockType == type) {
			blocks[i].IsHidden = 0;
		}		
	}
}

void Blocks::HideAll(int type) {
	Block* blocks = Blocks::GetBase();	
	for(int i = 1; i <= GM_BLOCK_COUNT; i++) {
		if(blocks[i].BlockType == type) {
			blocks[i].IsHidden = 0xFFFF;
		}		
	}
}