#include "cbase.h"
#include "tf_soldier_commando.h"
#include "tf_gamerules.h"
#include "particle_parse.h"
#include "effect_dispatch_data.h"
#include "te_effect_dispatch.h"
#include "tf_fx.h"
#include "soundenvelope.h"
#include <in_buttons.h>


#include "tf_wearable_weapons.h"

//-------------------------------------------------------------------------
// Convars
//-------------------------------------------------------------------------

ConVar tf_sc_enabled("tf_sc_enabled", "1", FCVAR_NOTIFY,
    "Enable somando");

ConVar tf_sc_bullrush_doubletap_window("tf_sc_bullrush_doubletap_window", "3", FCVAR_NOTIFY,
    "How much time we need to hold doubletap window open?");


ConVar tf_sc_bullrush_cooldown("tf_sc_bullrush_cooldown", "30.0", FCVAR_NOTIFY,
    "Commando's bullrush CD");

ConVar tf_sc_charge_crash_stun("tf_sc_charge_crash_stun", "2.0", FCVAR_NOTIFY,
    "Commando's stun time.");


ConVar tf_sc_charge_crash_speed_eps("tf_sc_charge_crash_speed_eps", "50.0", FCVAR_NOTIFY,
    "The speed threshold after which we stop");

ConVar tf_sc_charge_crash_min_moving_time("tf_sc_charge_crash_min_moving_time", "0.15", FCVAR_NOTIFY);

// Adrenaline
ConVar tf_sc_adrenaline_enabled("tf_sc_adrenaline_enabled", "1", FCVAR_NOTIFY);
ConVar tf_sc_adrenaline_duration("tf_sc_adrenaline_duration", "10", FCVAR_NOTIFY);
ConVar tf_sc_adrenaline_cooldown("tf_sc_adrenaline_cooldown", "60", FCVAR_NOTIFY);

// Battlecry
ConVar tf_sc_battlecry_enabled("tf_sc_battlecry_enabled", "1", FCVAR_NOTIFY);
ConVar tf_sc_battlecry_radius("tf_sc_battlecry_radius", "512", FCVAR_NOTIFY);
ConVar tf_sc_battlecry_duration("tf_sc_battlecry_duration", "10", FCVAR_NOTIFY);
ConVar tf_sc_battlecry_cooldown("tf_sc_battlecry_cooldown", "60", FCVAR_NOTIFY);

// Boot
ConVar tf_sc_boot_enabled("tf_sc_boot_enabled", "1", FCVAR_NOTIFY);
ConVar tf_sc_boot_range("tf_sc_boot_range", "48", FCVAR_NOTIFY);
ConVar tf_sc_boot_damage("tf_sc_boot_damage", "25", FCVAR_NOTIFY);
ConVar tf_sc_boot_cooldown("tf_sc_boot_cooldown", "1.5", FCVAR_NOTIFY);

//-------------------------------------------------------------------------
// Per-player storage
//-------------------------------------------------------------------------
static CUtlMap< CTFPlayer*, CSoldierCommando* > g_CommandoMap(DefLessFunc(CTFPlayer*));

CSoldierCommando* GetSoldierCommando(CTFPlayer* pPlayer)
{
    if (!pPlayer)
        return NULL;

    int idx = g_CommandoMap.Find(pPlayer);
    if (idx == g_CommandoMap.InvalidIndex())
    {
        CSoldierCommando* p = new CSoldierCommando();
        p->SetOwner(pPlayer);
        g_CommandoMap.Insert(pPlayer, p);
        return p;
    }
    return g_CommandoMap[idx];
}

//-------------------------------------------------------------------------
CSoldierCommando::CSoldierCommando()
{
    m_bWaitSecondTap = false;
    m_flDoubleTapWindowEnd = 0.f;
    m_flNextAdrenalineTime = 0.f;
    m_flNextBattlecryTime = 0.f;
    m_flNextBootTime = 0.f;
    m_bChargeCamActive = false;
}

CSoldierCommando::~CSoldierCommando()
{
}

void CSoldierCommando::OnSpawn()
{
    m_flNextAdrenalineTime = 0.f;
    m_flNextBattlecryTime = 0.f;
    m_flNextBootTime = 0.f;
    m_bWaitSecondTap = false;
    m_flDoubleTapWindowEnd = 0.f;
 //Bullrush stun
    m_bChargeCrashed = false;
    m_flChargeMovingSince = 0.f;
//Demo's shield
    m_hChargeShield = NULL;

  ClearChargeCamera();
}

void CSoldierCommando::OnDeath()
{
  ClearChargeCamera();
}


