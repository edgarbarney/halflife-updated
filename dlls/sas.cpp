// Thanks to Solokiller's OP4 for ally dudes
//
// Why didn't I do it myself?
// Lazyness.

//=========================================================
// Hit groups!	
//=========================================================
/*

  1 - Head
  2 - Stomach
  3 - Gun

*/


#include	"extdll.h"
#include	"plane.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"defaultai.h"
#include	"animation.h"
#include	"squadmonster.h"
#include	"weapons.h"
#include	"talkmonster.h"
#include	"COFAllyMonster.h"
#include	"COFSquadTalkMonster.h"
#include	"soundent.h"
#include	"effects.h"
#include	"customentity.h"

int g_fGruntAllyQuestion;				// true if an idle grunt asked a question. Cleared when someone answers.

extern DLL_GLOBAL int		g_iSkillLevel;

//=========================================================
// monster-specific DEFINE's
//=========================================================
#define	SAS_MP5_CLIP_SIZE				36 // how many bullets in a clip? - NOTE: 3 round burst sound, so keep as 3 * x!
#define SAS_SHOTGUN_CLIP_SIZE			8
#define SAS_AR16_CLIP_SIZE				36
#define SAS_VOL						0.35		// volume of grunt sounds
#define SAS_ATTN						ATTN_NORM	// attenutation of grunt sentences
#define HSAS_LIMP_HEALTH				20
#define HSAS_DMG_HEADSHOT				( DMG_BULLET | DMG_CLUB )	// damage types that can kill a grunt with a single headshot.
#define HSAS_NUM_HEADS				2 // how many grunt heads are there? 
#define HSAS_MINIMUM_HEADSHOT_DAMAGE	15 // must do at least this much damage in one shot to head to score a headshot kill
#define	HSAS_SENTENCE_VOLUME			(float)0.35 // volume of grunt sentences

namespace SASWeaponFlag
{
	enum SASWeaponFlag
	{
		MP5 = 1 << 0,
		HandGrenade = 1 << 1,
		GrenadeLauncher = 1 << 2,
		Shotgun = 1 << 3,
		AR16 = 1 << 4
	};
}

namespace SASBodygroup
{
	enum SASBodygroup
	{
		Head = 1,
		Torso = 2,
		Weapons = 3
	};
}

namespace SASHead
{
	enum SASHead
	{
		Default = -1,
		GasMask = 0,
		Leader
	};
}

namespace SASSkin
{
	enum SASSkin
	{
		Default = -1,
		WhiteBrown = 0,
		WhiteBlu,
		ScotBrown,
		ScotBlu,
		LeaderWhite,
		LeaderScot,
		SKINCOUNT_SOLDIERS = 3, // Total heads (Leaders Excluded);
		SKINCOUNT_LEADERS = 1, // Total heads (Leaders Only);
	};
}

namespace SASTorso
{
	enum SASTorso
	{
		Normal = 0,
		AR16,
		Nothing,
		Shotgun
	};
}

namespace SASWeapon
{
	enum SASWeapon
	{
		MP5 = 0,
		Shotgun,
		AR16,
		None
	};
}

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		HSAS_AE_RELOAD		( 2 )
#define		HSAS_AE_KICK			( 3 )
#define		HSAS_AE_BURST1		( 4 )
#define		HSAS_AE_BURST2		( 5 ) 
#define		HSAS_AE_BURST3		( 6 ) 
#define		HSAS_AE_GREN_TOSS		( 7 )
#define		HSAS_AE_GREN_LAUNCH	( 8 )
#define		HSAS_AE_GREN_DROP		( 9 )
#define		HSAS_AE_CAUGHT_ENEMY	( 10) // grunt established sight with an enemy (player only) that had previously eluded the squad.
#define		HSAS_AE_DROP_GUN		( 11) // grunt (probably dead) is dropping his mp5.

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_SAS_SUPPRESS = LAST_TALKMONSTER_SCHEDULE + 1,
	SCHED_SAS_ESTABLISH_LINE_OF_FIRE,// move to a location to set up an attack against the enemy. (usually when a friendly is in the way).
	SCHED_SAS_COVER_AND_RELOAD,
	SCHED_SAS_SWEEP,
	SCHED_SAS_FOUND_ENEMY,
	SCHED_SAS_REPEL,
	SCHED_SAS_REPEL_ATTACK,
	SCHED_SAS_REPEL_LAND,
	SCHED_SAS_WAIT_FACE_ENEMY,
	SCHED_SAS_TAKECOVER_FAILED,// special schedule type that forces analysis of conditions and picks the best possible schedule to recover from this type of failure.
	SCHED_SAS_ELOF_FAIL,
};

//=========================================================
// monster-specific tasks
//=========================================================
enum
{
	TASK_SAS_FACE_TOSS_DIR = LAST_TALKMONSTER_TASK + 1,
	TASK_SAS_SPEAK_SENTENCE,
	TASK_SAS_CHECK_FIRE,
};

//=========================================================
// monster-specific conditions
//=========================================================
#define bits_COND_SAS_NOFIRE	( bits_COND_SPECIAL1 )

class CSAS : public COFSquadTalkMonster
{
public:
	void Spawn(void);
	void Precache(void);
	void SetYawSpeed(void);
	int  Classify(void);
	int ISoundMask(void);
	void HandleAnimEvent(MonsterEvent_t* pEvent);
	BOOL FCanCheckAttacks(void);
	BOOL CheckMeleeAttack1(float flDot, float flDist);
	BOOL CheckRangeAttack1(float flDot, float flDist);
	BOOL CheckRangeAttack2(float flDot, float flDist);
	void CheckAmmo(void);
	void SetActivity(Activity NewActivity);
	void StartTask(Task_t* pTask);
	void RunTask(Task_t* pTask);
	void DeathSound(void);
	void PainSound(void);
	void IdleSound(void);
	Vector GetGunPosition(void);
	void Shoot(void);
	void Shotgun(void);
	void PrescheduleThink(void);
	void GibMonster(void);
	void SpeakSentence(void);

	int	Save(CSave& save);
	int Restore(CRestore& restore);

	CBaseEntity* Kick(void);
	Schedule_t* GetSchedule(void);
	Schedule_t* GetScheduleOfType(int Type);
	void TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType);
	int TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);

	BOOL FOkToSpeak(void);
	void JustSpoke(void);

	int GetCount();

	int ObjectCaps() override;

	void TalkInit();

	void AlertSound() override;

	void DeclineFollowing() override;

	void ShootAR16();

	void KeyValue(KeyValueData* pkvd) override;

	void Killed(entvars_t* pevAttacker, int iGib) override;

	MONSTERSTATE GetIdealState()
	{
		return COFSquadTalkMonster::GetIdealState();
	}

	CUSTOM_SCHEDULES;
	static TYPEDESCRIPTION m_SaveData[];

	BOOL m_lastAttackCheck;
	float m_flPlayerDamage;

	// checking the feasibility of a grenade toss is kind of costly, so we do it every couple of seconds,
	// not every server frame.
	float m_flNextGrenadeCheck;
	float m_flNextPainTime;
	float m_flLastEnemySightTime;

	Vector m_vecTossVelocity;

	BOOL m_fThrowGrenade;
	BOOL m_fStanding;
	BOOL m_fFirstEncounter;// only put on the handsign show in the squad's first encounter.
	int m_cClipSize;

	int m_iBrassShell;
	int m_iShotgunShell;
	int m_iAR16Shell;
	int m_iAR16Link;

	int m_iSentence;

	int m_iWeaponIdx;
	int m_iGruntHead;
	int m_iGruntTorso;

	static const char* pGruntSentences[];
};

LINK_ENTITY_TO_CLASS(monster_sas, CSAS);

TYPEDESCRIPTION	CSAS::m_SaveData[] =
{
	DEFINE_FIELD(CSAS, m_flPlayerDamage, FIELD_FLOAT),
	DEFINE_FIELD(CSAS, m_flNextGrenadeCheck, FIELD_TIME),
	DEFINE_FIELD(CSAS, m_flNextPainTime, FIELD_TIME),
	//	DEFINE_FIELD( CSAS, m_flLastEnemySightTime, FIELD_TIME ), // don't save, go to zero
	DEFINE_FIELD(CSAS, m_vecTossVelocity, FIELD_VECTOR),
	DEFINE_FIELD(CSAS, m_fThrowGrenade, FIELD_BOOLEAN),
	DEFINE_FIELD(CSAS, m_fStanding, FIELD_BOOLEAN),
	DEFINE_FIELD(CSAS, m_fFirstEncounter, FIELD_BOOLEAN),
	DEFINE_FIELD(CSAS, m_cClipSize, FIELD_INTEGER),
	//	DEFINE_FIELD( CSAS, m_voicePitch, FIELD_INTEGER ),
	//  DEFINE_FIELD( CShotgun, m_iBrassShell, FIELD_INTEGER ),
	//  DEFINE_FIELD( CShotgun, m_iShotgunShell, FIELD_INTEGER ),
	DEFINE_FIELD(CSAS, m_iSentence, FIELD_INTEGER),
	DEFINE_FIELD(CSAS, m_iWeaponIdx, FIELD_INTEGER),
	DEFINE_FIELD(CSAS, m_iGruntHead, FIELD_INTEGER),
	DEFINE_FIELD(CSAS, m_iGruntTorso, FIELD_INTEGER),
	DEFINE_FIELD(CSAS, m_deadMates, FIELD_INTEGER),
	DEFINE_FIELD(CSAS, m_canSayUsLeft, FIELD_INTEGER),
	DEFINE_FIELD(CSAS, m_canLoseSquad, FIELD_BOOLEAN),
};

IMPLEMENT_SAVERESTORE(CSAS, COFSquadTalkMonster);

const char* CSAS::pGruntSentences[] =
{
	"SAS_GREN", // grenade scared grunt
	"SAS_ALERT", // sees player
	"SAS_MONSTER", // sees monster
	"SAS_COVER", // running to cover
	"SAS_THROW", // about to throw grenade
	"SAS_CHARGE",  // running out to get the enemy
	"SAS_TAUNT", // say rude things
};

enum
{
	HSAS_SENT_NONE = -1,
	HSAS_SENT_GREN = 0,
	HSAS_SENT_ALERT,
	HSAS_SENT_MONSTER,
	HSAS_SENT_COVER,
	HSAS_SENT_THROW,
	HSAS_SENT_CHARGE,
	HSAS_SENT_TAUNT,
} HSAS_ALLY_SENTENCE_TYPES;

//=========================================================
// Speak Sentence - say your cued up sentence.
//
// Some grunt sentences (take cover and charge) rely on actually
// being able to execute the intended action. It's really lame
// when a grunt says 'COVER ME' and then doesn't move. The problem
// is that the sentences were played when the decision to TRY
// to move to cover was made. Now the sentence is played after 
// we know for sure that there is a valid path. The schedule
// may still fail but in most cases, well after the grunt has 
// started moving.
//=========================================================
void CSAS::SpeakSentence(void)
{
	if (m_iSentence == HSAS_SENT_NONE)
	{
		// no sentence cued up.
		return;
	}

	if (FOkToSpeak())
	{
		SENTENCEG_PlayRndSz(ENT(pev), pGruntSentences[m_iSentence], HSAS_SENTENCE_VOLUME, SAS_ATTN, 0, m_voicePitch);
		JustSpoke();
	}
}

