//++ BulliT
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "game.h"
#include "player.h"
#include "hudtimer.h"

const char* g_CountdownSounds[] = {
	"vox/one",
	"vox/two",
	"vox/three",
	"vox/four",
	"vox/five",
	"vox/six",
	"vox/seven",
	"vox/eight",
	"vox/nine",
	"vox/ten"
};

HudTimer::HudTimer()
{
	m_fNextTimerUpdate = gpGlobals->time + 3.0; // Dont start timer directly.
	m_fLastTimeCheck = gpGlobals->time;
	m_fEffectiveTime = 0.0;
	m_pmp_timelimit = CVAR_GET_POINTER("mp_timelimit");
}

HudTimer::~HudTimer()
{
}

void HudTimer::Think()
{
	if (!mp_clock.value)
		return;

	// Calculate effective time
	m_fEffectiveTime += gpGlobals->time - m_fLastTimeCheck;
	m_fLastTimeCheck = gpGlobals->time;

	if (m_fNextTimerUpdate <= m_fEffectiveTime)
	{
		// Sanity time check
		if (timelimit.value > 4320)
			CVAR_SET_FLOAT("mp_timelimit", 4320); // Three days maximum

		// Write the time. (negative turns off timer on client)
		long lTime = (m_pmp_timelimit->value * 60) - m_fEffectiveTime;

		char szTime[128];

		if (lTime > 0 || timelimit.value > 0)
		{
			long days = lTime / 86400;
			long hours = (lTime % 86400) / 3600;
			long minutes = (lTime % 3600) / 60;
			long seconds = lTime % 60;

			if (days > 0)
			{
				sprintf(szTime, "%ldd %ldh %02ldm %02lds", days, hours, minutes, seconds);
			}
			else if (hours > 0)
			{
				sprintf(szTime, "%ldh %02ldm %02lds", hours, minutes, seconds);
			}
			else if (minutes > 0)
			{
				sprintf(szTime, "%ldm %02lds", minutes, seconds);
			}
			else
			{
				sprintf(szTime, "%lds", seconds);
			}

			if (lTime <= 10 && lTime > 0)
			{
				const char* countdownSound = g_CountdownSounds[lTime - 1];
				UTIL_SpeakAll(countdownSound);
			}

			char szText[256];
			sprintf(szText, "Map: %s\nNext: %s\n%s\n", CVAR_GET_STRING( "dm_map" ), CVAR_GET_STRING( "dm_nextmap" ), szTime );

			UTIL_DrawHudMessageAll(CHAN_TIMER, Vector(250, 160, 0), Vector(0, 60, 0), szText);
			m_fNextTimerUpdate += 1;
		}
		else
		{
			char szText[256];
			sprintf(szText, "Map: %s", CVAR_GET_STRING( "dm_map" ));

			UTIL_DrawHudMessageAll(CHAN_TIMER, Vector(250, 160, 0), Vector(0, 60, 0), szText);
			m_fNextTimerUpdate += 1;
		}
	}
}
//-- Martin Webrant
