// botman's Half-Life bot example
//
// http://planethalflife.com/botman/
//
// bot_combat.cpp
//

#include "extdll.h"
#include "util.h"
#include "client.h"
#include "cbase.h"
#include "player.h"
#include "items.h"
#include "effects.h"
#include "weapons.h"
#include "soundent.h"
#include "gamerules.h"
#include "animation.h"

#include "bot.h"


extern int f_Observer;  // flag to indicate if player is in observer mode

ammo_check_t ammo_check[] = {
   {"ammo_glockclip", "9mm", _9MM_MAX_CARRY},
   {"ammo_9mmclip", "9mm", _9MM_MAX_CARRY},
   {"ammo_9mmAR", "9mm", _9MM_MAX_CARRY},
   {"ammo_9mmbox", "9mm", _9MM_MAX_CARRY},
   {"ammo_mp5clip", "9mm", _9MM_MAX_CARRY},
   {"ammo_chainboxclip", "9mm", _9MM_MAX_CARRY},
   {"ammo_mp5grenades", "ARgrenades", M203_GRENADE_MAX_CARRY},
   {"ammo_ARgrenades", "ARgrenades", M203_GRENADE_MAX_CARRY},
   {"ammo_buckshot", "buckshot", BUCKSHOT_MAX_CARRY},
   {"ammo_crossbow", "bolts", BOLT_MAX_CARRY},
   {"ammo_357", "357", _357_MAX_CARRY},
   {"ammo_rpgclip", "rockets", ROCKET_MAX_CARRY},
   {"ammo_egonclip", "uranium", URANIUM_MAX_CARRY},
   {"ammo_gaussclip", "uranium", URANIUM_MAX_CARRY},
   {"", 0, 0}};

// sounds for Bot taunting after a kill...
char barney_taunt[][30] = { BA_TNT1, BA_TNT2, BA_TNT3, BA_TNT4, BA_TNT5 };
char scientist_taunt[][30] = { SC_TNT1, SC_TNT2, SC_TNT3, SC_TNT4, SC_TNT5 };