//=========================================================
// GibMonster - make gun fly through the air.
//=========================================================
void CSAS::GibMonster(void)
{
	if (m_hWaitMedic)
	{
		auto pMedic = m_hWaitMedic.Entity<COFSquadTalkMonster>();

		if (pMedic->pev->deadflag != DEAD_NO)
			m_hWaitMedic = nullptr;
		else
			pMedic->HealMe(nullptr);
	}

	Vector	vecGunPos;
	Vector	vecGunAngles;

	if (m_iWeaponIdx != SASWeapon::None)
	{// throw a gun if the grunt has one
		GetAttachment(0, vecGunPos, vecGunAngles);

		CBaseEntity* pGun;
		if (FBitSet(pev->weapons, SASWeaponFlag::Shotgun))
		{
			pGun = DropItem("weapon_shotgun", vecGunPos, vecGunAngles);
		}
		else if (FBitSet(pev->weapons, SASWeaponFlag::AR16))
		{
			pGun = DropItem("weapon_ar16", vecGunPos, vecGunAngles);
		}
		else
		{
			pGun = DropItem("weapon_9mmAR", vecGunPos, vecGunAngles);
		}
		if (pGun)
		{
			pGun->pev->velocity = Vector(RANDOM_FLOAT(-100, 100), RANDOM_FLOAT(-100, 100), RANDOM_FLOAT(200, 300));
			pGun->pev->avelocity = Vector(0, RANDOM_FLOAT(200, 400), 0);
		}

		if (FBitSet(pev->weapons, SASWeaponFlag::GrenadeLauncher))
		{
			pGun = DropItem("ammo_ARgrenades", vecGunPos, vecGunAngles);
			if (pGun)
			{
				pGun->pev->velocity = Vector(RANDOM_FLOAT(-100, 100), RANDOM_FLOAT(-100, 100), RANDOM_FLOAT(200, 300));
				pGun->pev->avelocity = Vector(0, RANDOM_FLOAT(200, 400), 0);
			}
		}

		m_iWeaponIdx = SASWeapon::None;
	}

	CBaseMonster::GibMonster();
}

//=========================================================
// ISoundMask - Overidden for human grunts because they 
// hear the DANGER sound that is made by hand grenades and
// other dangerous items.
//=========================================================
int CSAS::ISoundMask(void)
{
	return	bits_SOUND_WORLD |
		bits_SOUND_COMBAT |
		bits_SOUND_PLAYER |
		bits_SOUND_DANGER |
		bits_SOUND_CARCASS |
		bits_SOUND_MEAT |
		bits_SOUND_GARBAGE;
}

//=========================================================
// someone else is talking - don't speak
//=========================================================
BOOL CSAS::FOkToSpeak(void)
{
	// if someone else is talking, don't speak
	if (gpGlobals->time <= COFSquadTalkMonster::g_talkWaitTime)
		return FALSE;

	if (pev->spawnflags & SF_MONSTER_GAG)
	{
		if (m_MonsterState != MONSTERSTATE_COMBAT)
		{
			// no talking outside of combat if gagged.
			return FALSE;
		}
	}

	// if player is not in pvs, don't speak
//	if (FNullEnt(FIND_CLIENT_IN_PVS(edict())))
//		return FALSE;

	return TRUE;
}

//=========================================================
//=========================================================
void CSAS::JustSpoke(void)
{
	COFSquadTalkMonster::g_talkWaitTime = gpGlobals->time + RANDOM_FLOAT(1.5, 2.0);
	m_iSentence = HSAS_SENT_NONE;
}

//=========================================================
// PrescheduleThink - this function runs after conditions
// are collected and before scheduling code is run.
//=========================================================
void CSAS::PrescheduleThink(void)
{
	if (InSquad() && m_hEnemy != NULL)
	{
		if (HasConditions(bits_COND_SEE_ENEMY))
		{
			// update the squad's last enemy sighting time.
			MySquadLeader()->m_flLastEnemySightTime = gpGlobals->time;
		}
		else
		{
			if (gpGlobals->time - MySquadLeader()->m_flLastEnemySightTime > 5)
			{
				// been a while since we've seen the enemy
				MySquadLeader()->m_fEnemyEluded = TRUE;
			}
		}
	}
}

//=========================================================
// FCanCheckAttacks - this is overridden for human grunts
// because they can throw/shoot grenades when they can't see their
// target and the base class doesn't check attacks if the monster
// cannot see its enemy.
//
// !!!BUGBUG - this gets called before a 3-round burst is fired
// which means that a friendly can still be hit with up to 2 rounds. 
// ALSO, grenades will not be tossed if there is a friendly in front,
// this is a bad bug. Friendly machine gun fire avoidance
// will unecessarily prevent the throwing of a grenade as well.
//=========================================================
BOOL CSAS::FCanCheckAttacks(void)
{
	if (!HasConditions(bits_COND_ENEMY_TOOFAR))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


//=========================================================
// CheckMeleeAttack1
//=========================================================
BOOL CSAS::CheckMeleeAttack1(float flDot, float flDist)
{
	CBaseMonster* pEnemy;

	if (m_hEnemy != NULL)
	{
		pEnemy = m_hEnemy->MyMonsterPointer();

		if (!pEnemy)
		{
			return FALSE;
		}
	}

	if (flDist <= 64 && flDot >= 0.7 &&
		pEnemy->Classify() != CLASS_ALIEN_BIOWEAPON &&
		pEnemy->Classify() != CLASS_PLAYER_BIOWEAPON)
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// CheckRangeAttack1 - overridden for HGrunt, cause 
// FCanCheckAttacks() doesn't disqualify all attacks based
// on whether or not the enemy is occluded because unlike
// the base class, the HGrunt can attack when the enemy is
// occluded (throw grenade over wall, etc). We must 
// disqualify the machine gun attack if the enemy is occluded.
//=========================================================
BOOL CSAS::CheckRangeAttack1(float flDot, float flDist)
{
	//Only if we have a weapon
	if (pev->weapons)
	{
		const auto maxDistance = pev->weapons & SASWeaponFlag::Shotgun ? 640 : 1024;

		//Friendly fire is allowed
		if (!HasConditions(bits_COND_ENEMY_OCCLUDED) && flDist <= maxDistance && flDot >= 0.5 /*&& NoFriendlyFire()*/)
		{
			TraceResult	tr;

			auto pEnemy = m_hEnemy.Entity<CBaseEntity>();

			if (!pEnemy->IsPlayer() && flDist <= 64)
			{
				// kick nonclients, but don't shoot at them.
				return FALSE;
			}

			//TODO: kinda odd that this doesn't use GetGunPosition like the original
			Vector vecSrc = pev->origin + Vector(0, 0, 55);

			//Fire at last known position, adjusting for target origin being offset from entity origin
			const auto targetOrigin = pEnemy->BodyTarget(vecSrc);

			const auto targetPosition = targetOrigin - pEnemy->pev->origin + m_vecEnemyLKP;

			// verify that a bullet fired from the gun will hit the enemy before the world.
			UTIL_TraceLine(vecSrc, targetPosition, dont_ignore_monsters, ENT(pev), &tr);

			m_lastAttackCheck = tr.flFraction == 1.0 ? true : tr.pHit && GET_PRIVATE(tr.pHit) == pEnemy;

			return m_lastAttackCheck;
		}
	}

	return FALSE;
}

//=========================================================
// CheckRangeAttack2 - this checks the Grunt's grenade
// attack. 
//=========================================================
BOOL CSAS::CheckRangeAttack2(float flDot, float flDist)
{
	if (!FBitSet(pev->weapons, (SASWeaponFlag::HandGrenade | SASWeaponFlag::GrenadeLauncher)))
	{
		return FALSE;
	}

	// if the grunt isn't moving, it's ok to check.
	if (m_flGroundSpeed != 0)
	{
		m_fThrowGrenade = FALSE;
		return m_fThrowGrenade;
	}

	// assume things haven't changed too much since last time
	if (gpGlobals->time < m_flNextGrenadeCheck)
	{
		return m_fThrowGrenade;
	}

	if (!FBitSet(m_hEnemy->pev->flags, FL_ONGROUND) && m_hEnemy->pev->waterlevel == 0 && m_vecEnemyLKP.z > pev->absmax.z)
	{
		//!!!BUGBUG - we should make this check movetype and make sure it isn't FLY? Players who jump a lot are unlikely to 
		// be grenaded.
		// don't throw grenades at anything that isn't on the ground!
		m_fThrowGrenade = FALSE;
		return m_fThrowGrenade;
	}

	Vector vecTarget;

	if (FBitSet(pev->weapons, SASWeaponFlag::HandGrenade))
	{
		// find feet
		if (RANDOM_LONG(0, 1))
		{
			// magically know where they are
			vecTarget = Vector(m_hEnemy->pev->origin.x, m_hEnemy->pev->origin.y, m_hEnemy->pev->absmin.z);
		}
		else
		{
			// toss it to where you last saw them
			vecTarget = m_vecEnemyLKP;
		}
		// vecTarget = m_vecEnemyLKP + (m_hEnemy->BodyTarget( pev->origin ) - m_hEnemy->pev->origin);
		// estimate position
		// vecTarget = vecTarget + m_hEnemy->pev->velocity * 2;
	}
	else
	{
		// find target
		// vecTarget = m_hEnemy->BodyTarget( pev->origin );
		vecTarget = m_vecEnemyLKP + (m_hEnemy->BodyTarget(pev->origin) - m_hEnemy->pev->origin);
		// estimate position
		if (HasConditions(bits_COND_SEE_ENEMY))
			vecTarget = vecTarget + ((vecTarget - pev->origin).Length() / gSkillData.SASGrenadeSpeed) * m_hEnemy->pev->velocity;
	}

	// are any of my squad members near the intended grenade impact area?
	if (InSquad())
	{
		if (SquadMemberInRange(vecTarget, 256))
		{
			// crap, I might blow my own guy up. Don't throw a grenade and don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
			m_fThrowGrenade = FALSE;
		}
	}

	if ((vecTarget - pev->origin).Length2D() <= 256)
	{
		// crap, I don't want to blow myself up
		m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
		m_fThrowGrenade = FALSE;
		return m_fThrowGrenade;
	}


	if (FBitSet(pev->weapons, SASWeaponFlag::HandGrenade))
	{
		Vector vecToss = VecCheckToss(pev, GetGunPosition(), vecTarget, 0.5);

		if (vecToss != g_vecZero)
		{
			m_vecTossVelocity = vecToss;

			// throw a hand grenade
			m_fThrowGrenade = TRUE;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time; // 1/3 second.
		}
		else
		{
			// don't throw
			m_fThrowGrenade = FALSE;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
		}
	}
	else
	{
		Vector vecToss = VecCheckThrow(pev, GetGunPosition(), vecTarget, gSkillData.SASGrenadeSpeed, 0.5);

		if (vecToss != g_vecZero)
		{
			m_vecTossVelocity = vecToss;

			// throw a hand grenade
			m_fThrowGrenade = TRUE;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 0.3; // 1/3 second.
		}
		else
		{
			// don't throw
			m_fThrowGrenade = FALSE;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
		}
	}



	return m_fThrowGrenade;
}


//=========================================================
// TraceAttack - make sure we're not taking it in the helmet
//=========================================================
void CSAS::TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType)
{
	// check for helmet shot
	if (ptr->iHitgroup == 11)
	{
		// make sure we're wearing one
		//TODO: disabled for ally
		if (/*GetBodygroup( SASBodygroup::Head ) == SASHead::GasMask &&*/ (bitsDamageType & (DMG_BULLET | DMG_SLASH | DMG_CLUB)))
		{
			// absorb damage
			flDamage -= 20;
			if (flDamage <= 0)
			{
				UTIL_Ricochet(ptr->vecEndPos, 1.0);
				flDamage = 0.01;
			}
		}
		// it's head shot anyways
		ptr->iHitgroup = HITGROUP_HEAD;
	}
	//PCV absorbs some damage types
	else if ((ptr->iHitgroup == HITGROUP_CHEST || ptr->iHitgroup == HITGROUP_STOMACH)
		&& (bitsDamageType & (DMG_BLAST | DMG_BULLET | DMG_SLASH)))
	{
		flDamage *= 0.5;
	}

	COFSquadTalkMonster::TraceAttack(pevAttacker, flDamage, vecDir, ptr, bitsDamageType);
}


//=========================================================
// TakeDamage - overridden for the grunt because the grunt
// needs to forget that he is in cover if he's hurt. (Obviously
// not in a safe place anymore).
//=========================================================
int CSAS::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	// make sure friends talk about it if player hurts talkmonsters...
	int ret = COFSquadTalkMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);

	if (pev->deadflag != DEAD_NO)
		return ret;

	Forget(bits_MEMORY_INCOVER);

	if (m_MonsterState != MONSTERSTATE_PRONE && (pevAttacker->flags & FL_CLIENT))
	{
		m_flPlayerDamage += flDamage;

		// This is a heurstic to determine if the player intended to harm me
		// If I have an enemy, we can't establish intent (may just be crossfire)
		if (m_hEnemy == NULL)
		{
			// Hey, be careful with that
			Remember(bits_MEMORY_SUSPICIOUS);

			if (GetCount() < 2) 
			{
				PlaySentence("SAS_FSHOT", 4, VOL_NORM, ATTN_NORM);
			}

			if (4.0 > gpGlobals->time - m_flLastHitByPlayer)
				++m_iPlayerHits;
			else
				m_iPlayerHits = 0;

			m_flLastHitByPlayer = gpGlobals->time;

			ALERT(at_console, "HGrunt Ally is now SUSPICIOUS!\n");
		}
		else if (!m_hEnemy->IsPlayer())
		{
			PlaySentence("SAS_SHOT", 4, VOL_NORM, ATTN_NORM);
		}
	}

	return ret;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CSAS::SetYawSpeed(void)
{
	int ys;

	switch (m_Activity)
	{
	case ACT_IDLE:
		ys = 150;
		break;
	case ACT_RUN:
		ys = 150;
		break;
	case ACT_WALK:
		ys = 180;
		break;
	case ACT_RANGE_ATTACK1:
		ys = 120;
		break;
	case ACT_RANGE_ATTACK2:
		ys = 120;
		break;
	case ACT_MELEE_ATTACK1:
		ys = 120;
		break;
	case ACT_MELEE_ATTACK2:
		ys = 120;
		break;
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 180;
		break;
	case ACT_GLIDE:
	case ACT_FLY:
		ys = 30;
		break;
	default:
		ys = 90;
		break;
	}

	pev->yaw_speed = ys;
}

