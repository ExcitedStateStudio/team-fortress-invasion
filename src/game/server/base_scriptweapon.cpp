#include "cbase.h"
#include "base_scriptweapon.h"
#include "script_weapon_parse.h"

#if !defined( CLIENT_DLL )
#include "tf_player.h"
#else
#include "c_tf_player.h"
#endif

// memdbgon must be the last include file
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS(base_scriptweapon, CBaseScriptWeapon);

IMPLEMENT_NETWORKCLASS_ALIASED(BaseScriptWeapon, DT_BaseScriptWeapon)

BEGIN_NETWORK_TABLE(CBaseScriptWeapon, DT_BaseScriptWeapon)
#if !defined( CLIENT_DLL )
SendPropBool(SENDINFO(m_bReflectAnims)),
#else
RecvPropBool(RECVINFO(m_bReflectAnims)),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CBaseScriptWeapon)
END_PREDICTION_DATA()

//-----------------------------------------------------------------------------
CBaseScriptWeapon::CBaseScriptWeapon()
{
    m_bReflectAnims = false;
    m_iLastReflectedActivity = -1;
}

//-----------------------------------------------------------------------------
void CBaseScriptWeapon::Precache(void)
{
    BaseClass::Precache();

    ScriptWeaponInfo_t* pInfo = GetScriptWpnData();
    if (!pInfo) return;

    if (pInfo->szViewModel[0])
        PrecacheModel(pInfo->szViewModel);

    if (pInfo->szWorldModel[0])
        PrecacheModel(pInfo->szWorldModel);

    for (int i = 0; i < pInfo->sounds.Count(); i++)
    {
        if (pInfo->sounds[i].szSoundFile[0])
            PrecacheScriptSound(pInfo->sounds[i].szSoundFile);
    }
}

//-----------------------------------------------------------------------------
void CBaseScriptWeapon::Spawn(void)
{
    BaseClass::Spawn();

 
    ScriptWeaponInfo_t* pInfo = GetScriptWpnData();
    if (pInfo)
    {
        if (pInfo->szViewModel[0])
            m_iViewModelIndex = PrecacheModel(pInfo->szViewModel);

        if (pInfo->szWorldModel[0])
            m_iWorldModelIndex = PrecacheModel(pInfo->szWorldModel);

        SetModel(pInfo->szWorldModel);
    }
}

//-----------------------------------------------------------------------------
ScriptWeaponInfo_t* CBaseScriptWeapon::GetScriptWpnData() 
{
    return GetScriptWeaponSystem()->GetInfo(GetClassname());
}

//-----------------------------------------------------------------------------
const char* CBaseScriptWeapon::GetViewModel(int iViewModel)
{
    ScriptWeaponInfo_t* pInfo = GetScriptWpnData();
    if (pInfo && pInfo->szViewModel[0])
        return pInfo->szViewModel;
    return BaseClass::GetViewModel();
}

const char* CBaseScriptWeapon::GetWorldModel(void)
{
    ScriptWeaponInfo_t* pInfo = GetScriptWpnData();
    if (pInfo && pInfo->szWorldModel[0])
        return pInfo->szWorldModel;
    return BaseClass::GetWorldModel();
}

//-----------------------------------------------------------------------------
bool CBaseScriptWeapon::Deploy(void)
{
    return BaseClass::Deploy();
}

bool CBaseScriptWeapon::Holster(CBaseCombatWeapon* pSwitchingTo)
{
    return BaseClass::Holster(pSwitchingTo);
}

//-----------------------------------------------------------------------------
void CBaseScriptWeapon::ItemPostFrame(void)
{
    BaseClass::ItemPostFrame();
}

void CBaseScriptWeapon::ItemBusyFrame(void)
{
    BaseClass::ItemBusyFrame();
}

void CBaseScriptWeapon::WeaponIdle(void)
{
    BaseClass::WeaponIdle();
}

void CBaseScriptWeapon::PrimaryAttack(void)
{
    
}

void CBaseScriptWeapon::SecondaryAttack(void)
{
    
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
bool CBaseScriptWeapon::SendWeaponAnim(int iActivity)
{
    return BaseClass::SendWeaponAnim(iActivity);
}