CBaseEntity * CBot::BotFindEnemy( void )
{
   Vector vecEnd;
   static BOOL flag=TRUE;
   char sound[40];  // for taunting sounds

   if (pBotEnemy != NULL)  // does the bot already have an enemy?
   {
      vecEnd = pBotEnemy->EyePosition();

      // if the enemy is dead or has switched to botcam mode...
      if (!pBotEnemy->IsAlive() || (pBotEnemy->pev->effects & EF_NODRAW))
      {
         if (!pBotEnemy->IsAlive())  // is the enemy dead?, assume bot killed it
         {
            // the enemy is dead, jump for joy about 10% of the time
            if (RANDOM_LONG(1, 100) <= 10)
               pev->button |= IN_JUMP;

            // check if this player is not a bot (i.e. not fake client)...
            if (pBotEnemy->IsNetClient() && !IS_DEDICATED_SERVER())
            {
               // speak taunt sounds about 10% of the time
               if (RANDOM_LONG(1, 100) <= 10)
               {
                  if (bot_model == MODEL_BARNEY)
                     strcpy( sound, barney_taunt[RANDOM_LONG(0,4)] );
                  else if (bot_model == MODEL_SCIENTIST)
                     strcpy( sound, scientist_taunt[RANDOM_LONG(0,4)] );

                  EMIT_SOUND(ENT(pBotEnemy->pev), CHAN_VOICE, sound,
                             RANDOM_FLOAT(0.9, 1.0), ATTN_NORM);
               }
            }
         }

         // don't have an enemy anymore so null out the pointer...
         pBotEnemy = NULL;
      }
      else if (FInViewCone( &vecEnd ) && FVisible( vecEnd ))
      {
         // if enemy is still visible and in field of view, keep it

         // face the enemy
         Vector v_enemy = pBotEnemy->pev->origin - pev->origin;
         Vector bot_angles = UTIL_VecToAngles( v_enemy );

         pev->ideal_yaw = bot_angles.y;

         // check for wrap around of angle...
         if (pev->ideal_yaw > 180)
            pev->ideal_yaw -= 360;
         if (pev->ideal_yaw < -180)
            pev->ideal_yaw += 360;

         return (pBotEnemy);
      }
   }

   int i;
   float nearestdistance = 1000;
   CBaseEntity *pNewEnemy = NULL;

   // search the world for players...
   for (i = 1; i <= gpGlobals->maxClients; i++)
   {
      CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );

      // skip invalid players and skip self (i.e. this bot)
      if ((!pPlayer) || (pPlayer == this))
         continue;

      // skip this player if not alive (i.e. dead or dying)
      if (pPlayer->pev->deadflag != DEAD_NO)
         continue;

      // skip players that are not bots in observer mode...
      if (pPlayer->IsNetClient() && f_Observer)
         continue;

      // skip players that are in botcam mode...
      if (pPlayer->pev->effects & EF_NODRAW)
         continue;

// BigGuy - START
      // is team play enabled?
      if (g_pGameRules->IsTeamplay())
      {
         // don't target your teammates if team names match...
         if (UTIL_TeamsMatch(g_pGameRules->GetTeamID(this),
                             g_pGameRules->GetTeamID(pPlayer)))
            continue;
      }
// BigGuy - END

      vecEnd = pPlayer->EyePosition();

      // see if bot can see the player...
      if (FInViewCone( &vecEnd ) && FVisible( vecEnd ))
      {
         float distance = (pPlayer->pev->origin - pev->origin).Length();
         if (distance < nearestdistance)
         {
            nearestdistance = distance;
            pNewEnemy = pPlayer;

            pBotUser = NULL;  // don't follow user when enemy found
         }
      }
   }

   if (pNewEnemy)
   {
      // face the enemy
      Vector v_enemy = pNewEnemy->pev->origin - pev->origin;
      Vector bot_angles = UTIL_VecToAngles( v_enemy );

      pev->ideal_yaw = bot_angles.y;

      // check for wrap around of angle...
      if (pev->ideal_yaw > 180)
         pev->ideal_yaw -= 360;
      if (pev->ideal_yaw < -180)
         pev->ideal_yaw += 360;
   }

   return (pNewEnemy);
}


Vector CBot::BotBodyTarget( CBaseEntity *pBotEnemy )
{
   Vector target;
   float f_distance;
   float f_scale;
   int d_x, d_y, d_z;

   f_distance = (pBotEnemy->pev->origin - pev->origin).Length();

   if (f_distance > 1000)
      f_scale = 1.0;
   else if (f_distance > 100)
      f_scale = f_distance / 1000.0;
   else
      f_scale = 0.1;

   switch (bot_skill)
   {
      case 0:
         // VERY GOOD, same as from CBasePlayer::BodyTarget (in player.h)
         target = pBotEnemy->Center() + pBotEnemy->pev->view_ofs * RANDOM_FLOAT( 0.5, 1.1 );
         d_x = 0;  // no offset
         d_y = 0;
         d_z = 0;
         break;
      case 1:
         // GOOD, offset a little for x, y, and z
         target = pBotEnemy->Center() + pBotEnemy->pev->view_ofs;
         d_x = RANDOM_FLOAT(-5, 5) * f_scale;
         d_y = RANDOM_FLOAT(-5, 5) * f_scale;
         d_z = RANDOM_FLOAT(-9, 9) * f_scale;
         break;
      case 2:
         // FAIR, offset somewhat for x, y, and z
         target = pBotEnemy->Center() + pBotEnemy->pev->view_ofs;
         d_x = RANDOM_FLOAT(-9, 9) * f_scale;
         d_y = RANDOM_FLOAT(-9, 9) * f_scale;
         d_z = RANDOM_FLOAT(-15, 15) * f_scale;
         break;
      case 3:
         // POOR, offset for x, y, and z
         target = pBotEnemy->Center() + pBotEnemy->pev->view_ofs;
         d_x = RANDOM_FLOAT(-16, 16) * f_scale;
         d_y = RANDOM_FLOAT(-16, 16) * f_scale;
         d_z = RANDOM_FLOAT(-20, 20) * f_scale;
         break;
      case 4:
         // BAD, offset lots for x, y, and z
         target = pBotEnemy->Center() + pBotEnemy->pev->view_ofs;
         d_x = RANDOM_FLOAT(-20, 20) * f_scale;
         d_y = RANDOM_FLOAT(-20, 20) * f_scale;
         d_z = RANDOM_FLOAT(-27, 27) * f_scale;
         break;
   }

   target = target + Vector(d_x, d_y, d_z);

   return target;
}