int CSAS::GetCount() 
{
	CBaseEntity* pMate = NULL;
	int totCount = 0;
	while ((pMate = UTIL_FindEntityByClassname(pMate, "monster_sas")) != NULL)
	{
		ALERT(at_console, "Found one. Count is now %d", totCount);
		totCount++;
	}
	ALERT(at_console, "Finished. Returning %d", totCount);
	return totCount;
}

void CSAS::IdleSound(void)
{
	if (FOkToSpeak() && (g_fGruntAllyQuestion || RANDOM_LONG(0, 1)))
	{
		if (!g_fGruntAllyQuestion)
		{
			//ALERT(at_console, "\n - %s - %s - \n", m_canLoseSquad ? "true" : "false", m_canSayUsLeft ? "true" : "false");
			if (m_canSayUsLeft > 1 && m_canLoseSquad && GetCount() == 2)
			{
				PlaySentence("SAS_GONBD", 4, VOL_NORM, ATTN_NORM);
				m_canSayUsLeft--;
			}
			else if (m_canSayUsLeft > 0 && m_canLoseSquad && GetCount() < 2)
			{
				// Its just us left
				SENTENCEG_PlayRndSz(ENT(pev), "SAS_JULSN", HSAS_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				m_canSayUsLeft--;
			}
			else if (m_canLoseSquad && GetCount() < 2 && RANDOM_LONG(0, 2) == 2)
			{
				// I wish my friends was here
				SENTENCEG_PlayRndSz(ENT(pev), "SAS_MOURN", HSAS_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
			}
			else
			{
				// ask question or make statement
				switch (RANDOM_LONG(0, 2))
				{
				case 0: // check in
					SENTENCEG_PlayRndSz(ENT(pev), "SAS_CHECK", HSAS_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
					g_fGruntAllyQuestion = 1;
					break;
				case 1: // question
					SENTENCEG_PlayRndSz(ENT(pev), "SAS_QUEST", HSAS_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
					g_fGruntAllyQuestion = 2;
					break;
				case 2: // statement
					SENTENCEG_PlayRndSz(ENT(pev), "SAS_IDLE", HSAS_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
					break;
				}
			}
		}
		else
		{
			switch (g_fGruntAllyQuestion)
			{
			case 1: // check in
				SENTENCEG_PlayRndSz(ENT(pev), "SAS_CLEAR", HSAS_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				break;
			case 2: // question 
				SENTENCEG_PlayRndSz(ENT(pev), "SAS_ANSWER", HSAS_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				break;
			}
			g_fGruntAllyQuestion = 0;
		}
		JustSpoke();
	}
}

//=========================================================
// CheckAmmo - overridden for the grunt because he actually
// uses ammo! (base class doesn't)
//=========================================================
void CSAS::CheckAmmo(void)
{
	if (m_cAmmoLoaded <= 0)
	{
		SetConditions(bits_COND_NO_AMMO_LOADED);
	}
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CSAS::Classify(void)
{
	return	CLASS_PLAYER_ALLY;
}

//=========================================================
//=========================================================
CBaseEntity* CSAS::Kick(void)
{
	TraceResult tr;

	UTIL_MakeVectors(pev->angles);
	Vector vecStart = pev->origin;
	vecStart.z += pev->size.z * 0.5;
	Vector vecEnd = vecStart + (gpGlobals->v_forward * 70);

	UTIL_TraceHull(vecStart, vecEnd, dont_ignore_monsters, head_hull, ENT(pev), &tr);

	if (tr.pHit)
	{
		CBaseEntity* pEntity = CBaseEntity::Instance(tr.pHit);
		return pEntity;
	}

	return NULL;
}

//=========================================================
// GetGunPosition	return the end of the barrel
//=========================================================

Vector CSAS::GetGunPosition()
{
	if (m_fStanding)
	{
		return pev->origin + Vector(0, 0, 60);
	}
	else
	{
		return pev->origin + Vector(0, 0, 48);
	}
}

//=========================================================
// Shoot
//=========================================================
void CSAS::Shoot(void)
{
	if (m_hEnemy == NULL)
	{
		return;
	}

	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy(vecShootOrigin);

	UTIL_MakeVectors(pev->angles);

	Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40, 90) + gpGlobals->v_up * RANDOM_FLOAT(75, 200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
	EjectBrass(vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iBrassShell, TE_BOUNCE_SHELL);
	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_10DEGREES, 2048, BULLET_MONSTER_MP5); // shoot +-5 degrees

	pev->effects |= EF_MUZZLEFLASH;

	m_cAmmoLoaded--;// take away a bullet!

	Vector angDir = UTIL_VecToAngles(vecShootDir);
	SetBlending(0, angDir.x);
}

//=========================================================
// Shoot
//=========================================================
void CSAS::Shotgun(void)
{
	if (m_hEnemy == NULL)
	{
		return;
	}

	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy(vecShootOrigin);

	UTIL_MakeVectors(pev->angles);

	Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40, 90) + gpGlobals->v_up * RANDOM_FLOAT(75, 200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
	EjectBrass(vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iShotgunShell, TE_BOUNCE_SHOTSHELL);
	FireBullets(gSkillData.SASShotgunPellets, vecShootOrigin, vecShootDir, VECTOR_CONE_15DEGREES, 2048, BULLET_PLAYER_BUCKSHOT, 0); // shoot +-7.5 degrees

	pev->effects |= EF_MUZZLEFLASH;

	m_cAmmoLoaded--;// take away a bullet!

	Vector angDir = UTIL_VecToAngles(vecShootDir);
	SetBlending(0, angDir.x);
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CSAS::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	Vector	vecShootDir;
	Vector	vecShootOrigin;

	switch (pEvent->event)
	{
	case HSAS_AE_DROP_GUN:
	{
		Vector	vecGunPos;
		Vector	vecGunAngles;

		GetAttachment(0, vecGunPos, vecGunAngles);

		// switch to body group with no gun.
		SetBodygroup(SASBodygroup::Weapons, SASWeapon::None);

		// now spawn a gun.
		if (FBitSet(pev->weapons, SASWeaponFlag::Shotgun))
		{
			DropItem("weapon_shotgun", vecGunPos, vecGunAngles);
		}
		else if (FBitSet(pev->weapons, SASWeaponFlag::AR16))
		{
			DropItem("weapon_ar16", vecGunPos, vecGunAngles);
		}
		else
		{
			DropItem("weapon_9mmAR", vecGunPos, vecGunAngles);
		}

		if (FBitSet(pev->weapons, SASWeaponFlag::GrenadeLauncher))
		{
			DropItem("ammo_ARgrenades", BodyTarget(pev->origin), vecGunAngles);
		}

		m_iWeaponIdx = SASWeapon::None;
	}
	break;

	case HSAS_AE_RELOAD:
		if (FBitSet(pev->weapons, SASWeaponFlag::AR16))
		{
			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/AR16_reload.wav", 1, ATTN_NORM);
		}
		else
			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "hgrunt/gr_reload1.wav", 1, ATTN_NORM);

		m_cAmmoLoaded = m_cClipSize;
		ClearConditions(bits_COND_NO_AMMO_LOADED);
		break;

	case HSAS_AE_GREN_TOSS:
	{
		UTIL_MakeVectors(pev->angles);
		// CGrenade::ShootTimed( pev, pev->origin + gpGlobals->v_forward * 34 + Vector (0, 0, 32), m_vecTossVelocity, 3.5 );
		CGrenade::ShootTimed(pev, GetGunPosition(), m_vecTossVelocity, 3.5);

		m_fThrowGrenade = FALSE;
		m_flNextGrenadeCheck = gpGlobals->time + 6;// wait six seconds before even looking again to see if a grenade can be thrown.
		// !!!LATER - when in a group, only try to throw grenade if ordered.
	}
	break;

	case HSAS_AE_GREN_LAUNCH:
	{
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/glauncher.wav", 0.8, ATTN_NORM);
		CGrenade::ShootContact(pev, GetGunPosition(), m_vecTossVelocity);
		m_fThrowGrenade = FALSE;
		if (g_iSkillLevel == SKILL_HARD)
			m_flNextGrenadeCheck = gpGlobals->time + RANDOM_FLOAT(2, 5);// wait a random amount of time before shooting again
		else
			m_flNextGrenadeCheck = gpGlobals->time + 6;// wait six seconds before even looking again to see if a grenade can be thrown.
	}
	break;

	case HSAS_AE_GREN_DROP:
	{
		UTIL_MakeVectors(pev->angles);
		CGrenade::ShootTimed(pev, pev->origin + gpGlobals->v_forward * 17 - gpGlobals->v_right * 27 + gpGlobals->v_up * 6, g_vecZero, 3);
	}
	break;

	case HSAS_AE_BURST1:
	{
		if (FBitSet(pev->weapons, SASWeaponFlag::MP5))
		{
			Shoot();

			// the first round of the three round burst plays the sound and puts a sound in the world sound list.
			if (RANDOM_LONG(0, 1))
			{
				EMIT_SOUND(ENT(pev), CHAN_WEAPON, "hgrunt/gr_mgun1.wav", 1, ATTN_NORM);
			}
			else
			{
				EMIT_SOUND(ENT(pev), CHAN_WEAPON, "hgrunt/gr_mgun2.wav", 1, ATTN_NORM);
			}
		}
		else if (FBitSet(pev->weapons, SASWeaponFlag::AR16))
		{
			ShootAR16();
		}
		else
		{
			Shotgun();

			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/sbarrel1.wav", 1, ATTN_NORM);
		}

		CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, 384, 0.3);
	}
	break;

	case HSAS_AE_BURST2:
	case HSAS_AE_BURST3:
		if (FBitSet(pev->weapons, SASWeaponFlag::MP5))
		{
			Shoot();
		}
		else if (FBitSet(pev->weapons, SASWeaponFlag::AR16))
		{
			ShootAR16();
		}
		break;

	case HSAS_AE_KICK:
	{
		CBaseEntity* pHurt = Kick();

		if (pHurt)
		{
			// SOUND HERE!
			UTIL_MakeVectors(pev->angles);
			pHurt->pev->punchangle.x = 15;
			pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 100 + gpGlobals->v_up * 50;
			pHurt->TakeDamage(pev, pev, gSkillData.SASDmgKick, DMG_CLUB);
		}
	}
	break;

	case HSAS_AE_CAUGHT_ENEMY:
	{
		if (FOkToSpeak())
		{
			SENTENCEG_PlayRndSz(ENT(pev), "SAS_ALERT", HSAS_SENTENCE_VOLUME, SAS_ATTN, 0, m_voicePitch);
			JustSpoke();
		}

	}

	default:
		COFSquadTalkMonster::HandleAnimEvent(pEvent);
		break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CSAS::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/sas.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->effects = 0;
	pev->health = gSkillData.SASHealth;
	m_flFieldOfView = 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	m_flNextGrenadeCheck = gpGlobals->time + 1;
	m_flNextPainTime = gpGlobals->time;
	m_iSentence = HSAS_SENT_NONE;

	m_deadMates = 0;

	m_afCapability = bits_CAP_SQUAD | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP | bits_CAP_HEAR;

	m_fEnemyEluded = FALSE;
	m_fFirstEncounter = TRUE;// this is true when the grunt spawns, because he hasn't encountered an enemy yet.

	m_HackedGunPos = Vector(0, 0, 55);

	m_canSayUsLeft = 2;

	//Note: this code has been rewritten to use SetBodygroup since it relies on hardcoded offsets in the original
	pev->body = 0;
	m_iGruntTorso = SASTorso::Normal;

	if (pev->weapons & SASWeaponFlag::MP5)
	{
		m_iWeaponIdx = SASWeapon::MP5;
		m_cClipSize = SAS_MP5_CLIP_SIZE;
		pev->controller[1] = 255;
	}
	else if (pev->weapons & SASWeaponFlag::Shotgun)
	{
		m_cClipSize = SAS_SHOTGUN_CLIP_SIZE;
		m_iWeaponIdx = SASWeapon::Shotgun;
		m_iGruntTorso = SASTorso::Shotgun;
		pev->controller[1] = 0;
	}
	else if (pev->weapons & SASWeaponFlag::AR16)
	{
		m_iWeaponIdx = SASWeapon::AR16;
		m_cClipSize = SAS_AR16_CLIP_SIZE;
		m_iGruntTorso = SASTorso::AR16;
		pev->controller[1] = 0;
	}
	else
	{
		m_iWeaponIdx = SASWeapon::None;
		m_cClipSize = 0;
	}

	m_cAmmoLoaded = m_cClipSize;

	/*

	if (m_iGruntHead == SASHead::Default)
	{
		if (pev->spawnflags & SF_SQUADMONSTER_LEADER)
		{
			m_iGruntHead = SASHead::BeretWhite;
		}
		else if (m_iWeaponIdx == SASWeapon::Shotgun)
		{
			m_iGruntHead = SASHead::OpsMask;
		}
		else if (m_iWeaponIdx == SASWeapon::AR16)
		{
			m_iGruntHead = RANDOM_LONG(0, 1) + SASHead::BandanaWhite;
		}
		else if (m_iWeaponIdx == SASWeapon::MP5)
		{
			m_iGruntHead = SASHead::MilitaryPolice;
		}
		else
		{
			m_iGruntHead = SASHead::GasMask;
		}
	}

	*/

	//TODO: probably also needs this for head SASHead::BeretBlack
	//if (m_iGruntHead == SASHead::OpsMask || m_iGruntHead == SASHead::BandanaBlack)
	//	m_voicePitch = 90;

	//Random Skin
	if (pev->spawnflags & SF_SQUADMONSTER_LEADER)
	{
		pev->skin = SASSkin::LeaderScot;
		m_iGruntHead = SASHead::Leader;
	}
	else
	{
		pev->skin = RANDOM_LONG(1, SASSkin::SKINCOUNT_SOLDIERS);
		m_iGruntHead = SASHead::GasMask;
	}

	// get voice pitch
	if (RANDOM_LONG(0, 1) && !(pev->spawnflags & SF_SQUADMONSTER_LEADER))
	{ 
		m_voicePitch = 100 + RANDOM_LONG(0, 18);
	}
	else
	{
		m_voicePitch = 100;
	}

	SetBodygroup(SASBodygroup::Head, m_iGruntHead);
	//SetBodygroup(SASBodygroup::Torso, m_iGruntTorso); // Theres just one torso
	SetBodygroup(SASBodygroup::Weapons, m_iWeaponIdx);

	m_iAR16Shell = PRECACHE_MODEL("models/556shell.mdl");
	//m_iAR16Link = PRECACHE_MODEL("models/AR16_link.mdl");

	COFAllyMonster::g_talkWaitTime = 0;

	m_flMedicWaitTime = gpGlobals->time;

	MonsterInit();

	SetUse(&CSAS::FollowerUse);
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CSAS::Precache()
{
	PRECACHE_MODEL("models/sas.mdl");

	TalkInit();

	PRECACHE_SOUND("hgrunt/gr_mgun1.wav");
	PRECACHE_SOUND("hgrunt/gr_mgun2.wav");

	PRECACHE_SOUND("sas/ct_death01.wav");
	PRECACHE_SOUND("sas/ct_death02.wav");
	PRECACHE_SOUND("sas/ct_death03.wav");
	PRECACHE_SOUND("sas/ct_death04.wav");
	PRECACHE_SOUND("sas/ct_death05.wav");
	PRECACHE_SOUND("sas/ct_death06.wav");

	PRECACHE_SOUND("fgrunt/pain1.wav");
	PRECACHE_SOUND("fgrunt/pain2.wav");
	PRECACHE_SOUND("fgrunt/pain3.wav");
	PRECACHE_SOUND("fgrunt/pain4.wav");
	PRECACHE_SOUND("fgrunt/pain5.wav");
	PRECACHE_SOUND("fgrunt/pain6.wav");

	PRECACHE_SOUND("hgrunt/gr_reload1.wav");

	PRECACHE_SOUND("weapons/AR16_fire1.wav");
	PRECACHE_SOUND("weapons/AR16_fire2.wav");
	PRECACHE_SOUND("weapons/AR16_fire3.wav");
	PRECACHE_SOUND("weapons/AR16_reload.wav");

	PRECACHE_SOUND("weapons/glauncher.wav");

	PRECACHE_SOUND("weapons/sbarrel1.wav");

	PRECACHE_SOUND("sas/help04.wav");

	PRECACHE_SOUND("zombie/claw_miss2.wav");// because we use the basemonster SWIPE animation event

	m_iBrassShell = PRECACHE_MODEL("models/shell.mdl");// brass shell
	m_iShotgunShell = PRECACHE_MODEL("models/shotgunshell.mdl");

	COFSquadTalkMonster::Precache();
}

//=========================================================
// start task
//=========================================================
void CSAS::StartTask(Task_t* pTask)
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch (pTask->iTask)
	{
	case TASK_SAS_CHECK_FIRE:
		if (!NoFriendlyFire())
		{
			SetConditions(bits_COND_SAS_NOFIRE);
		}
		TaskComplete();
		break;

	case TASK_SAS_SPEAK_SENTENCE:
		SpeakSentence();
		TaskComplete();
		break;

	case TASK_WALK_PATH:
	case TASK_RUN_PATH:
		// grunt no longer assumes he is covered if he moves
		Forget(bits_MEMORY_INCOVER);
		COFSquadTalkMonster::StartTask(pTask);
		break;

	case TASK_RELOAD:
		m_IdealActivity = ACT_RELOAD;
		break;

	case TASK_SAS_FACE_TOSS_DIR:
		break;

	case TASK_FACE_IDEAL:
	case TASK_FACE_ENEMY:
		COFSquadTalkMonster::StartTask(pTask);
		if (pev->movetype == MOVETYPE_FLY)
		{
			m_IdealActivity = ACT_GLIDE;
		}
		break;

	default:
		COFSquadTalkMonster::StartTask(pTask);
		break;
	}
}

//=========================================================
// RunTask
//=========================================================
void CSAS::RunTask(Task_t* pTask)
{
	switch (pTask->iTask)
	{
	case TASK_SAS_FACE_TOSS_DIR:
	{
		// project a point along the toss vector and turn to face that point.
		MakeIdealYaw(pev->origin + m_vecTossVelocity * 64);
		ChangeYaw(pev->yaw_speed);

		if (FacingIdeal())
		{
			m_iTaskStatus = TASKSTATUS_COMPLETE;
		}
		break;
	}
	default:
	{
		COFSquadTalkMonster::RunTask(pTask);
		break;
	}
	}
}

//=========================================================
// PainSound
//=========================================================
void CSAS::PainSound(void)
{
	if (gpGlobals->time > m_flNextPainTime)
	{
#if 0
		if (RANDOM_LONG(0, 99) < 5)
		{
			// pain sentences are rare
			if (FOkToSpeak())
			{
				//SENTENCEG_PlayRndSz(ENT(pev), "SAS_PAIN", HSAS_SENTENCE_VOLUME, ATTN_NORM, 0, PITCH_NORM);
				JustSpoke();
				return;
			}
		}
#endif 
		/*
		switch (RANDOM_LONG(0, 7))
		{
		case 0:
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "fgrunt/pain3.wav", 1, ATTN_NORM);
			break;
		case 1:
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "fgrunt/pain4.wav", 1, ATTN_NORM);
			break;
		case 2:
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "fgrunt/pain5.wav", 1, ATTN_NORM);
			break;
		case 3:
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "fgrunt/pain1.wav", 1, ATTN_NORM);
			break;
		case 4:
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "fgrunt/pain2.wav", 1, ATTN_NORM);
			break;
		case 5:
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "fgrunt/pain6.wav", 1, ATTN_NORM);
			break;
		}
		*/
		m_flNextPainTime = gpGlobals->time + 1;
	}
}

//=========================================================
// DeathSound 
//=========================================================
void CSAS::DeathSound(void)
{
	//TODO: these sounds don't exist, the gr_ prefix is wrong
	switch (RANDOM_LONG(0, 5))
	{
	case 0:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "sas/ct_death01.wav", 1, ATTN_IDLE);
		break;
	case 1:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "sas/ct_death02.wav", 1, ATTN_IDLE);
		break;
	case 2:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "sas/ct_death03.wav", 1, ATTN_IDLE);
		break;
	case 3:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "sas/ct_death04.wav", 1, ATTN_IDLE);
		break;
	case 4:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "sas/ct_death05.wav", 1, ATTN_IDLE);
		break;
	case 5:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "sas/ct_death06.wav", 1, ATTN_IDLE);
		break;
	}
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

//=========================================================
// GruntFail
//=========================================================
Task_t	tlGruntAllyFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)2		},
	{ TASK_WAIT_PVS,			(float)0		},
};