//-------------------------------------------------------------------------
//Giving shield to player...(Econ bypass)
//-------------------------------------------------------------------------
CTFWearableDemoShield* CSoldierCommando::EnsureChargeShield()
{
    CTFPlayer* pOwner = m_hOwner.Get();
    if (!pOwner)
        return NULL;


    for (int i = 0; i < pOwner->GetNumWearables(); ++i)
    {
        CTFWearableDemoShield* pShield =
            dynamic_cast<CTFWearableDemoShield*>(pOwner->GetWearable(i));
        if (pShield)
            return pShield;
    }


    if (m_hChargeShield)
        return m_hChargeShield;

    //WITHOUT ECON. --AGR
    CTFWearableDemoShield* pShield =
        dynamic_cast<CTFWearableDemoShield*>(CreateEntityByName("tf_wearable_demoshield"));

    if (!pShield)
    {
        Warning("Failed to give shield to player!\n");
        return NULL;
    }


    pShield->SetAbsOrigin(pOwner->GetAbsOrigin());
    pShield->SetAbsAngles(pOwner->GetAbsAngles());
    pShield->SetLocalAngles(pOwner->GetLocalAngles());

    DispatchSpawn(pShield);


    pOwner->EquipWearable(pShield);

   
    pShield->AddEffects(EF_NODRAW | EF_NOSHADOW);
    pShield->AddSolidFlags(FSOLID_NOT_SOLID);

    m_hChargeShield = pShield;
    return pShield;
}

//AGR START
void CSoldierCommando::ThinkChargeCrash()
{
    CTFPlayer* pOwner = m_hOwner.Get();
    if (!pOwner || !pOwner->IsAlive())
        return;

    if (!pOwner->m_Shared.InCond(TF_COND_SHIELD_CHARGE))
    {
        m_bChargeCrashed = false;
        m_flChargeMovingSince = 0.f;
        return;
    }

    if (m_bChargeCrashed)
        return;

    Vector vecVel = pOwner->GetAbsVelocity();
    vecVel.z = 0.f;
    float flSpeed2D = vecVel.Length();

    float flEps = tf_sc_charge_crash_speed_eps.GetFloat();

    if (flSpeed2D > flEps)
    {
        if (m_flChargeMovingSince == 0.f)
            m_flChargeMovingSince = gpGlobals->curtime;
        return;
    }

    if (m_flChargeMovingSince == 0.f)
        return;
    if (gpGlobals->curtime - m_flChargeMovingSince < tf_sc_charge_crash_min_moving_time.GetFloat())
        return;

    Vector vecForward = pOwner->BodyDirection2D();
    Vector vecStart = pOwner->GetAbsOrigin();
    Vector vecEnd = vecStart + vecForward * 40.f;

    trace_t tr;
    UTIL_TraceLine(vecStart, vecEnd, MASK_SOLID_BRUSHONLY, pOwner, COLLISION_GROUP_NONE, &tr);

    if (tr.fraction >= 1.f && !tr.startsolid)
        return; 

    m_bChargeCrashed = true;
    pOwner->m_Shared.RemoveCond(TF_COND_SHIELD_CHARGE);
    pOwner->SetAbsVelocity(vec3_origin);
    pOwner->m_Shared.AddCond(TF_COND_STUNNED, tf_sc_charge_crash_stun.GetFloat());
    pOwner->EmitSound("Player.DenyWeaponSelection");
}
//AGR END
//-------------------------------------------------------------------------
// Tapes
//-------------------------------------------------------------------------
void CSoldierCommando::NoteButtonState(int nButtons, int nOldButtons)
{
    if (!tf_sc_enabled.GetBool())
        return;

    CTFPlayer* pOwner = m_hOwner.Get();
    if (!pOwner || !pOwner->IsAlive())
        return;

    if (!pOwner->IsPlayerClass(TF_CLASS_SOLDIER))
        return;


    if (pOwner->m_Shared.InCond(TF_COND_SHIELD_CHARGE))
    {
        m_bWaitSecondTap = false;
        m_flDoubleTapWindowEnd = 0.f;
        return;
    }

    bool bForwardPressed = (nButtons & IN_FORWARD) != 0;
    bool bForwardWasDown = (nOldButtons & IN_FORWARD) != 0;
    bool bForwardJustDown = bForwardPressed && !bForwardWasDown;

 

    if (nButtons & (IN_BACK | IN_MOVELEFT | IN_MOVERIGHT | IN_JUMP | IN_DUCK))
    {
        m_bWaitSecondTap = false;
        m_flDoubleTapWindowEnd = 0.f;
        return;
    }

    if (!bForwardJustDown)
        return;


    if (m_bWaitSecondTap && gpGlobals->curtime <= m_flDoubleTapWindowEnd)
    {
        m_bWaitSecondTap = false;
        m_flDoubleTapWindowEnd = 0.f;

        CTFWearableDemoShield* pShield = EnsureChargeShield();
        if (pShield && pShield->CanCharge(pOwner))
        {

            pShield->DoSpecialAction(pOwner);
        }
        else
        {
            pOwner->EmitSound("Player.DenyWeaponSelection");
        }
    }
    else
    {
   
        m_bWaitSecondTap = true;
        m_flDoubleTapWindowEnd = gpGlobals->curtime + tf_sc_bullrush_doubletap_window.GetFloat();
    }
}

