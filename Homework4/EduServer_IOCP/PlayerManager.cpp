#include "stdafx.h"
#include "Player.h"
#include "PlayerManager.h"

PlayerManager* GPlayerManager = nullptr;

PlayerManager::PlayerManager() : mLock(LO_ECONOMLY_CLASS), mCurrentIssueId(0)
{

}

int PlayerManager::RegisterPlayer(std::shared_ptr<Player> player)
{
	FastSpinlockGuard exclusive(mLock);

	mPlayerMap[++mCurrentIssueId] = player;

	return mCurrentIssueId;
}

void PlayerManager::UnregisterPlayer(int playerId)
{
	FastSpinlockGuard exclusive(mLock);

	mPlayerMap.erase(playerId);
}


int PlayerManager::GetCurrentPlayers(PlayerList& outList)
{
	//DONE: mLock을 read모드로 접근해서 outList에 현재 플레이어들의 정보를 담고 total에는 현재 플레이어의 총 수를 반환.
	FastSpinlockGuard readLock(mLock, false);

	int total = mPlayerMap.size();

	for (auto& player : mPlayerMap)
	{
		outList.push_back(player.second);
	}

	return total;
}