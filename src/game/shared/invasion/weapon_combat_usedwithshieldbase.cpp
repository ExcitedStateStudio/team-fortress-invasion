#include "cbase.h"
//#include "basetfplayer_shared.h"
#include "weapon_combat_usedwithshieldbase.h"
#include "weapon_combatshield.h"
#include "weapon_twohandedcontainer.h"
#if defined( CLIENT_DLL )
#include <c_tf_player.h>
#else
#include <tf_player.h>
#endif



//-----------------------------------------------------------------------------
// Purpose: Make sure we're not switching directly to this weapon, since this 
//			weapon can only be "switched" to by the twohandedcontainer weapon.
//-----------------------------------------------------------------------------
bool CWeaponCombatUsedWithShieldBase::CanDeploy(void)
{
	CTFPlayer* pPlayer = ToTFPlayer(GetOwner());
	if (!pPlayer)
		return false;


	CWeaponTwoHandedContainer* pContainer =
		dynamic_cast<CWeaponTwoHandedContainer*>(pPlayer->GetActiveWeapon());

	if (!pContainer)
	{
		
		return false;
	}

	return BaseClass::CanDeploy();
}
void CWeaponCombatUsedWithShieldBase::AllowShieldPostFrame(bool allow)
{
	CTFPlayer* pOwner = ToTFPlayer(GetOwner());
	if (!pOwner)
		return;

	CWeaponCombatShield* shield = dynamic_cast<CWeaponCombatShield*>(
		pOwner->Weapon_OwnsThisType("weapon_combat_shield"));
	if (!shield)
		return;

	shield->SetAllowPostFrame(allow);
}

int CWeaponCombatUsedWithShieldBase::GetShieldState(void)
{
	CTFPlayer* pOwner = ToTFPlayer(GetOwner());
	if (!pOwner)
		return SS_DOWN;

	
	CWeaponCombatShield* pShield = dynamic_cast<CWeaponCombatShield*>(
		pOwner->Weapon_OwnsThisType("weapon_combat_shield"));
	if (!pShield)
		return SS_DOWN;

	return pShield->GetShieldState(); 
}
//-----------------------------------------------------------------------------
// Purpose: Mirror the values in the container, if there is one
// Input  : *pPlayer - 
// Output : int
//-----------------------------------------------------------------------------
int CWeaponCombatUsedWithShieldBase::UpdateClientData( CBasePlayer *pPlayer )
{
	if ( !pPlayer )
		return BaseClass::UpdateClientData( pPlayer );

	CWeaponTwoHandedContainer *pContainer = ( CWeaponTwoHandedContainer * )pPlayer->Weapon_OwnsThisType( "weapon_twohandedcontainer" );
	if ( !pContainer || pContainer != pPlayer->GetActiveWeapon() )
		return BaseClass::UpdateClientData( pPlayer );

	// Make sure this weapon is one of the container's active weapons
	if ( pContainer->GetLeftWeapon() != this && pContainer->GetRightWeapon() != this )
		return BaseClass::UpdateClientData( pPlayer );

	int retval = pContainer->UpdateClientData( pPlayer );
	m_iState =  pContainer->m_iState;
	return retval;
}

LINK_ENTITY_TO_CLASS( weapon_combat_usedwithshieldbase, CWeaponCombatUsedWithShieldBase );

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponCombatUsedWithShieldBase, DT_WeaponCombatUsedWithShieldBase )

BEGIN_NETWORK_TABLE( CWeaponCombatUsedWithShieldBase, DT_WeaponCombatUsedWithShieldBase )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponCombatUsedWithShieldBase )
END_PREDICTION_DATA()
