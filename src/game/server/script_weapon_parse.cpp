#include "cbase.h"
#include "script_weapon_parse.h"
#include "filesystem.h"
#include "KeyValues.h"

static CScriptWeaponSystem g_ScriptWeaponSystem;

CScriptWeaponSystem* GetScriptWeaponSystem()
{
    return &g_ScriptWeaponSystem;
}

//-----------------------------------------------------------------------------
CScriptWeaponSystem::CScriptWeaponSystem() {}
CScriptWeaponSystem::~CScriptWeaponSystem()
{
    m_Info.PurgeAndDeleteElements();
}

//-----------------------------------------------------------------------------
ScriptWeaponInfo_t* CScriptWeaponSystem::GetInfo(const char* szClassname)
{
    for (int i = 0; i < m_Info.Count(); i++)
    {
        if (!Q_stricmp(m_Info[i]->szClassName, szClassname))
            return m_Info[i];
    }
    return LoadFromFile(szClassname);
}

//-----------------------------------------------------------------------------
ScriptWeaponInfo_t* CScriptWeaponSystem::LoadFromFile(const char* szClassname)
{
    char szPath[128];
    Q_snprintf(szPath, sizeof(szPath), "scripts/%s.txt", szClassname);

    KeyValues* pKV = new KeyValues("WeaponData");
    if (!pKV->LoadFromFile(filesystem, szPath, "GAME"))
    {
        pKV->deleteThis();
        return NULL;
    }

    ScriptWeaponInfo_t* pInfo = new ScriptWeaponInfo_t;
    Q_memset(pInfo, 0, sizeof(*pInfo));

    Q_strncpy(pInfo->szClassName, szClassname, sizeof(pInfo->szClassName));
    Q_strncpy(pInfo->szPrintName, pKV->GetString("printname", szClassname), sizeof(pInfo->szPrintName));
    Q_strncpy(pInfo->szViewModel, pKV->GetString("viewmodel", ""), sizeof(pInfo->szViewModel));
    Q_strncpy(pInfo->szWorldModel, pKV->GetString("playermodel", ""), sizeof(pInfo->szWorldModel));

    pInfo->iBucket = pKV->GetInt("bucket", 0);
    pInfo->iBucketPosition = pKV->GetInt("bucket_position", 0);

    pInfo->iMaxClip1 = pKV->GetInt("clip_size", -1);
    pInfo->iDefaultClip1 = pKV->GetInt("default_clip", pInfo->iMaxClip1);
    pInfo->iMaxClip2 = pKV->GetInt("clip2_size", -1);
    pInfo->iDefaultClip2 = pKV->GetInt("default_clip2", pInfo->iMaxClip2);

    pInfo->iPrimaryAmmoType = pKV->GetInt("primary_ammo_index", -1);
    pInfo->iSecondaryAmmoType = pKV->GetInt("secondary_ammo_index", -1);

    pInfo->flTimeFireDelay = pKV->GetFloat("TimeFireDelay", 0.1f);
    pInfo->flTimeIdle = pKV->GetFloat("TimeIdle", 0.5f);
    pInfo->flTimeReload = pKV->GetFloat("TimeReload", 1.0f);
    pInfo->flTimeReloadStart = pKV->GetFloat("TimeReloadStart", 0.5f);

    pInfo->iDamage = pKV->GetInt("Damage", 0);
    pInfo->flRange = pKV->GetFloat("Range", 1000.0f);
    pInfo->iBulletsPerShot = pKV->GetInt("BulletsPerShot", 1);
    pInfo->flSpread = pKV->GetFloat("Spread", 0.0f);
    pInfo->flPunchAngle = pKV->GetFloat("PunchAngle", 0.0f);

    // Звуки
    KeyValues* pSound = pKV->FindKey("SoundData");
    if (pSound)
    {
        for (KeyValues* pSub = pSound->GetFirstSubKey(); pSub; pSub = pSub->GetNextKey())
        {
            ScriptWeaponInfo_t::ScriptSound_t s;
            Q_strncpy(s.szName, pSub->GetName(), sizeof(s.szName));
            Q_strncpy(s.szSoundFile, pSub->GetString(), sizeof(s.szSoundFile));
            pInfo->sounds.AddToTail(s);
        }
    }

    pKV->deleteThis();
    m_Info.AddToTail(pInfo);
    return pInfo;
}

//-----------------------------------------------------------------------------
void CScriptWeaponSystem::Reload()
{
    m_Info.PurgeAndDeleteElements();
}