void CBot::BotWeaponInventory( void )
{
   int i;

   // initialize the elements of the weapons arrays...
   for (i = 0; i < MAX_WEAPONS; i++)
   {
      weapon_ptr[i] = NULL;
      primary_ammo[i] = 0;
      secondary_ammo[i] = 0;
   }

   // find out which weapons the bot is carrying...
   for (i = 0; i < MAX_ITEM_TYPES; i++)
   {
      CBasePlayerItem *pItem = NULL;

      if (m_rgpPlayerItems[i])
      {
         pItem = m_rgpPlayerItems[i];
         while (pItem)
         {
            weapon_ptr[pItem->m_iId] = pItem;  // store pointer to item

            pItem = pItem->m_pNext;
         }
      }
   }

   // find out how much ammo of each type the bot is carrying...
   for (i = 0; i < MAX_AMMO_SLOTS; i++)
   {
      if (!CBasePlayerItem::AmmoInfoArray[i].pszName)
         continue;

      if (strcmp("9mm", CBasePlayerItem::AmmoInfoArray[i].pszName) == 0)
      {
         primary_ammo[WEAPON_GLOCK] = m_rgAmmo[i];
         primary_ammo[WEAPON_MP5] = m_rgAmmo[i];
      }
      else if (strcmp("357", CBasePlayerItem::AmmoInfoArray[i].pszName) == 0)
      {
         primary_ammo[WEAPON_PYTHON] = m_rgAmmo[i];
      }
      else if (strcmp("ARgrenades", CBasePlayerItem::AmmoInfoArray[i].pszName) == 0)
         secondary_ammo[WEAPON_MP5] = m_rgAmmo[i];
      else if (strcmp("bolts", CBasePlayerItem::AmmoInfoArray[i].pszName) == 0)
      {
         primary_ammo[WEAPON_CROSSBOW] = m_rgAmmo[i];
      }
      else if (stricmp("buckshot", CBasePlayerItem::AmmoInfoArray[i].pszName) == 0)
      {
         primary_ammo[WEAPON_SHOTGUN] = m_rgAmmo[i];
      }
      else if (stricmp("rockets", CBasePlayerItem::AmmoInfoArray[i].pszName) == 0)
         primary_ammo[WEAPON_RPG] = m_rgAmmo[i];
      else if (strcmp("uranium", CBasePlayerItem::AmmoInfoArray[i].pszName) == 0)
      {
         primary_ammo[WEAPON_GAUSS] = m_rgAmmo[i];
         primary_ammo[WEAPON_EGON] = m_rgAmmo[i];
      }
      else if (stricmp("Hornets", CBasePlayerItem::AmmoInfoArray[i].pszName) == 0)
         primary_ammo[WEAPON_HORNETGUN] = m_rgAmmo[i];
      else if (stricmp("Hand Grenade", CBasePlayerItem::AmmoInfoArray[i].pszName) == 0)
         primary_ammo[WEAPON_HANDGRENADE] = m_rgAmmo[i];
      else if (stricmp("Trip Mine", CBasePlayerItem::AmmoInfoArray[i].pszName) == 0)
         primary_ammo[WEAPON_TRIPMINE] = m_rgAmmo[i];
      else if (stricmp("Satchel Charge", CBasePlayerItem::AmmoInfoArray[i].pszName) == 0)
         primary_ammo[WEAPON_SATCHEL] = m_rgAmmo[i];
      else if (stricmp("Snarks", CBasePlayerItem::AmmoInfoArray[i].pszName) == 0)
         primary_ammo[WEAPON_SNARK] = m_rgAmmo[i];
   }

}


