#include "cbase.h"
#include "tf_soldier_commando.h"
#include "tf_gamerules.h"
#include "particle_parse.h"
#include "effect_dispatch_data.h"
#include "te_effect_dispatch.h"
#include "tf_fx.h"
#include "soundenvelope.h"
#include <in_buttons.h>

//-------------------------------------------------------------------------
// Конвары — крутятся в консоли, ничего пересобирать не надо
//-------------------------------------------------------------------------
ConVar tf_sc_enabled("tf_sc_enabled", "1", FCVAR_NOTIFY,
    "Включить фичи коммандо для солдата");

ConVar tf_sc_speed_max("tf_sc_speed_max", "200", FCVAR_NOTIFY,
    "Скорость солдата-коммандо");
ConVar tf_sc_health_max("tf_sc_health_max", "200", FCVAR_NOTIFY,
    "Somando's hp");

// Bull Rush
ConVar tf_sc_bullrush_enabled("tf_sc_bullrush_enabled", "1", FCVAR_NOTIFY);
ConVar tf_sc_bullrush_doubletap_window("tf_sc_bullrush_doubletap_window", "3", FCVAR_NOTIFY,
    "Окно между двумя нажатиями W для старта рывка (сек)");
ConVar tf_sc_bullrush_duration("tf_sc_bullrush_duration", "2.0", FCVAR_NOTIFY,
    "Длительность рывка (сек)");
//Moved to tf_player_shared
//ConVar tf_sc_bullrush_speed_mult("tf_sc_bullrush_speed_mult", "10.0", FCVAR_NOTIFY,
 //   "Множитель скорости во время рывка");
ConVar tf_sc_bullrush_damage("tf_sc_bullrush_damage", "100", FCVAR_NOTIFY,
    "Урон при столкновении во время рывка");

ConVar tf_sc_bullrush_cooldown("tf_sc_bullrush_cooldown", "30.0", FCVAR_NOTIFY,
    "Кулдаун рывка солдата-коммандо (сек)");


ConVar tf_sc_bullrush_wall_damage("tf_sc_bullrush_wall_damage", "25", FCVAR_NOTIFY,
    "Урон коммандо при ударе в стену во время рывка");
ConVar tf_sc_bullrush_wall_stun("tf_sc_bullrush_wall_stun", "3.0", FCVAR_NOTIFY,
    "Длительность стана коммандо после удара в стену (сек)");

// Adrenaline
ConVar tf_sc_adrenaline_enabled("tf_sc_adrenaline_enabled", "1", FCVAR_NOTIFY);
ConVar tf_sc_adrenaline_duration("tf_sc_adrenaline_duration", "10", FCVAR_NOTIFY);
ConVar tf_sc_adrenaline_cooldown("tf_sc_adrenaline_cooldown", "60", FCVAR_NOTIFY);
//ConVar tf_sc_adrenaline_speed_mult("tf_sc_adrenaline_speed_mult", "1.4", FCVAR_NOTIFY);

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
    m_bBullRush = false;
    m_flBullRushEndTime = 0.f;
    m_flDoubleTapWindowEnd = 0.f;
    m_bWaitSecondTap = false;
    m_vecBullRushDir.Init();
    m_flNextAdrenalineTime = 0.f;
    m_flNextBattlecryTime = 0.f;
    m_flNextBootTime = 0.f;
    m_flNextBullRushTime = 0.f;
}

CSoldierCommando::~CSoldierCommando()
{
    StopBullRush();
}

void CSoldierCommando::OnSpawn()
{
    StopBullRush();
    m_flNextAdrenalineTime = 0.f;
    m_flNextBattlecryTime = 0.f;
    m_flNextBootTime = 0.f;
    m_bWaitSecondTap = false;
    m_flDoubleTapWindowEnd = 0.f;
    m_flNextBullRushTime = 0.f;
    m_vecHitPlayers.RemoveAll();
}

void CSoldierCommando::OnDeath()
{
    m_flNextBullRushTime = 0.f;
    StopBullRush();
}

//-------------------------------------------------------------------------
// Детектор двойного тапа вперёд. Вызывается из PlayerRunCommand/PreThink.
//-------------------------------------------------------------------------
void CSoldierCommando::NoteButtonState(int nButtons, int nOldButtons)
{
    if (!tf_sc_enabled.GetBool() || !tf_sc_bullrush_enabled.GetBool())
        return;

    if (m_bBullRush)
        return;

    CTFPlayer* pOwner = m_hOwner.Get();
    if (!pOwner || !pOwner->IsAlive())
        return;

    if (!pOwner->IsPlayerClass(TF_CLASS_SOLDIER))
        return;

 
    if (gpGlobals->curtime < m_flNextBullRushTime)
    {
        m_bWaitSecondTap = false;
        m_flDoubleTapWindowEnd = 0.f;
        return;
    }

    bool bForwardPressed = (nButtons & IN_FORWARD) != 0;
    bool bForwardWasDown = (nOldButtons & IN_FORWARD) != 0;
    bool bForwardJustDown = bForwardPressed && !bForwardWasDown;

    // Любая другая клавиша движения сбрасывает окно
    if (nButtons & (IN_BACK | IN_MOVELEFT | IN_MOVERIGHT | IN_JUMP | IN_DUCK))
    {
        m_bWaitSecondTap = false;
        m_flDoubleTapWindowEnd = 0.f;
        return;
    }

    if (bForwardJustDown)
    {
        if (m_bWaitSecondTap && gpGlobals->curtime <= m_flDoubleTapWindowEnd)
        {
            // Двойной тап!
            m_bWaitSecondTap = false;
            m_flDoubleTapWindowEnd = 0.f;
            StartBullRush();
        }
        else
        {
            // Первый тап
            m_bWaitSecondTap = true;
            m_flDoubleTapWindowEnd = gpGlobals->curtime + tf_sc_bullrush_doubletap_window.GetFloat();
        }
    }
}