Schedule_t	slGruntAllyFail[] =
{
	{
		tlGruntAllyFail,
		ARRAYSIZE(tlGruntAllyFail),
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_RANGE_ATTACK2 |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK2,
		0,
		"Grunt Fail"
	},
};

//=========================================================
// Grunt Combat Fail
//=========================================================
Task_t	tlGruntAllyCombatFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT_FACE_ENEMY,		(float)2		},
	{ TASK_WAIT_PVS,			(float)0		},
};

Schedule_t	slGruntAllyCombatFail[] =
{
	{
		tlGruntAllyCombatFail,
		ARRAYSIZE(tlGruntAllyCombatFail),
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_RANGE_ATTACK2,
		0,
		"Grunt Combat Fail"
	},
};

//=========================================================
// Victory dance!
//=========================================================
Task_t	tlGruntAllyVictoryDance[] =
{
	{ TASK_STOP_MOVING,						(float)0					},
	{ TASK_FACE_ENEMY,						(float)0					},
	{ TASK_WAIT,							(float)1.5					},
	{ TASK_GET_PATH_TO_ENEMY_CORPSE,		(float)0					},
	{ TASK_WALK_PATH,						(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,				(float)0					},
	{ TASK_FACE_ENEMY,						(float)0					},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_VICTORY_DANCE	},
};