// specifing a weapon_choice allows you to choose the weapon the bot will
// use (assuming enough ammo exists for that weapon)
// BotFireWeapon will return TRUE if weapon was fired, FALSE otherwise
// primary is used to indicate whether you want primary or secondary fire
// if you have specified a weapon using weapon_choice

BOOL CBot::BotFireWeapon( Vector v_enemy_origin, int weapon_choice, BOOL primary )
{
   CBasePlayerItem *new_weapon;
   BOOL enemy_below;
   int best_weapon = WEAPON_NONE;
   int best_weight = -9999;

   #define CONSIDER_WEAPON(weapon_id) \
      do \
      { \
         int weight = CBasePlayerItem::ItemInfoArray[(weapon_id)].iWeight; \
         if (weight > best_weight) \
         { \
            best_weight = weight; \
            best_weapon = (weapon_id); \
         } \
      } while (0)

   // is it time to check weapons inventory yet?
   if (f_weapon_inventory_time <= gpGlobals->time)
   {
      // check weapon and ammo inventory then update check time...
      BotWeaponInventory();
      f_weapon_inventory_time = gpGlobals->time + 1.0;
   }

   Vector v_enemy = v_enemy_origin - GetGunPosition( );

   float distance = v_enemy.Length();  // how far away is the enemy?

   // is enemy at least 45 units below bot? (for handgrenades and snarks)
   if (v_enemy_origin.z < (pev->origin.z - 45))
      enemy_below = TRUE;
   else
      enemy_below = FALSE;

   if (weapon_choice == 0)
   {
      if ((pev->weapons & (1<<WEAPON_CROWBAR)) && (distance <= 40))
         CONSIDER_WEAPON(WEAPON_CROWBAR);

      if ((pev->weapons & (1<<WEAPON_HANDGRENADE)) && enemy_below &&
          (distance > 250) && (distance < 750))
         CONSIDER_WEAPON(WEAPON_HANDGRENADE);

      if ((pev->weapons & (1<<WEAPON_SNARK)) && (pev->waterlevel != 3) &&
          enemy_below && (distance > 150) && (distance < 500))
         CONSIDER_WEAPON(WEAPON_SNARK);

      if ((pev->weapons & (1<<WEAPON_EGON)) && (pev->waterlevel != 3) &&
          (primary_ammo[WEAPON_EGON] > 0))
         CONSIDER_WEAPON(WEAPON_EGON);

      if ((pev->weapons & (1<<WEAPON_GAUSS)) && (pev->waterlevel != 3) &&
          (primary_ammo[WEAPON_GAUSS] > 1))
         CONSIDER_WEAPON(WEAPON_GAUSS);

      if ((pev->weapons & (1<<WEAPON_SHOTGUN)) && (pev->waterlevel != 3) &&
          (distance < 200) && (primary_ammo[WEAPON_SHOTGUN] > 0))
         CONSIDER_WEAPON(WEAPON_SHOTGUN);

      if ((pev->weapons & (1<<WEAPON_PYTHON)) && (pev->waterlevel != 3) &&
          (distance > 30) && (distance < 700) && (primary_ammo[WEAPON_PYTHON] > 0))
         CONSIDER_WEAPON(WEAPON_PYTHON);

      if ((pev->weapons & (1<<WEAPON_HORNETGUN)) && (distance > 30) &&
          (distance < 1000) && (primary_ammo[WEAPON_HORNETGUN] > 0))
         CONSIDER_WEAPON(WEAPON_HORNETGUN);

      if ((pev->weapons & (1<<WEAPON_MP5)) && (pev->waterlevel != 3) &&
          (distance > 200) && (primary_ammo[WEAPON_MP5] > 0))
         CONSIDER_WEAPON(WEAPON_MP5);

      if ((pev->weapons & (1<<WEAPON_CROSSBOW)) && (distance > 100) &&
          (distance < 1000) && (primary_ammo[WEAPON_CROSSBOW] > 0))
         CONSIDER_WEAPON(WEAPON_CROSSBOW);

      if ((pev->weapons & (1<<WEAPON_RPG)) && (distance > 300) &&
          (primary_ammo[WEAPON_RPG] > 0))
         CONSIDER_WEAPON(WEAPON_RPG);

      if ((pev->weapons & (1<<WEAPON_GLOCK)) && (distance < 1200) &&
          (primary_ammo[WEAPON_GLOCK] > 0))
         CONSIDER_WEAPON(WEAPON_GLOCK);

      if (best_weapon != WEAPON_NONE)
         weapon_choice = best_weapon;
   }

   // if bot is carrying the crowbar...
   if (pev->weapons & (1<<WEAPON_CROWBAR))
   {
      // if close to enemy, use the crowbar
      if (((distance <= 120) && (weapon_choice == 0)) ||
          (weapon_choice == WEAPON_CROWBAR))
      {
         new_weapon = weapon_ptr[WEAPON_CROWBAR];

         // check if the bot isn't already using this item...
         if (m_pActiveItem != new_weapon)
            SelectItem("weapon_crowbar");  // select the crowbar

         if (distance <= 40)
         {
            pev->button |= IN_ATTACK;  // use primary attack (whack! whack!)
         }
         else
         {
            pev->button |= IN_ATTACK2;  // throw crowbar
         }

         // set next time to "shoot"
         f_shoot_time = 0;
         return TRUE;
      }
   }

   // if bot is carrying any hand grenades and enemy is below bot...
   if ((pev->weapons & (1<<WEAPON_HANDGRENADE)) && (enemy_below))
   {
      long use_grenade = RANDOM_LONG(1,100);

      // use hand grenades about 30% of the time...
      if (((distance > 250) && (distance < 750) &&
           (weapon_choice == 0) && (use_grenade <= 30)) ||
          (weapon_choice == WEAPON_HANDGRENADE))
      {
// BigGuy - START
         new_weapon = weapon_ptr[WEAPON_HANDGRENADE];

         // check if the bot isn't already using this item...
         if (m_pActiveItem != new_weapon)
            SelectItem("weapon_handgrenade");  // select the hand grenades

         pev->button |= IN_ATTACK;  // use primary attack (boom!)

         // set next time to "shoot"
         f_shoot_time = 0;
         return TRUE;
// BigGuy - END
      }
   }

   // if bot is carrying any snarks (can't use underwater) and enemy is below bot...
   if ((pev->weapons & (1<<WEAPON_SNARK)) && (pev->waterlevel != 3) &&
       (enemy_below))
   {
      long use_snark = RANDOM_LONG(1,100);

      // use snarks about 50% of the time...
      if (((distance > 150) && (distance < 500) &&
           (weapon_choice == 0) && (use_snark <= 50)) ||
          (weapon_choice == WEAPON_SNARK))
      {
// BigGuy - START
         new_weapon = weapon_ptr[WEAPON_SNARK];

         // check if the bot isn't already using this item...
         if (m_pActiveItem != new_weapon)
            SelectItem("weapon_snark");  // select the "squeak grenades"

         pev->button |= IN_ATTACK;  // use primary attack (eek! eek!)

         // set next time to "shoot"
         f_shoot_time = 0;
         return TRUE;
// BigGuy - END
      }
   }

   // if the bot is carrying the egon gun (can't use underwater)...
   if ((pev->weapons & (1<<WEAPON_EGON)) && (pev->waterlevel != 3))
   {
      if ((weapon_choice == 0) || (weapon_choice == WEAPON_EGON))
      {
         new_weapon = weapon_ptr[WEAPON_EGON];

         // check if the bot has any ammo left for this weapon...
         if (primary_ammo[WEAPON_EGON] > 0)
         {
            // check if the bot isn't already using this item...
            if (m_pActiveItem != new_weapon)
               SelectItem("weapon_egon");  // select the egon gun

            pev->button |= IN_ATTACK;  // use primary attack (bang! bang!)

            // set next time to shoot
            f_shoot_time = 0;

            return TRUE;
         }
      }
   }

   // if the bot is carrying the gauss gun (can't use underwater)...
   if ((pev->weapons & (1<<WEAPON_GAUSS)) && (pev->waterlevel != 3))
   {
      if ((weapon_choice == 0) || (weapon_choice == WEAPON_GAUSS))
      {
         new_weapon = weapon_ptr[WEAPON_GAUSS];

         // check if the bot has any ammo left for this weapon...
         if (primary_ammo[WEAPON_GAUSS] > 1)
         {
            // check if the bot isn't already using this item...
            if (m_pActiveItem != new_weapon)
               SelectItem("weapon_gauss");  // select the gauss gun

            pev->button |= IN_ATTACK;  // use primary attack (bang! bang!)

            // set next time to shoot
            f_shoot_time = 0;

            return TRUE;
         }
      }
   }

   // if the bot is carrying the shotgun (can't use underwater)...
   if ((pev->weapons & (1<<WEAPON_SHOTGUN)) && (pev->waterlevel != 3))
   {
      // if close enough for good shotgun blasts...
      if (((distance > 20) && (distance < 200) && (weapon_choice == 0)) ||
          (weapon_choice == WEAPON_SHOTGUN))
      {
         new_weapon = weapon_ptr[WEAPON_SHOTGUN];

         // check if the bot has any ammo left for this weapon...
         if (primary_ammo[WEAPON_SHOTGUN] > 0)
         {
            // check if the bot isn't already using this item...
            if (m_pActiveItem != new_weapon)
               SelectItem("weapon_shotgun");  // select the shotgun

            long use_secondary = RANDOM_LONG(1,100);

            // use secondary attack about 30% of the time
            if ((distance > 40) && (primary_ammo[WEAPON_SHOTGUN] >= 2))
            {
// BigGuy - START
               pev->button |= IN_ATTACK2;  // use secondary attack (bang! bang!)

               // set next time to shoot
               f_shoot_time = 0;
            }
// BigGuy - END
            else
            {
               pev->button |= IN_ATTACK;  // use primary attack (bang! bang!)

               // set next time to shoot
               f_shoot_time = 0;
            }

            return TRUE;
         }
      }
   }

   // if the bot is carrying the 357/PYTHON, (can't use underwater)...
   if ((pev->weapons & (1<<WEAPON_PYTHON)) && (pev->waterlevel != 3))
   {
      // if close enough for 357 shot...
      if (((distance > 80) && (distance < 700) && (weapon_choice == 0)) ||
          (weapon_choice == WEAPON_PYTHON))
      {
         new_weapon = weapon_ptr[WEAPON_PYTHON];

         // check if the bot has any ammo left for this weapon...
         if (primary_ammo[WEAPON_PYTHON] > 0)
         {
            // check if the bot isn't already using this item...
            if (m_pActiveItem != new_weapon)
               SelectItem("weapon_357");  // select the 357 python

            pev->button |= IN_ATTACK;  // use primary attack (bang! bang!)

            // set next time to shoot
            f_shoot_time = 0;
            return TRUE;
         }
      }
   }

   // if the bot is carrying the hornet gun...
   if (pev->weapons & (1<<WEAPON_HORNETGUN))
   {
      // if close enough for hornet gun...
      if (((distance > 30) && (distance < 1000) && (weapon_choice == 0)) ||
          (weapon_choice == WEAPON_HORNETGUN))
      {
         new_weapon = weapon_ptr[WEAPON_HORNETGUN];

         // check if the bot has any ammo left for this weapon...
         if (primary_ammo[WEAPON_HORNETGUN] > 0)
         {
            // check if the bot isn't already using this item...
            if (m_pActiveItem != new_weapon)
               SelectItem("weapon_hornetgun");  // select the hornet gun

            long use_secondary = RANDOM_LONG(1,100);

            // use secondary attack about 50% of the time (if fully reloaded)
            if ((use_secondary <= 50) &&
                (primary_ammo[WEAPON_HORNETGUN] >= HORNET_MAX_CARRY))
            {
// BigGuy - START
               pev->button |= IN_ATTACK2;  // use secondary attack (buzz! buzz!)

               // set next time to shoot
               f_shoot_time = 0;
// BigGuy - END
            }
            else
            {
               pev->button |= IN_ATTACK;  // use primary attack (buzz! buzz!)

               f_shoot_time = 0;
            }

            return TRUE;
         }
      }
   }

   // if the bot is carrying the MP5 (can't use underwater)...
   if ((pev->weapons & (1<<WEAPON_MP5)) && (pev->waterlevel != 3))
   {
      // if close enough for good MP5 shot...
      if (((distance < 250) && (weapon_choice == 0)) ||
          (weapon_choice == WEAPON_MP5))
      {
         new_weapon = weapon_ptr[WEAPON_MP5];

         // check if the bot has any ammo left for this weapon...
         if (primary_ammo[WEAPON_MP5] > 0)
         {
            // check if the bot isn't already using this item...
            if (m_pActiveItem != new_weapon)
               SelectItem("weapon_9mmAR");  // select the 9mmAR (MP5)

            pev->button |= IN_ATTACK;  // use primary attack (bang! bang!)

            // set next time to shoot
            f_shoot_time = 0;
            return TRUE;
         }
      }
   }

   // if the bot is carrying the crossbow...
   if (pev->weapons & (1<<WEAPON_CROSSBOW))
   {
      if (bot_skill < 3)
         pev->button |= IN_ATTACK2;  // use secondary attack to zoom

      // if bot is not too close for crossbow...
      if (((distance > 250)) && (weapon_choice == 0))
          (weapon_choice == WEAPON_CROSSBOW);
      {
         new_weapon = weapon_ptr[WEAPON_CROSSBOW];

         // check if the bot has any ammo left for this weapon...
         if (primary_ammo[WEAPON_CROSSBOW] > 0)
         {
            // check if the bot isn't already using this item...
            if (m_pActiveItem != new_weapon)
               SelectItem("weapon_crossbow");  // select the crossbow

            pev->button |= IN_ATTACK;  // use primary attack (bang! bang!)

            // set next time to shoot
            f_shoot_time = 0;
            return TRUE;
         }
      }
   }

   // if the bot is carrying the RPG...
   if (pev->weapons & (1<<WEAPON_RPG))
   {
      // don't use the RPG unless the enemy is pretty far away...
      if (((distance > 300) && (weapon_choice == 0)) ||
          (weapon_choice == WEAPON_RPG))
      {
         new_weapon = weapon_ptr[WEAPON_RPG];

         // check if the bot has any ammo left for this weapon...
         if (primary_ammo[WEAPON_RPG] > 0)
         {
            // check if the bot isn't already using this item...
            if (m_pActiveItem != new_weapon)
               SelectItem("weapon_rpg");  // select the RPG rocket launcher

            pev->button |= IN_ATTACK;  // use primary attack (bang! bang!)

            // set next time to shoot
            f_shoot_time = 0;
            return TRUE;
         }
      }
   }

   // if the bot is carrying the 9mm glock...
   if (pev->weapons & (1<<WEAPON_GLOCK))
   {
      // if nothing else was selected, try the good ol' 9mm glock...
      if (((distance < 1200) && (weapon_choice == 0)) ||
          (weapon_choice == WEAPON_GLOCK))
      {
         new_weapon = weapon_ptr[WEAPON_GLOCK];

         // check if the bot has any ammo left for this weapon...
         if (primary_ammo[WEAPON_GLOCK] > 0)
         {
            // check if the bot isn't already using this item...
            if (m_pActiveItem != new_weapon)
               SelectItem("weapon_9mmhandgun");  // select the trusty 9mm glock

            long use_secondary = RANDOM_LONG(1,100);

            // use secondary attack about 30% of the time
            if (use_secondary <= 30)
            {
// BigGuy - START
               pev->button |= IN_ATTACK2;  // use secondary attack (bang! bang!)

               // set next time to shoot
               f_shoot_time = 0;
// BigGuy - END
            }
            else
            {
               pev->button |= IN_ATTACK;  // use primary attack (bang! bang!)

               // set next time to shoot
               f_shoot_time = 0;
            }

            return TRUE;
         }
      }
   }

   // didn't have any available weapons or ammo, return FALSE
   return FALSE;
}