//-------------------------------------------------------------------------
// Adrenaline
//-------------------------------------------------------------------------
//FIXME --Needs techtree.
void CSoldierCommando::ActivateAdrenaline()
{
    if (!tf_sc_enabled.GetBool() || !tf_sc_adrenaline_enabled.GetBool())
        return;

    CTFPlayer* pOwner = m_hOwner.Get();
    if (!pOwner || !pOwner->IsAlive())
        return;
    if (!pOwner->IsPlayerClass(TF_CLASS_SOLDIER))
        return;

    if (gpGlobals->curtime < m_flNextAdrenalineTime)
    {
        pOwner->EmitSound("Player.DenyWeaponSelection");
        return;
    }

    m_flNextAdrenalineTime = gpGlobals->curtime + tf_sc_adrenaline_cooldown.GetFloat();

    pOwner->m_bAdrenalineActive = true;
    pOwner->m_flAdrenalineEndTime = gpGlobals->curtime + tf_sc_adrenaline_duration.GetFloat();
    pOwner->m_flAdrenalineSpeedMult = tf_sc_adrenaline_speed_mult.GetFloat();
    pOwner->TeamFortress_SetSpeed();

    pOwner->EmitSound("Commando.Adrenaline");
    DispatchParticleEffect("adrenaline_rush", PATTACH_ABSORIGIN_FOLLOW, pOwner);
}

void CSoldierCommando::ThinkAdrenaline()
{
    CTFPlayer* pOwner = m_hOwner.Get();
    if (!pOwner) return;

    if (pOwner->m_bAdrenalineActive &&
        gpGlobals->curtime >= pOwner->m_flAdrenalineEndTime)
    {
        pOwner->m_bAdrenalineActive = false;
        pOwner->m_flAdrenalineEndTime = 0.f;
        pOwner->TeamFortress_SetSpeed();
    }
}

//-------------------------------------------------------------------------
// Battlecry
//-------------------------------------------------------------------------
void CSoldierCommando::ActivateBattlecry()
{
    if (!tf_sc_enabled.GetBool() || !tf_sc_battlecry_enabled.GetBool())
        return;

    CTFPlayer* pOwner = m_hOwner.Get();
    if (!pOwner || !pOwner->IsAlive())
        return;
    if (!pOwner->IsPlayerClass(TF_CLASS_SOLDIER))
        return;

    if (gpGlobals->curtime < m_flNextBattlecryTime)
        return;

    m_flNextBattlecryTime = gpGlobals->curtime + tf_sc_battlecry_cooldown.GetFloat();

    float flRadius = tf_sc_battlecry_radius.GetFloat();
    float flDuration = tf_sc_battlecry_duration.GetFloat();

    CUtlVector< CTFPlayer* > vecTeam;
    CollectPlayers(&vecTeam, pOwner->GetTeamNumber(), COLLECT_ONLY_LIVING_PLAYERS);

    Vector vecMyOrigin = pOwner->GetAbsOrigin();
    int nBuffed = 0;

    FOR_EACH_VEC(vecTeam, i)
    {
        CTFPlayer* pMate = vecTeam[i];
        if (!pMate || pMate == pOwner)
            continue;
        if ((pMate->GetAbsOrigin() - vecMyOrigin).Length() > flRadius)
            continue;

        trace_t tr;
        UTIL_TraceLine(pOwner->EyePosition(), pMate->EyePosition(),
            MASK_SOLID_BRUSHONLY, pOwner, COLLISION_GROUP_NONE, &tr);
        if (tr.fraction < 1.f && tr.m_pEnt != pMate)
            continue;

        pMate->m_flAdrenalineEndTime = gpGlobals->curtime + flDuration;
        pMate->m_flAdrenalineSpeedMult = tf_sc_adrenaline_speed_mult.GetFloat();
        pMate->TeamFortress_SetSpeed();
        pMate->EmitSound("Commando.BattlecryRecv");
        nBuffed++;
    }

    pOwner->EmitSound("Commando.Battlecry");
    DispatchParticleEffect("battlecry_wave", PATTACH_ABSORIGIN_FOLLOW, pOwner);
}

