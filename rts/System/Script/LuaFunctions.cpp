// This file contains implementations of various functions exposed through lua
// The goal is to include as few headers in LuaBinder.cpp as possible, and include
// them here instead to avoid recompiling LuaBinder too often.

#include "LuaFunctions.h"
#include "Game/StartScripts/Script.h"
#include "float3.h"
#include "Game/UI/InfoConsole.h"
#include "GlobalStuff.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitLoader.h"
#include "TdfParser.h"
#include "Game/command.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Misc/QuadField.h"
#include "Game/Game.h"
#include "Game/UI/EndGameBox.h"

using namespace std;

namespace luafunctions {

// This should use net stuff instead of duplicating code here
void EndGame()
{
	new CEndGameBox();
	game->gameOver = true;
}

void UnitGiveCommand(CUnit_pointer* u, Command* c)
{
	if (!u->unit)
		return;

	u->unit->commandAI->GiveCommand(*c);
}

void CommandAddParam(Command* c, float p)
{
	c->params.push_back(p);
}

CUnit_pointer* UnitLoaderLoadUnit(string name, float3 pos, int team, bool buil)
{
	CUnit *x = unitLoader.LoadUnit(name, pos, team, buil);
	return new CUnit_pointer(x);
}

//	vector<CUnit*> GetUnits(const float3& pos,float radius);
//	vector<CUnit*> GetUnitsExact(const float3& pos,float radius);

// Probably nice to be able to get the actual list as well, need to find out how to return a table
int GetNumUnitsAt(const float3& pos, float radius)
{
	vector<CUnit*> x = qf->GetUnits(pos, radius);
	return x.size();
}

};