void CBot::BotShootAtEnemy( void )
{
   float f_distance;

   // aim for the head and/or body
   Vector v_enemy = BotBodyTarget( pBotEnemy ) - GetGunPosition();

   Vector target_angles = UTIL_VecToAngles( v_enemy );

   pev->ideal_yaw = target_angles.y;
   pev->idealpitch = target_angles.x;

   // check for wrap around of angle...
   if (pev->ideal_yaw > 180)
      pev->ideal_yaw -= 360;
   if (pev->ideal_yaw < -180)
      pev->ideal_yaw += 360;

   target_angles.x = -target_angles.x;  // pitch

   float pitch_delta = target_angles.x - pev->v_angle.x;
   while (pitch_delta > 180.0f)  pitch_delta -= 360.0f;
   while (pitch_delta < -180.0f) pitch_delta += 360.0f;

   float max_pitch_step = 20.0f;
   if (pitch_delta > max_pitch_step)  pitch_delta = max_pitch_step;
   if (pitch_delta < -max_pitch_step) pitch_delta = -max_pitch_step;

   pev->v_angle.x += pitch_delta;

   pev->v_angle.x += pitch_delta;

   float yaw_delta = target_angles.y - pev->v_angle.y;
   while (yaw_delta > 180.0f)  yaw_delta -= 360.0f;
   while (yaw_delta < -180.0f) yaw_delta += 360.0f;

   float max_yaw_step   = 10.0f;
   if (yaw_delta > max_yaw_step)  yaw_delta = max_yaw_step;
   if (yaw_delta < -max_yaw_step) yaw_delta = -max_yaw_step;

   pev->v_angle.x += pitch_delta;
   pev->v_angle.y += yaw_delta;

   float shake_yaw = 0.0f;
   if (bot_skill == 3) shake_yaw = 2.0f;
   else if (bot_skill == 2) shake_yaw = 1.0f;
   else if (bot_skill == 1) shake_yaw = 0.3f;
   else if (bot_skill == 4) shake_yaw = 3.0f;
   else shake_yaw = 5.0f;

   pev->ideal_yaw += RANDOM_FLOAT(-shake_yaw, shake_yaw);

   float shake_pitch = 0.0f;
   if (bot_skill == 3) shake_pitch = 0.3f;
   else if (bot_skill == 2) shake_pitch = 0.2f;
   else if (bot_skill == 1) shake_pitch = 0.1f;
   else if (bot_skill == 4) shake_pitch = 0.5f;
   else shake_pitch = 1.0f;

   pev->idealpitch += RANDOM_FLOAT(-shake_pitch, shake_pitch);

   // is it time to shoot yet?
   if (f_shoot_time <= gpGlobals->time)
   {
      // select the best weapon to use at this distance and fire...
      BotFireWeapon( pBotEnemy->pev->origin );
   }

   v_enemy.z = 0;  // ignore z component (up & down)

   f_distance = v_enemy.Length();  // how far away is the enemy scum?

   if (f_distance > 200)      // run if distance to enemy is far
      f_move_speed = f_max_speed;
   else if (f_distance > 20)  // walk if distance is closer
      f_move_speed = f_max_speed / 2;
   else                     // don't move if close enough
      f_move_speed = 0.0;
}