//-------------------------------------------------------------------------
void CSoldierCommando::StartBullRush()
{
    CTFPlayer* pOwner = m_hOwner.Get();
    if (!pOwner || !pOwner->IsAlive())
        return;

    if (!pOwner->IsPlayerClass(TF_CLASS_SOLDIER))
        return;

    // --- Кулдаун ---
    if (gpGlobals->curtime < m_flNextBullRushTime)
    {
        pOwner->EmitSound("Player.DenyWeaponSelection");
        return;
    }

    // --- В прыжке нельзя ---
    if (!(pOwner->GetFlags() & FL_ONGROUND))
    {
        pOwner->EmitSound("Player.DenyWeaponSelection");
        return;
    }

    // --- В ноуклипе / обсервере нельзя ---
    int nMoveType = pOwner->GetMoveType();
    if (nMoveType == MOVETYPE_NOCLIP || nMoveType == MOVETYPE_OBSERVER)
    {
        pOwner->EmitSound("Player.DenyWeaponSelection");
        return;
    }

    m_flNextBullRushTime = gpGlobals->curtime + tf_sc_bullrush_cooldown.GetFloat();

    pOwner->m_bBullRushActive = true;
    pOwner->TeamFortress_SetSpeed();

    m_bBullRush = true;
    m_flBullRushEndTime = gpGlobals->curtime + tf_sc_bullrush_duration.GetFloat();
    m_vecHitPlayers.RemoveAll();

    QAngle ang = pOwner->EyeAngles();
    ang[PITCH] = 0;
    ang[ROLL] = 0;
    AngleVectors(ang, &m_vecBullRushDir);
    m_vecBullRushDir.z = 0;
    VectorNormalize(m_vecBullRushDir);

    pOwner->EmitSound("Commando.BullRushScream");
    DispatchParticleEffect("bullrush_trail", PATTACH_ABSORIGIN_FOLLOW, pOwner);
}
void CSoldierCommando::StopBullRush()
{
    if (!m_bBullRush)
        return;

    if (CTFPlayer* pOwner = m_hOwner.Get())
    {
        pOwner->m_bBullRushActive = false;
        pOwner->TeamFortress_SetSpeed();
    }

    m_bBullRush = false;
    m_flBullRushEndTime = 0.f;
    m_vecHitPlayers.RemoveAll();

}
//-------------------------------------------------------------------------
// Тик рывка: держим скорость, ищем столкновения с врагами
//-------------------------------------------------------------------------
void CSoldierCommando::ThinkBullRush()
{
    CTFPlayer* pOwner = m_hOwner.Get();
    if (!pOwner)
        return;

    if (!m_bBullRush)
        return;

    if (!pOwner->IsAlive() || gpGlobals->curtime >= m_flBullRushEndTime)
    {
        StopBullRush();
        return;
    }

    pOwner->TeamFortress_SetSpeed();

    // Углы — по зафиксированному направлению рывка
    QAngle angNew = pOwner->GetLocalAngles();
    angNew[YAW] = m_vecBullRushDir.y == 0 && m_vecBullRushDir.x == 0
        ? angNew[YAW]
        : RAD2DEG(atan2f(m_vecBullRushDir.y, m_vecBullRushDir.x));
    angNew[PITCH] = 0;
    pOwner->SetLocalAngles(angNew);

    QAngle angLock(0, angNew[YAW], 0);
    pOwner->ForcePlayerViewAngles(angLock);

    // 1. Попали в игрока? → стоп без урона себе
    if (BullRushTouchEnemies())
    {
        StopBullRush();
        return;
    }

    // 2. Влетели в стену? → стоп + урон + стан
    if (BullRushCheckWall())
    {
        // Внутри всё уже сделано (урон, стан, StopBullRush)
        return;
    }
}

