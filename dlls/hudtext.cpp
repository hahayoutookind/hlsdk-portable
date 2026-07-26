// hahayoutookind
// Macro for CHAN_HUDTEXT
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "game.h"
#include "player.h"
#include "hudtext.h"

HudText::HudText()
{
	m_fNextTextUpdate = gpGlobals->time + 0.1;
	m_fLastTextCheck = gpGlobals->time;
	m_fEffectiveText = 0.0;
}

HudText::~HudText()
{
}

void HudText::DrawText( char *text, Vector maxv, Vector minv)
{
	// Calculate effective time
	m_fEffectiveText += gpGlobals->time - m_fLastTextCheck;
	m_fLastTextCheck = gpGlobals->time;

	char szText[256];
	sprintf(szText, "%c", *text);

	UTIL_DrawHudMessageAll(CHAN_HUDTEXT, maxv, minv, szText);
	m_fNextTextUpdate += 0.1;
}