Schedule_t	slGruntAllyVictoryDance[] =
{
	{
		tlGruntAllyVictoryDance,
		ARRAYSIZE(tlGruntAllyVictoryDance),
		bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE,
		0,
		"GruntVictoryDance"
	},
};

//=========================================================
// Establish line of fire - move to a position that allows
// the grunt to attack.
//=========================================================
Task_t tlGruntAllyEstablishLineOfFire[] =
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_SAS_ELOF_FAIL	},
	{ TASK_GET_PATH_TO_ENEMY,	(float)0						},
	{ TASK_SAS_SPEAK_SENTENCE,(float)0						},
	{ TASK_RUN_PATH,			(float)0						},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0						},
};

Schedule_t slGruntAllyEstablishLineOfFire[] =
{
	{
		tlGruntAllyEstablishLineOfFire,
		ARRAYSIZE(tlGruntAllyEstablishLineOfFire),
		bits_COND_NEW_ENEMY |
		bits_COND_ENEMY_DEAD |
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_CAN_RANGE_ATTACK2 |
		bits_COND_CAN_MELEE_ATTACK2 |
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"GruntEstablishLineOfFire"
	},
};

//=========================================================
// GruntFoundEnemy - grunt established sight with an enemy
// that was hiding from the squad.
//=========================================================
Task_t	tlGruntAllyFoundEnemy[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,(float)ACT_SIGNAL1			},
};

Schedule_t	slGruntAllyFoundEnemy[] =
{
	{
		tlGruntAllyFoundEnemy,
		ARRAYSIZE(tlGruntAllyFoundEnemy),
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"GruntFoundEnemy"
	},
};

//=========================================================
// GruntCombatFace Schedule
//=========================================================
Task_t	tlGruntAllyCombatFace1[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_WAIT,					(float)1.5					},
	{ TASK_SET_SCHEDULE,			(float)SCHED_SAS_SWEEP	},
};

Schedule_t	slGruntAllyCombatFace[] =
{
	{
		tlGruntAllyCombatFace1,
		ARRAYSIZE(tlGruntAllyCombatFace1),
		bits_COND_NEW_ENEMY |
		bits_COND_ENEMY_DEAD |
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_RANGE_ATTACK2,
		0,
		"Combat Face"
	},
};

//=========================================================
// Suppressing fire - don't stop shooting until the clip is
// empty or grunt gets hurt.
//=========================================================
Task_t	tlGruntAllySignalSuppress[] =
{
	{ TASK_STOP_MOVING,					0						},
	{ TASK_FACE_IDEAL,					(float)0				},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,	(float)ACT_SIGNAL2		},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_SAS_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_SAS_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_SAS_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_SAS_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_SAS_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
};

Schedule_t	slGruntAllySignalSuppress[] =
{
	{
		tlGruntAllySignalSuppress,
		ARRAYSIZE(tlGruntAllySignalSuppress),
		bits_COND_ENEMY_DEAD |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND |
		bits_COND_SAS_NOFIRE |
		bits_COND_NO_AMMO_LOADED,

		bits_SOUND_DANGER,
		"SignalSuppress"
	},
};

Task_t	tlGruntAllySuppress[] =
{
	{ TASK_STOP_MOVING,			0							},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_SAS_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_SAS_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_SAS_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_SAS_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_SAS_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
};

Schedule_t	slGruntAllySuppress[] =
{
	{
		tlGruntAllySuppress,
		ARRAYSIZE(tlGruntAllySuppress),
		bits_COND_ENEMY_DEAD |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND |
		bits_COND_SAS_NOFIRE |
		bits_COND_NO_AMMO_LOADED,

		bits_SOUND_DANGER,
		"Suppress"
	},
};


//=========================================================
// grunt wait in cover - we don't allow danger or the ability
// to attack to break a grunt's run to cover schedule, but
// when a grunt is in cover, we do want them to attack if they can.
//=========================================================
Task_t	tlGruntAllyWaitInCover[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_WAIT_FACE_ENEMY,			(float)1					},
};

Schedule_t	slGruntAllyWaitInCover[] =
{
	{
		tlGruntAllyWaitInCover,
		ARRAYSIZE(tlGruntAllyWaitInCover),
		bits_COND_NEW_ENEMY |
		bits_COND_HEAR_SOUND |
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_RANGE_ATTACK2 |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK2,

		bits_SOUND_DANGER,
		"GruntWaitInCover"
	},
};

//=========================================================
// run to cover.
// !!!BUGBUG - set a decent fail schedule here.
//=========================================================
Task_t	tlGruntAllyTakeCover1[] =
{
	{ TASK_STOP_MOVING,				(float)0							},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_SAS_TAKECOVER_FAILED	},
	{ TASK_WAIT,					(float)0.2							},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0							},
	{ TASK_SAS_SPEAK_SENTENCE,	(float)0							},
	{ TASK_RUN_PATH,				(float)0							},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0							},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER			},
	{ TASK_SET_SCHEDULE,			(float)SCHED_SAS_WAIT_FACE_ENEMY	},
};

Schedule_t	slGruntAllyTakeCover[] =
{
	{
		tlGruntAllyTakeCover1,
		ARRAYSIZE(tlGruntAllyTakeCover1),
		0,
		0,
		"TakeCover"
	},
};

//=========================================================
// drop grenade then run to cover.
//=========================================================
Task_t	tlGruntAllyGrenadeCover1[] =
{
	{ TASK_STOP_MOVING,						(float)0							},
	{ TASK_FIND_COVER_FROM_ENEMY,			(float)99							},
	{ TASK_FIND_FAR_NODE_COVER_FROM_ENEMY,	(float)384							},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_SPECIAL_ATTACK1			},
	{ TASK_CLEAR_MOVE_WAIT,					(float)0							},
	{ TASK_RUN_PATH,						(float)0							},
	{ TASK_WAIT_FOR_MOVEMENT,				(float)0							},
	{ TASK_SET_SCHEDULE,					(float)SCHED_SAS_WAIT_FACE_ENEMY	},
};

Schedule_t	slGruntAllyGrenadeCover[] =
{
	{
		tlGruntAllyGrenadeCover1,
		ARRAYSIZE(tlGruntAllyGrenadeCover1),
		0,
		0,
		"GrenadeCover"
	},
};


//=========================================================
// drop grenade then run to cover.
//=========================================================
Task_t	tlGruntAllyTossGrenadeCover1[] =
{
	{ TASK_FACE_ENEMY,						(float)0							},
	{ TASK_RANGE_ATTACK2, 					(float)0							},
	{ TASK_SET_SCHEDULE,					(float)SCHED_TAKE_COVER_FROM_ENEMY	},
};

Schedule_t	slGruntAllyTossGrenadeCover[] =
{
	{
		tlGruntAllyTossGrenadeCover1,
		ARRAYSIZE(tlGruntAllyTossGrenadeCover1),
		0,
		0,
		"TossGrenadeCover"
	},
};

//=========================================================
// hide from the loudest sound source (to run from grenade)
//=========================================================
Task_t	tlGruntAllyTakeCoverFromBestSound[] =
{
	{ TASK_SET_FAIL_SCHEDULE,			(float)SCHED_COWER			},// duck and cover if cannot move from explosion
	{ TASK_STOP_MOVING,					(float)0					},
	{ TASK_FIND_COVER_FROM_BEST_SOUND,	(float)0					},
	{ TASK_RUN_PATH,					(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,			(float)0					},
	{ TASK_REMEMBER,					(float)bits_MEMORY_INCOVER	},
	{ TASK_TURN_LEFT,					(float)179					},
};

Schedule_t	slGruntAllyTakeCoverFromBestSound[] =
{
	{
		tlGruntAllyTakeCoverFromBestSound,
		ARRAYSIZE(tlGruntAllyTakeCoverFromBestSound),
		0,
		0,
		"GruntTakeCoverFromBestSound"
	},
};

//=========================================================
// Grunt reload schedule
//=========================================================
Task_t	tlGruntAllyHideReload[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_RELOAD			},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0					},
	{ TASK_RUN_PATH,				(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0					},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER	},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_RELOAD			},
};

Schedule_t slGruntAllyHideReload[] =
{
	{
		tlGruntAllyHideReload,
		ARRAYSIZE(tlGruntAllyHideReload),
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"GruntHideReload"
	}
};

//=========================================================
// Do a turning sweep of the area
//=========================================================
Task_t	tlGruntAllySweep[] =
{
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)1	},
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)1	},
};

Schedule_t	slGruntAllySweep[] =
{
	{
		tlGruntAllySweep,
		ARRAYSIZE(tlGruntAllySweep),

		bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_RANGE_ATTACK2 |
		bits_COND_HEAR_SOUND,

		bits_SOUND_WORLD |// sound flags
		bits_SOUND_DANGER |
		bits_SOUND_PLAYER,

		"Grunt Sweep"
	},
};

//=========================================================
// primary range attack. Overriden because base class stops attacking when the enemy is occluded.
// grunt's grenade toss requires the enemy be occluded.
//=========================================================
Task_t	tlGruntAllyRangeAttack1A[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,		(float)ACT_CROUCH },
	{ TASK_SAS_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_SAS_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_SAS_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_SAS_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
};

Schedule_t	slGruntAllyRangeAttack1A[] =
{
	{
		tlGruntAllyRangeAttack1A,
		ARRAYSIZE(tlGruntAllyRangeAttack1A),
		bits_COND_NEW_ENEMY |
		bits_COND_ENEMY_DEAD |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_ENEMY_OCCLUDED |
		bits_COND_HEAR_SOUND |
		bits_COND_SAS_NOFIRE |
		bits_COND_NO_AMMO_LOADED,

		bits_SOUND_DANGER,
		"Range Attack1A"
	},
};


//=========================================================
// primary range attack. Overriden because base class stops attacking when the enemy is occluded.
// grunt's grenade toss requires the enemy be occluded.
//=========================================================
Task_t	tlGruntAllyRangeAttack1B[] =
{
	{ TASK_STOP_MOVING,				(float)0		},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,(float)ACT_IDLE_ANGRY  },
	{ TASK_SAS_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_SAS_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_SAS_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_SAS_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
};

Schedule_t	slGruntAllyRangeAttack1B[] =
{
	{
		tlGruntAllyRangeAttack1B,
		ARRAYSIZE(tlGruntAllyRangeAttack1B),
		bits_COND_NEW_ENEMY |
		bits_COND_ENEMY_DEAD |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_ENEMY_OCCLUDED |
		bits_COND_NO_AMMO_LOADED |
		bits_COND_SAS_NOFIRE |
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"Range Attack1B"
	},
};

//=========================================================
// secondary range attack. Overriden because base class stops attacking when the enemy is occluded.
// grunt's grenade toss requires the enemy be occluded.
//=========================================================
Task_t	tlGruntAllyRangeAttack2[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SAS_FACE_TOSS_DIR,		(float)0					},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_RANGE_ATTACK2	},
	{ TASK_SET_SCHEDULE,			(float)SCHED_SAS_WAIT_FACE_ENEMY	},// don't run immediately after throwing grenade.
};

Schedule_t	slGruntAllyRangeAttack2[] =
{
	{
		tlGruntAllyRangeAttack2,
		ARRAYSIZE(tlGruntAllyRangeAttack2),
		0,
		0,
		"RangeAttack2"
	},
};


//=========================================================
// repel 
//=========================================================
Task_t	tlGruntAllyRepel[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_GLIDE 	},
};

Schedule_t	slGruntAllyRepel[] =
{
	{
		tlGruntAllyRepel,
		ARRAYSIZE(tlGruntAllyRepel),
		bits_COND_SEE_ENEMY |
		bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER |
		bits_SOUND_COMBAT |
		bits_SOUND_PLAYER,
		"Repel"
	},
};


//=========================================================
// repel 
//=========================================================
Task_t	tlGruntAllyRepelAttack[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_FLY 	},
};

Schedule_t	slGruntAllyRepelAttack[] =
{
	{
		tlGruntAllyRepelAttack,
		ARRAYSIZE(tlGruntAllyRepelAttack),
		bits_COND_ENEMY_OCCLUDED,
		0,
		"Repel Attack"
	},
};