//-------------------------------------------------------------------------
bool CSoldierCommando::BullRushTouchEnemies()
{
    CTFPlayer* pOwner = m_hOwner.Get();
    if (!pOwner)
        return false;

    Vector vecCenter = pOwner->WorldSpaceCenter() + m_vecBullRushDir * 32.f;
    CBaseEntity* pList[32];
    int nCount = UTIL_EntitiesInSphere(pList, 32, vecCenter, 48.f, FL_CLIENT);

    bool bHitAnyone = false;

    for (int i = 0; i < nCount; ++i)
    {
        CTFPlayer* pVictim = ToTFPlayer(pList[i]);
        if (!pVictim || pVictim == pOwner)
            continue;
        if (pVictim->InSameTeam(pOwner))
            continue;
        if (!pVictim->IsAlive())
            continue;
        if (m_vecHitPlayers.Find(pVictim) != m_vecHitPlayers.InvalidIndex())
            continue;

        CTakeDamageInfo info(pOwner, pOwner, tf_sc_bullrush_damage.GetFloat(), DMG_CLUB);
        Vector vecDir = pVictim->WorldSpaceCenter() - pOwner->WorldSpaceCenter();
        VectorNormalize(vecDir);
        vecDir.z = 0.5f;
        info.SetDamageForce(vecDir * 600.f);
        pVictim->TakeDamage(info);

        pOwner->EmitSound("Commando.BullRushFlesh");

        m_vecHitPlayers.AddToTail(pVictim);
        bHitAnyone = true;
    }

    return bHitAnyone;
}

bool CSoldierCommando::BullRushCheckWall()
{
    CTFPlayer* pOwner = m_hOwner.Get();
    if (!pOwner)
        return false;

    // Смотрим вперёд по направлению рывка
    Vector vecStart = pOwner->WorldSpaceCenter();
    Vector vecEnd = vecStart + m_vecBullRushDir * 40.f;

    trace_t tr;
    CTraceFilterSimple filter(pOwner, COLLISION_GROUP_PLAYER);
    UTIL_TraceHull(vecStart, vecEnd, VEC_HULL_MIN, VEC_HULL_MAX,
        MASK_PLAYERSOLID, &filter, &tr);

    if (tr.fraction >= 1.0f || !tr.m_pEnt)
        return false;

    // Нас интересует именно стена/браш, не игрок и не проп
    if (!tr.m_pEnt->IsWorld() && !tr.m_pEnt->IsBSPModel())
        return false;

    // --- Урон ---
    CTakeDamageInfo info(pOwner, pOwner,
        tf_sc_bullrush_wall_damage.GetFloat(), DMG_CLUB);
    pOwner->TakeDamage(info);

    // --- Стан ---
    pOwner->m_Shared.StunPlayer(tf_sc_bullrush_wall_stun.GetFloat(), 1.f,
        TF_STUN_BOTH | TF_STUN_NO_EFFECTS, pOwner);

    // --- Эффекты ---
    pOwner->EmitSound("Player.FallDamageDealt");
    pOwner->EmitSound("Commando.BullRushWallImpact");  // свой звук, если есть
    DispatchParticleEffect("impact_wallbang_heavy", PATTACH_ABSORIGIN_FOLLOW, pOwner);

    StopBullRush();
    return true;
}

//-------------------------------------------------------------------------
// Adrenaline
//-------------------------------------------------------------------------
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

    pOwner->m_flAdrenalineEndTime = gpGlobals->curtime + tf_sc_adrenaline_duration.GetFloat();
    pOwner->m_flAdrenalineSpeedMult = tf_sc_adrenaline_speed_mult.GetFloat();


    pOwner->m_bAdrenalineActive = true;
    pOwner->m_flAdrenalineEndTime = gpGlobals->curtime + tf_sc_adrenaline_duration.GetFloat();
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

        // LOS
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
    // Ничего периодического, кулдаун проверяется в Activate
}

//-------------------------------------------------------------------------
// Boot — автопинок при враге впритык
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

        // Удар
        CTakeDamageInfo info(pOwner, pOwner, tf_sc_boot_damage.GetFloat(), DMG_CLUB);
        Vector vecPush = vecDir;
        vecPush.z = Max(0.8f, vecPush.z);
        VectorNormalize(vecPush);
        info.SetDamageForce(vecPush * 500.f);
        pTarget->TakeDamage(info);

        // Подброс
        Vector vecVel = pTarget->GetAbsVelocity();
        vecVel.z += 250.f;
        pTarget->SetAbsVelocity(vecVel);

        pOwner->EmitSound("Commando.BootHit");

        m_flNextBootTime = gpGlobals->curtime + tf_sc_boot_cooldown.GetFloat();
        break;
    }
}

//-------------------------------------------------------------------------
// Главный тик
//-------------------------------------------------------------------------
void CSoldierCommando::Think()
{
    if (!tf_sc_enabled.GetBool())
        return;

    CTFPlayer* pOwner = m_hOwner.Get();
    if (!pOwner)
        return;

    if (!pOwner->IsAlive())
    {
        StopBullRush();
        return;
    }

    if (!pOwner->IsPlayerClass(TF_CLASS_SOLDIER))
        return;

    ThinkBullRush();
    ThinkAdrenaline();
    ThinkBattlecry();
   // ThinkBoot();
}