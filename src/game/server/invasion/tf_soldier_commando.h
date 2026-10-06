#ifndef TF_SOLDIER_COMMANDO_H
#define TF_SOLDIER_COMMANDO_H

#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "tf_player.h"
#include <tf_wearable_weapons.h>

extern ConVar tf_sc_bullrush_speed_mult;
extern ConVar tf_sc_adrenaline_speed_mult;
extern ConVar tf_sc_bullrush_anim_speed;
extern ConVar tf_sc_adrenaline_speed_mult;

class CTFWearableDemoShield;

class CSoldierCommando
{
public:
    CSoldierCommando();
    ~CSoldierCommando();

    void    SetOwner(CTFPlayer* pOwner) { m_hOwner = pOwner; }


    void    Think();

   
    void    OnSpawn();
    void    OnDeath();


    void    ActivateAdrenaline();
    void    ActivateBattlecry();


    void    NoteButtonState(int nButtons, int nOldButtons);

//Rushing VIA shield.
   // bool    InBullRush() const { return m_bBullRush; }

private:
    void    ThinkAdrenaline();
   // void    ThinkBullRush();
    void    ThinkBattlecry();
    void    ThinkBoot();
    void    ThinkChargeRecharge();


    bool    BullRushTouchEnemies();   
    bool    BullRushCheckWall();    

    bool m_bChargeCamActive;
    void ThinkChargeCamera();
    void ClearChargeCamera();


    CHandle<CTFPlayer>  m_hOwner;

    CTFWearableDemoShield* EnsureChargeShield();

    //CHandle<CTFPlayer>              m_hOwner;
    CHandle<CTFWearableDemoShield>  m_hChargeShield;




    bool    m_bWaitSecondTap;
    float   m_flDoubleTapWindowEnd;
    float   m_flNextAdrenalineTime;
    float   m_flNextBattlecryTime;
    float   m_flNextBootTime;
};

extern CSoldierCommando* GetSoldierCommando(CTFPlayer* pPlayer);

#endif
