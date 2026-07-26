class HudText
{
	float m_fNextTextUpdate;
	float m_fLastTextCheck;
	float m_fEffectiveText;

public:
	HudText();
	virtual ~HudText();

	void DrawText( char *text, Vector maxv, Vector minv);
};