//=========================================================
// repel land
//=========================================================
Task_t	tlGruntAllyRepelLand[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_LAND	},
	{ TASK_GET_PATH_TO_LASTPOSITION,(float)0				},
	{ TASK_RUN_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_CLEAR_LASTPOSITION,		(float)0				},
};

Schedule_t	slGruntAllyRepelLand[] =
{
	{
		tlGruntAllyRepelLand,
		ARRAYSIZE(tlGruntAllyRepelLand),
		bits_COND_SEE_ENEMY |
		bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER |
		bits_SOUND_COMBAT |
		bits_SOUND_PLAYER,
		"Repel Land"
	},
};

Task_t	tlGruntAllyFollow[] =
{
	{ TASK_MOVE_TO_TARGET_RANGE,(float)128		},	// Move within 128 of target ent (client)
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_FACE },
};

Schedule_t	slGruntAllyFollow[] =
{
	{
		tlGruntAllyFollow,
		ARRAYSIZE(tlGruntAllyFollow),
		bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"Follow"
	},
};

Task_t	tlGruntAllyFaceTarget[] =
{
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_FACE_TARGET,			(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_CHASE },
};

Schedule_t	slGruntAllyFaceTarget[] =
{
	{
		tlGruntAllyFaceTarget,
		ARRAYSIZE(tlGruntAllyFaceTarget),
		bits_COND_CLIENT_PUSH |
		bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"FaceTarget"
	},
};

Task_t	tlGruntAllyIdleStand[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)2		}, // repick IDLESTAND every two seconds.
	{ TASK_TLK_HEADRESET,		(float)0		}, // reset head position
};

Schedule_t	slGruntAllyIdleStand[] =
{
	{
		tlGruntAllyIdleStand,
		ARRAYSIZE(tlGruntAllyIdleStand),
		bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND |
		bits_COND_SMELL |
		bits_COND_PROVOKED,

		bits_SOUND_COMBAT |// sound flags - change these, and you'll break the talking code.
		//bits_SOUND_PLAYER		|
		//bits_SOUND_WORLD		|

		bits_SOUND_DANGER |
		bits_SOUND_MEAT |// scents
		bits_SOUND_CARCASS |
		bits_SOUND_GARBAGE,
		"IdleStand"
	},
};

DEFINE_CUSTOM_SCHEDULES(CSAS)
{
	slGruntAllyFollow,
		slGruntAllyFaceTarget,
		slGruntAllyIdleStand,
		slGruntAllyFail,
		slGruntAllyCombatFail,
		slGruntAllyVictoryDance,
		slGruntAllyEstablishLineOfFire,
		slGruntAllyFoundEnemy,
		slGruntAllyCombatFace,
		slGruntAllySignalSuppress,
		slGruntAllySuppress,
		slGruntAllyWaitInCover,
		slGruntAllyTakeCover,
		slGruntAllyGrenadeCover,
		slGruntAllyTossGrenadeCover,
		slGruntAllyTakeCoverFromBestSound,
		slGruntAllyHideReload,
		slGruntAllySweep,
		slGruntAllyRangeAttack1A,
		slGruntAllyRangeAttack1B,
		slGruntAllyRangeAttack2,
		slGruntAllyRepel,
		slGruntAllyRepelAttack,
		slGruntAllyRepelLand,
};

IMPLEMENT_CUSTOM_SCHEDULES(CSAS, COFSquadTalkMonster);

//=========================================================
// SetActivity 
//=========================================================
void CSAS::SetActivity(Activity NewActivity)
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;
	void* pmodel = GET_MODEL_PTR(ENT(pev));

	switch (NewActivity)
	{
	case ACT_RANGE_ATTACK1:
		// grunt is either shooting standing or shooting crouched
		if (FBitSet(pev->weapons, SASWeaponFlag::MP5))
		{
			if (m_fStanding)
			{
				// get aimable sequence
				iSequence = LookupSequence("standing_mp5");
			}
			else
			{
				// get crouching shoot
				iSequence = LookupSequence("crouching_mp5");
			}
		}
		else if (FBitSet(pev->weapons, SASWeaponFlag::AR16))
		{
			if (m_fStanding)
			{
				// get aimable sequence
				//iSequence = LookupSequence("standing_AR16");
				iSequence = LookupSequence("standing_mp5");
			}
			else
			{
				// get crouching shoot
				//iSequence = LookupSequence("crouching_AR16");
				iSequence = LookupSequence("crouching_mp5");
			}
		}
		else
		{
			if (m_fStanding)
			{
				// get aimable sequence
				iSequence = LookupSequence("standing_shotgun");
			}
			else
			{
				// get crouching shoot
				iSequence = LookupSequence("crouching_shotgun");
			}
		}
		break;
	case ACT_RANGE_ATTACK2:
		// grunt is going to a secondary long range attack. This may be a thrown 
		// grenade or fired grenade, we must determine which and pick proper sequence
		if (pev->weapons & SASWeaponFlag::HandGrenade)
		{
			// get toss anim
			iSequence = LookupSequence("throwgrenade");
		}
		else
		{
			// get launch anim
			iSequence = LookupSequence("launchgrenade");
		}
		break;
	case ACT_RUN:
		if (pev->health <= HSAS_LIMP_HEALTH)
		{
			// limp!
			iSequence = LookupActivity(ACT_RUN_HURT);
		}
		else
		{
			iSequence = LookupActivity(NewActivity);
		}
		break;
	case ACT_WALK:
		if (pev->health <= HSAS_LIMP_HEALTH)
		{
			// limp!
			iSequence = LookupActivity(ACT_WALK_HURT);
		}
		else
		{
			iSequence = LookupActivity(NewActivity);
		}
		break;
	case ACT_IDLE:
		if (m_MonsterState == MONSTERSTATE_COMBAT)
		{
			NewActivity = ACT_IDLE_ANGRY;
		}
		iSequence = LookupActivity(NewActivity);
		break;
	default:
		iSequence = LookupActivity(NewActivity);
		break;
	}

	m_Activity = NewActivity; // Go ahead and set this so it doesn't keep trying when the anim is not present

	// Set to the desired anim, or default anim if the desired is not present
	if (iSequence > ACTIVITY_NOT_AVAILABLE)
	{
		if (pev->sequence != iSequence || !m_fSequenceLoops)
		{
			pev->frame = 0;
		}

		pev->sequence = iSequence;	// Set to the reset anim (if it's there)
		ResetSequenceInfo();
		SetYawSpeed();
	}
	else
	{
		// Not available try to get default anim
		ALERT(at_console, "%s has no sequence for act:%d\n", STRING(pev->classname), NewActivity);
		pev->sequence = 0;	// Set to the reset anim (if it's there)
	}
}