void CSoldierCommando::ThinkBattlecry()
{

}



void CSoldierCommando::ThinkChargeRecharge()
{
    CTFPlayer* pOwner = m_hOwner.Get();
    if (!pOwner || !pOwner->IsAlive())
        return;


    if (pOwner->m_Shared.InCond(TF_COND_SHIELD_CHARGE))
        return;

    float flMeter = pOwner->m_Shared.GetDemomanChargeMeter();
    if (flMeter >= 100.f)
        return;

    float flCooldown = Max(tf_sc_bullrush_cooldown.GetFloat(), 0.01f);
    flMeter += gpGlobals->frametime * (100.f / flCooldown);
    if (flMeter > 100.f)
        flMeter = 100.f;

    pOwner->m_Shared.SetDemomanChargeMeter(flMeter);
}

//-------------------------------------------------------------------------
// Auto melee
//-------------------------------------------------------------------------
void CSoldierCommando::ThinkBoot()
{
    if (!tf_sc_enabled.GetBool() || !tf_sc_boot_enabled.GetBool())
        return;

    CTFPlayer* pOwner = m_hOwner.Get();
    if (!pOwner || !pOwner->IsAlive())
        return;
    if (!pOwner->IsPlayerClass(TF_CLASS_SOLDIER))
        return;

    if (gpGlobals->curtime < m_flNextBootTime)
        return;

    Vector vecSrc = pOwner->Weapon_ShootPosition();
    Vector vecDir = pOwner->BodyDirection2D();
    Vector vecTarget = vecSrc + vecDir * tf_sc_boot_range.GetFloat();

    CBaseEntity* pList[16];
    int nCount = UTIL_EntitiesInSphere(pList, 16, vecTarget, 24.f, FL_CLIENT);
    for (int i = 0; i < nCount; ++i)
    {
        CTFPlayer* pTarget = ToTFPlayer(pList[i]);
        if (!pTarget || pTarget == pOwner)
            continue;
        if (pTarget->InSameTeam(pOwner) || !pTarget->IsAlive())
            continue;

        CTakeDamageInfo info(pOwner, pOwner, tf_sc_boot_damage.GetFloat(), DMG_CLUB);
        Vector vecPush = vecDir;
        vecPush.z = Max(0.8f, vecPush.z);
        VectorNormalize(vecPush);
        info.SetDamageForce(vecPush * 500.f);
        pTarget->TakeDamage(info);

        Vector vecVel = pTarget->GetAbsVelocity();
        vecVel.z += 250.f;
        pTarget->SetAbsVelocity(vecVel);

        pOwner->EmitSound("Commando.BootHit");

        m_flNextBootTime = gpGlobals->curtime + tf_sc_boot_cooldown.GetFloat();
        break;
    }
}

//-------------------------------------------------------------------------
// thirdperson camer while rushing
//-------------------------------------------------------------------------
void CSoldierCommando::ThinkChargeCamera()
{
  CTFPlayer* pOwner = m_hOwner.Get();
  if (!pOwner)
    return;
  //AGR START
  if (pOwner->m_Shared.InCond(TF_COND_STUNNED))
  {
      if (m_bChargeCamActive)
      {
          pOwner->SetForcedTauntCam(0);
          m_bChargeCamActive = false;
      }
      return;
  }
  //AGR END

  bool bCharging = pOwner->m_Shared.InCond(TF_COND_SHIELD_CHARGE);

  if (bCharging && !m_bChargeCamActive)
  {
    pOwner->SetForcedTauntCam(1);
    m_bChargeCamActive = true;
  }
  else if (!bCharging && m_bChargeCamActive)
  {
    ClearChargeCamera();
  }
}

void CSoldierCommando::ClearChargeCamera()
{
  if (!m_bChargeCamActive)
    return;

  CTFPlayer* pOwner = m_hOwner.Get();
  if (pOwner)
    pOwner->SetForcedTauntCam(0);

  m_bChargeCamActive = false;
}

//-------------------------------------------------------------------------
// Main think
//-------------------------------------------------------------------------
void CSoldierCommando::Think()
{
    if (!tf_sc_enabled.GetBool())
        return;

    CTFPlayer* pOwner = m_hOwner.Get();
    if (!pOwner)
        return;

    if (!pOwner->IsAlive())
        return;

    if (!pOwner->IsPlayerClass(TF_CLASS_SOLDIER))
        return;

    ThinkAdrenaline();
    ThinkBattlecry();
    ThinkChargeRecharge();
    ThinkChargeCrash();
    ThinkChargeCamera();
    // ThinkBoot();
}
