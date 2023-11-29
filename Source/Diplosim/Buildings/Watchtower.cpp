#include "Buildings/Watchtower.h"

#include "AttackComponent.h"

AWatchtower::AWatchtower()
{
	AttackComponent = CreateDefaultSubobject<UAttackComponent>(TEXT("AttackComponent"));
}