//=========================================================
// Get Schedule!
//=========================================================
Schedule_t* CSAS::GetSchedule(void)
{

	// clear old sentence
	m_iSentence = HSAS_SENT_NONE;

	// flying? If PRONE, barnacle has me. IF not, it's assumed I am rapelling. 
	if (pev->movetype == MOVETYPE_FLY && m_MonsterState != MONSTERSTATE_PRONE)
	{
		if (pev->flags & FL_ONGROUND)
		{
			// just landed
			pev->movetype = MOVETYPE_STEP;
			return GetScheduleOfType(SCHED_SAS_REPEL_LAND);
		}
		else
		{
			// repel down a rope, 
			if (m_MonsterState == MONSTERSTATE_COMBAT)
				return GetScheduleOfType(SCHED_SAS_REPEL_ATTACK);
			else
				return GetScheduleOfType(SCHED_SAS_REPEL);
		}
	}

	// grunts place HIGH priority on running away from danger sounds.
	if (HasConditions(bits_COND_HEAR_SOUND))
	{
		CSound* pSound;
		pSound = PBestSound();

		ASSERT(pSound != NULL);
		if (pSound)
		{
			if (pSound->m_iType & bits_SOUND_DANGER)
			{
				// dangerous sound nearby!

				//!!!KELLY - currently, this is the grunt's signal that a grenade has landed nearby,
				// and the grunt should find cover from the blast
				// good place for "SHIT!" or some other colorful verbal indicator of dismay.
				// It's not safe to play a verbal order here "Scatter", etc cause 
				// this may only affect a single individual in a squad. 

				if (FOkToSpeak())
				{
					SENTENCEG_PlayRndSz(ENT(pev), "SAS_GREN", HSAS_SENTENCE_VOLUME, SAS_ATTN, 0, m_voicePitch);
					JustSpoke();
				}
				return GetScheduleOfType(SCHED_TAKE_COVER_FROM_BEST_SOUND);
			}
			/*
			if (!HasConditions( bits_COND_SEE_ENEMY ) && ( pSound->m_iType & (bits_SOUND_PLAYER | bits_SOUND_COMBAT) ))
			{
				MakeIdealYaw( pSound->m_vecOrigin );
			}
			*/
		}
	}
	switch (m_MonsterState)
	{
	case MONSTERSTATE_COMBAT:
	{
		// dead enemy
		if (HasConditions(bits_COND_ENEMY_DEAD))
		{
			if (FOkToSpeak())
			{
				PlaySentence("SAS_KILL", 4, VOL_NORM, ATTN_NORM);
			}

			// call base class, all code to handle dead enemies is centralized there.
			return COFSquadTalkMonster::GetSchedule();
		}

		if (m_hWaitMedic)
		{
			auto pMedic = m_hWaitMedic.Entity<COFSquadTalkMonster>();

			if (pMedic->pev->deadflag != DEAD_NO)
				m_hWaitMedic = nullptr;
			else
				pMedic->HealMe(nullptr);

			m_flMedicWaitTime = gpGlobals->time + 5.0;
		}

		// new enemy
					//Do not fire until fired upon
		if (HasAllConditions(bits_COND_NEW_ENEMY | bits_COND_LIGHT_DAMAGE))
		{
			if (InSquad())
			{
				MySquadLeader()->m_fEnemyEluded = FALSE;

				if (!IsLeader())
				{
					return GetScheduleOfType(SCHED_TAKE_COVER_FROM_ENEMY);
				}
				else
				{
					//!!!KELLY - the leader of a squad of grunts has just seen the player or a 
					// monster and has made it the squad's enemy. You
					// can check pev->flags for FL_CLIENT to determine whether this is the player
					// or a monster. He's going to immediately start
					// firing, though. If you'd like, we can make an alternate "first sight" 
					// schedule where the leader plays a handsign anim
					// that gives us enough time to hear a short sentence or spoken command
					// before he starts pluggin away.
					if (FOkToSpeak())// && RANDOM_LONG(0,1))
					{
						if ((m_hEnemy != NULL) && m_hEnemy->IsPlayer())
							// player
							SENTENCEG_PlayRndSz(ENT(pev), "SAS_ALERT", HSAS_SENTENCE_VOLUME, SAS_ATTN, 0, m_voicePitch);
						else if ((m_hEnemy != NULL) &&
							(m_hEnemy->Classify() != CLASS_PLAYER_ALLY) &&
							(m_hEnemy->Classify() != CLASS_HUMAN_PASSIVE) &&
							(m_hEnemy->Classify() != CLASS_MACHINE))
							// monster
							SENTENCEG_PlayRndSz(ENT(pev), "SAS_MONST", HSAS_SENTENCE_VOLUME, SAS_ATTN, 0, m_voicePitch);

						JustSpoke();
					}

					if (HasConditions(bits_COND_CAN_RANGE_ATTACK1))
					{
						return GetScheduleOfType(SCHED_SAS_SUPPRESS);
					}
					else
					{
						return GetScheduleOfType(SCHED_SAS_ESTABLISH_LINE_OF_FIRE);
					}
				}
			}

			return GetScheduleOfType(SCHED_SMALL_FLINCH);
		}

		else if (HasConditions(bits_COND_HEAVY_DAMAGE))
			return GetScheduleOfType(SCHED_TAKE_COVER_FROM_ENEMY);
		// no ammo
					//Only if the grunt has a weapon
		else if (pev->weapons && HasConditions(bits_COND_NO_AMMO_LOADED))
		{
			//!!!KELLY - this individual just realized he's out of bullet ammo. 
			// He's going to try to find cover to run to and reload, but rarely, if 
			// none is available, he'll drop and reload in the open here. 
			return GetScheduleOfType(SCHED_SAS_COVER_AND_RELOAD);
		}

		// damaged just a little
		else if (HasConditions(bits_COND_LIGHT_DAMAGE))
		{
			// if hurt:
			// 90% chance of taking cover
			// 10% chance of flinch.
			int iPercent = RANDOM_LONG(0, 99);

			if (iPercent <= 90 && m_hEnemy != NULL)
			{
				// only try to take cover if we actually have an enemy!

				//!!!KELLY - this grunt was hit and is going to run to cover.
				if (FOkToSpeak()) // && RANDOM_LONG(0,1))
				{
					//SENTENCEG_PlayRndSz( ENT(pev), "SAS_COVER", HSAS_SENTENCE_VOLUME, SAS_ATTN, 0, m_voicePitch);
					m_iSentence = HSAS_SENT_COVER;
					//JustSpoke();
				}
				return GetScheduleOfType(SCHED_TAKE_COVER_FROM_ENEMY);
			}
			else
			{
				return GetScheduleOfType(SCHED_SMALL_FLINCH);
			}
		}
		// can kick
		else if (HasConditions(bits_COND_CAN_MELEE_ATTACK1))
		{
			return GetScheduleOfType(SCHED_MELEE_ATTACK1);
		}
		// can grenade launch

		else if (FBitSet(pev->weapons, SASWeaponFlag::GrenadeLauncher) && HasConditions(bits_COND_CAN_RANGE_ATTACK2) && OccupySlot(bits_SLOTS_HGRUNT_GRENADE))
		{
			// shoot a grenade if you can
			return GetScheduleOfType(SCHED_RANGE_ATTACK2);
		}
		// can shoot
		else if (HasConditions(bits_COND_CAN_RANGE_ATTACK1))
		{
			if (InSquad())
			{
				// if the enemy has eluded the squad and a squad member has just located the enemy
				// and the enemy does not see the squad member, issue a call to the squad to waste a 
				// little time and give the player a chance to turn.
				if (MySquadLeader()->m_fEnemyEluded && !HasConditions(bits_COND_ENEMY_FACING_ME))
				{
					MySquadLeader()->m_fEnemyEluded = FALSE;
					return GetScheduleOfType(SCHED_SAS_FOUND_ENEMY);
				}
			}

			if (OccupySlot(bits_SLOTS_HGRUNT_ENGAGE))
			{
				// try to take an available ENGAGE slot
				return GetScheduleOfType(SCHED_RANGE_ATTACK1);
			}
			else if (HasConditions(bits_COND_CAN_RANGE_ATTACK2) && OccupySlot(bits_SLOTS_HGRUNT_GRENADE))
			{
				// throw a grenade if can and no engage slots are available
				return GetScheduleOfType(SCHED_RANGE_ATTACK2);
			}
			else
			{
				// hide!
				return GetScheduleOfType(SCHED_TAKE_COVER_FROM_ENEMY);
			}
		}
		// can't see enemy
		else if (HasConditions(bits_COND_ENEMY_OCCLUDED))
		{
			if (HasConditions(bits_COND_CAN_RANGE_ATTACK2) && OccupySlot(bits_SLOTS_HGRUNT_GRENADE))
			{
				//!!!KELLY - this grunt is about to throw or fire a grenade at the player. Great place for "fire in the hole"  "frag out" etc
				if (FOkToSpeak())
				{
					SENTENCEG_PlayRndSz(ENT(pev), "SAS_THROW", HSAS_SENTENCE_VOLUME, SAS_ATTN, 0, m_voicePitch);
					JustSpoke();
				}
				return GetScheduleOfType(SCHED_RANGE_ATTACK2);
			}
			else if (OccupySlot(bits_SLOTS_HGRUNT_ENGAGE))
			{
				//!!!KELLY - grunt cannot see the enemy and has just decided to 
				// charge the enemy's position. 
				if (FOkToSpeak())// && RANDOM_LONG(0,1))
				{
					//SENTENCEG_PlayRndSz( ENT(pev), "SAS_CHARGE", HSAS_SENTENCE_VOLUME, SAS_ATTN, 0, m_voicePitch);
					m_iSentence = HSAS_SENT_CHARGE;
					//JustSpoke();
				}

				return GetScheduleOfType(SCHED_SAS_ESTABLISH_LINE_OF_FIRE);
			}
			else
			{
				//!!!KELLY - grunt is going to stay put for a couple seconds to see if
				// the enemy wanders back out into the open, or approaches the
				// grunt's covered position. Good place for a taunt, I guess?
				if (FOkToSpeak() && RANDOM_LONG(0, 1))
				{
					SENTENCEG_PlayRndSz(ENT(pev), "SAS_TAUNT", HSAS_SENTENCE_VOLUME, SAS_ATTN, 0, m_voicePitch);
					JustSpoke();
				}
				return GetScheduleOfType(SCHED_STANDOFF);
			}
		}

		//Only if not following a player
		if (!m_hTargetEnt || !m_hTargetEnt->IsPlayer())
		{
			if (HasConditions(bits_COND_SEE_ENEMY) && !HasConditions(bits_COND_CAN_RANGE_ATTACK1))
			{
				return GetScheduleOfType(SCHED_SAS_ESTABLISH_LINE_OF_FIRE);
			}
		}

		//Don't fall through to idle schedules
		break;
	}

	case MONSTERSTATE_ALERT:
	case MONSTERSTATE_IDLE:
		if (HasConditions(bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE))
		{
			// flinch if hurt
			return GetScheduleOfType(SCHED_SMALL_FLINCH);
		}

		//if we're not waiting on a medic and we're hurt, call out for a medic
		if (!m_hWaitMedic
			&& gpGlobals->time > m_flMedicWaitTime
			&& pev->health <= 20.0)
		{
			auto pMedic = MySquadMedic();

			if (!pMedic)
			{
				pMedic = FindSquadMedic(1024);
			}

			if (pMedic)
			{
				if (pMedic->pev->deadflag == DEAD_NO)
				{
					ALERT(at_aiconsole, "Injured Grunt found Medic\n");

					if (pMedic->HealMe(this))
					{
						ALERT(at_aiconsole, "Injured Grunt called for Medic\n");

						EMIT_SOUND_DYN(edict(), CHAN_VOICE, "sas/help04.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);

						JustSpoke();
						m_flMedicWaitTime = gpGlobals->time + 5.0;
					}
				}
			}
		}

		if (m_hEnemy == NULL && IsFollowing())
		{
			if (!m_hTargetEnt->IsAlive())
			{
				// UNDONE: Comment about the recently dead player here?
				StopFollowing(FALSE);
				break;
			}
			else
			{
				if (HasConditions(bits_COND_CLIENT_PUSH))
				{
					return GetScheduleOfType(SCHED_MOVE_AWAY_FOLLOW);
				}
				return GetScheduleOfType(SCHED_TARGET_FACE);
			}
		}

		if (HasConditions(bits_COND_CLIENT_PUSH))
		{
			return GetScheduleOfType(SCHED_MOVE_AWAY);
		}

		// try to say something about smells
		TrySmellTalk();
		break;
	}

	// no special cases here, call the base class
	return COFSquadTalkMonster::GetSchedule();
}

//=========================================================
//=========================================================
Schedule_t* CSAS::GetScheduleOfType(int Type)
{
	switch (Type)
	{
	case SCHED_TAKE_COVER_FROM_ENEMY:
	{
		if (InSquad())
		{
			if (g_iSkillLevel == SKILL_HARD && HasConditions(bits_COND_CAN_RANGE_ATTACK2) && OccupySlot(bits_SLOTS_HGRUNT_GRENADE))
			{
				if (FOkToSpeak())
				{
					SENTENCEG_PlayRndSz(ENT(pev), "SAS_THROW", HSAS_SENTENCE_VOLUME, SAS_ATTN, 0, m_voicePitch);
					JustSpoke();
				}
				return slGruntAllyTossGrenadeCover;
			}
			else
			{
				return &slGruntAllyTakeCover[0];
			}
		}
		else
		{
			//if ( RANDOM_LONG(0,1) )
			//{
			return &slGruntAllyTakeCover[0];
			//}
			//else
			//{
			//	return &slGruntAllyGrenadeCover[ 0 ];
			//}
		}
	}
	case SCHED_TAKE_COVER_FROM_BEST_SOUND:
	{
		return &slGruntAllyTakeCoverFromBestSound[0];
	}
	case SCHED_SAS_TAKECOVER_FAILED:
	{
		if (HasConditions(bits_COND_CAN_RANGE_ATTACK1) && OccupySlot(bits_SLOTS_HGRUNT_ENGAGE))
		{
			return GetScheduleOfType(SCHED_RANGE_ATTACK1);
		}

		return GetScheduleOfType(SCHED_FAIL);
	}
	break;
	case SCHED_SAS_ELOF_FAIL:
	{
		// human grunt is unable to move to a position that allows him to attack the enemy.
		return GetScheduleOfType(SCHED_TAKE_COVER_FROM_ENEMY);
	}
	break;
	case SCHED_SAS_ESTABLISH_LINE_OF_FIRE:
	{
		return &slGruntAllyEstablishLineOfFire[0];
	}
	break;
	case SCHED_RANGE_ATTACK1:
	{
		//Always stand when using AR16
		if (pev->weapons & SASWeaponFlag::AR16)
		{
			m_fStanding = true;
			return &slGruntAllyRangeAttack1B[0];
		}

		// randomly stand or crouch
		if (RANDOM_LONG(0, 9) == 0)
			m_fStanding = RANDOM_LONG(0, 1);

		if (m_fStanding)
			return &slGruntAllyRangeAttack1B[0];
		else
			return &slGruntAllyRangeAttack1A[0];
	}
	case SCHED_RANGE_ATTACK2:
	{
		return &slGruntAllyRangeAttack2[0];
	}
	case SCHED_COMBAT_FACE:
	{
		return &slGruntAllyCombatFace[0];
	}
	case SCHED_SAS_WAIT_FACE_ENEMY:
	{
		return &slGruntAllyWaitInCover[0];
	}
	case SCHED_SAS_SWEEP:
	{
		return &slGruntAllySweep[0];
	}
	case SCHED_SAS_COVER_AND_RELOAD:
	{
		return &slGruntAllyHideReload[0];
	}
	case SCHED_SAS_FOUND_ENEMY:
	{
		return &slGruntAllyFoundEnemy[0];
	}
	case SCHED_VICTORY_DANCE:
	{
		if (InSquad())
		{
			if (!IsLeader())
			{
				if (MySquadLeader()->m_deadMates > 2)
				{
					//Sad Victory Dance Taunt
					MySquadLeader()->PlaySentence("SAS_VDNSD", 4, VOL_NORM, ATTN_NORM);
				}
				else if (MySquadLeader()->m_deadMates == 2)
				{
					//"That Was Close" Victory Dance Taunt
					MySquadLeader()->PlaySentence("SAS_VDNCL", 4, VOL_NORM, ATTN_NORM);
					}
				else
				{
					//Happy Victory Dance Taunt
					MySquadLeader()->PlaySentence("SAS_VDNHP", 4, VOL_NORM, ATTN_NORM);
				}
				return &slGruntAllyFail[0];
			}
		}

		return &slGruntAllyVictoryDance[0];
	}

	case SCHED_SAS_SUPPRESS:
	{
		if (m_hEnemy->IsPlayer() && m_fFirstEncounter)
		{
			m_fFirstEncounter = FALSE;// after first encounter, leader won't issue handsigns anymore when he has a new enemy
			return &slGruntAllySignalSuppress[0];
		}
		else
		{
			return &slGruntAllySuppress[0];
		}
	}
	case SCHED_FAIL:
	{
		if (m_hEnemy != NULL)
		{
			// grunt has an enemy, so pick a different default fail schedule most likely to help recover.
			return &slGruntAllyCombatFail[0];
		}

		return &slGruntAllyFail[0];
	}
	case SCHED_SAS_REPEL:
	{
		if (pev->velocity.z > -128)
			pev->velocity.z -= 32;
		return &slGruntAllyRepel[0];
	}
	case SCHED_SAS_REPEL_ATTACK:
	{
		if (pev->velocity.z > -128)
			pev->velocity.z -= 32;
		return &slGruntAllyRepelAttack[0];
	}
	case SCHED_SAS_REPEL_LAND:
	{
		return &slGruntAllyRepelLand[0];
	}

	case SCHED_TARGET_CHASE:
		return slGruntAllyFollow;

	case SCHED_TARGET_FACE:
	{
		auto pSchedule = COFSquadTalkMonster::GetScheduleOfType(SCHED_TARGET_FACE);

		if (pSchedule == slIdleStand)
			return slGruntAllyFaceTarget;
		return pSchedule;
	}

	case SCHED_IDLE_STAND:
	{
		auto pSchedule = COFSquadTalkMonster::GetScheduleOfType(SCHED_IDLE_STAND);

		if (pSchedule == slIdleStand)
			return slGruntAllyIdleStand;
		return pSchedule;
	}

	case SCHED_CANT_FOLLOW:
		return &slGruntAllyFail[0];

	default:
	{
		return COFSquadTalkMonster::GetScheduleOfType(Type);
	}
	}
}

int CSAS::ObjectCaps()
{
	return FCAP_ACROSS_TRANSITION | FCAP_IMPULSE_USE;
}

void CSAS::TalkInit()
{
	COFSquadTalkMonster::TalkInit();

	m_szGrp[TLK_ANSWER] = "SAS_ANSWER";
	m_szGrp[TLK_QUESTION] = "SAS_QUESTION";
	m_szGrp[TLK_IDLE] = "SAS_IDLE";
	m_szGrp[TLK_STARE] = "SAS_STARE";
	m_szGrp[TLK_USE] = "SAS_OK";
	m_szGrp[TLK_UNUSE] = "SAS_WAIT";
	m_szGrp[TLK_STOP] = "SAS_STOP";

	m_szGrp[TLK_NOSHOOT] = "SAS_FSHOT";
	m_szGrp[TLK_FRKILL] = "SAS_FKIL";
	m_szGrp[TLK_HELLO] = "SAS_ALERT";

	m_szGrp[TLK_PLHURT1] = "!SAS_CUREA";
	m_szGrp[TLK_PLHURT2] = "!SAS_CUREB";
	m_szGrp[TLK_PLHURT3] = "!SAS_CUREC";

	m_szGrp[TLK_PHELLO] = NULL;	//"BA_PHELLO";		// UNDONE
	m_szGrp[TLK_PIDLE] = NULL;	//"BA_PIDLE";			// UNDONE
	m_szGrp[TLK_PQUESTION] = "SAS_PQUEST";		// UNDONE

	m_szGrp[TLK_SMELL] = "SAS_SMELL";

	m_szGrp[TLK_WOUND] = "SAS_WOUND";
	m_szGrp[TLK_MORTAL] = "SAS_MORTAL";

	// get voice for head - just one voice for now
	m_voicePitch = 100;
}

void CSAS::AlertSound()
{
	if (m_hEnemy && FOkToSpeak())
	{
		PlaySentence("SAS_ATTACK", RANDOM_FLOAT(2.8, 3.2), VOL_NORM, ATTN_NORM);
	}
}

void CSAS::DeclineFollowing()
{
	PlaySentence("SAS_POK", 2, VOL_NORM, ATTN_NORM);
}

void CSAS::ShootAR16()
{
	if (m_hEnemy == NULL)
	{
		return;
	}

	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy(vecShootOrigin);

	UTIL_MakeVectors(pev->angles);

	switch (RANDOM_LONG(0, 1))
	{
	case 0:
	{
		auto vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(75, 200) + gpGlobals->v_up * RANDOM_FLOAT(150, 200) + gpGlobals->v_forward * 25.0;
		EjectBrass(vecShootOrigin - vecShootDir * 6, vecShellVelocity, pev->angles.y, m_iAR16Link, TE_BOUNCE_SHELL);
		break;
	}

	case 1:
	{
		auto vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(100, 250) + gpGlobals->v_up * RANDOM_FLOAT(100, 150) + gpGlobals->v_forward * 25.0;
		EjectBrass(vecShootOrigin - vecShootDir * 6, vecShellVelocity, pev->angles.y, m_iAR16Shell, TE_BOUNCE_SHELL);
		break;
	}
	}

	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_5DEGREES, 8192, BULLET_PLAYER_556, 2); // shoot +-5 degrees

	switch (RANDOM_LONG(0, 2))
	{
	case 0: EMIT_SOUND_DYN(edict(), CHAN_WEAPON, "weapons/AR16_fire1.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(0, 15) + 94); break;
	case 1: EMIT_SOUND_DYN(edict(), CHAN_WEAPON, "weapons/AR16_fire2.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(0, 15) + 94); break;
	case 2: EMIT_SOUND_DYN(edict(), CHAN_WEAPON, "weapons/AR16_fire3.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(0, 15) + 94); break;
	}

	pev->effects |= EF_MUZZLEFLASH;

	m_cAmmoLoaded--;// take away a bullet!

	Vector angDir = UTIL_VecToAngles(vecShootDir);
	SetBlending(0, angDir.x);
}

void CSAS::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "head"))
	{
		m_iGruntHead = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		COFSquadTalkMonster::KeyValue(pkvd);
}

void CSAS::Killed(entvars_t* pevAttacker, int iGib)
{
	if (m_MonsterState != MONSTERSTATE_DEAD)
	{
		if (HasMemory(bits_MEMORY_SUSPICIOUS) || IsFacing(pevAttacker, pev->origin))
		{
			Remember(bits_MEMORY_PROVOKED);

			StopFollowing(true);
		}
	}

	if (m_hWaitMedic)
	{
		auto v4 = m_hWaitMedic.Entity<COFSquadTalkMonster>();
		if (v4->pev->deadflag)
			m_hWaitMedic = nullptr;
		else
			v4->HealMe(nullptr);
	}

	SetUse(nullptr);
	COFSquadTalkMonster::Killed(pevAttacker, iGib);
}

//=========================================================
// CSASRepel - when triggered, spawns a monster_sas
// repelling down a line.
//=========================================================

class CSASRepel : public CBaseMonster
{
public:
	void KeyValue(KeyValueData* pkvd) override;

	void Spawn(void);
	void Precache(void);
	void EXPORT RepelUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);
	int m_iSpriteTexture;	// Don't save, precache

	//TODO: needs save/restore (not in op4)
	int m_iGruntHead;
	int m_iszUse;
	int m_iszUnUse;
};

