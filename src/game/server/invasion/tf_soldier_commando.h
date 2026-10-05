#ifndef TF_SOLDIER_COMMANDO_H
#define TF_SOLDIER_COMMANDO_H

#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "tf_player.h"

extern ConVar tf_sc_bullrush_speed_mult;
extern ConVar tf_sc_adrenaline_speed_mult;
extern ConVar tf_sc_bullrush_anim_speed;
extern ConVar tf_sc_adrenaline_speed_mult;
//=========================================================================
// Отдельная система фич коммандо для солдата.
// Не использует: TF_COND_SHIELD_CHARGE, старый CPlayerClassCommando,
// демоменовский charge. Всё своё.
//=========================================================================

class CSoldierCommando
{
public:
    CSoldierCommando();
    ~CSoldierCommando();

    void    SetOwner(CTFPlayer* pOwner) { m_hOwner = pOwner; }

    // Главный тик, вызывается из CTFPlayer::TFPlayerThink()
    void    Think();

    // Сброс состояния при спавне/смерти
    void    OnSpawn();
    void    OnDeath();

    // Активируемые способности
    void    ActivateAdrenaline();
    void    ActivateBattlecry();

    // Детектор двойного тапа вперёд
    void    NoteButtonState(int nButtons, int nOldButtons);

    bool    InBullRush() const { return m_bBullRush; }

private:
    void    ThinkAdrenaline();
    void    ThinkBullRush();
    void    ThinkBattlecry();
    void    ThinkBoot();

    void    StartBullRush();
    void    StopBullRush();
    bool    BullRushTouchEnemies();   
    bool    BullRushCheckWall();    

    CHandle<CTFPlayer>  m_hOwner;

    // --- Bull Rush ---
    bool    m_bBullRush;
    float   m_flBullRushEndTime;
    float   m_flDoubleTapWindowEnd;
   
    float   m_flNextBullRushTime;

    bool    m_bWaitSecondTap;
    Vector  m_vecBullRushDir;      // зафиксированное направление рывка
    CUtlVector<CHandle<CTFPlayer>> m_vecHitPlayers;  // кто уже получил урон за рывок

    // --- Adrenaline ---
    float   m_flNextAdrenalineTime;

    // --- Battlecry ---
    float   m_flNextBattlecryTime;

    // --- Boot ---
    float   m_flNextBootTime;
};

extern CSoldierCommando* GetSoldierCommando(CTFPlayer* pPlayer);

#endif