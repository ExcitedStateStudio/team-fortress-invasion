#ifndef BASE_SCRIPTWEAPON_H
#define BASE_SCRIPTWEAPON_H
#ifdef _WIN32
#pragma once
#endif

#include "basecombatweapon_shared.h"
#include "script_weapon_parse.h"

#if defined( CLIENT_DLL )
#define CBaseScriptWeapon C_BaseScriptWeapon
#endif

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
#define SCRIPT_WEAPON_SLOT  5

class CBaseScriptWeapon : public CBaseCombatWeapon
{
    DECLARE_CLASS(CBaseScriptWeapon, CBaseCombatWeapon);
public:
    DECLARE_NETWORKCLASS();
    DECLARE_PREDICTABLE();

    CBaseScriptWeapon();

    virtual void    Spawn(void);
    virtual void    Precache(void);
    virtual bool    Deploy(void);
    virtual bool    Holster(CBaseCombatWeapon* pSwitchingTo);
    virtual void    ItemPostFrame(void);
    virtual void    ItemBusyFrame(void);
    virtual void    WeaponIdle(void);

    virtual int     GetSlot(void) const { return SCRIPT_WEAPON_SLOT; }

    virtual void    PrimaryAttack(void);
    virtual void    SecondaryAttack(void);

    ScriptWeaponInfo_t* GetScriptWpnData() ;


    virtual bool    SendWeaponAnim(int iActivity);
    virtual const char* GetViewModel(int iViewModel = 0);
    virtual const char* GetWorldModel(void);

    virtual bool    IsReflectingAnimations(void) const { return m_bReflectAnims; }
    virtual void    SetReflectViewModelAnimations(bool b) { m_bReflectAnims = b; }
    virtual int     GetLastReflectedActivity(void) const { return m_iLastReflectedActivity; }


    virtual bool    SupportsTwoHanded(void) { return false; }

protected:
    CNetworkVar(bool, m_bReflectAnims);
    int             m_iLastReflectedActivity;

private:
    CBaseScriptWeapon(const CBaseScriptWeapon&);
};

#endif // BASE_SCRIPTWEAPON_H