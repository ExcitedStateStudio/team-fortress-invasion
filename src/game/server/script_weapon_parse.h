#ifndef SCRIPT_WEAPON_PARSE_H
#define SCRIPT_WEAPON_PARSE_H
#ifdef _WIN32
#pragma once
#endif

#include "tier1/utlvector.h"

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
struct ScriptWeaponInfo_t
{
    char    szClassName[64];
    char    szPrintName[64];

    char    szViewModel[128];
    char    szWorldModel[128];

    int     iBucket;
    int     iBucketPosition;

    int     iMaxClip1;
    int     iDefaultClip1;
    int     iMaxClip2;
    int     iDefaultClip2;

    int     iPrimaryAmmoType;
    int     iSecondaryAmmoType;

    float   flTimeFireDelay;
    float   flTimeIdle;
    float   flTimeReload;
    float   flTimeReloadStart;

    int     iDamage;
    float   flRange;
    int     iBulletsPerShot;
    float   flSpread;
    float   flPunchAngle;

    // Звуки
    struct ScriptSound_t
    {
        char szName[64];
        char szSoundFile[128];
    };
    CUtlVector<ScriptSound_t> sounds;
};

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
class CScriptWeaponSystem
{
public:
    CScriptWeaponSystem();
    ~CScriptWeaponSystem();

    void                    LoadAll();
    void                    Reload();

    ScriptWeaponInfo_t* GetInfo(const char* szClassname);

private:
    ScriptWeaponInfo_t* LoadFromFile(const char* szClassname);

    CUtlVector<ScriptWeaponInfo_t*> m_Info;
};

extern CScriptWeaponSystem* GetScriptWeaponSystem();

#endif