LINK_ENTITY_TO_CLASS(monster_sas_repel, CSASRepel);

void CSASRepel::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "head"))
	{
		m_iGruntHead = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "UseSentence"))
	{
		m_iszUse = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "UnUseSentence"))
	{
		m_iszUnUse = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue(pkvd);
}

void CSASRepel::Spawn(void)
{
	Precache();
	pev->solid = SOLID_NOT;

	SetUse(&CSASRepel::RepelUse);
}

void CSASRepel::Precache(void)
{
	UTIL_PrecacheOther("monster_sas");
	m_iSpriteTexture = PRECACHE_MODEL("sprites/rope.spr");
}

void CSASRepel::RepelUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	TraceResult tr;
	UTIL_TraceLine(pev->origin, pev->origin + Vector(0, 0, -4096.0), dont_ignore_monsters, ENT(pev), &tr);
	/*
	if ( tr.pHit && Instance( tr.pHit )->pev->solid != SOLID_BSP)
		return NULL;
	*/

	CBaseEntity* pEntity = Create("monster_sas", pev->origin, pev->angles);
	auto pGrunt = static_cast<CSAS*>(pEntity->MySquadTalkMonsterPointer());

	if (pGrunt)
	{
		pGrunt->pev->weapons = pev->weapons;
		pGrunt->pev->netname = pev->netname;

		//Carry over these spawn flags
		pGrunt->pev->spawnflags |= pev->spawnflags
			& (SF_MONSTER_WAIT_TILL_SEEN
				| SF_MONSTER_GAG
				| SF_MONSTER_HITMONSTERCLIP
				| SF_MONSTER_PRISONER
				| SF_SQUADMONSTER_LEADER
				| SF_MONSTER_PREDISASTER);

		pGrunt->m_iGruntHead = m_iGruntHead;
		pGrunt->m_iszUse = m_iszUse;
		pGrunt->m_iszUnUse = m_iszUnUse;

		//Run logic to set up body groups (Spawn was already called once by Create above)
		pGrunt->Spawn();

		pGrunt->pev->movetype = MOVETYPE_FLY;
		pGrunt->pev->velocity = Vector(0, 0, RANDOM_FLOAT(-196, -128));
		pGrunt->SetActivity(ACT_GLIDE);
		// UNDONE: position?
		pGrunt->m_vecLastPosition = tr.vecEndPos;

		CBeam* pBeam = CBeam::BeamCreate("sprites/rope.spr", 10);
		pBeam->PointEntInit(pev->origin + Vector(0, 0, 112), pGrunt->entindex());
		pBeam->SetFlags(BEAM_FSOLID);
		pBeam->SetColor(255, 255, 255);
		pBeam->SetThink(&CBeam::SUB_Remove);
		pBeam->pev->nextthink = gpGlobals->time + -4096.0 * tr.flFraction / pGrunt->pev->velocity.z + 0.5;

		UTIL_Remove(this);
	}
}



//=========================================================
// DEAD HGRUNT PROP
//=========================================================
class CDeadSAS : public CBaseMonster
{
public:
	void Spawn(void);
	int	Classify(void) { return	CLASS_PLAYER_ALLY; }

	void KeyValue(KeyValueData* pkvd);

	int	m_iPose;// which sequence to display	-- temporary, don't need to save
	int m_iGruntHead;
	static char* m_szPoses[7];
};

char* CDeadSAS::m_szPoses[] = { "deadstomach", "deadside", "deadsitting", "dead_on_back", "HSAS_dead_stomach", "dead_headcrabed", "dead_canyon" };

void CDeadSAS::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "head"))
	{
		m_iGruntHead = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue(pkvd);
}

LINK_ENTITY_TO_CLASS(monster_sas_dead, CDeadSAS);

//=========================================================
// ********** DeadSAS SPAWN **********
//=========================================================
void CDeadSAS::Spawn(void)
{
	PRECACHE_MODEL("models/sas.mdl");
	SET_MODEL(ENT(pev), "models/sas.mdl");

	pev->effects = 0;
	pev->yaw_speed = 8;
	pev->sequence = 0;
	m_bloodColor = BLOOD_COLOR_RED;

	pev->sequence = LookupSequence(m_szPoses[m_iPose]);

	if (pev->sequence == -1)
	{
		ALERT(at_console, "Dead hgrunt with bad pose\n");
	}

	// Corpses have less health
	pev->health = 8;

	if (pev->weapons & SASWeaponFlag::MP5)
	{
		//SetBodygroup(SASBodygroup::Torso, SASTorso::Normal);
		SetBodygroup(SASBodygroup::Weapons, SASWeapon::MP5);
	}
	else if (pev->weapons & SASWeaponFlag::Shotgun)
	{
		//SetBodygroup(SASBodygroup::Torso, SASTorso::Shotgun);
		SetBodygroup(SASBodygroup::Weapons, SASWeapon::Shotgun);
	}
	else if (pev->weapons & SASWeaponFlag::AR16)
	{
		//SetBodygroup(SASBodygroup::Torso, SASTorso::AR16);
		SetBodygroup(SASBodygroup::Weapons, SASWeapon::AR16);
	}
	else
	{
		//SetBodygroup(SASBodygroup::Torso, SASTorso::Normal);
		SetBodygroup(SASBodygroup::Weapons, SASWeapon::None);
	}

	//SetBodygroup(SASBodygroup::Head, m_iGruntHead);

	pev->skin = 0;

	MonsterInitDead();
}