/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/math.h>
#include <base/system.h>

#include <engine/graphics.h>
#include <engine/shared/config.h>
#include <engine/shared/linereader.h>
#include <engine/storage.h>
#include <engine/textrender.h>
#include <engine/updater.h>

#include <game/generated/protocol.h>

#include <game/client/animstate.h>
#include <game/client/components/chat.h>
#include <game/client/components/menu_background.h>
#include <game/client/components/sounds.h>
#include <game/client/gameclient.h>
#include <game/client/render.h>
#include <game/client/skin.h>
#include <game/client/ui.h>
#include <game/client/ui_listbox.h>
#include <game/client/ui_scrollregion.h>
#include <game/localization.h>

#include "binds.h"
#include "countryflags.h"
#include "menus.h"
#include "skins.h"

#include <array>
#include <chrono>
#include <memory>
#include <numeric>
#include <string>
#include <vector>

using namespace FontIcons;
using namespace std::chrono_literals;

CMenusKeyBinder CMenus::m_Binder;

CMenusKeyBinder::CMenusKeyBinder()
{
	m_TakeKey = false;
	m_GotKey = false;
	m_ModifierCombination = 0;
}

bool CMenusKeyBinder::OnInput(const IInput::CEvent &Event)
{
	if(m_TakeKey)
	{
		int TriggeringEvent = (Event.m_Key == KEY_MOUSE_1) ? IInput::FLAG_PRESS : IInput::FLAG_RELEASE;
		if(Event.m_Flags & TriggeringEvent)
		{
			m_Key = Event;
			m_GotKey = true;
			m_TakeKey = false;

			m_ModifierCombination = CBinds::GetModifierMask(Input());
			if(m_ModifierCombination == CBinds::GetModifierMaskOfKey(Event.m_Key))
			{
				m_ModifierCombination = 0;
			}
		}
		return true;
	}

	return false;
}

void CMenus::RenderSettingsGeneral(CUIRect MainView)
{
	char aBuf[128 + IO_MAX_PATH_LENGTH];
	CUIRect Label, Button, Left, Right, Game, Client;
	MainView.HSplitTop(150.0f, &Game, &Client);

	// game
	{
		// headline
		Game.HSplitTop(20.0f, &Label, &Game);
		UI()->DoLabel(&Label, Localize("Game"), 20.0f, TEXTALIGN_ML);
		Game.Margin(5.0f, &Game);
		Game.VSplitMid(&Left, &Right);
		Left.VSplitRight(5.0f, &Left, 0);
		Right.VMargin(5.0f, &Right);

		// dynamic camera
		Left.HSplitTop(20.0f, &Button, &Left);
		bool IsDyncam = g_Config.m_ClDyncam || g_Config.m_ClMouseFollowfactor > 0;
		if(DoButton_CheckBox(&g_Config.m_ClDyncam, Localize("Dynamic Camera"), IsDyncam, &Button))
		{
			if(IsDyncam)
			{
				g_Config.m_ClDyncam = 0;
				g_Config.m_ClMouseFollowfactor = 0;
			}
			else
			{
				g_Config.m_ClDyncam = 1;
			}
		}

		// smooth dynamic camera
		Left.HSplitTop(5.0f, 0, &Left);
		Left.HSplitTop(20.0f, &Button, &Left);
		if(g_Config.m_ClDyncam)
		{
			if(DoButton_CheckBox(&g_Config.m_ClDyncamSmoothness, Localize("Smooth Dynamic Camera"), g_Config.m_ClDyncamSmoothness, &Button))
			{
				if(g_Config.m_ClDyncamSmoothness)
				{
					g_Config.m_ClDyncamSmoothness = 0;
				}
				else
				{
					g_Config.m_ClDyncamSmoothness = 50;
					g_Config.m_ClDyncamStabilizing = 50;
				}
			}
		}

		// weapon pickup
		Left.HSplitTop(5.0f, 0, &Left);
		Left.HSplitTop(20.0f, &Button, &Left);
		if(DoButton_CheckBox(&g_Config.m_ClAutoswitchWeapons, Localize("Switch weapon on pickup"), g_Config.m_ClAutoswitchWeapons, &Button))
			g_Config.m_ClAutoswitchWeapons ^= 1;

		// weapon out of ammo autoswitch
		Left.HSplitTop(5.0f, 0, &Left);
		Left.HSplitTop(20.0f, &Button, &Left);
		if(DoButton_CheckBox(&g_Config.m_ClAutoswitchWeaponsOutOfAmmo, Localize("Switch weapon when out of ammo"), g_Config.m_ClAutoswitchWeaponsOutOfAmmo, &Button))
			g_Config.m_ClAutoswitchWeaponsOutOfAmmo ^= 1;
	}

	// client
	{
		// headline
		Client.HSplitTop(20.0f, &Label, &Client);
		UI()->DoLabel(&Label, Localize("Client"), 20.0f, TEXTALIGN_ML);
		Client.Margin(5.0f, &Client);
		Client.VSplitMid(&Left, &Right);
		Left.VSplitRight(5.0f, &Left, 0);
		Right.VMargin(5.0f, &Right);

		// skip main menu
		Left.HSplitTop(5.0f, 0, &Left);
		Left.HSplitTop(20.0f, &Button, &Left);
		if(DoButton_CheckBox(&g_Config.m_ClSkipStartMenu, Localize("Skip the main menu"), g_Config.m_ClSkipStartMenu, &Button))
			g_Config.m_ClSkipStartMenu ^= 1;

		float SliderGroupMargin = 10.0f;

		// auto demo settings
		{
			Right.HSplitTop(40.0f, nullptr, &Right);
			Right.HSplitTop(20.0f, &Button, &Right);
			if(DoButton_CheckBox(&g_Config.m_ClAutoDemoRecord, Localize("Automatically record demos"), g_Config.m_ClAutoDemoRecord, &Button))
				g_Config.m_ClAutoDemoRecord ^= 1;

			Right.HSplitTop(2 * 20.0f, &Button, &Right);
			if(g_Config.m_ClAutoDemoRecord)
				UI()->DoScrollbarOption(&g_Config.m_ClAutoDemoMax, &g_Config.m_ClAutoDemoMax, &Button, Localize("Max demos"), 1, 1000, &CUI::ms_LinearScrollbarScale, CUI::SCROLLBAR_OPTION_INFINITE | CUI::SCROLLBAR_OPTION_MULTILINE);

			Right.HSplitTop(SliderGroupMargin, nullptr, &Right);
			Right.HSplitTop(20.0f, &Button, &Right);
			if(DoButton_CheckBox(&g_Config.m_ClAutoScreenshot, Localize("Automatically take game over screenshot"), g_Config.m_ClAutoScreenshot, &Button))
				g_Config.m_ClAutoScreenshot ^= 1;

			Right.HSplitTop(2 * 20.0f, &Button, &Right);
			if(g_Config.m_ClAutoScreenshot)
				UI()->DoScrollbarOption(&g_Config.m_ClAutoScreenshotMax, &g_Config.m_ClAutoScreenshotMax, &Button, Localize("Max Screenshots"), 1, 1000, &CUI::ms_LinearScrollbarScale, CUI::SCROLLBAR_OPTION_INFINITE | CUI::SCROLLBAR_OPTION_MULTILINE);
		}

		Left.HSplitTop(10.0f, nullptr, &Left);
		Left.HSplitTop(20.0f, &Button, &Left);
		UI()->DoScrollbarOption(&g_Config.m_ClRefreshRate, &g_Config.m_ClRefreshRate, &Button, Localize("Refresh Rate"), 10, 10000, &CUI::ms_LogarithmicScrollbarScale, CUI::SCROLLBAR_OPTION_INFINITE, " Hz");
		Left.HSplitTop(5.0f, nullptr, &Left);
		Left.HSplitTop(20.0f, &Button, &Left);
		int s_LowerRefreshRate;
		if(DoButton_CheckBox(&s_LowerRefreshRate, Localize("Save power by lowering refresh rate (higher input latency)"), g_Config.m_ClRefreshRate <= 480 && g_Config.m_ClRefreshRate != 0, &Button))
			g_Config.m_ClRefreshRate = g_Config.m_ClRefreshRate > 480 || g_Config.m_ClRefreshRate == 0 ? 480 : 0;

		CUIRect SettingsButton;
		Left.HSplitBottom(25.0f, &Left, &SettingsButton);

		SettingsButton.HSplitTop(5.0f, 0, &SettingsButton);
		static CButtonContainer s_SettingsButtonID;
		if(DoButton_Menu(&s_SettingsButtonID, Localize("Settings file"), 0, &SettingsButton))
		{
			Storage()->GetCompletePath(IStorage::TYPE_SAVE, CONFIG_FILE, aBuf, sizeof(aBuf));
			if(!open_file(aBuf))
			{
				dbg_msg("menus", "couldn't open file '%s'", aBuf);
			}
		}
		GameClient()->m_Tooltips.DoToolTip(&s_SettingsButtonID, &SettingsButton, Localize("Open the settings file"));

		Left.HSplitTop(15.0f, 0, &Left);
		CUIRect ConfigButton;
		Left.HSplitBottom(25.0f, &Left, &ConfigButton);

		ConfigButton.HSplitTop(5.0f, 0, &ConfigButton);
		static CButtonContainer s_ConfigButtonID;
		if(DoButton_Menu(&s_ConfigButtonID, Localize("Config directory"), 0, &ConfigButton))
		{
			Storage()->GetCompletePath(IStorage::TYPE_SAVE, "", aBuf, sizeof(aBuf));
			if(!open_file(aBuf))
			{
				dbg_msg("menus", "couldn't open file '%s'", aBuf);
			}
		}
		GameClient()->m_Tooltips.DoToolTip(&s_ConfigButtonID, &ConfigButton, Localize("Open the directory that contains the configuration and user files"));

		Left.HSplitTop(15.0f, 0, &Left);
		CUIRect DirectoryButton;
		Left.HSplitBottom(25.0f, &Left, &DirectoryButton);
		RenderThemeSelection(Left);

		DirectoryButton.HSplitTop(5.0f, 0, &DirectoryButton);
		static CButtonContainer s_ThemesButtonID;
		if(DoButton_Menu(&s_ThemesButtonID, Localize("Themes directory"), 0, &DirectoryButton))
		{
			Storage()->GetCompletePath(IStorage::TYPE_SAVE, "themes", aBuf, sizeof(aBuf));
			Storage()->CreateFolder("themes", IStorage::TYPE_SAVE);
			if(!open_file(aBuf))
			{
				dbg_msg("menus", "couldn't open file '%s'", aBuf);
			}
		}
		GameClient()->m_Tooltips.DoToolTip(&s_ThemesButtonID, &DirectoryButton, Localize("Open the directory to add custom themes"));

		// auto statboard screenshot
		{
			Right.HSplitTop(SliderGroupMargin, nullptr, &Right);
			Right.HSplitTop(20.0f, &Button, &Right);
			if(DoButton_CheckBox(&g_Config.m_ClAutoStatboardScreenshot, Localize("Automatically take statboard screenshot"), g_Config.m_ClAutoStatboardScreenshot, &Button))
			{
				g_Config.m_ClAutoStatboardScreenshot ^= 1;
			}

			Right.HSplitTop(2 * 20.0f, &Button, &Right);
			if(g_Config.m_ClAutoStatboardScreenshot)
				UI()->DoScrollbarOption(&g_Config.m_ClAutoStatboardScreenshotMax, &g_Config.m_ClAutoStatboardScreenshotMax, &Button, Localize("Max Screenshots"), 1, 1000, &CUI::ms_LinearScrollbarScale, CUI::SCROLLBAR_OPTION_INFINITE | CUI::SCROLLBAR_OPTION_MULTILINE);
		}

		// auto statboard csv
		{
			Right.HSplitTop(SliderGroupMargin, nullptr, &Right);
			Right.HSplitTop(20.0f, &Button, &Right);
			if(DoButton_CheckBox(&g_Config.m_ClAutoCSV, Localize("Automatically create statboard csv"), g_Config.m_ClAutoCSV, &Button))
			{
				g_Config.m_ClAutoCSV ^= 1;
			}

			Right.HSplitTop(2 * 20.0f, &Button, &Right);
			if(g_Config.m_ClAutoCSV)
				UI()->DoScrollbarOption(&g_Config.m_ClAutoCSVMax, &g_Config.m_ClAutoCSVMax, &Button, Localize("Max CSVs"), 1, 1000, &CUI::ms_LinearScrollbarScale, CUI::SCROLLBAR_OPTION_INFINITE | CUI::SCROLLBAR_OPTION_MULTILINE);
		}
	}
}

void CMenus::SetNeedSendInfo()
{
	if(m_Dummy)
		m_NeedSendDummyinfo = true;
	else
		m_NeedSendinfo = true;
}

void CMenus::RenderSettingsPlayer(CUIRect MainView)
{
	CUIRect Button, Label, Dummy;
	MainView.HSplitTop(10.0f, 0, &MainView);

	char *pName = g_Config.m_PlayerName;
	const char *pNameFallback = Client()->PlayerName();
	char *pClan = g_Config.m_PlayerClan;
	int *pCountry = &g_Config.m_PlayerCountry;

	if(m_Dummy)
	{
		pName = g_Config.m_ClDummyName;
		pNameFallback = Client()->DummyName();
		pClan = g_Config.m_ClDummyClan;
		pCountry = &g_Config.m_ClDummyCountry;
	}

	// player name
	MainView.HSplitTop(20.0f, &Button, &MainView);
	Button.VSplitLeft(80.0f, &Label, &Button);
	Button.VSplitLeft(150.0f, &Button, 0);
	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "%s:", Localize("Name"));
	UI()->DoLabel(&Label, aBuf, 14.0f, TEXTALIGN_ML);
	static CLineInput s_NameInput;
	s_NameInput.SetBuffer(pName, sizeof(g_Config.m_PlayerName));
	s_NameInput.SetEmptyText(pNameFallback);
	if(UI()->DoEditBox(&s_NameInput, &Button, 14.0f))
	{
		SetNeedSendInfo();
	}

	// player clan
	MainView.HSplitTop(5.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	Button.VSplitLeft(80.0f, &Label, &Button);
	Button.VSplitLeft(200.0f, &Button, &Dummy);
	Button.VSplitLeft(150.0f, &Button, 0);
	str_format(aBuf, sizeof(aBuf), "%s:", Localize("Clan"));
	UI()->DoLabel(&Label, aBuf, 14.0f, TEXTALIGN_ML);
	static CLineInput s_ClanInput;
	s_ClanInput.SetBuffer(pClan, sizeof(g_Config.m_PlayerClan));
	if(UI()->DoEditBox(&s_ClanInput, &Button, 14.0f))
	{
		SetNeedSendInfo();
	}

	if(DoButton_CheckBox(&m_Dummy, Localize("Dummy settings"), m_Dummy, &Dummy))
	{
		m_Dummy ^= 1;
	}

	// country flag selector
	MainView.HSplitTop(20.0f, 0, &MainView);
	int OldSelected = -1;
	static CListBox s_ListBox;
	if(UI()->CheckActiveItem(&s_ClanInput) || UI()->CheckActiveItem(&s_NameInput))
		s_ListBox.SetActive(false);
	s_ListBox.DoStart(50.0f, m_pClient->m_CountryFlags.Num(), 10, 3, OldSelected, &MainView);

	for(size_t i = 0; i < m_pClient->m_CountryFlags.Num(); ++i)
	{
		const CCountryFlags::CCountryFlag *pEntry = m_pClient->m_CountryFlags.GetByIndex(i);
		if(pEntry->m_CountryCode == *pCountry)
			OldSelected = i;

		const CListboxItem Item = s_ListBox.DoNextItem(&pEntry->m_CountryCode, OldSelected >= 0 && (size_t)OldSelected == i);
		if(!Item.m_Visible)
			continue;

		CUIRect FlagRect;
		Item.m_Rect.Margin(5.0f, &FlagRect);
		FlagRect.HSplitBottom(12.0f, &FlagRect, &Label);
		Label.HSplitTop(2.0f, nullptr, &Label);
		float OldWidth = FlagRect.w;
		FlagRect.w = FlagRect.h * 2;
		FlagRect.x += (OldWidth - FlagRect.w) / 2.0f;
		m_pClient->m_CountryFlags.Render(pEntry->m_CountryCode, ColorRGBA(1.0f, 1.0f, 1.0f, 1.0f), FlagRect.x, FlagRect.y, FlagRect.w, FlagRect.h);

		if(pEntry->m_Texture.IsValid())
		{
			UI()->DoLabel(&Label, pEntry->m_aCountryCodeString, 10.0f, TEXTALIGN_MC);
		}
	}

	const int NewSelected = s_ListBox.DoEnd();
	if(OldSelected != NewSelected)
	{
		*pCountry = m_pClient->m_CountryFlags.GetByIndex(NewSelected)->m_CountryCode;
		SetNeedSendInfo();
	}
}

struct CUISkin
{
	const CSkin *m_pSkin;

	CUISkin() :
		m_pSkin(nullptr) {}
	CUISkin(const CSkin *pSkin) :
		m_pSkin(pSkin) {}

	bool operator<(const CUISkin &Other) const { return str_comp_nocase(m_pSkin->GetName(), Other.m_pSkin->GetName()) < 0; }

	bool operator<(const char *pOther) const { return str_comp_nocase(m_pSkin->GetName(), pOther) < 0; }
	bool operator==(const char *pOther) const { return !str_comp_nocase(m_pSkin->GetName(), pOther); }
};

void CMenus::RefreshSkins()
{
	auto SkinStartLoadTime = time_get_nanoseconds();
	m_pClient->m_Skins.Refresh([&](int) {
		// if skin refreshing takes to long, swap to a loading screen
		if(time_get_nanoseconds() - SkinStartLoadTime > 500ms)
		{
			RenderLoading(Localize("Loading skin files"), "", 0, false);
		}
	});
	if(Client()->State() >= IClient::STATE_ONLINE)
	{
		m_pClient->RefindSkins();
	}
}

void CMenus::RandomSkin()
{
	static const float s_aSchemes[] = {1.0f / 2.0f, 1.0f / 3.0f, 1.0f / -3.0f, 1.0f / 12.0f, 1.0f / -12.0f}; // complementary, triadic, analogous

	float GoalSat = random_float(0.3f, 1.0f);
	float MaxBodyLht = 1.0f - GoalSat * GoalSat; // max allowed lightness before we start losing saturation

	ColorHSLA Body;
	Body.h = random_float();
	Body.l = random_float(0.0f, MaxBodyLht);
	Body.s = clamp(GoalSat * GoalSat / (1.0f - Body.l), 0.0f, 1.0f);

	ColorHSLA Feet;
	Feet.h = std::fmod(Body.h + s_aSchemes[rand() % std::size(s_aSchemes)], 1.0f);
	Feet.l = random_float();
	Feet.s = clamp(GoalSat * GoalSat / (1.0f - Feet.l), 0.0f, 1.0f);

	const char *pRandomSkinName = CSkins::VANILLA_SKINS[rand() % (std::size(CSkins::VANILLA_SKINS) - 2)]; // last 2 skins are x_ninja and x_spec

	char *pSkinName = !m_Dummy ? g_Config.m_ClPlayerSkin : g_Config.m_ClDummySkin;
	unsigned *pColorBody = !m_Dummy ? &g_Config.m_ClPlayerColorBody : &g_Config.m_ClDummyColorBody;
	unsigned *pColorFeet = !m_Dummy ? &g_Config.m_ClPlayerColorFeet : &g_Config.m_ClDummyColorFeet;

	mem_copy(pSkinName, pRandomSkinName, sizeof(g_Config.m_ClPlayerSkin));
	*pColorBody = Body.Pack(false);
	*pColorFeet = Feet.Pack(false);

	SetNeedSendInfo();
}

void CMenus::Con_AddFavoriteSkin(IConsole::IResult *pResult, void *pUserData)
{
	auto *pSelf = (CMenus *)pUserData;
	if(pResult->NumArguments() >= 1)
	{
		pSelf->m_SkinFavorites.emplace(pResult->GetString(0));
		pSelf->m_SkinFavoritesChanged = true;
	}
}

void CMenus::Con_RemFavoriteSkin(IConsole::IResult *pResult, void *pUserData)
{
	auto *pSelf = (CMenus *)pUserData;
	if(pResult->NumArguments() >= 1)
	{
		const auto it = pSelf->m_SkinFavorites.find(pResult->GetString(0));
		if(it != pSelf->m_SkinFavorites.end())
		{
			pSelf->m_SkinFavorites.erase(it);
			pSelf->m_SkinFavoritesChanged = true;
		}
	}
}

void CMenus::ConfigSaveCallback(IConfigManager *pConfigManager, void *pUserData)
{
	auto *pSelf = (CMenus *)pUserData;
	pSelf->OnConfigSave(pConfigManager);
}

void CMenus::OnConfigSave(IConfigManager *pConfigManager)
{
	for(const auto &Entry : m_SkinFavorites)
	{
		char aBuffer[256];
		char aNameEscaped[256];
		char *pDst = aNameEscaped;
		str_escape(&pDst, Entry.c_str(), aNameEscaped + std::size(aNameEscaped));
		str_format(aBuffer, std::size(aBuffer), "add_favorite_skin \"%s\"", Entry.c_str());
		pConfigManager->WriteLine(aBuffer);
	}
}

void CMenus::RenderSettingsTee(CUIRect MainView)
{
	CUIRect Button, Label, Dummy, DummyLabel, SkinList, QuickSearch, QuickSearchClearButton, SkinDB, SkinPrefix, SkinPrefixLabel, DirectoryButton, RefreshButton, Eyes, EyesLabel, EyesTee, EyesRight;

	static bool s_InitSkinlist = true;
	Eyes = MainView;

	char *pSkinName = g_Config.m_ClPlayerSkin;
	int *pUseCustomColor = &g_Config.m_ClPlayerUseCustomColor;
	unsigned *pColorBody = &g_Config.m_ClPlayerColorBody;
	unsigned *pColorFeet = &g_Config.m_ClPlayerColorFeet;

	if(m_Dummy)
	{
		pSkinName = g_Config.m_ClDummySkin;
		pUseCustomColor = &g_Config.m_ClDummyUseCustomColor;
		pColorBody = &g_Config.m_ClDummyColorBody;
		pColorFeet = &g_Config.m_ClDummyColorFeet;
	}

	MainView.HSplitTop(10.0f, &Label, &MainView);
	Label.VSplitLeft(280.0f, &Label, &Dummy);
	Label.VSplitLeft(230.0f, &Label, 0);
	Dummy.VSplitLeft(170.0f, &Dummy, &SkinPrefix);
	SkinPrefix.VSplitLeft(120.0f, &SkinPrefix, &EyesRight);
	char aBuf[128 + IO_MAX_PATH_LENGTH];
	str_format(aBuf, sizeof(aBuf), "%s:", Localize("Your skin"));
	UI()->DoLabel(&Label, aBuf, 14.0f, TEXTALIGN_ML);

	Dummy.HSplitTop(20.0f, &DummyLabel, &Dummy);

	if(DoButton_CheckBox(&m_Dummy, Localize("Dummy settings"), m_Dummy, &DummyLabel))
	{
		m_Dummy ^= 1;
	}
	GameClient()->m_Tooltips.DoToolTip(&m_Dummy, &DummyLabel, Localize("Toggle to edit your dummy settings"));

	Dummy.HSplitTop(20.0f, &DummyLabel, &Dummy);

	if(DoButton_CheckBox(&g_Config.m_ClDownloadSkins, Localize("Download skins"), g_Config.m_ClDownloadSkins, &DummyLabel))
	{
		g_Config.m_ClDownloadSkins ^= 1;
		RefreshSkins();
		s_InitSkinlist = true;
	}

	Dummy.HSplitTop(20.0f, &DummyLabel, &Dummy);

	if(DoButton_CheckBox(&g_Config.m_ClDownloadCommunitySkins, Localize("Download community skins"), g_Config.m_ClDownloadCommunitySkins, &DummyLabel))
	{
		g_Config.m_ClDownloadCommunitySkins ^= 1;
		RefreshSkins();
		s_InitSkinlist = true;
	}

	Dummy.HSplitTop(20.0f, &DummyLabel, &Dummy);

	if(DoButton_CheckBox(&g_Config.m_ClVanillaSkinsOnly, Localize("Vanilla skins only"), g_Config.m_ClVanillaSkinsOnly, &DummyLabel))
	{
		g_Config.m_ClVanillaSkinsOnly ^= 1;
		RefreshSkins();
		s_InitSkinlist = true;
	}

	Dummy.HSplitTop(20.0f, &DummyLabel, &Dummy);

	if(DoButton_CheckBox(&g_Config.m_ClFatSkins, Localize("Fat skins (DDFat)"), g_Config.m_ClFatSkins, &DummyLabel))
	{
		g_Config.m_ClFatSkins ^= 1;
	}

	SkinPrefix.HSplitTop(20.0f, &SkinPrefixLabel, &SkinPrefix);
	UI()->DoLabel(&SkinPrefixLabel, Localize("Skin prefix"), 14.0f, TEXTALIGN_ML);

	SkinPrefix.HSplitTop(20.0f, &SkinPrefixLabel, &SkinPrefix);
	static CLineInput s_SkinPrefixInput(g_Config.m_ClSkinPrefix, sizeof(g_Config.m_ClSkinPrefix));
	UI()->DoClearableEditBox(&s_SkinPrefixInput, &SkinPrefixLabel, 14.0f);

	SkinPrefix.HSplitTop(2.0f, 0, &SkinPrefix);
	{
		static const char *s_apSkinPrefixes[] = {"kitty", "santa"};
		static CButtonContainer s_aPrefixButtons[std::size(s_apSkinPrefixes)];
		for(size_t i = 0; i < std::size(s_apSkinPrefixes); i++)
		{
			SkinPrefix.HSplitTop(20.0f, &Button, &SkinPrefix);
			Button.HMargin(2.0f, &Button);
			if(DoButton_Menu(&s_aPrefixButtons[i], s_apSkinPrefixes[i], 0, &Button))
			{
				str_copy(g_Config.m_ClSkinPrefix, s_apSkinPrefixes[i]);
			}
		}
	}

	Dummy.HSplitTop(20.0f, &DummyLabel, &Dummy);

	// note: get the skin info after the settings buttons, because they can trigger a refresh
	// which invalidates the skin
	// skin info
	CTeeRenderInfo OwnSkinInfo;
	const CSkin *pSkin = m_pClient->m_Skins.Find(pSkinName);
	OwnSkinInfo.m_OriginalRenderSkin = pSkin->m_OriginalSkin;
	OwnSkinInfo.m_ColorableRenderSkin = pSkin->m_ColorableSkin;
	OwnSkinInfo.m_SkinMetrics = pSkin->m_Metrics;
	OwnSkinInfo.m_CustomColoredSkin = *pUseCustomColor;
	if(*pUseCustomColor)
	{
		OwnSkinInfo.m_ColorBody = color_cast<ColorRGBA>(ColorHSLA(*pColorBody).UnclampLighting());
		OwnSkinInfo.m_ColorFeet = color_cast<ColorRGBA>(ColorHSLA(*pColorFeet).UnclampLighting());
	}
	else
	{
		OwnSkinInfo.m_ColorBody = ColorRGBA(1.0f, 1.0f, 1.0f);
		OwnSkinInfo.m_ColorFeet = ColorRGBA(1.0f, 1.0f, 1.0f);
	}
	OwnSkinInfo.m_Size = 50.0f;

	MainView.HSplitTop(50.0f, &Label, &MainView);
	Label.VSplitLeft(260.0f, &Label, 0);
	const CAnimState *pIdleState = CAnimState::GetIdle();
	vec2 OffsetToMid;
	RenderTools()->GetRenderTeeOffsetToRenderedTee(pIdleState, &OwnSkinInfo, OffsetToMid);
	vec2 TeeRenderPos(Label.x + 30.0f, Label.y + Label.h / 2.0f + OffsetToMid.y);
	int Emote = m_Dummy ? g_Config.m_ClDummyDefaultEyes : g_Config.m_ClPlayerDefaultEyes;
	RenderTools()->RenderTee(pIdleState, &OwnSkinInfo, Emote, vec2(1, 0), TeeRenderPos);
	Label.VSplitLeft(70.0f, 0, &Label);
	Label.HMargin(15.0f, &Label);

	// default eyes
	bool RenderEyesBelow = MainView.w < 750.0f;
	if(RenderEyesBelow)
	{
		Eyes.VSplitLeft(190.0f, 0, &Eyes);
		Eyes.HSplitTop(105.0f, 0, &Eyes);
	}
	else
	{
		Eyes = EyesRight;
		if(MainView.w < 810.0f)
			Eyes.VSplitRight(205.0f, 0, &Eyes);
		Eyes.HSplitTop(50.0f, &Eyes, 0);
	}
	Eyes.HSplitTop(120.0f, &EyesLabel, &Eyes);
	EyesLabel.VSplitLeft(20.0f, 0, &EyesLabel);
	EyesLabel.HSplitTop(50.0f, &EyesLabel, &Eyes);

	static CButtonContainer s_aEyeButtons[6];
	static int s_aEyesToolTip[6];
	for(int CurrentEyeEmote = 0; CurrentEyeEmote < 6; CurrentEyeEmote++)
	{
		EyesLabel.VSplitLeft(10.0f, 0, &EyesLabel);
		EyesLabel.VSplitLeft(50.0f, &EyesTee, &EyesLabel);

		if(CurrentEyeEmote == 2 && !RenderEyesBelow)
		{
			Eyes.HSplitTop(60.0f, &EyesLabel, 0);
			EyesLabel.HSplitTop(10.0f, 0, &EyesLabel);
		}
		float Highlight = (m_Dummy ? g_Config.m_ClDummyDefaultEyes == CurrentEyeEmote : g_Config.m_ClPlayerDefaultEyes == CurrentEyeEmote) ? 1.0f : 0.0f;
		if(DoButton_Menu(&s_aEyeButtons[CurrentEyeEmote], "", 0, &EyesTee, 0, IGraphics::CORNER_ALL, 10.0f, 0.0f, vec4(1, 1, 1, 0.5f + Highlight * 0.25f), vec4(1, 1, 1, 0.25f + Highlight * 0.25f)))
		{
			if(m_Dummy)
			{
				g_Config.m_ClDummyDefaultEyes = CurrentEyeEmote;
				if(g_Config.m_ClDummy)
					GameClient()->m_Emoticon.EyeEmote(CurrentEyeEmote);
			}
			else
			{
				g_Config.m_ClPlayerDefaultEyes = CurrentEyeEmote;
				if(!g_Config.m_ClDummy)
					GameClient()->m_Emoticon.EyeEmote(CurrentEyeEmote);
			}
		}
		GameClient()->m_Tooltips.DoToolTip(&s_aEyesToolTip[CurrentEyeEmote], &EyesTee, Localize("Choose default eyes when joining a server"));
		RenderTools()->RenderTee(pIdleState, &OwnSkinInfo, CurrentEyeEmote, vec2(1, 0), vec2(EyesTee.x + 25.0f, EyesTee.y + EyesTee.h / 2.0f + OffsetToMid.y));
	}

	Label.VSplitRight(34.0f, &Label, &Button);

	static CLineInput s_SkinInput;
	s_SkinInput.SetBuffer(pSkinName, sizeof(g_Config.m_ClPlayerSkin));
	s_SkinInput.SetEmptyText("default");
	if(UI()->DoClearableEditBox(&s_SkinInput, &Label, 14.0f))
	{
		SetNeedSendInfo();
	}

	// random skin button
	Button.VSplitRight(30.0f, 0, &Button);
	static CButtonContainer s_RandomSkinButtonID;
	static const char *s_apDice[] = {FONT_ICON_DICE_ONE, FONT_ICON_DICE_TWO, FONT_ICON_DICE_THREE, FONT_ICON_DICE_FOUR, FONT_ICON_DICE_FIVE, FONT_ICON_DICE_SIX};
	static int s_CurrentDie = rand() % std::size(s_apDice);
	TextRender()->SetFontPreset(EFontPreset::ICON_FONT);
	TextRender()->SetRenderFlags(ETextRenderFlags::TEXT_RENDER_FLAG_ONLY_ADVANCE_WIDTH | ETextRenderFlags::TEXT_RENDER_FLAG_NO_X_BEARING | ETextRenderFlags::TEXT_RENDER_FLAG_NO_Y_BEARING | ETextRenderFlags::TEXT_RENDER_FLAG_NO_PIXEL_ALIGMENT | ETextRenderFlags::TEXT_RENDER_FLAG_NO_OVERSIZE);
	if(DoButton_Menu(&s_RandomSkinButtonID, s_apDice[s_CurrentDie], 1, &Button, nullptr, IGraphics::CORNER_ALL, 5.0f, -0.2f))
	{
		RandomSkin();
		s_CurrentDie = rand() % std::size(s_apDice);
	}
	TextRender()->SetRenderFlags(0);
	TextRender()->SetFontPreset(EFontPreset::DEFAULT_FONT);
	GameClient()->m_Tooltips.DoToolTip(&s_RandomSkinButtonID, &Button, Localize("Create a random skin"));

	// custom color selector
	MainView.HSplitTop(20.0f + RenderEyesBelow * 25.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	Button.VSplitLeft(150.0f, &Button, 0);
	static int s_CustomColorID = 0;
	if(DoButton_CheckBox(&s_CustomColorID, Localize("Custom colors"), *pUseCustomColor, &Button))
	{
		*pUseCustomColor = *pUseCustomColor ? 0 : 1;
		SetNeedSendInfo();
	}

	MainView.HSplitTop(5.0f, 0, &MainView);
	MainView.HSplitTop(82.5f, &Label, &MainView);
	if(*pUseCustomColor)
	{
		CUIRect aRects[2];
		Label.VSplitMid(&aRects[0], &aRects[1], 20.0f);

		unsigned *apColors[2] = {pColorBody, pColorFeet};
		const char *apParts[] = {Localize("Body"), Localize("Feet")};

		for(int i = 0; i < 2; i++)
		{
			aRects[i].HSplitTop(20.0f, &Label, &aRects[i]);
			UI()->DoLabel(&Label, apParts[i], 14.0f, TEXTALIGN_ML);
			aRects[i].VSplitLeft(10.0f, 0, &aRects[i]);
			aRects[i].HSplitTop(2.5f, 0, &aRects[i]);

			unsigned PrevColor = *apColors[i];
			RenderHSLScrollbars(&aRects[i], apColors[i], false, true);

			if(PrevColor != *apColors[i])
			{
				SetNeedSendInfo();
			}
		}
	}

	// skin selector
	MainView.HSplitTop(20.0f, 0, &MainView);
	MainView.HSplitTop(230.0f - RenderEyesBelow * 25.0f, &SkinList, &MainView);
	static std::vector<CUISkin> s_vSkinList;
	static std::vector<CUISkin> s_vSkinListHelper;
	static std::vector<CUISkin> s_vFavoriteSkinListHelper;
	static int s_SkinCount = 0;
	static CListBox s_ListBox;

	// be nice to the CPU
	static auto s_SkinLastRebuildTime = time_get_nanoseconds();
	const auto CurTime = time_get_nanoseconds();
	if(s_InitSkinlist || m_pClient->m_Skins.Num() != s_SkinCount || m_SkinFavoritesChanged || (m_pClient->m_Skins.IsDownloadingSkins() && (CurTime - s_SkinLastRebuildTime > 500ms)))
	{
		s_SkinLastRebuildTime = CurTime;
		s_vSkinList.clear();
		s_vSkinListHelper.clear();
		s_vFavoriteSkinListHelper.clear();
		// set skin count early, since Find of the skin class might load
		// a downloading skin
		s_SkinCount = m_pClient->m_Skins.Num();
		m_SkinFavoritesChanged = false;

		auto &&SkinNotFiltered = [&](const CSkin *pSkinToBeSelected) {
			// filter quick search
			if(g_Config.m_ClSkinFilterString[0] != '\0' && !str_utf8_find_nocase(pSkinToBeSelected->GetName(), g_Config.m_ClSkinFilterString))
				return false;

			// no special skins
			if((pSkinToBeSelected->GetName()[0] == 'x' && pSkinToBeSelected->GetName()[1] == '_'))
				return false;

			return true;
		};

		for(const auto &it : m_SkinFavorites)
		{
			const CSkin *pSkinToBeSelected = m_pClient->m_Skins.FindOrNullptr(it.c_str());

			if(pSkinToBeSelected == nullptr || !SkinNotFiltered(pSkinToBeSelected))
				continue;

			s_vFavoriteSkinListHelper.emplace_back(pSkinToBeSelected);
		}
		for(const auto &SkinIt : m_pClient->m_Skins.GetSkinsUnsafe())
		{
			const auto &pSkinToBeSelected = SkinIt.second;
			if(!SkinNotFiltered(pSkinToBeSelected.get()))
				continue;

			if(std::find(m_SkinFavorites.begin(), m_SkinFavorites.end(), pSkinToBeSelected->GetName()) == m_SkinFavorites.end())
				s_vSkinListHelper.emplace_back(pSkinToBeSelected.get());
		}
		std::sort(s_vSkinListHelper.begin(), s_vSkinListHelper.end());
		std::sort(s_vFavoriteSkinListHelper.begin(), s_vFavoriteSkinListHelper.end());
		s_vSkinList = s_vFavoriteSkinListHelper;
		s_vSkinList.insert(s_vSkinList.end(), s_vSkinListHelper.begin(), s_vSkinListHelper.end());
		s_InitSkinlist = false;
	}

	auto &&RenderFavIcon = [&](const CUIRect &FavIcon, bool AsFav) {
		TextRender()->SetFontPreset(EFontPreset::ICON_FONT);
		TextRender()->SetRenderFlags(ETextRenderFlags::TEXT_RENDER_FLAG_ONLY_ADVANCE_WIDTH | ETextRenderFlags::TEXT_RENDER_FLAG_NO_X_BEARING | ETextRenderFlags::TEXT_RENDER_FLAG_NO_Y_BEARING | ETextRenderFlags::TEXT_RENDER_FLAG_NO_PIXEL_ALIGMENT | ETextRenderFlags::TEXT_RENDER_FLAG_NO_OVERSIZE);
		if(AsFav)
			TextRender()->TextColor({1, 1, 0, 1});
		else
			TextRender()->TextColor({0.5f, 0.5f, 0.5f, 1});
		TextRender()->TextOutlineColor(TextRender()->DefaultTextOutlineColor());
		SLabelProperties Props;
		Props.m_MaxWidth = FavIcon.w;
		UI()->DoLabel(&FavIcon, FONT_ICON_STAR, 12.0f, TEXTALIGN_MR, Props);
		TextRender()->TextColor(TextRender()->DefaultTextColor());
		TextRender()->SetRenderFlags(0);
		TextRender()->SetFontPreset(EFontPreset::DEFAULT_FONT);
	};

	int OldSelected = -1;
	s_ListBox.DoStart(50.0f, s_vSkinList.size(), 4, 1, OldSelected, &SkinList);
	for(size_t i = 0; i < s_vSkinList.size(); ++i)
	{
		const CSkin *pSkinToBeDraw = s_vSkinList[i].m_pSkin;

		if(str_comp(pSkinToBeDraw->GetName(), pSkinName) == 0)
			OldSelected = i;

		const CListboxItem Item = s_ListBox.DoNextItem(pSkinToBeDraw, OldSelected >= 0 && (size_t)OldSelected == i);
		if(!Item.m_Visible)
			continue;

		const CUIRect OriginalRect = Item.m_Rect;

		CTeeRenderInfo Info = OwnSkinInfo;
		Info.m_CustomColoredSkin = *pUseCustomColor;

		Info.m_OriginalRenderSkin = pSkinToBeDraw->m_OriginalSkin;
		Info.m_ColorableRenderSkin = pSkinToBeDraw->m_ColorableSkin;
		Info.m_SkinMetrics = pSkinToBeDraw->m_Metrics;

		RenderTools()->GetRenderTeeOffsetToRenderedTee(pIdleState, &Info, OffsetToMid);
		TeeRenderPos = vec2(OriginalRect.x + 30, OriginalRect.y + OriginalRect.h / 2 + OffsetToMid.y);
		RenderTools()->RenderTee(pIdleState, &Info, Emote, vec2(1.0f, 0.0f), TeeRenderPos);

		OriginalRect.VSplitLeft(60.0f, 0, &Label);
		{
			SLabelProperties Props;
			Props.m_MaxWidth = Label.w - 5.0f;
			UI()->DoLabel(&Label, pSkinToBeDraw->GetName(), 12.0f, TEXTALIGN_ML, Props);
		}
		if(g_Config.m_Debug)
		{
			ColorRGBA BloodColor = *pUseCustomColor ? color_cast<ColorRGBA>(ColorHSLA(*pColorBody).UnclampLighting()) : pSkinToBeDraw->m_BloodColor;
			Graphics()->TextureClear();
			Graphics()->QuadsBegin();
			Graphics()->SetColor(BloodColor.r, BloodColor.g, BloodColor.b, 1.0f);
			IGraphics::CQuadItem QuadItem(Label.x, Label.y, 12.0f, 12.0f);
			Graphics()->QuadsDrawTL(&QuadItem, 1);
			Graphics()->QuadsEnd();
		}

		// render skin favorite icon
		{
			const auto SkinItFav = m_SkinFavorites.find(pSkinToBeDraw->GetName());
			const auto IsFav = SkinItFav != m_SkinFavorites.end();
			CUIRect FavIcon;
			OriginalRect.HSplitTop(20.0f, &FavIcon, nullptr);
			FavIcon.VSplitRight(20.0f, nullptr, &FavIcon);
			if(IsFav)
			{
				RenderFavIcon(FavIcon, IsFav);
			}
			else
			{
				if(UI()->MouseInside(&FavIcon))
				{
					RenderFavIcon(FavIcon, IsFav);
				}
			}
			if(UI()->DoButtonLogic(&pSkinToBeDraw->m_Metrics.m_Body, 0, &FavIcon))
			{
				if(IsFav)
				{
					m_SkinFavorites.erase(SkinItFav);
				}
				else
				{
					m_SkinFavorites.emplace(pSkinToBeDraw->GetName());
				}
				s_InitSkinlist = true;
			}
		}
	}

	const int NewSelected = s_ListBox.DoEnd();
	if(OldSelected != NewSelected)
	{
		mem_copy(pSkinName, s_vSkinList[NewSelected].m_pSkin->GetName(), sizeof(g_Config.m_ClPlayerSkin));
		SetNeedSendInfo();
	}

	// render quick search
	{
		MainView.HSplitBottom(ms_ButtonHeight, &MainView, &QuickSearch);
		QuickSearch.VSplitLeft(240.0f, &QuickSearch, &SkinDB);
		QuickSearch.HSplitTop(5.0f, 0, &QuickSearch);
		TextRender()->SetFontPreset(EFontPreset::ICON_FONT);
		TextRender()->SetRenderFlags(ETextRenderFlags::TEXT_RENDER_FLAG_ONLY_ADVANCE_WIDTH | ETextRenderFlags::TEXT_RENDER_FLAG_NO_X_BEARING | ETextRenderFlags::TEXT_RENDER_FLAG_NO_Y_BEARING | ETextRenderFlags::TEXT_RENDER_FLAG_NO_PIXEL_ALIGMENT | ETextRenderFlags::TEXT_RENDER_FLAG_NO_OVERSIZE);

		UI()->DoLabel(&QuickSearch, FONT_ICON_MAGNIFYING_GLASS, 14.0f, TEXTALIGN_ML);
		float wSearch = TextRender()->TextWidth(14.0f, FONT_ICON_MAGNIFYING_GLASS, -1, -1.0f);
		TextRender()->SetRenderFlags(0);
		TextRender()->SetFontPreset(EFontPreset::DEFAULT_FONT);
		QuickSearch.VSplitLeft(wSearch, 0, &QuickSearch);
		QuickSearch.VSplitLeft(5.0f, 0, &QuickSearch);
		QuickSearch.VSplitLeft(QuickSearch.w - 15.0f, &QuickSearch, &QuickSearchClearButton);
		static CLineInput s_SkinFilterInput(g_Config.m_ClSkinFilterString, sizeof(g_Config.m_ClSkinFilterString));
		if(Input()->KeyPress(KEY_F) && Input()->ModifierIsPressed())
		{
			UI()->SetActiveItem(&s_SkinFilterInput);
			s_SkinFilterInput.SelectAll();
		}
		s_SkinFilterInput.SetEmptyText(Localize("Search"));
		if(UI()->DoClearableEditBox(&s_SkinFilterInput, &QuickSearch, 14.0f))
			s_InitSkinlist = true;
	}

	SkinDB.VSplitLeft(150.0f, &SkinDB, &DirectoryButton);
	SkinDB.HSplitTop(5.0f, 0, &SkinDB);
	static CButtonContainer s_SkinDBDirID;
	if(DoButton_Menu(&s_SkinDBDirID, Localize("Skin Database"), 0, &SkinDB))
	{
		const char *pLink = "https://ddnet.org/skins/";
		if(!open_link(pLink))
		{
			dbg_msg("menus", "couldn't open link '%s'", pLink);
		}
	}

	DirectoryButton.HSplitTop(5.0f, 0, &DirectoryButton);
	DirectoryButton.VSplitRight(175.0f, 0, &DirectoryButton);
	DirectoryButton.VSplitRight(25.0f, &DirectoryButton, &RefreshButton);
	DirectoryButton.VSplitRight(10.0f, &DirectoryButton, 0);
	static CButtonContainer s_DirectoryButtonID;
	if(DoButton_Menu(&s_DirectoryButtonID, Localize("Skins directory"), 0, &DirectoryButton))
	{
		Storage()->GetCompletePath(IStorage::TYPE_SAVE, "skins", aBuf, sizeof(aBuf));
		Storage()->CreateFolder("skins", IStorage::TYPE_SAVE);
		if(!open_file(aBuf))
		{
			dbg_msg("menus", "couldn't open file '%s'", aBuf);
		}
	}
	GameClient()->m_Tooltips.DoToolTip(&s_DirectoryButtonID, &DirectoryButton, Localize("Open the directory to add custom skins"));

	TextRender()->SetFontPreset(EFontPreset::ICON_FONT);
	TextRender()->SetRenderFlags(ETextRenderFlags::TEXT_RENDER_FLAG_ONLY_ADVANCE_WIDTH | ETextRenderFlags::TEXT_RENDER_FLAG_NO_X_BEARING | ETextRenderFlags::TEXT_RENDER_FLAG_NO_Y_BEARING | ETextRenderFlags::TEXT_RENDER_FLAG_NO_PIXEL_ALIGMENT | ETextRenderFlags::TEXT_RENDER_FLAG_NO_OVERSIZE);
	static CButtonContainer s_SkinRefreshButtonID;
	if(DoButton_Menu(&s_SkinRefreshButtonID, FONT_ICON_ARROW_ROTATE_RIGHT, 0, &RefreshButton, nullptr, IGraphics::CORNER_ALL, 5, 0, vec4(1.0f, 1.0f, 1.0f, 0.75f), vec4(1, 1, 1, 0.5f)))
	{
		// reset render flags for possible loading screen
		TextRender()->SetRenderFlags(0);
		TextRender()->SetFontPreset(EFontPreset::DEFAULT_FONT);
		RefreshSkins();
		s_InitSkinlist = true;
	}
	TextRender()->SetRenderFlags(0);
	TextRender()->SetFontPreset(EFontPreset::DEFAULT_FONT);
}

typedef struct
{
	CLocConstString m_Name;
	const char *m_pCommand;
	int m_KeyId;
	int m_ModifierCombination;
} CKeyInfo;

static CKeyInfo gs_aKeys[] =
	{
		{"Move left", "+left", 0, 0}, // Localize - these strings are localized within CLocConstString
		{"Move right", "+right", 0, 0},
		{"Jump", "+jump", 0, 0},
		{"Fire", "+fire", 0, 0},
		{"Hook", "+hook", 0, 0},
		{"Hook collisions", "+showhookcoll", 0, 0},
		{"Pause", "say /pause", 0, 0},
		{"Kill", "kill", 0, 0},
		{"Zoom in", "zoom+", 0, 0},
		{"Zoom out", "zoom-", 0, 0},
		{"Default zoom", "zoom", 0, 0},
		{"Show others", "say /showothers", 0, 0},
		{"Show all", "say /showall", 0, 0},
		{"Toggle dyncam", "toggle cl_dyncam 0 1", 0, 0},
		{"Toggle ghost", "toggle cl_race_show_ghost 0 1", 0, 0},

		{"Hammer", "+weapon1", 0, 0},
		{"Pistol", "+weapon2", 0, 0},
		{"Shotgun", "+weapon3", 0, 0},
		{"Grenade", "+weapon4", 0, 0},
		{"Laser", "+weapon5", 0, 0},
		{"Next weapon", "+nextweapon", 0, 0},
		{"Prev. weapon", "+prevweapon", 0, 0},

		{"Vote yes", "vote yes", 0, 0},
		{"Vote no", "vote no", 0, 0},

		{"Chat", "+show_chat; chat all", 0, 0},
		{"Team chat", "+show_chat; chat team", 0, 0},
		{"Converse", "+show_chat; chat all /c ", 0, 0},
		{"Chat command", "+show_chat; chat all /", 0, 0},
		{"Show chat", "+show_chat", 0, 0},

		{"Toggle dummy", "toggle cl_dummy 0 1", 0, 0},
		{"Dummy copy", "toggle cl_dummy_copy_moves 0 1", 0, 0},
		{"Hammerfly dummy", "toggle cl_dummy_hammer 0 1", 0, 0},

		{"Emoticon", "+emote", 0, 0},
		{"Spectator mode", "+spectate", 0, 0},
		{"Spectate next", "spectate_next", 0, 0},
		{"Spectate previous", "spectate_previous", 0, 0},
		{"Console", "toggle_local_console", 0, 0},
		{"Remote console", "toggle_remote_console", 0, 0},
		{"Screenshot", "screenshot", 0, 0},
		{"Scoreboard", "+scoreboard", 0, 0},
		{"Statboard", "+statboard", 0, 0},
		{"Lock team", "say /lock", 0, 0},
		{"Show entities", "toggle cl_overlay_entities 0 100", 0, 0},
		{"Show HUD", "toggle cl_showhud 0 1", 0, 0},
};

/*	This is for scripts/languages to work, don't remove!
	Localize("Move left");Localize("Move right");Localize("Jump");Localize("Fire");Localize("Hook");
	Localize("Hook collisions");Localize("Pause");Localize("Kill");Localize("Zoom in");Localize("Zoom out");
	Localize("Default zoom");Localize("Show others");Localize("Show all");Localize("Toggle dyncam");
	Localize("Toggle dummy");Localize("Toggle ghost");Localize("Dummy copy");Localize("Hammerfly dummy");
	Localize("Hammer");Localize("Pistol");Localize("Shotgun");Localize("Grenade");Localize("Laser");
	Localize("Next weapon");Localize("Prev. weapon");Localize("Vote yes");Localize("Vote no");
	Localize("Chat");Localize("Team chat");Localize("Converse");Localize("Show chat");Localize("Emoticon");
	Localize("Spectator mode");Localize("Spectate next");Localize("Spectate previous");Localize("Console");
	Localize("Remote console");Localize("Screenshot");Localize("Scoreboard");Localize("Statboard");
	Localize("Lock team");Localize("Show entities");Localize("Show HUD");Localize("Chat command");
*/

void CMenus::DoSettingsControlsButtons(int Start, int Stop, CUIRect View)
{
	for(int i = Start; i < Stop; i++)
	{
		CKeyInfo &Key = gs_aKeys[i];
		CUIRect Button, Label;
		View.HSplitTop(20.0f, &Button, &View);
		Button.VSplitLeft(135.0f, &Label, &Button);

		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "%s:", Localize((const char *)Key.m_Name));

		UI()->DoLabel(&Label, aBuf, 13.0f, TEXTALIGN_ML);
		int OldId = Key.m_KeyId, OldModifierCombination = Key.m_ModifierCombination, NewModifierCombination;
		int NewId = DoKeyReader((void *)&Key.m_Name, &Button, OldId, OldModifierCombination, &NewModifierCombination);
		if(NewId != OldId || NewModifierCombination != OldModifierCombination)
		{
			if(OldId != 0 || NewId == 0)
				m_pClient->m_Binds.Bind(OldId, "", false, OldModifierCombination);
			if(NewId != 0)
				m_pClient->m_Binds.Bind(NewId, gs_aKeys[i].m_pCommand, false, NewModifierCombination);
		}

		View.HSplitTop(2.0f, 0, &View);
	}
}

float CMenus::RenderSettingsControlsJoystick(CUIRect View)
{
	bool JoystickEnabled = g_Config.m_InpControllerEnable;
	int NumJoysticks = Input()->NumJoysticks();
	int NumOptions = 1; // expandable header
	if(JoystickEnabled)
	{
		if(NumJoysticks == 0)
			NumOptions++; // message
		else
		{
			if(NumJoysticks > 1)
				NumOptions++; // joystick selection
			NumOptions += 3; // mode, ui sens, tolerance
			if(!g_Config.m_InpControllerAbsolute)
				NumOptions++; // ingame sens
			NumOptions += Input()->GetActiveJoystick()->GetNumAxes() + 1; // axis selection + header
		}
	}
	const float ButtonHeight = 20.0f;
	const float Spacing = 2.0f;
	const float BackgroundHeight = NumOptions * (ButtonHeight + Spacing) + (NumOptions == 1 ? 0.0f : Spacing);
	if(View.h < BackgroundHeight)
		return BackgroundHeight;

	View.HSplitTop(BackgroundHeight, &View, nullptr);

	CUIRect Button;
	View.HSplitTop(Spacing, nullptr, &View);
	View.HSplitTop(ButtonHeight, &Button, &View);
	if(DoButton_CheckBox(&g_Config.m_InpControllerEnable, Localize("Enable controller"), g_Config.m_InpControllerEnable, &Button))
	{
		g_Config.m_InpControllerEnable ^= 1;
	}
	if(JoystickEnabled)
	{
		if(NumJoysticks > 0)
		{
			// show joystick device selection if more than one available or just the joystick name if there is only one
			{
				CUIRect JoystickDropDown;
				View.HSplitTop(Spacing, nullptr, &View);
				View.HSplitTop(ButtonHeight, &JoystickDropDown, &View);
				if(NumJoysticks > 1)
				{
					static std::vector<std::string> s_vJoystickNames;
					static std::vector<const char *> s_vpJoystickNames;
					s_vJoystickNames.resize(NumJoysticks);
					s_vpJoystickNames.resize(NumJoysticks);

					for(int i = 0; i < NumJoysticks; ++i)
					{
						char aBuf[256];
						str_format(aBuf, sizeof(aBuf), "%s %d: %s", Localize("Controller"), i, Input()->GetJoystick(i)->GetName());
						s_vJoystickNames[i] = aBuf;
						s_vpJoystickNames[i] = s_vJoystickNames[i].c_str();
					}

					static CUI::SDropDownState s_JoystickDropDownState;
					static CScrollRegion s_JoystickDropDownScrollRegion;
					s_JoystickDropDownState.m_SelectionPopupContext.m_pScrollRegion = &s_JoystickDropDownScrollRegion;
					const int CurrentJoystick = Input()->GetActiveJoystick()->GetIndex();
					const int NewJoystick = UI()->DoDropDown(&JoystickDropDown, CurrentJoystick, s_vpJoystickNames.data(), s_vpJoystickNames.size(), s_JoystickDropDownState);
					if(NewJoystick != CurrentJoystick)
					{
						Input()->SetActiveJoystick(NewJoystick);
					}
				}
				else
				{
					char aBuf[256];
					str_format(aBuf, sizeof(aBuf), "%s 0: %s", Localize("Controller"), Input()->GetJoystick(0)->GetName());
					UI()->DoLabel(&JoystickDropDown, aBuf, 13.0f, TEXTALIGN_ML);
				}
			}

			{
				View.HSplitTop(Spacing, nullptr, &View);
				View.HSplitTop(ButtonHeight, &Button, &View);
				CUIRect Label, ButtonRelative, ButtonAbsolute;
				Button.VSplitMid(&Label, &Button, 10.0f);
				Button.HMargin(2.0f, &Button);
				Button.VSplitMid(&ButtonRelative, &ButtonAbsolute);
				UI()->DoLabel(&Label, Localize("Ingame controller mode"), 13.0f, TEXTALIGN_ML);
				CButtonContainer s_RelativeButton;
				if(DoButton_Menu(&s_RelativeButton, Localize("Relative", "Ingame controller mode"), g_Config.m_InpControllerAbsolute == 0, &ButtonRelative, nullptr, IGraphics::CORNER_L))
				{
					g_Config.m_InpControllerAbsolute = 0;
				}
				CButtonContainer s_AbsoluteButton;
				if(DoButton_Menu(&s_AbsoluteButton, Localize("Absolute", "Ingame controller mode"), g_Config.m_InpControllerAbsolute == 1, &ButtonAbsolute, nullptr, IGraphics::CORNER_R))
				{
					g_Config.m_InpControllerAbsolute = 1;
				}
			}

			if(!g_Config.m_InpControllerAbsolute)
			{
				View.HSplitTop(Spacing, nullptr, &View);
				View.HSplitTop(ButtonHeight, &Button, &View);
				UI()->DoScrollbarOption(&g_Config.m_InpControllerSens, &g_Config.m_InpControllerSens, &Button, Localize("Ingame controller sens."), 1, 500, &CUI::ms_LogarithmicScrollbarScale, CUI::SCROLLBAR_OPTION_NOCLAMPVALUE);
			}

			View.HSplitTop(Spacing, nullptr, &View);
			View.HSplitTop(ButtonHeight, &Button, &View);
			UI()->DoScrollbarOption(&g_Config.m_UiControllerSens, &g_Config.m_UiControllerSens, &Button, Localize("UI controller sens."), 1, 500, &CUI::ms_LogarithmicScrollbarScale, CUI::SCROLLBAR_OPTION_NOCLAMPVALUE);

			View.HSplitTop(Spacing, nullptr, &View);
			View.HSplitTop(ButtonHeight, &Button, &View);
			UI()->DoScrollbarOption(&g_Config.m_InpControllerTolerance, &g_Config.m_InpControllerTolerance, &Button, Localize("Controller jitter tolerance"), 0, 50);

			View.HSplitTop(Spacing, nullptr, &View);
			View.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 0.125f), IGraphics::CORNER_ALL, 5.0f);
			DoJoystickAxisPicker(View);
		}
		else
		{
			View.HSplitTop(View.h - ButtonHeight, nullptr, &View);
			View.HSplitTop(ButtonHeight, &Button, &View);
			UI()->DoLabel(&Button, Localize("No controller found. Plug in a controller."), 13.0f, TEXTALIGN_ML);
		}
	}

	return BackgroundHeight;
}

void CMenus::DoJoystickAxisPicker(CUIRect View)
{
	const float FontSize = 13.0f;
	const float RowHeight = 20.0f;
	const float SpacingH = 2.0f;
	const float AxisWidth = 0.2f * View.w;
	const float StatusWidth = 0.4f * View.w;
	const float AimBindWidth = 90.0f;
	const float SpacingV = (View.w - AxisWidth - StatusWidth - AimBindWidth) / 2.0f;

	CUIRect Row, Axis, Status, AimBind;
	View.HSplitTop(SpacingH, nullptr, &View);
	View.HSplitTop(RowHeight, &Row, &View);
	Row.VSplitLeft(AxisWidth, &Axis, &Row);
	Row.VSplitLeft(SpacingV, nullptr, &Row);
	Row.VSplitLeft(StatusWidth, &Status, &Row);
	Row.VSplitLeft(SpacingV, nullptr, &Row);
	Row.VSplitLeft(AimBindWidth, &AimBind, &Row);

	UI()->DoLabel(&Axis, Localize("Axis"), FontSize, TEXTALIGN_MC);
	UI()->DoLabel(&Status, Localize("Status"), FontSize, TEXTALIGN_MC);
	UI()->DoLabel(&AimBind, Localize("Aim bind"), FontSize, TEXTALIGN_MC);

	IInput::IJoystick *pJoystick = Input()->GetActiveJoystick();
	static int s_aActive[NUM_JOYSTICK_AXES][2];
	for(int i = 0; i < minimum<int>(pJoystick->GetNumAxes(), NUM_JOYSTICK_AXES); i++)
	{
		const bool Active = g_Config.m_InpControllerX == i || g_Config.m_InpControllerY == i;

		View.HSplitTop(SpacingH, nullptr, &View);
		View.HSplitTop(RowHeight, &Row, &View);
		Row.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 0.125f), IGraphics::CORNER_ALL, 5.0f);
		Row.VSplitLeft(AxisWidth, &Axis, &Row);
		Row.VSplitLeft(SpacingV, nullptr, &Row);
		Row.VSplitLeft(StatusWidth, &Status, &Row);
		Row.VSplitLeft(SpacingV, nullptr, &Row);
		Row.VSplitLeft(AimBindWidth, &AimBind, &Row);

		// Axis label
		char aBuf[16];
		str_format(aBuf, sizeof(aBuf), "%d", i + 1);
		if(Active)
			TextRender()->TextColor(TextRender()->DefaultTextColor());
		else
			TextRender()->TextColor(0.7f, 0.7f, 0.7f, 1.0f);
		UI()->DoLabel(&Axis, aBuf, FontSize, TEXTALIGN_MC);

		// Axis status
		Status.HMargin(7.0f, &Status);
		DoJoystickBar(&Status, (pJoystick->GetAxisValue(i) + 1.0f) / 2.0f, g_Config.m_InpControllerTolerance / 50.0f, Active);

		// Bind to X/Y
		CUIRect AimBindX, AimBindY;
		AimBind.VSplitMid(&AimBindX, &AimBindY);
		if(DoButton_CheckBox(&s_aActive[i][0], "X", g_Config.m_InpControllerX == i, &AimBindX))
		{
			if(g_Config.m_InpControllerY == i)
				g_Config.m_InpControllerY = g_Config.m_InpControllerX;
			g_Config.m_InpControllerX = i;
		}
		if(DoButton_CheckBox(&s_aActive[i][1], "Y", g_Config.m_InpControllerY == i, &AimBindY))
		{
			if(g_Config.m_InpControllerX == i)
				g_Config.m_InpControllerX = g_Config.m_InpControllerY;
			g_Config.m_InpControllerY = i;
		}
	}
	TextRender()->TextColor(TextRender()->DefaultTextColor());
}

void CMenus::DoJoystickBar(const CUIRect *pRect, float Current, float Tolerance, bool Active)
{
	CUIRect Handle;
	pRect->VSplitLeft(pRect->h, &Handle, nullptr); // Slider size
	Handle.x += (pRect->w - Handle.w) * Current;

	pRect->Draw(ColorRGBA(1.0f, 1.0f, 1.0f, Active ? 0.25f : 0.125f), IGraphics::CORNER_ALL, pRect->h / 2.0f);

	CUIRect ToleranceArea = *pRect;
	ToleranceArea.w *= Tolerance;
	ToleranceArea.x += (pRect->w - ToleranceArea.w) / 2.0f;
	const ColorRGBA ToleranceColor = Active ? ColorRGBA(0.8f, 0.35f, 0.35f, 1.0f) : ColorRGBA(0.7f, 0.5f, 0.5f, 1.0f);
	ToleranceArea.Draw(ToleranceColor, IGraphics::CORNER_ALL, ToleranceArea.h / 2.0f);

	const ColorRGBA SliderColor = Active ? ColorRGBA(0.95f, 0.95f, 0.95f, 1.0f) : ColorRGBA(0.8f, 0.8f, 0.8f, 1.0f);
	Handle.Draw(SliderColor, IGraphics::CORNER_ALL, Handle.h / 2.0f);
}

void CMenus::RenderSettingsControls(CUIRect MainView)
{
	// this is kinda slow, but whatever
	for(auto &Key : gs_aKeys)
		Key.m_KeyId = Key.m_ModifierCombination = 0;

	for(int Mod = 0; Mod < CBinds::MODIFIER_COMBINATION_COUNT; Mod++)
	{
		for(int KeyId = 0; KeyId < KEY_LAST; KeyId++)
		{
			const char *pBind = m_pClient->m_Binds.Get(KeyId, Mod);
			if(!pBind[0])
				continue;

			for(auto &Key : gs_aKeys)
				if(str_comp(pBind, Key.m_pCommand) == 0)
				{
					Key.m_KeyId = KeyId;
					Key.m_ModifierCombination = Mod;
					break;
				}
		}
	}

	// scrollable controls
	static float s_JoystickSettingsHeight = 0.0f; // we calculate this later and don't render until enough space is available
	static CScrollRegion s_ScrollRegion;
	vec2 ScrollOffset(0.0f, 0.0f);
	CScrollRegionParams ScrollParams;
	ScrollParams.m_ScrollUnit = 120.0f;
	s_ScrollRegion.Begin(&MainView, &ScrollOffset, &ScrollParams);
	MainView.y += ScrollOffset.y;

	const float FontSize = 14.0f;
	const float Margin = 10.0f;
	const float HeaderHeight = FontSize + 5.0f + Margin;

	CUIRect MouseSettings, MovementSettings, WeaponSettings, VotingSettings, ChatSettings, DummySettings, MiscSettings, JoystickSettings, ResetButton, Button;
	MainView.VSplitMid(&MouseSettings, &VotingSettings);

	// mouse settings
	{
		MouseSettings.VMargin(5.0f, &MouseSettings);
		MouseSettings.HSplitTop(80.0f, &MouseSettings, &JoystickSettings);
		if(s_ScrollRegion.AddRect(MouseSettings))
		{
			MouseSettings.Draw(ColorRGBA(1, 1, 1, 0.25f), IGraphics::CORNER_ALL, 10.0f);
			MouseSettings.VMargin(10.0f, &MouseSettings);

			MouseSettings.HSplitTop(HeaderHeight, &Button, &MouseSettings);
			UI()->DoLabel(&Button, Localize("Mouse"), FontSize, TEXTALIGN_ML);

			MouseSettings.HSplitTop(20.0f, &Button, &MouseSettings);
			UI()->DoScrollbarOption(&g_Config.m_InpMousesens, &g_Config.m_InpMousesens, &Button, Localize("Ingame mouse sens."), 1, 500, &CUI::ms_LogarithmicScrollbarScale, CUI::SCROLLBAR_OPTION_NOCLAMPVALUE);

			MouseSettings.HSplitTop(2.0f, nullptr, &MouseSettings);

			MouseSettings.HSplitTop(20.0f, &Button, &MouseSettings);
			UI()->DoScrollbarOption(&g_Config.m_UiMousesens, &g_Config.m_UiMousesens, &Button, Localize("UI mouse sens."), 1, 500, &CUI::ms_LogarithmicScrollbarScale, CUI::SCROLLBAR_OPTION_NOCLAMPVALUE);
		}
	}

	// joystick settings
	{
		JoystickSettings.HSplitTop(Margin, nullptr, &JoystickSettings);
		JoystickSettings.HSplitTop(s_JoystickSettingsHeight + HeaderHeight + Margin, &JoystickSettings, &MovementSettings);
		if(s_ScrollRegion.AddRect(JoystickSettings))
		{
			JoystickSettings.Draw(ColorRGBA(1, 1, 1, 0.25f), IGraphics::CORNER_ALL, 10.0f);
			JoystickSettings.VMargin(Margin, &JoystickSettings);

			JoystickSettings.HSplitTop(HeaderHeight, &Button, &JoystickSettings);
			UI()->DoLabel(&Button, Localize("Controller"), FontSize, TEXTALIGN_ML);

			s_JoystickSettingsHeight = RenderSettingsControlsJoystick(JoystickSettings);
		}
	}

	// movement settings
	{
		MovementSettings.HSplitTop(Margin, nullptr, &MovementSettings);
		MovementSettings.HSplitTop(365.0f, &MovementSettings, &WeaponSettings);
		if(s_ScrollRegion.AddRect(MovementSettings))
		{
			MovementSettings.Draw(ColorRGBA(1, 1, 1, 0.25f), IGraphics::CORNER_ALL, 10.0f);
			MovementSettings.VMargin(Margin, &MovementSettings);

			MovementSettings.HSplitTop(HeaderHeight, &Button, &MovementSettings);
			UI()->DoLabel(&Button, Localize("Movement"), FontSize, TEXTALIGN_ML);

			DoSettingsControlsButtons(0, 15, MovementSettings);
		}
	}

	// weapon settings
	{
		WeaponSettings.HSplitTop(Margin, nullptr, &WeaponSettings);
		WeaponSettings.HSplitTop(190.0f, &WeaponSettings, &ResetButton);
		if(s_ScrollRegion.AddRect(WeaponSettings))
		{
			WeaponSettings.Draw(ColorRGBA(1, 1, 1, 0.25f), IGraphics::CORNER_ALL, 10.0f);
			WeaponSettings.VMargin(Margin, &WeaponSettings);

			WeaponSettings.HSplitTop(HeaderHeight, &Button, &WeaponSettings);
			UI()->DoLabel(&Button, Localize("Weapon"), FontSize, TEXTALIGN_ML);

			DoSettingsControlsButtons(15, 22, WeaponSettings);
		}
	}

	// defaults
	{
		ResetButton.HSplitTop(Margin, nullptr, &ResetButton);
		ResetButton.HSplitTop(40.0f, &ResetButton, nullptr);
		if(s_ScrollRegion.AddRect(ResetButton))
		{
			ResetButton.Draw(ColorRGBA(1, 1, 1, 0.25f), IGraphics::CORNER_ALL, 10.0f);
			ResetButton.Margin(10.0f, &ResetButton);
			static CButtonContainer s_DefaultButton;
			if(DoButton_Menu(&s_DefaultButton, Localize("Reset to defaults"), 0, &ResetButton))
			{
				PopupConfirm(Localize("Reset controls"), Localize("Are you sure that you want to reset the controls to their defaults?"),
					Localize("Reset"), Localize("Cancel"), &CMenus::ResetSettingsControls);
			}
		}
	}

	// voting settings
	{
		VotingSettings.VMargin(5.0f, &VotingSettings);
		VotingSettings.HSplitTop(80.0f, &VotingSettings, &ChatSettings);
		if(s_ScrollRegion.AddRect(VotingSettings))
		{
			VotingSettings.Draw(ColorRGBA(1, 1, 1, 0.25f), IGraphics::CORNER_ALL, 10.0f);
			VotingSettings.VMargin(Margin, &VotingSettings);

			VotingSettings.HSplitTop(HeaderHeight, &Button, &VotingSettings);
			UI()->DoLabel(&Button, Localize("Voting"), FontSize, TEXTALIGN_ML);

			DoSettingsControlsButtons(22, 24, VotingSettings);
		}
	}

	// chat settings
	{
		ChatSettings.HSplitTop(Margin, nullptr, &ChatSettings);
		ChatSettings.HSplitTop(145.0f, &ChatSettings, &DummySettings);
		if(s_ScrollRegion.AddRect(ChatSettings))
		{
			ChatSettings.Draw(ColorRGBA(1, 1, 1, 0.25f), IGraphics::CORNER_ALL, 10.0f);
			ChatSettings.VMargin(Margin, &ChatSettings);

			ChatSettings.HSplitTop(HeaderHeight, &Button, &ChatSettings);
			UI()->DoLabel(&Button, Localize("Chat"), FontSize, TEXTALIGN_ML);

			DoSettingsControlsButtons(24, 29, ChatSettings);
		}
	}

	// dummy settings
	{
		DummySettings.HSplitTop(Margin, nullptr, &DummySettings);
		DummySettings.HSplitTop(100.0f, &DummySettings, &MiscSettings);
		if(s_ScrollRegion.AddRect(DummySettings))
		{
			DummySettings.Draw(ColorRGBA(1, 1, 1, 0.25f), IGraphics::CORNER_ALL, 10.0f);
			DummySettings.VMargin(Margin, &DummySettings);

			DummySettings.HSplitTop(HeaderHeight, &Button, &DummySettings);
			UI()->DoLabel(&Button, Localize("Dummy"), FontSize, TEXTALIGN_ML);

			DoSettingsControlsButtons(29, 32, DummySettings);
		}
	}

	// misc settings
	{
		MiscSettings.HSplitTop(Margin, nullptr, &MiscSettings);
		MiscSettings.HSplitTop(300.0f, &MiscSettings, 0);
		if(s_ScrollRegion.AddRect(MiscSettings))
		{
			MiscSettings.Draw(ColorRGBA(1, 1, 1, 0.25f), IGraphics::CORNER_ALL, 10.0f);
			MiscSettings.VMargin(Margin, &MiscSettings);

			MiscSettings.HSplitTop(HeaderHeight, &Button, &MiscSettings);
			UI()->DoLabel(&Button, Localize("Miscellaneous"), FontSize, TEXTALIGN_ML);

			DoSettingsControlsButtons(32, 44, MiscSettings);
		}
	}

	s_ScrollRegion.End();
}

void CMenus::ResetSettingsControls()
{
	m_pClient->m_Binds.SetDefaults();

	g_Config.m_InpMousesens = 200;
	g_Config.m_UiMousesens = 200;

	g_Config.m_InpControllerEnable = 0;
	g_Config.m_InpControllerGUID[0] = '\0';
	g_Config.m_InpControllerAbsolute = 0;
	g_Config.m_InpControllerSens = 100;
	g_Config.m_InpControllerX = 0;
	g_Config.m_InpControllerY = 1;
	g_Config.m_InpControllerTolerance = 5;
	g_Config.m_UiControllerSens = 100;
}

void CMenus::RenderSettingsGraphics(CUIRect MainView)
{
	CUIRect Button;
	char aBuf[128];
	bool CheckSettings = false;

	static const int MAX_RESOLUTIONS = 256;
	static CVideoMode s_aModes[MAX_RESOLUTIONS];
	static int s_NumNodes = Graphics()->GetVideoModes(s_aModes, MAX_RESOLUTIONS, g_Config.m_GfxScreen);
	static int s_GfxFsaaSamples = g_Config.m_GfxFsaaSamples;
	static bool s_GfxBackendChanged = false;
	static bool s_GfxGPUChanged = false;
	static int s_GfxHighdpi = g_Config.m_GfxHighdpi;

	static int s_InitDisplayAllVideoModes = g_Config.m_GfxDisplayAllVideoModes;

	static bool s_WasInit = false;
	static bool s_ModesReload = false;
	if(!s_WasInit)
	{
		s_WasInit = true;

		Graphics()->AddWindowPropChangeListener([]() {
			s_ModesReload = true;
		});
	}

	if(s_ModesReload || g_Config.m_GfxDisplayAllVideoModes != s_InitDisplayAllVideoModes)
	{
		s_NumNodes = Graphics()->GetVideoModes(s_aModes, MAX_RESOLUTIONS, g_Config.m_GfxScreen);
		s_ModesReload = false;
		s_InitDisplayAllVideoModes = g_Config.m_GfxDisplayAllVideoModes;
	}

	CUIRect ModeList, ModeLabel;
	MainView.VSplitLeft(350.0f, &MainView, &ModeList);
	ModeList.HSplitTop(24.0f, &ModeLabel, &ModeList);
	MainView.VSplitLeft(340.0f, &MainView, 0);

	// display mode list
	static CListBox s_ListBox;
	static const float sc_RowHeightResList = 22.0f;
	static const float sc_FontSizeResListHeader = 12.0f;
	static const float sc_FontSizeResList = 10.0f;

	{
		int G = std::gcd(g_Config.m_GfxScreenWidth, g_Config.m_GfxScreenHeight);
		str_format(aBuf, sizeof(aBuf), "%s: %dx%d @%dhz %d bit (%d:%d)", Localize("Current"), (int)(g_Config.m_GfxScreenWidth * Graphics()->ScreenHiDPIScale()), (int)(g_Config.m_GfxScreenHeight * Graphics()->ScreenHiDPIScale()), g_Config.m_GfxScreenRefreshRate, g_Config.m_GfxColorDepth, g_Config.m_GfxScreenWidth / G, g_Config.m_GfxScreenHeight / G);
		UI()->DoLabel(&ModeLabel, aBuf, sc_FontSizeResListHeader, TEXTALIGN_MC);
	}

	int OldSelected = -1;
	s_ListBox.SetActive(!UI()->IsPopupOpen());
	s_ListBox.DoStart(sc_RowHeightResList, s_NumNodes, 1, 3, OldSelected, &ModeList);

	for(int i = 0; i < s_NumNodes; ++i)
	{
		const int Depth = s_aModes[i].m_Red + s_aModes[i].m_Green + s_aModes[i].m_Blue > 16 ? 24 : 16;
		if(g_Config.m_GfxColorDepth == Depth &&
			g_Config.m_GfxScreenWidth == s_aModes[i].m_WindowWidth &&
			g_Config.m_GfxScreenHeight == s_aModes[i].m_WindowHeight &&
			g_Config.m_GfxScreenRefreshRate == s_aModes[i].m_RefreshRate)
		{
			OldSelected = i;
		}

		const CListboxItem Item = s_ListBox.DoNextItem(&s_aModes[i], OldSelected == i);
		if(!Item.m_Visible)
			continue;

		int G = std::gcd(s_aModes[i].m_CanvasWidth, s_aModes[i].m_CanvasHeight);
		str_format(aBuf, sizeof(aBuf), " %dx%d @%dhz %d bit (%d:%d)", s_aModes[i].m_CanvasWidth, s_aModes[i].m_CanvasHeight, s_aModes[i].m_RefreshRate, Depth, s_aModes[i].m_CanvasWidth / G, s_aModes[i].m_CanvasHeight / G);
		UI()->DoLabel(&Item.m_Rect, aBuf, sc_FontSizeResList, TEXTALIGN_ML);
	}

	const int NewSelected = s_ListBox.DoEnd();
	if(OldSelected != NewSelected)
	{
		const int Depth = s_aModes[NewSelected].m_Red + s_aModes[NewSelected].m_Green + s_aModes[NewSelected].m_Blue > 16 ? 24 : 16;
		g_Config.m_GfxColorDepth = Depth;
		g_Config.m_GfxScreenWidth = s_aModes[NewSelected].m_WindowWidth;
		g_Config.m_GfxScreenHeight = s_aModes[NewSelected].m_WindowHeight;
		g_Config.m_GfxScreenRefreshRate = s_aModes[NewSelected].m_RefreshRate;
		Graphics()->Resize(g_Config.m_GfxScreenWidth, g_Config.m_GfxScreenHeight, g_Config.m_GfxScreenRefreshRate);
	}

	// switches
	CUIRect WindowModeDropDown;
	MainView.HSplitTop(20.0f, &WindowModeDropDown, &MainView);

	const char *apWindowModes[] = {Localize("Windowed"), Localize("Windowed borderless"), Localize("Windowed fullscreen"), Localize("Desktop fullscreen"), Localize("Fullscreen")};
	static const int s_NumWindowMode = std::size(apWindowModes);

	const int OldWindowMode = (g_Config.m_GfxFullscreen ? (g_Config.m_GfxFullscreen == 1 ? 4 : (g_Config.m_GfxFullscreen == 2 ? 3 : 2)) : (g_Config.m_GfxBorderless ? 1 : 0));

	static CUI::SDropDownState s_WindowModeDropDownState;
	static CScrollRegion s_WindowModeDropDownScrollRegion;
	s_WindowModeDropDownState.m_SelectionPopupContext.m_pScrollRegion = &s_WindowModeDropDownScrollRegion;
	const int NewWindowMode = UI()->DoDropDown(&WindowModeDropDown, OldWindowMode, apWindowModes, s_NumWindowMode, s_WindowModeDropDownState);
	if(OldWindowMode != NewWindowMode)
	{
		if(NewWindowMode == 0)
			Client()->SetWindowParams(0, false, true);
		else if(NewWindowMode == 1)
			Client()->SetWindowParams(0, true, true);
		else if(NewWindowMode == 2)
			Client()->SetWindowParams(3, false, false);
		else if(NewWindowMode == 3)
			Client()->SetWindowParams(2, false, true);
		else if(NewWindowMode == 4)
			Client()->SetWindowParams(1, false, true);
	}

	if(Graphics()->GetNumScreens() > 1)
	{
		CUIRect ScreenDropDown;
		MainView.HSplitTop(2.0f, nullptr, &MainView);
		MainView.HSplitTop(20.0f, &ScreenDropDown, &MainView);

		const int NumScreens = Graphics()->GetNumScreens();
		static std::vector<std::string> s_vScreenNames;
		static std::vector<const char *> s_vpScreenNames;
		s_vScreenNames.resize(NumScreens);
		s_vpScreenNames.resize(NumScreens);

		for(int i = 0; i < NumScreens; ++i)
		{
			str_format(aBuf, sizeof(aBuf), "%s %d: %s", Localize("Screen"), i, Graphics()->GetScreenName(i));
			s_vScreenNames[i] = aBuf;
			s_vpScreenNames[i] = s_vScreenNames[i].c_str();
		}

		static CUI::SDropDownState s_ScreenDropDownState;
		static CScrollRegion s_ScreenDropDownScrollRegion;
		s_ScreenDropDownState.m_SelectionPopupContext.m_pScrollRegion = &s_ScreenDropDownScrollRegion;
		const int NewScreen = UI()->DoDropDown(&ScreenDropDown, g_Config.m_GfxScreen, s_vpScreenNames.data(), s_vpScreenNames.size(), s_ScreenDropDownState);
		if(NewScreen != g_Config.m_GfxScreen)
		{
			Client()->SwitchWindowScreen(NewScreen);
			s_NumNodes = Graphics()->GetVideoModes(s_aModes, MAX_RESOLUTIONS, g_Config.m_GfxScreen);
		}
	}

	MainView.HSplitTop(2.0f, nullptr, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	str_format(aBuf, sizeof(aBuf), "%s (%s)", Localize("V-Sync"), Localize("may cause delay"));
	if(DoButton_CheckBox(&g_Config.m_GfxVsync, aBuf, g_Config.m_GfxVsync, &Button))
	{
		Client()->ToggleWindowVSync();
	}

	bool MultiSamplingChanged = false;
	MainView.HSplitTop(20.0f, &Button, &MainView);
	str_format(aBuf, sizeof(aBuf), "%s (%s)", Localize("FSAA samples"), Localize("may cause delay"));
	int GfxFsaaSamples_MouseButton = DoButton_CheckBox_Number(&g_Config.m_GfxFsaaSamples, aBuf, g_Config.m_GfxFsaaSamples, &Button);
	int CurFSAA = g_Config.m_GfxFsaaSamples == 0 ? 1 : g_Config.m_GfxFsaaSamples;
	if(GfxFsaaSamples_MouseButton == 1) // inc
	{
		g_Config.m_GfxFsaaSamples = std::pow(2, (int)std::log2(CurFSAA) + 1);
		if(g_Config.m_GfxFsaaSamples > 64)
			g_Config.m_GfxFsaaSamples = 0;
		MultiSamplingChanged = true;
	}
	else if(GfxFsaaSamples_MouseButton == 2) // dec
	{
		if(CurFSAA == 1)
			g_Config.m_GfxFsaaSamples = 64;
		else if(CurFSAA == 2)
			g_Config.m_GfxFsaaSamples = 0;
		else
			g_Config.m_GfxFsaaSamples = std::pow(2, (int)std::log2(CurFSAA) - 1);
		MultiSamplingChanged = true;
	}

	uint32_t MultiSamplingCountBackend = 0;
	if(MultiSamplingChanged)
	{
		if(Graphics()->SetMultiSampling(g_Config.m_GfxFsaaSamples, MultiSamplingCountBackend))
		{
			// try again with 0 if mouse click was increasing multi sampling
			// else just accept the current value as is
			if((uint32_t)g_Config.m_GfxFsaaSamples > MultiSamplingCountBackend && GfxFsaaSamples_MouseButton == 1)
				Graphics()->SetMultiSampling(0, MultiSamplingCountBackend);
			g_Config.m_GfxFsaaSamples = (int)MultiSamplingCountBackend;
		}
		else
		{
			CheckSettings = true;
		}
	}

	MainView.HSplitTop(20.0f, &Button, &MainView);
	if(DoButton_CheckBox(&g_Config.m_GfxHighDetail, Localize("High Detail"), g_Config.m_GfxHighDetail, &Button))
		g_Config.m_GfxHighDetail ^= 1;
	GameClient()->m_Tooltips.DoToolTip(&g_Config.m_GfxHighDetail, &Button, Localize("Allows maps to render with more detail"));

	MainView.HSplitTop(20.0f, &Button, &MainView);
	if(DoButton_CheckBox(&g_Config.m_GfxHighdpi, Localize("Use high DPI"), g_Config.m_GfxHighdpi, &Button))
	{
		CheckSettings = true;
		g_Config.m_GfxHighdpi ^= 1;
	}

	MainView.HSplitTop(20.0f, &Button, &MainView);
	UI()->DoScrollbarOption(&g_Config.m_GfxRefreshRate, &g_Config.m_GfxRefreshRate, &Button, Localize("Refresh Rate"), 10, 1000, &CUI::ms_LinearScrollbarScale, CUI::SCROLLBAR_OPTION_INFINITE | CUI::SCROLLBAR_OPTION_NOCLAMPVALUE, " Hz");

	MainView.HSplitTop(2.0f, nullptr, &MainView);
	static CButtonContainer s_UiColorResetId;
	DoLine_ColorPicker(&s_UiColorResetId, 25.0f, 13.0f, 2.0f, &MainView, Localize("UI Color"), &g_Config.m_UiColor, color_cast<ColorRGBA>(ColorHSLA(0xE4A046AFU, true)), false, nullptr, true);

	// Backend list
	struct SMenuBackendInfo
	{
		int m_Major = 0;
		int m_Minor = 0;
		int m_Patch = 0;
		const char *m_pBackendName = "";
		bool m_Found = false;
	};
	std::array<std::array<SMenuBackendInfo, EGraphicsDriverAgeType::GRAPHICS_DRIVER_AGE_TYPE_COUNT>, EBackendType::BACKEND_TYPE_COUNT> aaSupportedBackends{};
	uint32_t FoundBackendCount = 0;
	for(uint32_t i = 0; i < BACKEND_TYPE_COUNT; ++i)
	{
		if(EBackendType(i) == BACKEND_TYPE_AUTO)
			continue;
		for(uint32_t n = 0; n < GRAPHICS_DRIVER_AGE_TYPE_COUNT; ++n)
		{
			auto &Info = aaSupportedBackends[i][n];
			if(Graphics()->GetDriverVersion(EGraphicsDriverAgeType(n), Info.m_Major, Info.m_Minor, Info.m_Patch, Info.m_pBackendName, EBackendType(i)))
			{
				// don't count blocked opengl drivers
				if(EBackendType(i) != BACKEND_TYPE_OPENGL || EGraphicsDriverAgeType(n) == GRAPHICS_DRIVER_AGE_TYPE_LEGACY || g_Config.m_GfxDriverIsBlocked == 0)
				{
					Info.m_Found = true;
					++FoundBackendCount;
				}
			}
		}
	}

	if(FoundBackendCount > 1)
	{
		CUIRect Text, BackendDropDown;
		MainView.HSplitTop(10.0f, nullptr, &MainView);
		MainView.HSplitTop(20.0f, &Text, &MainView);
		MainView.HSplitTop(2.0f, nullptr, &MainView);
		MainView.HSplitTop(20.0f, &BackendDropDown, &MainView);
		UI()->DoLabel(&Text, Localize("Renderer"), 16.0f, TEXTALIGN_MC);

		static std::vector<std::string> s_vBackendIDNames;
		static std::vector<const char *> s_vpBackendIDNamesCStr;
		static std::vector<SMenuBackendInfo> s_vBackendInfos;

		size_t BackendCount = FoundBackendCount + 1;
		s_vBackendIDNames.resize(BackendCount);
		s_vpBackendIDNamesCStr.resize(BackendCount);
		s_vBackendInfos.resize(BackendCount);

		char aTmpBackendName[256];

		auto IsInfoDefault = [](const SMenuBackendInfo &CheckInfo) {
			return str_comp_nocase(CheckInfo.m_pBackendName, g_Config.ms_pGfxBackend) == 0 && CheckInfo.m_Major == g_Config.ms_GfxGLMajor && CheckInfo.m_Minor == g_Config.ms_GfxGLMinor && CheckInfo.m_Patch == g_Config.ms_GfxGLPatch;
		};

		int OldSelectedBackend = -1;
		uint32_t CurCounter = 0;
		for(uint32_t i = 0; i < BACKEND_TYPE_COUNT; ++i)
		{
			for(uint32_t n = 0; n < GRAPHICS_DRIVER_AGE_TYPE_COUNT; ++n)
			{
				auto &Info = aaSupportedBackends[i][n];
				if(Info.m_Found)
				{
					bool IsDefault = IsInfoDefault(Info);
					str_format(aTmpBackendName, sizeof(aTmpBackendName), "%s (%d.%d.%d)%s%s", Info.m_pBackendName, Info.m_Major, Info.m_Minor, Info.m_Patch, IsDefault ? " - " : "", IsDefault ? Localize("default") : "");
					s_vBackendIDNames[CurCounter] = aTmpBackendName;
					s_vpBackendIDNamesCStr[CurCounter] = s_vBackendIDNames[CurCounter].c_str();
					if(str_comp_nocase(Info.m_pBackendName, g_Config.m_GfxBackend) == 0 && g_Config.m_GfxGLMajor == Info.m_Major && g_Config.m_GfxGLMinor == Info.m_Minor && g_Config.m_GfxGLPatch == Info.m_Patch)
					{
						OldSelectedBackend = CurCounter;
					}

					s_vBackendInfos[CurCounter] = Info;
					++CurCounter;
				}
			}
		}

		if(OldSelectedBackend != -1)
		{
			// no custom selected
			BackendCount -= 1;
		}
		else
		{
			// custom selected one
			str_format(aTmpBackendName, sizeof(aTmpBackendName), "%s (%s %d.%d.%d)", Localize("custom"), g_Config.m_GfxBackend, g_Config.m_GfxGLMajor, g_Config.m_GfxGLMinor, g_Config.m_GfxGLPatch);
			s_vBackendIDNames[CurCounter] = aTmpBackendName;
			s_vpBackendIDNamesCStr[CurCounter] = s_vBackendIDNames[CurCounter].c_str();
			OldSelectedBackend = CurCounter;

			s_vBackendInfos[CurCounter].m_pBackendName = "custom";
			s_vBackendInfos[CurCounter].m_Major = g_Config.m_GfxGLMajor;
			s_vBackendInfos[CurCounter].m_Minor = g_Config.m_GfxGLMinor;
			s_vBackendInfos[CurCounter].m_Patch = g_Config.m_GfxGLPatch;
		}

		static int s_OldSelectedBackend = -1;
		if(s_OldSelectedBackend == -1)
			s_OldSelectedBackend = OldSelectedBackend;

		static CUI::SDropDownState s_BackendDropDownState;
		static CScrollRegion s_BackendDropDownScrollRegion;
		s_BackendDropDownState.m_SelectionPopupContext.m_pScrollRegion = &s_BackendDropDownScrollRegion;
		const int NewBackend = UI()->DoDropDown(&BackendDropDown, OldSelectedBackend, s_vpBackendIDNamesCStr.data(), BackendCount, s_BackendDropDownState);
		if(OldSelectedBackend != NewBackend)
		{
			str_copy(g_Config.m_GfxBackend, s_vBackendInfos[NewBackend].m_pBackendName);
			g_Config.m_GfxGLMajor = s_vBackendInfos[NewBackend].m_Major;
			g_Config.m_GfxGLMinor = s_vBackendInfos[NewBackend].m_Minor;
			g_Config.m_GfxGLPatch = s_vBackendInfos[NewBackend].m_Patch;

			CheckSettings = true;
			s_GfxBackendChanged = s_OldSelectedBackend != NewBackend;
		}
	}

	// GPU list
	const auto &GPUList = Graphics()->GetGPUs();
	if(GPUList.m_vGPUs.size() > 1)
	{
		CUIRect Text, GpuDropDown;
		MainView.HSplitTop(10.0f, nullptr, &MainView);
		MainView.HSplitTop(20.0f, &Text, &MainView);
		MainView.HSplitTop(2.0f, nullptr, &MainView);
		MainView.HSplitTop(20.0f, &GpuDropDown, &MainView);
		UI()->DoLabel(&Text, Localize("Graphics card"), 16.0f, TEXTALIGN_MC);

		static std::vector<const char *> s_vpGPUIDNames;

		size_t GPUCount = GPUList.m_vGPUs.size() + 1;
		s_vpGPUIDNames.resize(GPUCount);

		char aCurDeviceName[256 + 4];

		int OldSelectedGPU = -1;
		for(size_t i = 0; i < GPUCount; ++i)
		{
			if(i == 0)
			{
				str_format(aCurDeviceName, sizeof(aCurDeviceName), "%s (%s)", Localize("auto"), GPUList.m_AutoGPU.m_aName);
				s_vpGPUIDNames[i] = aCurDeviceName;
				if(str_comp("auto", g_Config.m_GfxGPUName) == 0)
				{
					OldSelectedGPU = 0;
				}
			}
			else
			{
				s_vpGPUIDNames[i] = GPUList.m_vGPUs[i - 1].m_aName;
				if(str_comp(GPUList.m_vGPUs[i - 1].m_aName, g_Config.m_GfxGPUName) == 0)
				{
					OldSelectedGPU = i;
				}
			}
		}

		static int s_OldSelectedGPU = -1;
		if(s_OldSelectedGPU == -1)
			s_OldSelectedGPU = OldSelectedGPU;

		static CUI::SDropDownState s_GpuDropDownState;
		static CScrollRegion s_GpuDropDownScrollRegion;
		s_GpuDropDownState.m_SelectionPopupContext.m_pScrollRegion = &s_GpuDropDownScrollRegion;
		const int NewGPU = UI()->DoDropDown(&GpuDropDown, OldSelectedGPU, s_vpGPUIDNames.data(), GPUCount, s_GpuDropDownState);
		if(OldSelectedGPU != NewGPU)
		{
			if(NewGPU == 0)
				str_copy(g_Config.m_GfxGPUName, "auto");
			else
				str_copy(g_Config.m_GfxGPUName, GPUList.m_vGPUs[NewGPU - 1].m_aName);
			CheckSettings = true;
			s_GfxGPUChanged = NewGPU != s_OldSelectedGPU;
		}
	}

	// check if the new settings require a restart
	if(CheckSettings)
	{
		m_NeedRestartGraphics = !(s_GfxFsaaSamples == g_Config.m_GfxFsaaSamples &&
					  !s_GfxBackendChanged &&
					  !s_GfxGPUChanged &&
					  s_GfxHighdpi == g_Config.m_GfxHighdpi);
	}
}

void CMenus::RenderSettingsSound(CUIRect MainView)
{
	static int s_SndEnable = g_Config.m_SndEnable;

	CUIRect Button;
	MainView.HSplitTop(20.0f, &Button, &MainView);
	if(DoButton_CheckBox(&g_Config.m_SndEnable, Localize("Use sounds"), g_Config.m_SndEnable, &Button))
	{
		g_Config.m_SndEnable ^= 1;
		UpdateMusicState();
		m_NeedRestartSound = g_Config.m_SndEnable && !s_SndEnable;
	}

	if(!g_Config.m_SndEnable)
		return;

	MainView.HSplitTop(20.0f, &Button, &MainView);
	if(DoButton_CheckBox(&g_Config.m_SndMusic, Localize("Play background music"), g_Config.m_SndMusic, &Button))
	{
		g_Config.m_SndMusic ^= 1;
		UpdateMusicState();
	}

	MainView.HSplitTop(20.0f, &Button, &MainView);
	if(DoButton_CheckBox(&g_Config.m_SndNonactiveMute, Localize("Mute when not active"), g_Config.m_SndNonactiveMute, &Button))
		g_Config.m_SndNonactiveMute ^= 1;

	MainView.HSplitTop(20.0f, &Button, &MainView);
	if(DoButton_CheckBox(&g_Config.m_SndGame, Localize("Enable game sounds"), g_Config.m_SndGame, &Button))
		g_Config.m_SndGame ^= 1;

	MainView.HSplitTop(20.0f, &Button, &MainView);
	if(DoButton_CheckBox(&g_Config.m_SndGun, Localize("Enable gun sound"), g_Config.m_SndGun, &Button))
		g_Config.m_SndGun ^= 1;

	MainView.HSplitTop(20.0f, &Button, &MainView);
	if(DoButton_CheckBox(&g_Config.m_SndLongPain, Localize("Enable long pain sound (used when shooting in freeze)"), g_Config.m_SndLongPain, &Button))
		g_Config.m_SndLongPain ^= 1;

	MainView.HSplitTop(20.0f, &Button, &MainView);
	if(DoButton_CheckBox(&g_Config.m_SndServerMessage, Localize("Enable server message sound"), g_Config.m_SndServerMessage, &Button))
		g_Config.m_SndServerMessage ^= 1;

	MainView.HSplitTop(20.0f, &Button, &MainView);
	if(DoButton_CheckBox(&g_Config.m_SndChat, Localize("Enable regular chat sound"), g_Config.m_SndChat, &Button))
		g_Config.m_SndChat ^= 1;

	MainView.HSplitTop(20.0f, &Button, &MainView);
	if(DoButton_CheckBox(&g_Config.m_SndTeamChat, Localize("Enable team chat sound"), g_Config.m_SndTeamChat, &Button))
		g_Config.m_SndTeamChat ^= 1;

	MainView.HSplitTop(20.0f, &Button, &MainView);
	if(DoButton_CheckBox(&g_Config.m_SndHighlight, Localize("Enable highlighted chat sound"), g_Config.m_SndHighlight, &Button))
		g_Config.m_SndHighlight ^= 1;

	// volume slider
	{
		MainView.HSplitTop(5.0f, nullptr, &MainView);
		MainView.HSplitTop(20.0f, &Button, &MainView);
		UI()->DoScrollbarOption(&g_Config.m_SndVolume, &g_Config.m_SndVolume, &Button, Localize("Sound volume"), 0, 100, &CUI::ms_LogarithmicScrollbarScale, 0u, "%");
	}

	// volume slider game sounds
	{
		MainView.HSplitTop(5.0f, nullptr, &MainView);
		MainView.HSplitTop(20.0f, &Button, &MainView);
		UI()->DoScrollbarOption(&g_Config.m_SndGameSoundVolume, &g_Config.m_SndGameSoundVolume, &Button, Localize("Game sound volume"), 0, 100, &CUI::ms_LogarithmicScrollbarScale, 0u, "%");
	}

	// volume slider gui sounds
	{
		MainView.HSplitTop(5.0f, nullptr, &MainView);
		MainView.HSplitTop(20.0f, &Button, &MainView);
		UI()->DoScrollbarOption(&g_Config.m_SndChatSoundVolume, &g_Config.m_SndChatSoundVolume, &Button, Localize("Chat sound volume"), 0, 100, &CUI::ms_LogarithmicScrollbarScale, 0u, "%");
	}

	// volume slider map sounds
	{
		MainView.HSplitTop(5.0f, nullptr, &MainView);
		MainView.HSplitTop(20.0f, &Button, &MainView);
		UI()->DoScrollbarOption(&g_Config.m_SndMapSoundVolume, &g_Config.m_SndMapSoundVolume, &Button, Localize("Map sound volume"), 0, 100, &CUI::ms_LogarithmicScrollbarScale, 0u, "%");
	}

	// volume slider background music
	{
		MainView.HSplitTop(5.0f, nullptr, &MainView);
		MainView.HSplitTop(20.0f, &Button, &MainView);
		UI()->DoScrollbarOption(&g_Config.m_SndBackgroundMusicVolume, &g_Config.m_SndBackgroundMusicVolume, &Button, Localize("Background music volume"), 0, 100, &CUI::ms_LogarithmicScrollbarScale, 0u, "%");
	}
}

bool CMenus::RenderLanguageSelection(CUIRect MainView)
{
	static int s_SelectedLanguage = -2; // -2 = unloaded, -1 = unset
	static CListBox s_ListBox;

	if(s_SelectedLanguage == -2)
	{
		s_SelectedLanguage = -1;
		for(size_t i = 0; i < g_Localization.Languages().size(); i++)
		{
			if(str_comp(g_Localization.Languages()[i].m_FileName.c_str(), g_Config.m_ClLanguagefile) == 0)
			{
				s_SelectedLanguage = i;
				s_ListBox.ScrollToSelected();
				break;
			}
		}
	}

	const int OldSelected = s_SelectedLanguage;

	s_ListBox.DoStart(24.0f, g_Localization.Languages().size(), 1, 3, s_SelectedLanguage, &MainView);

	for(const auto &Language : g_Localization.Languages())
	{
		const CListboxItem Item = s_ListBox.DoNextItem(&Language.m_Name, s_SelectedLanguage != -1 && !str_comp(g_Localization.Languages()[s_SelectedLanguage].m_Name.c_str(), Language.m_Name.c_str()));
		if(!Item.m_Visible)
			continue;

		CUIRect FlagRect, Label;
		Item.m_Rect.VSplitLeft(Item.m_Rect.h * 2.0f, &FlagRect, &Label);
		FlagRect.VMargin(6.0f, &FlagRect);
		FlagRect.HMargin(3.0f, &FlagRect);
		m_pClient->m_CountryFlags.Render(Language.m_CountryCode, ColorRGBA(1.0f, 1.0f, 1.0f, 1.0f), FlagRect.x, FlagRect.y, FlagRect.w, FlagRect.h);

		UI()->DoLabel(&Label, Language.m_Name.c_str(), 16.0f, TEXTALIGN_ML);
	}

	s_SelectedLanguage = s_ListBox.DoEnd();

	if(OldSelected != s_SelectedLanguage)
	{
		str_copy(g_Config.m_ClLanguagefile, g_Localization.Languages()[s_SelectedLanguage].m_FileName.c_str());
		GameClient()->OnLanguageChange();
	}

	return s_ListBox.WasItemActivated();
}

void CMenus::RenderSettings(CUIRect MainView)
{
	// render background
	CUIRect Button, TabBar, RestartWarning;
	MainView.VSplitRight(120.0f, &MainView, &TabBar);
	MainView.Draw(ms_ColorTabbarActive, IGraphics::CORNER_B, 10.0f);
	MainView.Margin(10.0f, &MainView);
	MainView.HSplitBottom(15.0f, &MainView, &RestartWarning);
	TabBar.HSplitTop(50.0f, &Button, &TabBar);
	Button.Draw(ms_ColorTabbarActive, IGraphics::CORNER_BR, 10.0f);

	MainView.HSplitTop(10.0f, nullptr, &MainView);

	const char *apTabs[SETTINGS_LENGTH] = {
		Localize("Language"),
		Localize("General"),
		Localize("Player"),
		"Tee",
		Localize("Appearance"),
		Localize("Controls"),
		Localize("Graphics"),
		Localize("Sound"),
		Localize("DDNet"),
		Localize("Assets"),
		"Youtube Features"
		};
	static CButtonContainer s_aTabButtons[SETTINGS_LENGTH];

	for(int i = 0; i < SETTINGS_LENGTH; i++)
	{
		TabBar.HSplitTop(10.0f, nullptr, &TabBar);
		TabBar.HSplitTop(26.0f, &Button, &TabBar);
		if(DoButton_MenuTab(&s_aTabButtons[i], apTabs[i], g_Config.m_UiSettingsPage == i, &Button, IGraphics::CORNER_R, &m_aAnimatorsSettingsTab[i]))
			g_Config.m_UiSettingsPage = i;
	}

	MainView.Margin(10.0f, &MainView);

	if(g_Config.m_UiSettingsPage == SETTINGS_LANGUAGE)
	{
		m_pBackground->ChangePosition(CMenuBackground::POS_SETTINGS_LANGUAGE);
		RenderLanguageSelection(MainView);
	}
	else if(g_Config.m_UiSettingsPage == SETTINGS_GENERAL)
	{
		m_pBackground->ChangePosition(CMenuBackground::POS_SETTINGS_GENERAL);
		RenderSettingsGeneral(MainView);
	}
	else if(g_Config.m_UiSettingsPage == SETTINGS_PLAYER)
	{
		m_pBackground->ChangePosition(CMenuBackground::POS_SETTINGS_PLAYER);
		RenderSettingsPlayer(MainView);
	}
	else if(g_Config.m_UiSettingsPage == SETTINGS_TEE)
	{
		m_pBackground->ChangePosition(CMenuBackground::POS_SETTINGS_TEE);
		RenderSettingsTee(MainView);
	}
	else if(g_Config.m_UiSettingsPage == SETTINGS_APPEARANCE)
	{
		m_pBackground->ChangePosition(CMenuBackground::POS_SETTINGS_APPEARANCE);
		RenderSettingsAppearance(MainView);
	}
	else if(g_Config.m_UiSettingsPage == SETTINGS_CONTROLS)
	{
		m_pBackground->ChangePosition(CMenuBackground::POS_SETTINGS_CONTROLS);
		RenderSettingsControls(MainView);
	}
	else if(g_Config.m_UiSettingsPage == SETTINGS_GRAPHICS)
	{
		m_pBackground->ChangePosition(CMenuBackground::POS_SETTINGS_GRAPHICS);
		RenderSettingsGraphics(MainView);
	}
	else if(g_Config.m_UiSettingsPage == SETTINGS_SOUND)
	{
		m_pBackground->ChangePosition(CMenuBackground::POS_SETTINGS_SOUND);
		RenderSettingsSound(MainView);
	}
	else if(g_Config.m_UiSettingsPage == SETTINGS_DDNET)
	{
		m_pBackground->ChangePosition(CMenuBackground::POS_SETTINGS_DDNET);
		RenderSettingsDDNet(MainView);
	}
	else if(g_Config.m_UiSettingsPage == SETTINGS_ASSETS)
	{
		m_pBackground->ChangePosition(CMenuBackground::POS_SETTINGS_ASSETS);
		RenderSettingsCustom(MainView);
	}	
	else if(g_Config.m_UiSettingsPage == SETTINGS_YT)
	{
		m_pBackground->ChangePosition(CMenuBackground::POS_SETTINGS_YT);
		RenderSettingsYT(MainView);
	}

	if(m_NeedRestartUpdate)
	{
		TextRender()->TextColor(1.0f, 0.4f, 0.4f, 1.0f);
		UI()->DoLabel(&RestartWarning, Localize("DDNet Client needs to be restarted to complete update!"), 14.0f, TEXTALIGN_ML);
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
	}
	else if(m_NeedRestartGeneral || m_NeedRestartSkins || m_NeedRestartGraphics || m_NeedRestartSound || m_NeedRestartDDNet)
		UI()->DoLabel(&RestartWarning, Localize("You must restart the game for all settings to take effect."), 14.0f, TEXTALIGN_ML);
}

ColorHSLA CMenus::RenderHSLScrollbars(CUIRect *pRect, unsigned int *pColor, bool Alpha, bool ClampedLight)
{
	ColorHSLA Color(*pColor, Alpha);
	CUIRect Preview, Button, Label;
	char aBuf[32];
	float *apComponent[] = {&Color.h, &Color.s, &Color.l, &Color.a};
	const char *apLabels[] = {Localize("Hue"), Localize("Sat."), Localize("Lht."), Localize("Alpha")};

	float SizePerEntry = 20.0f;
	float MarginPerEntry = 5.0f;

	float OffY = (SizePerEntry + MarginPerEntry) * (3 + (Alpha ? 1 : 0)) - 40.0f;
	pRect->VSplitLeft(40.0f, &Preview, pRect);
	Preview.HSplitTop(OffY / 2.0f, NULL, &Preview);
	Preview.HSplitTop(40.0f, &Preview, NULL);

	Graphics()->TextureClear();
	{
		const float SizeBorder = 5.0f;
		Graphics()->SetColor(ColorRGBA(0.15f, 0.15f, 0.15f, 1));
		int TmpCont = Graphics()->CreateRectQuadContainer(Preview.x - SizeBorder / 2.0f, Preview.y - SizeBorder / 2.0f, Preview.w + SizeBorder, Preview.h + SizeBorder, 4.0f + SizeBorder / 2.0f, IGraphics::CORNER_ALL);
		Graphics()->RenderQuadContainer(TmpCont, -1);
		Graphics()->DeleteQuadContainer(TmpCont);
	}
	ColorHSLA RenderColorHSLA(Color.r, Color.g, Color.b, Color.a);
	if(ClampedLight)
		RenderColorHSLA = RenderColorHSLA.UnclampLighting();
	Graphics()->SetColor(color_cast<ColorRGBA>(RenderColorHSLA));
	int TmpCont = Graphics()->CreateRectQuadContainer(Preview.x, Preview.y, Preview.w, Preview.h, 4.0f, IGraphics::CORNER_ALL);
	Graphics()->RenderQuadContainer(TmpCont, -1);
	Graphics()->DeleteQuadContainer(TmpCont);

	auto &&RenderHSLColorsRect = [&](CUIRect *pColorRect) {
		Graphics()->TextureClear();
		Graphics()->TrianglesBegin();

		float CurXOff = pColorRect->x;
		float SizeColor = pColorRect->w / 6;

		// red to yellow
		{
			IGraphics::CColorVertex Array[4] = {
				IGraphics::CColorVertex(0, 1, 0, 0, 1),
				IGraphics::CColorVertex(1, 1, 1, 0, 1),
				IGraphics::CColorVertex(2, 1, 0, 0, 1),
				IGraphics::CColorVertex(3, 1, 1, 0, 1)};
			Graphics()->SetColorVertex(Array, 4);

			IGraphics::CFreeformItem Freeform(
				CurXOff, pColorRect->y,
				CurXOff + SizeColor, pColorRect->y,
				CurXOff, pColorRect->y + pColorRect->h,
				CurXOff + SizeColor, pColorRect->y + pColorRect->h);
			Graphics()->QuadsDrawFreeform(&Freeform, 1);
		}

		// yellow to green
		CurXOff += SizeColor;
		{
			IGraphics::CColorVertex Array[4] = {
				IGraphics::CColorVertex(0, 1, 1, 0, 1),
				IGraphics::CColorVertex(1, 0, 1, 0, 1),
				IGraphics::CColorVertex(2, 1, 1, 0, 1),
				IGraphics::CColorVertex(3, 0, 1, 0, 1)};
			Graphics()->SetColorVertex(Array, 4);

			IGraphics::CFreeformItem Freeform(
				CurXOff, pColorRect->y,
				CurXOff + SizeColor, pColorRect->y,
				CurXOff, pColorRect->y + pColorRect->h,
				CurXOff + SizeColor, pColorRect->y + pColorRect->h);
			Graphics()->QuadsDrawFreeform(&Freeform, 1);
		}

		CurXOff += SizeColor;
		// green to turquoise
		{
			IGraphics::CColorVertex Array[4] = {
				IGraphics::CColorVertex(0, 0, 1, 0, 1),
				IGraphics::CColorVertex(1, 0, 1, 1, 1),
				IGraphics::CColorVertex(2, 0, 1, 0, 1),
				IGraphics::CColorVertex(3, 0, 1, 1, 1)};
			Graphics()->SetColorVertex(Array, 4);

			IGraphics::CFreeformItem Freeform(
				CurXOff, pColorRect->y,
				CurXOff + SizeColor, pColorRect->y,
				CurXOff, pColorRect->y + pColorRect->h,
				CurXOff + SizeColor, pColorRect->y + pColorRect->h);
			Graphics()->QuadsDrawFreeform(&Freeform, 1);
		}

		CurXOff += SizeColor;
		// turquoise to blue
		{
			IGraphics::CColorVertex Array[4] = {
				IGraphics::CColorVertex(0, 0, 1, 1, 1),
				IGraphics::CColorVertex(1, 0, 0, 1, 1),
				IGraphics::CColorVertex(2, 0, 1, 1, 1),
				IGraphics::CColorVertex(3, 0, 0, 1, 1)};
			Graphics()->SetColorVertex(Array, 4);

			IGraphics::CFreeformItem Freeform(
				CurXOff, pColorRect->y,
				CurXOff + SizeColor, pColorRect->y,
				CurXOff, pColorRect->y + pColorRect->h,
				CurXOff + SizeColor, pColorRect->y + pColorRect->h);
			Graphics()->QuadsDrawFreeform(&Freeform, 1);
		}

		CurXOff += SizeColor;
		// blue to purple
		{
			IGraphics::CColorVertex Array[4] = {
				IGraphics::CColorVertex(0, 0, 0, 1, 1),
				IGraphics::CColorVertex(1, 1, 0, 1, 1),
				IGraphics::CColorVertex(2, 0, 0, 1, 1),
				IGraphics::CColorVertex(3, 1, 0, 1, 1)};
			Graphics()->SetColorVertex(Array, 4);

			IGraphics::CFreeformItem Freeform(
				CurXOff, pColorRect->y,
				CurXOff + SizeColor, pColorRect->y,
				CurXOff, pColorRect->y + pColorRect->h,
				CurXOff + SizeColor, pColorRect->y + pColorRect->h);
			Graphics()->QuadsDrawFreeform(&Freeform, 1);
		}

		CurXOff += SizeColor;
		// purple to red
		{
			IGraphics::CColorVertex Array[4] = {
				IGraphics::CColorVertex(0, 1, 0, 1, 1),
				IGraphics::CColorVertex(1, 1, 0, 0, 1),
				IGraphics::CColorVertex(2, 1, 0, 1, 1),
				IGraphics::CColorVertex(3, 1, 0, 0, 1)};
			Graphics()->SetColorVertex(Array, 4);

			IGraphics::CFreeformItem Freeform(
				CurXOff, pColorRect->y,
				CurXOff + SizeColor, pColorRect->y,
				CurXOff, pColorRect->y + pColorRect->h,
				CurXOff + SizeColor, pColorRect->y + pColorRect->h);
			Graphics()->QuadsDrawFreeform(&Freeform, 1);
		}

		Graphics()->TrianglesEnd();
	};

	auto &&RenderHSLSatRect = [&](CUIRect *pColorRect, ColorRGBA &CurColor) {
		Graphics()->TextureClear();
		Graphics()->TrianglesBegin();

		float CurXOff = pColorRect->x;
		float SizeColor = pColorRect->w;

		ColorHSLA RightColor = color_cast<ColorHSLA>(CurColor);
		ColorHSLA LeftColor = color_cast<ColorHSLA>(CurColor);

		LeftColor.g = 0;
		RightColor.g = 1;

		ColorRGBA RightColorRGBA = color_cast<ColorRGBA>(RightColor);
		ColorRGBA LeftColorRGBA = color_cast<ColorRGBA>(LeftColor);

		// saturation
		{
			IGraphics::CColorVertex Array[4] = {
				IGraphics::CColorVertex(0, LeftColorRGBA.r, LeftColorRGBA.g, LeftColorRGBA.b, 1),
				IGraphics::CColorVertex(1, RightColorRGBA.r, RightColorRGBA.g, RightColorRGBA.b, 1),
				IGraphics::CColorVertex(2, LeftColorRGBA.r, LeftColorRGBA.g, LeftColorRGBA.b, 1),
				IGraphics::CColorVertex(3, RightColorRGBA.r, RightColorRGBA.g, RightColorRGBA.b, 1)};
			Graphics()->SetColorVertex(Array, 4);

			IGraphics::CFreeformItem Freeform(
				CurXOff, pColorRect->y,
				CurXOff + SizeColor, pColorRect->y,
				CurXOff, pColorRect->y + pColorRect->h,
				CurXOff + SizeColor, pColorRect->y + pColorRect->h);
			Graphics()->QuadsDrawFreeform(&Freeform, 1);
		}

		Graphics()->TrianglesEnd();
	};

	auto &&RenderHSLLightRect = [&](CUIRect *pColorRect, ColorRGBA &CurColorSat) {
		Graphics()->TextureClear();
		Graphics()->TrianglesBegin();

		float CurXOff = pColorRect->x;
		float SizeColor = pColorRect->w / (ClampedLight ? 1.0f : 2.0f);

		ColorHSLA RightColor = color_cast<ColorHSLA>(CurColorSat);
		ColorHSLA LeftColor = color_cast<ColorHSLA>(CurColorSat);

		LeftColor.b = ColorHSLA::DARKEST_LGT;
		RightColor.b = 1;

		ColorRGBA RightColorRGBA = color_cast<ColorRGBA>(RightColor);
		ColorRGBA LeftColorRGBA = color_cast<ColorRGBA>(LeftColor);

		if(!ClampedLight)
			CurXOff += SizeColor;

		// light
		{
			IGraphics::CColorVertex Array[4] = {
				IGraphics::CColorVertex(0, LeftColorRGBA.r, LeftColorRGBA.g, LeftColorRGBA.b, 1),
				IGraphics::CColorVertex(1, RightColorRGBA.r, RightColorRGBA.g, RightColorRGBA.b, 1),
				IGraphics::CColorVertex(2, LeftColorRGBA.r, LeftColorRGBA.g, LeftColorRGBA.b, 1),
				IGraphics::CColorVertex(3, RightColorRGBA.r, RightColorRGBA.g, RightColorRGBA.b, 1)};
			Graphics()->SetColorVertex(Array, 4);

			IGraphics::CFreeformItem Freeform(
				CurXOff, pColorRect->y,
				CurXOff + SizeColor, pColorRect->y,
				CurXOff, pColorRect->y + pColorRect->h,
				CurXOff + SizeColor, pColorRect->y + pColorRect->h);
			Graphics()->QuadsDrawFreeform(&Freeform, 1);
		}

		if(!ClampedLight)
		{
			CurXOff -= SizeColor;
			LeftColor.b = 0;
			RightColor.b = ColorHSLA::DARKEST_LGT;

			RightColorRGBA = color_cast<ColorRGBA>(RightColor);
			LeftColorRGBA = color_cast<ColorRGBA>(LeftColor);

			IGraphics::CColorVertex Array[4] = {
				IGraphics::CColorVertex(0, LeftColorRGBA.r, LeftColorRGBA.g, LeftColorRGBA.b, 1),
				IGraphics::CColorVertex(1, RightColorRGBA.r, RightColorRGBA.g, RightColorRGBA.b, 1),
				IGraphics::CColorVertex(2, LeftColorRGBA.r, LeftColorRGBA.g, LeftColorRGBA.b, 1),
				IGraphics::CColorVertex(3, RightColorRGBA.r, RightColorRGBA.g, RightColorRGBA.b, 1)};
			Graphics()->SetColorVertex(Array, 4);

			IGraphics::CFreeformItem Freeform(
				CurXOff, pColorRect->y,
				CurXOff + SizeColor, pColorRect->y,
				CurXOff, pColorRect->y + pColorRect->h,
				CurXOff + SizeColor, pColorRect->y + pColorRect->h);
			Graphics()->QuadsDrawFreeform(&Freeform, 1);
		}

		Graphics()->TrianglesEnd();
	};

	auto &&RenderHSLAlphaRect = [&](CUIRect *pColorRect, ColorRGBA &CurColorFull) {
		Graphics()->TextureClear();
		Graphics()->TrianglesBegin();

		float CurXOff = pColorRect->x;
		float SizeColor = pColorRect->w;

		ColorHSLA RightColor = color_cast<ColorHSLA>(CurColorFull);
		ColorHSLA LeftColor = color_cast<ColorHSLA>(CurColorFull);

		LeftColor.a = 0;
		RightColor.a = 1;

		ColorRGBA RightColorRGBA = color_cast<ColorRGBA>(RightColor);
		ColorRGBA LeftColorRGBA = color_cast<ColorRGBA>(LeftColor);

		// alpha
		{
			IGraphics::CColorVertex Array[4] = {
				IGraphics::CColorVertex(0, LeftColorRGBA.r, LeftColorRGBA.g, LeftColorRGBA.b, LeftColorRGBA.a),
				IGraphics::CColorVertex(1, RightColorRGBA.r, RightColorRGBA.g, RightColorRGBA.b, RightColorRGBA.a),
				IGraphics::CColorVertex(2, LeftColorRGBA.r, LeftColorRGBA.g, LeftColorRGBA.b, LeftColorRGBA.a),
				IGraphics::CColorVertex(3, RightColorRGBA.r, RightColorRGBA.g, RightColorRGBA.b, RightColorRGBA.a)};
			Graphics()->SetColorVertex(Array, 4);

			IGraphics::CFreeformItem Freeform(
				CurXOff, pColorRect->y,
				CurXOff + SizeColor, pColorRect->y,
				CurXOff, pColorRect->y + pColorRect->h,
				CurXOff + SizeColor, pColorRect->y + pColorRect->h);
			Graphics()->QuadsDrawFreeform(&Freeform, 1);
		}

		Graphics()->TrianglesEnd();
	};

	for(int i = 0; i < 3 + Alpha; i++)
	{
		pRect->HSplitTop(SizePerEntry, &Button, pRect);
		pRect->HSplitTop(MarginPerEntry, NULL, pRect);
		Button.VSplitLeft(10.0f, 0, &Button);
		Button.VSplitLeft(100.0f, &Label, &Button);

		Button.Draw(ColorRGBA(0.15f, 0.15f, 0.15f, 1.0f), IGraphics::CORNER_ALL, 1.0f);

		CUIRect Rail;
		Button.Margin(2.0f, &Rail);

		str_format(aBuf, sizeof(aBuf), "%s: %03d", apLabels[i], (int)(*apComponent[i] * 255));
		UI()->DoLabel(&Label, aBuf, 14.0f, TEXTALIGN_ML);

		ColorHSLA CurColorPureHSLA(RenderColorHSLA.r, 1, 0.5f, 1);
		ColorRGBA CurColorPure = color_cast<ColorRGBA>(CurColorPureHSLA);
		ColorRGBA ColorInner(1, 1, 1, 0.25f);

		if(i == 0)
		{
			ColorInner = CurColorPure;
			RenderHSLColorsRect(&Rail);
		}
		else if(i == 1)
		{
			RenderHSLSatRect(&Rail, CurColorPure);
			ColorInner = color_cast<ColorRGBA>(ColorHSLA(CurColorPureHSLA.r, *apComponent[1], CurColorPureHSLA.b, 1));
		}
		else if(i == 2)
		{
			ColorRGBA CurColorSat = color_cast<ColorRGBA>(ColorHSLA(CurColorPureHSLA.r, *apComponent[1], 0.5f, 1));
			RenderHSLLightRect(&Rail, CurColorSat);
			float LightVal = *apComponent[2];
			if(ClampedLight)
				LightVal = ColorHSLA::DARKEST_LGT + LightVal * (1.0f - ColorHSLA::DARKEST_LGT);
			ColorInner = color_cast<ColorRGBA>(ColorHSLA(CurColorPureHSLA.r, *apComponent[1], LightVal, 1));
		}
		else if(i == 3)
		{
			ColorRGBA CurColorFull = color_cast<ColorRGBA>(ColorHSLA(CurColorPureHSLA.r, *apComponent[1], *apComponent[2], 1));
			RenderHSLAlphaRect(&Rail, CurColorFull);
			float LightVal = *apComponent[2];
			if(ClampedLight)
				LightVal = ColorHSLA::DARKEST_LGT + LightVal * (1.0f - ColorHSLA::DARKEST_LGT);
			ColorInner = color_cast<ColorRGBA>(ColorHSLA(CurColorPureHSLA.r, *apComponent[1], LightVal, *apComponent[3]));
		}

		*apComponent[i] = UI()->DoScrollbarH(&((char *)pColor)[i], &Button, *apComponent[i], &ColorInner);
	}

	*pColor = Color.Pack(Alpha);
	return Color;
}

enum
{
	APPEARANCE_TAB_HUD = 0,
	APPEARANCE_TAB_CHAT = 1,
	APPEARANCE_TAB_NAME_PLATE = 2,
	APPEARANCE_TAB_HOOK_COLLISION = 3,
	APPEARANCE_TAB_KILL_MESSAGES = 4,
	APPEARANCE_TAB_LASER = 5,
	NUMBER_OF_APPEARANCE_TABS = 6,
};

void CMenus::RenderSettingsAppearance(CUIRect MainView)
{
	char aBuf[128];
	static int s_CurTab = 0;

	CUIRect TabBar, Page1Tab, Page2Tab, Page3Tab, Page4Tab, Page5Tab, Page6Tab, LeftView, RightView, Section, Button, Label;

	MainView.HSplitTop(20, &TabBar, &MainView);
	float TabsW = TabBar.w;
	TabBar.VSplitLeft(TabsW / NUMBER_OF_APPEARANCE_TABS, &Page1Tab, &Page2Tab);
	Page2Tab.VSplitLeft(TabsW / NUMBER_OF_APPEARANCE_TABS, &Page2Tab, &Page3Tab);
	Page3Tab.VSplitLeft(TabsW / NUMBER_OF_APPEARANCE_TABS, &Page3Tab, &Page4Tab);
	Page4Tab.VSplitLeft(TabsW / NUMBER_OF_APPEARANCE_TABS, &Page4Tab, &Page5Tab);
	Page5Tab.VSplitLeft(TabsW / NUMBER_OF_APPEARANCE_TABS, &Page5Tab, &Page6Tab);

	static CButtonContainer s_aPageTabs[NUMBER_OF_APPEARANCE_TABS] = {};

	if(DoButton_MenuTab(&s_aPageTabs[APPEARANCE_TAB_HUD], Localize("HUD"), s_CurTab == APPEARANCE_TAB_HUD, &Page1Tab, IGraphics::CORNER_L, NULL, NULL, NULL, NULL, 4))
		s_CurTab = APPEARANCE_TAB_HUD;
	if(DoButton_MenuTab(&s_aPageTabs[APPEARANCE_TAB_CHAT], Localize("Chat"), s_CurTab == APPEARANCE_TAB_CHAT, &Page2Tab, 0, NULL, NULL, NULL, NULL, 4))
		s_CurTab = APPEARANCE_TAB_CHAT;
	if(DoButton_MenuTab(&s_aPageTabs[APPEARANCE_TAB_NAME_PLATE], Localize("Name Plate"), s_CurTab == APPEARANCE_TAB_NAME_PLATE, &Page3Tab, 0, NULL, NULL, NULL, NULL, 4))
		s_CurTab = APPEARANCE_TAB_NAME_PLATE;
	if(DoButton_MenuTab(&s_aPageTabs[APPEARANCE_TAB_HOOK_COLLISION], Localize("Hook Collisions"), s_CurTab == APPEARANCE_TAB_HOOK_COLLISION, &Page4Tab, 0, NULL, NULL, NULL, NULL, 4))
		s_CurTab = APPEARANCE_TAB_HOOK_COLLISION;
	if(DoButton_MenuTab(&s_aPageTabs[APPEARANCE_TAB_KILL_MESSAGES], Localize("Kill Messages"), s_CurTab == APPEARANCE_TAB_KILL_MESSAGES, &Page5Tab, 0, NULL, NULL, NULL, NULL, 4))
		s_CurTab = APPEARANCE_TAB_KILL_MESSAGES;
	if(DoButton_MenuTab(&s_aPageTabs[APPEARANCE_TAB_LASER], Localize("Laser"), s_CurTab == APPEARANCE_TAB_LASER, &Page6Tab, IGraphics::CORNER_R, NULL, NULL, NULL, NULL, 4))
		s_CurTab = APPEARANCE_TAB_LASER;

	MainView.HSplitTop(10.0f, nullptr, &MainView);

	const float LineSize = 20.0f;
	const float ColorPickerLineSize = 25.0f;
	const float SectionMargin = 5.0f;
	const float SectionTotalMargin = SectionMargin * 2;
	const float HeadlineFontSize = 20.0f;
	const float HeadlineAndVMargin = HeadlineFontSize + SectionTotalMargin;
	const float MarginToNextSection = 5.0f;

	const float ColorPickerLabelSize = 13.0f;
	const float ColorPickerLineSpacing = 5.0f;

	if(s_CurTab == APPEARANCE_TAB_HUD)
	{
		MainView.VSplitMid(&LeftView, &RightView);

		// ***** HUD ***** //
		LeftView.HSplitTop(HeadlineAndVMargin, &Label, &LeftView);
		UI()->DoLabel(&Label, Localize("HUD"), HeadlineFontSize, TEXTALIGN_ML);

		// Switch of the entire HUD
		LeftView.HSplitTop(SectionTotalMargin + LineSize, &Section, &LeftView);
		Section.Margin(SectionMargin, &Section);

		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_ClShowhud, Localize("Show ingame HUD"), &g_Config.m_ClShowhud, &Section, LineSize);

		// Switches of the various normal HUD elements
		LeftView.HSplitTop(SectionTotalMargin + 5 * LineSize, &Section, &LeftView);
		Section.Margin(SectionMargin, &Section);

		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_ClShowhudHealthAmmo, Localize("Show health, shields and ammo"), &g_Config.m_ClShowhudHealthAmmo, &Section, LineSize);
		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_ClShowChat, Localize("Show chat"), &g_Config.m_ClShowChat, &Section, LineSize);
		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_ClNameplates, Localize("Show name plates"), &g_Config.m_ClNameplates, &Section, LineSize);
		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_ClShowKillMessages, Localize("Show kill messages"), &g_Config.m_ClShowKillMessages, &Section, LineSize);
		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_ClShowhudScore, Localize("Show score"), &g_Config.m_ClShowhudScore, &Section, LineSize);

		// Settings of the HUD element for votes
		LeftView.HSplitTop(SectionTotalMargin + LineSize, &Section, &LeftView);
		Section.Margin(SectionMargin, &Section);

		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_ClShowVotesAfterVoting, Localize("Show votes window after voting"), &g_Config.m_ClShowVotesAfterVoting, &Section, LineSize);

		// ***** DDRace HUD ***** //
		RightView.HSplitTop(HeadlineAndVMargin, &Label, &RightView);
		UI()->DoLabel(&Label, Localize("DDRace HUD"), HeadlineFontSize, TEXTALIGN_ML);

		// Switches of various DDRace HUD elements
		RightView.HSplitTop(SectionTotalMargin + 4 * LineSize, &Section, &RightView);
		Section.Margin(SectionMargin, &Section);

		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_ClDDRaceScoreBoard, Localize("Use DDRace Scoreboard"), &g_Config.m_ClDDRaceScoreBoard, &Section, LineSize);
		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_ClShowIDs, Localize("Show client IDs in scoreboard"), &g_Config.m_ClShowIDs, &Section, LineSize);
		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_ClShowhudDDRace, Localize("Show DDRace HUD"), &g_Config.m_ClShowhudDDRace, &Section, LineSize);
		if(g_Config.m_ClShowhudDDRace)
		{
			DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_ClShowhudJumpsIndicator, Localize("Show jumps indicator"), &g_Config.m_ClShowhudJumpsIndicator, &Section, LineSize);
		}
		else
		{
			Section.HSplitTop(LineSize, 0x0, &Section); // Create empty space for hidden option
		}

		// Switch for dummy actions display
		RightView.HSplitTop(SectionTotalMargin + LineSize, &Section, &RightView);
		Section.Margin(SectionMargin, &Section);

		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_ClShowhudDummyActions, Localize("Show dummy actions"), &g_Config.m_ClShowhudDummyActions, &Section, LineSize);

		// Player movement information display settings
		RightView.HSplitTop(SectionTotalMargin + 3 * LineSize, &Section, &RightView);
		Section.Margin(SectionMargin, &Section);

		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_ClShowhudPlayerPosition, Localize("Show player position"), &g_Config.m_ClShowhudPlayerPosition, &Section, LineSize);
		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_ClShowhudPlayerSpeed, Localize("Show player speed"), &g_Config.m_ClShowhudPlayerSpeed, &Section, LineSize);
		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_ClShowhudPlayerAngle, Localize("Show player target angle"), &g_Config.m_ClShowhudPlayerAngle, &Section, LineSize);

		// Freeze bar settings
		RightView.HSplitTop(SectionTotalMargin + 3 * LineSize, &Section, &RightView);
		Section.Margin(SectionMargin, &Section);

		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_ClShowFreezeBars, Localize("Show freeze bars"), &g_Config.m_ClShowFreezeBars, &Section, LineSize);
		{
			Section.HSplitTop(2 * LineSize, &Button, &Section);
			if(g_Config.m_ClShowFreezeBars)
			{
				UI()->DoScrollbarOption(&g_Config.m_ClFreezeBarsAlphaInsideFreeze, &g_Config.m_ClFreezeBarsAlphaInsideFreeze, &Button, Localize("Opacity of freeze bars inside freeze"), 0, 100, &CUI::ms_LinearScrollbarScale, CUI::SCROLLBAR_OPTION_MULTILINE, "%");
			}
		}
	}
	else if(s_CurTab == APPEARANCE_TAB_CHAT)
	{
		MainView.VSplitMid(&LeftView, &RightView);

		// ***** Chat ***** //
		LeftView.HSplitTop(HeadlineAndVMargin, &Label, &LeftView);
		UI()->DoLabel(&Label, Localize("Chat"), HeadlineFontSize, TEXTALIGN_ML);

		// General chat settings
		LeftView.HSplitTop(SectionTotalMargin + 3 * LineSize, &Section, &LeftView);
		Section.Margin(SectionMargin, &Section);

		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_ClChatTeamColors, Localize("Show names in chat in team colors"), &g_Config.m_ClChatTeamColors, &Section, LineSize);
		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_ClShowChatFriends, Localize("Show only chat messages from friends"), &g_Config.m_ClShowChatFriends, &Section, LineSize);

		if(DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_ClChatOld, Localize("Use old chat style"), &g_Config.m_ClChatOld, &Section, LineSize))
			GameClient()->m_Chat.RebuildChat();

		// ***** Messages ***** //
		LeftView.HSplitTop(MarginToNextSection, 0x0, &LeftView);
		LeftView.HSplitTop(HeadlineAndVMargin, &Label, &LeftView);
		UI()->DoLabel(&Label, Localize("Messages"), HeadlineFontSize, TEXTALIGN_ML);

		// Message Colors and extra settings
		LeftView.HSplitTop(SectionTotalMargin + 6 * ColorPickerLineSize, &Section, &LeftView);
		Section.Margin(SectionMargin, &Section);

		int i = 0;
		static CButtonContainer s_aResetIDs[24];

		DoLine_ColorPicker(&s_aResetIDs[i++], ColorPickerLineSize, ColorPickerLabelSize, ColorPickerLineSpacing, &Section, Localize("System message"), &g_Config.m_ClMessageSystemColor, ColorRGBA(1.0f, 1.0f, 0.5f), true, &g_Config.m_ClShowChatSystem);
		DoLine_ColorPicker(&s_aResetIDs[i++], ColorPickerLineSize, ColorPickerLabelSize, ColorPickerLineSpacing, &Section, Localize("Highlighted message"), &g_Config.m_ClMessageHighlightColor, ColorRGBA(1.0f, 0.5f, 0.5f));
		DoLine_ColorPicker(&s_aResetIDs[i++], ColorPickerLineSize, ColorPickerLabelSize, ColorPickerLineSpacing, &Section, Localize("Team message"), &g_Config.m_ClMessageTeamColor, ColorRGBA(0.65f, 1.0f, 0.65f));
		DoLine_ColorPicker(&s_aResetIDs[i++], ColorPickerLineSize, ColorPickerLabelSize, ColorPickerLineSpacing, &Section, Localize("Friend message"), &g_Config.m_ClMessageFriendColor, ColorRGBA(1.0f, 0.137f, 0.137f), true, &g_Config.m_ClMessageFriend);
		DoLine_ColorPicker(&s_aResetIDs[i++], ColorPickerLineSize, ColorPickerLabelSize, ColorPickerLineSpacing, &Section, Localize("Normal message"), &g_Config.m_ClMessageColor, ColorRGBA(1.0f, 1.0f, 1.0f));

		str_format(aBuf, sizeof(aBuf), "%s (echo)", Localize("Client message"));
		DoLine_ColorPicker(&s_aResetIDs[i++], ColorPickerLineSize, ColorPickerLabelSize, ColorPickerLineSpacing, &Section, aBuf, &g_Config.m_ClMessageClientColor, ColorRGBA(0.5f, 0.78f, 1.0f));

		// ***** Chat Preview ***** //
		RightView.HSplitTop(HeadlineAndVMargin, &Label, &RightView);
		UI()->DoLabel(&Label, Localize("Preview"), HeadlineFontSize, TEXTALIGN_ML);

		// Use the rest of the view for preview
		Section = RightView;
		Section.Margin(SectionMargin, &Section);

		Section.Draw(ColorRGBA(1, 1, 1, 0.1f), IGraphics::CORNER_ALL, 8.0f);

		Section.HSplitTop(10.0f, 0x0, &Section); // Margin

		ColorRGBA SystemColor = color_cast<ColorRGBA, ColorHSLA>(ColorHSLA(g_Config.m_ClMessageSystemColor));
		ColorRGBA HighlightedColor = color_cast<ColorRGBA, ColorHSLA>(ColorHSLA(g_Config.m_ClMessageHighlightColor));
		ColorRGBA TeamColor = color_cast<ColorRGBA, ColorHSLA>(ColorHSLA(g_Config.m_ClMessageTeamColor));
		ColorRGBA FriendColor = color_cast<ColorRGBA, ColorHSLA>(ColorHSLA(g_Config.m_ClMessageFriendColor));
		ColorRGBA NormalColor = color_cast<ColorRGBA, ColorHSLA>(ColorHSLA(g_Config.m_ClMessageColor));
		ColorRGBA ClientColor = color_cast<ColorRGBA, ColorHSLA>(ColorHSLA(g_Config.m_ClMessageClientColor));
		ColorRGBA DefaultNameColor(0.8f, 0.8f, 0.8f, 1.0f);

		constexpr float RealFontSize = CChat::FONT_SIZE * 2;
		const float RealMsgPaddingX = (!g_Config.m_ClChatOld ? CChat::MESSAGE_PADDING_X : 0) * 2;
		const float RealMsgPaddingY = (!g_Config.m_ClChatOld ? CChat::MESSAGE_PADDING_Y : 0) * 2;
		const float RealMsgPaddingTee = (!g_Config.m_ClChatOld ? CChat::MESSAGE_TEE_SIZE + CChat::MESSAGE_TEE_PADDING_RIGHT : 0) * 2;
		const float RealOffsetY = RealFontSize + RealMsgPaddingY;

		const float X = 5.0f + RealMsgPaddingX / 2.0f + Section.x;
		float Y = Section.y;

		CTextCursor Cursor;
		TextRender()->SetCursor(&Cursor, X, Y, RealFontSize, TEXTFLAG_RENDER);

		str_copy(aBuf, Client()->PlayerName());

		const CAnimState *pIdleState = CAnimState::GetIdle();
		constexpr int PreviewTeeCount = 4;
		constexpr float RealTeeSize = CChat::MESSAGE_TEE_SIZE * 2;
		constexpr float RealTeeSizeHalved = CChat::MESSAGE_TEE_SIZE;
		constexpr float TWSkinUnreliableOffset = -0.25f;
		constexpr float OffsetTeeY = RealTeeSizeHalved;
		const float FullHeightMinusTee = RealOffsetY - RealTeeSize;

		CTeeRenderInfo aRenderInfo[PreviewTeeCount];

		// Backgrounds first
		if(!g_Config.m_ClChatOld)
		{
			Graphics()->TextureClear();
			Graphics()->QuadsBegin();
			Graphics()->SetColor(0, 0, 0, 0.12f);

			char aLineBuilder[128];
			float Width;
			float TempY = Y;
			constexpr float RealBackgroundRounding = CChat::MESSAGE_ROUNDING * 2.0f;

			if(g_Config.m_ClShowChatSystem)
			{
				str_format(aLineBuilder, sizeof(aLineBuilder), "*** '%s' entered and joined the game", aBuf);
				Width = TextRender()->TextWidth(RealFontSize, aLineBuilder, -1, -1);
				Graphics()->DrawRectExt(X - RealMsgPaddingX / 2.0f, TempY - RealMsgPaddingY / 2.0f, Width + RealMsgPaddingX, RealFontSize + RealMsgPaddingY, RealBackgroundRounding, IGraphics::CORNER_ALL);
				TempY += RealOffsetY;
			}

			str_format(aLineBuilder, sizeof(aLineBuilder), "%sRandom Tee: Hey, how are you %s?", g_Config.m_ClShowIDs ? " 7: " : "", aBuf);
			Width = TextRender()->TextWidth(RealFontSize, aLineBuilder, -1, -1);
			Graphics()->DrawRectExt(X - RealMsgPaddingX / 2.0f, TempY - RealMsgPaddingY / 2.0f, Width + RealMsgPaddingX + RealMsgPaddingTee, RealFontSize + RealMsgPaddingY, RealBackgroundRounding, IGraphics::CORNER_ALL);
			TempY += RealOffsetY;

			str_format(aLineBuilder, sizeof(aLineBuilder), "%sYour Teammate: Let's speedrun this!", g_Config.m_ClShowIDs ? "11: " : "");
			Width = TextRender()->TextWidth(RealFontSize, aLineBuilder, -1, -1);
			Graphics()->DrawRectExt(X - RealMsgPaddingX / 2.0f, TempY - RealMsgPaddingY / 2.0f, Width + RealMsgPaddingX + RealMsgPaddingTee, RealFontSize + RealMsgPaddingY, RealBackgroundRounding, IGraphics::CORNER_ALL);
			TempY += RealOffsetY;

			str_format(aLineBuilder, sizeof(aLineBuilder), "%s%sFriend: Hello there", g_Config.m_ClMessageFriend ? "♥ " : "", g_Config.m_ClShowIDs ? " 8: " : "");
			Width = TextRender()->TextWidth(RealFontSize, aLineBuilder, -1, -1);
			Graphics()->DrawRectExt(X - RealMsgPaddingX / 2.0f, TempY - RealMsgPaddingY / 2.0f, Width + RealMsgPaddingX + RealMsgPaddingTee, RealFontSize + RealMsgPaddingY, RealBackgroundRounding, IGraphics::CORNER_ALL);
			TempY += RealOffsetY;

			str_format(aLineBuilder, sizeof(aLineBuilder), "%sSpammer [6]: Hey fools, I'm spamming here!", g_Config.m_ClShowIDs ? " 9: " : "");
			Width = TextRender()->TextWidth(RealFontSize, aLineBuilder, -1, -1);
			Graphics()->DrawRectExt(X - RealMsgPaddingX / 2.0f, TempY - RealMsgPaddingY / 2.0f, Width + RealMsgPaddingX + RealMsgPaddingTee, RealFontSize + RealMsgPaddingY, RealBackgroundRounding, IGraphics::CORNER_ALL);
			TempY += RealOffsetY;

			Width = TextRender()->TextWidth(RealFontSize, "— Echo command executed", -1, -1);
			Graphics()->DrawRectExt(X - RealMsgPaddingX / 2.0f, TempY - RealMsgPaddingY / 2.0f, Width + RealMsgPaddingX, RealFontSize + RealMsgPaddingY, RealBackgroundRounding, IGraphics::CORNER_ALL);

			Graphics()->QuadsEnd();

			// Load skins

			const auto *pDefaultSkin = GameClient()->m_Skins.Find("default");

			for(auto &Info : aRenderInfo)
			{
				Info.m_Size = RealTeeSize;
				Info.m_CustomColoredSkin = false;
			}

			const CSkin *pSkin = nullptr;
			int pos = 0;

			aRenderInfo[pos++].m_OriginalRenderSkin = pDefaultSkin->m_OriginalSkin;
			aRenderInfo[pos++].m_OriginalRenderSkin = (pSkin = GameClient()->m_Skins.FindOrNullptr("pinky")) != nullptr ? pSkin->m_OriginalSkin : aRenderInfo[0].m_OriginalRenderSkin;
			aRenderInfo[pos++].m_OriginalRenderSkin = (pSkin = GameClient()->m_Skins.FindOrNullptr("cammostripes")) != nullptr ? pSkin->m_OriginalSkin : aRenderInfo[0].m_OriginalRenderSkin;
			aRenderInfo[pos++].m_OriginalRenderSkin = (pSkin = GameClient()->m_Skins.FindOrNullptr("beast")) != nullptr ? pSkin->m_OriginalSkin : aRenderInfo[0].m_OriginalRenderSkin;
		}

		// System
		if(g_Config.m_ClShowChatSystem)
		{
			TextRender()->TextColor(SystemColor);
			TextRender()->TextEx(&Cursor, "*** '", -1);
			TextRender()->TextEx(&Cursor, aBuf, -1);
			TextRender()->TextEx(&Cursor, "' entered and joined the game", -1);
			TextRender()->SetCursorPosition(&Cursor, X, Y += RealOffsetY);
		}

		// Highlighted
		TextRender()->MoveCursor(&Cursor, RealMsgPaddingTee, 0);
		TextRender()->TextColor(DefaultNameColor);
		if(g_Config.m_ClShowIDs)
			TextRender()->TextEx(&Cursor, " 7: ", -1);
		TextRender()->TextEx(&Cursor, "Random Tee: ", -1);
		TextRender()->TextColor(HighlightedColor);
		TextRender()->TextEx(&Cursor, "Hey, how are you ", -1);
		TextRender()->TextEx(&Cursor, aBuf, -1);
		TextRender()->TextEx(&Cursor, "?", -1);
		if(!g_Config.m_ClChatOld)
			RenderTools()->RenderTee(pIdleState, &aRenderInfo[1], EMOTE_NORMAL, vec2(1, 0.1f), vec2(X + RealTeeSizeHalved, Y + OffsetTeeY + FullHeightMinusTee / 2.0f + TWSkinUnreliableOffset));
		TextRender()->SetCursorPosition(&Cursor, X, Y += RealOffsetY);

		// Team
		TextRender()->MoveCursor(&Cursor, RealMsgPaddingTee, 0);
		TextRender()->TextColor(TeamColor);
		if(g_Config.m_ClShowIDs)
			TextRender()->TextEx(&Cursor, "11: ", -1);
		TextRender()->TextEx(&Cursor, "Your Teammate: Let's speedrun this!", -1);
		if(!g_Config.m_ClChatOld)
			RenderTools()->RenderTee(pIdleState, &aRenderInfo[0], EMOTE_NORMAL, vec2(1, 0.1f), vec2(X + RealTeeSizeHalved, Y + OffsetTeeY + FullHeightMinusTee / 2.0f + TWSkinUnreliableOffset));
		TextRender()->SetCursorPosition(&Cursor, X, Y += RealOffsetY);

		// Friend
		TextRender()->MoveCursor(&Cursor, RealMsgPaddingTee, 0);
		if(g_Config.m_ClMessageFriend)
		{
			TextRender()->TextColor(FriendColor);
			TextRender()->TextEx(&Cursor, "♥ ", -1);
		}
		TextRender()->TextColor(DefaultNameColor);
		if(g_Config.m_ClShowIDs)
			TextRender()->TextEx(&Cursor, " 8: ", -1);
		TextRender()->TextEx(&Cursor, "Friend: ", -1);
		TextRender()->TextColor(NormalColor);
		TextRender()->TextEx(&Cursor, "Hello there", -1);
		if(!g_Config.m_ClChatOld)
			RenderTools()->RenderTee(pIdleState, &aRenderInfo[2], EMOTE_NORMAL, vec2(1, 0.1f), vec2(X + RealTeeSizeHalved, Y + OffsetTeeY + FullHeightMinusTee / 2.0f + TWSkinUnreliableOffset));
		TextRender()->SetCursorPosition(&Cursor, X, Y += RealOffsetY);

		// Normal
		TextRender()->MoveCursor(&Cursor, RealMsgPaddingTee, 0);
		TextRender()->TextColor(DefaultNameColor);
		if(g_Config.m_ClShowIDs)
			TextRender()->TextEx(&Cursor, " 9: ", -1);
		TextRender()->TextEx(&Cursor, "Spammer ", -1);
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, 0.3f);
		TextRender()->TextEx(&Cursor, "[6]", -1);
		TextRender()->TextColor(NormalColor);
		TextRender()->TextEx(&Cursor, ": Hey fools, I'm spamming here!", -1);
		if(!g_Config.m_ClChatOld)
			RenderTools()->RenderTee(pIdleState, &aRenderInfo[3], EMOTE_NORMAL, vec2(1, 0.1f), vec2(X + RealTeeSizeHalved, Y + OffsetTeeY + FullHeightMinusTee / 2.0f + TWSkinUnreliableOffset));
		TextRender()->SetCursorPosition(&Cursor, X, Y += RealOffsetY);

		// Client
		TextRender()->TextColor(ClientColor);
		TextRender()->TextEx(&Cursor, "— Echo command executed", -1);
		TextRender()->SetCursorPosition(&Cursor, X, Y);

		TextRender()->TextColor(TextRender()->DefaultTextColor());
	}
	else if(s_CurTab == APPEARANCE_TAB_NAME_PLATE)
	{
		MainView.VSplitMid(&LeftView, &RightView);

		// ***** Name Plate ***** //
		LeftView.HSplitTop(HeadlineAndVMargin, &Label, &LeftView);
		UI()->DoLabel(&Label, Localize("Name Plate"), HeadlineFontSize, TEXTALIGN_ML);

		// General chat settings
		LeftView.HSplitTop(SectionTotalMargin + 9 * LineSize, &Section, &LeftView);
		Section.Margin(SectionMargin, &Section);
		Section.HSplitTop(2 * LineSize, &Button, &Section);
		UI()->DoScrollbarOption(&g_Config.m_ClNameplatesSize, &g_Config.m_ClNameplatesSize, &Button, Localize("Name plates size"), 0, 100, &CUI::ms_LinearScrollbarScale, CUI::SCROLLBAR_OPTION_MULTILINE);

		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_ClNameplatesClan, Localize("Show clan above name plates"), &g_Config.m_ClNameplatesClan, &Section, LineSize);
		Section.HSplitTop(2 * LineSize, &Button, &Section);
		if(g_Config.m_ClNameplatesClan)
		{
			UI()->DoScrollbarOption(&g_Config.m_ClNameplatesClanSize, &g_Config.m_ClNameplatesClanSize, &Button, Localize("Clan plates size"), 0, 100, &CUI::ms_LinearScrollbarScale, CUI::SCROLLBAR_OPTION_MULTILINE);
		}

		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_ClNameplatesTeamcolors, Localize("Use team colors for name plates"), &g_Config.m_ClNameplatesTeamcolors, &Section, LineSize);
		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_ClNameplatesStrong, Localize("Show hook strength indicator"), &g_Config.m_ClNameplatesStrong, &Section, LineSize);

		Section.HSplitTop(LineSize, &Button, &Section);
		if(DoButton_CheckBox(&g_Config.m_ClShowDirection, Localize("Show other players' key presses"), g_Config.m_ClShowDirection >= 1, &Button))
		{
			g_Config.m_ClShowDirection = g_Config.m_ClShowDirection >= 1 ? 0 : 1;
		}

		Section.HSplitTop(LineSize, &Button, &Section);
		static int s_ShowLocalPlayer = 0;
		if(DoButton_CheckBox(&s_ShowLocalPlayer, Localize("Show local player's key presses"), g_Config.m_ClShowDirection == 2, &Button))
		{
			g_Config.m_ClShowDirection = g_Config.m_ClShowDirection != 2 ? 2 : 1;
		}
	}
	else if(s_CurTab == APPEARANCE_TAB_HOOK_COLLISION)
	{
		MainView.VSplitMid(&LeftView, &RightView);

		// ***** Hookline ***** //
		LeftView.HSplitTop(HeadlineAndVMargin, &Label, &LeftView);
		UI()->DoLabel(&Label, Localize("Hook collision line"), HeadlineFontSize, TEXTALIGN_ML);

		// General hookline settings
		LeftView.HSplitTop(SectionTotalMargin + 6 * LineSize + 3 * ColorPickerLineSize, &Section, &LeftView);
		Section.Margin(SectionMargin, &Section);

		Section.HSplitTop(LineSize, &Button, &Section);
		if(DoButton_CheckBox(&g_Config.m_ClShowHookCollOther, Localize("Show other players' hook collision lines"), g_Config.m_ClShowHookCollOther, &Button))
		{
			g_Config.m_ClShowHookCollOther = g_Config.m_ClShowHookCollOther >= 1 ? 0 : 1;
		}

		Section.HSplitTop(2 * LineSize, &Button, &Section);
		UI()->DoScrollbarOption(&g_Config.m_ClHookCollSize, &g_Config.m_ClHookCollSize, &Button, Localize("Hook collision line width"), 0, 100, &CUI::ms_LinearScrollbarScale, CUI::SCROLLBAR_OPTION_MULTILINE);

		Section.HSplitTop(2 * LineSize, &Button, &Section);
		UI()->DoScrollbarOption(&g_Config.m_ClHookCollAlpha, &g_Config.m_ClHookCollAlpha, &Button, Localize("Hook collision line opacity"), 0, 100, &CUI::ms_LinearScrollbarScale, CUI::SCROLLBAR_OPTION_MULTILINE, "%");

		static CButtonContainer s_HookCollNoCollResetID, s_HookCollHookableCollResetID, s_HookCollTeeCollResetID;
		static int s_HookCollToolTip;

		Section.HSplitTop(LineSize, &Label, &Section);
		UI()->DoLabel(&Label, Localize("Colors of the hook collision line, in case of a possible collision with:"), 13.0f, TEXTALIGN_ML);
		GameClient()->m_Tooltips.DoToolTip(&s_HookCollToolTip, &Label, Localize("Your movements are not taken into account when calculating the line colors"));
		DoLine_ColorPicker(&s_HookCollNoCollResetID, ColorPickerLineSize, ColorPickerLabelSize, ColorPickerLineSpacing, &Section, Localize("Nothing hookable"), &g_Config.m_ClHookCollColorNoColl, ColorRGBA(1.0f, 0.0f, 0.0f, 1.0f), false);
		DoLine_ColorPicker(&s_HookCollHookableCollResetID, ColorPickerLineSize, ColorPickerLabelSize, ColorPickerLineSpacing, &Section, Localize("Something hookable"), &g_Config.m_ClHookCollColorHookableColl, ColorRGBA(130.0f / 255.0f, 232.0f / 255.0f, 160.0f / 255.0f, 1.0f), false);
		DoLine_ColorPicker(&s_HookCollTeeCollResetID, ColorPickerLineSize, ColorPickerLabelSize, ColorPickerLineSpacing, &Section, Localize("A Tee"), &g_Config.m_ClHookCollColorTeeColl, ColorRGBA(1.0f, 1.0f, 0.0f, 1.0f), false);
	}
	else if(s_CurTab == APPEARANCE_TAB_KILL_MESSAGES)
	{
		MainView.VSplitMid(&LeftView, &RightView);

		// ***** Kill Messages ***** //
		LeftView.HSplitTop(HeadlineAndVMargin, &Label, &LeftView);
		UI()->DoLabel(&Label, Localize("Kill Messages"), HeadlineFontSize, TEXTALIGN_ML);

		// General kill messages settings
		LeftView.HSplitTop(SectionTotalMargin + 2 * ColorPickerLineSize, &Section, &LeftView);
		Section.Margin(SectionMargin, &Section);

		static CButtonContainer s_KillMessageNormalColorID, s_KillMessageHighlightColorID;

		DoLine_ColorPicker(&s_KillMessageNormalColorID, ColorPickerLineSize, ColorPickerLabelSize, ColorPickerLineSpacing, &Section, Localize("Normal Color"), &g_Config.m_ClKillMessageNormalColor, ColorRGBA(1.0f, 1.0f, 1.0f), false);
		DoLine_ColorPicker(&s_KillMessageHighlightColorID, ColorPickerLineSize, ColorPickerLabelSize, ColorPickerLineSpacing, &Section, Localize("Highlight Color"), &g_Config.m_ClKillMessageHighlightColor, ColorRGBA(1.0f, 1.0f, 1.0f), false);
	}
	else if(s_CurTab == APPEARANCE_TAB_LASER)
	{
		MainView.VSplitMid(&LeftView, &RightView);

		// ***** Weapons ***** //
		LeftView.HSplitTop(HeadlineAndVMargin, &Label, &LeftView);
		UI()->DoLabel(&Label, Localize("Weapons"), HeadlineFontSize, TEXTALIGN_ML);

		// General weapon laser settings
		LeftView.HSplitTop(SectionTotalMargin + 4 * ColorPickerLineSize, &Section, &LeftView);
		Section.Margin(SectionMargin, &Section);

		static CButtonContainer s_LaserRifleOutResetID, s_LaserRifleInResetID, s_LaserShotgunOutResetID, s_LaserShotgunInResetID;

		ColorHSLA LaserRifleOutlineColor = DoLine_ColorPicker(&s_LaserRifleOutResetID, ColorPickerLineSize, ColorPickerLabelSize, ColorPickerLineSpacing, &Section, Localize("Rifle Laser Outline Color"), &g_Config.m_ClLaserRifleOutlineColor, ColorRGBA(0.074402f, 0.074402f, 0.247166f, 1.0f), false);
		ColorHSLA LaserRifleInnerColor = DoLine_ColorPicker(&s_LaserRifleInResetID, ColorPickerLineSize, ColorPickerLabelSize, ColorPickerLineSpacing, &Section, Localize("Rifle Laser Inner Color"), &g_Config.m_ClLaserRifleInnerColor, ColorRGBA(0.498039f, 0.498039f, 1.0f, 1.0f), false);
		ColorHSLA LaserShotgunOutlineColor = DoLine_ColorPicker(&s_LaserShotgunOutResetID, ColorPickerLineSize, ColorPickerLabelSize, ColorPickerLineSpacing, &Section, Localize("Shotgun Laser Outline Color"), &g_Config.m_ClLaserShotgunOutlineColor, ColorRGBA(0.125490f, 0.098039f, 0.043137f, 1.0f), false);
		ColorHSLA LaserShotgunInnerColor = DoLine_ColorPicker(&s_LaserShotgunInResetID, ColorPickerLineSize, ColorPickerLabelSize, ColorPickerLineSpacing, &Section, Localize("Shotgun Laser Inner Color"), &g_Config.m_ClLaserShotgunInnerColor, ColorRGBA(0.570588f, 0.417647f, 0.252941f, 1.0f), false);

		// ***** Entities ***** //
		LeftView.HSplitTop(MarginToNextSection * 2.0f, 0x0, &LeftView);
		LeftView.HSplitTop(HeadlineAndVMargin, &Label, &LeftView);
		UI()->DoLabel(&Label, Localize("Entities"), HeadlineFontSize, TEXTALIGN_ML);

		// General entity laser settings
		LeftView.HSplitTop(SectionTotalMargin + 4 * ColorPickerLineSize, &Section, &LeftView);
		Section.Margin(SectionMargin, &Section);

		static CButtonContainer s_LaserDoorOutResetID, s_LaserDoorInResetID, s_LaserFreezeOutResetID, s_LaserFreezeInResetID;

		ColorHSLA LaserDoorOutlineColor = DoLine_ColorPicker(&s_LaserDoorOutResetID, ColorPickerLineSize, ColorPickerLabelSize, ColorPickerLineSpacing, &Section, Localize("Door Laser Outline Color"), &g_Config.m_ClLaserDoorOutlineColor, ColorRGBA(0.0f, 0.131372f, 0.096078f, 1.0f), false);
		ColorHSLA LaserDoorInnerColor = DoLine_ColorPicker(&s_LaserDoorInResetID, ColorPickerLineSize, ColorPickerLabelSize, ColorPickerLineSpacing, &Section, Localize("Door Laser Inner Color"), &g_Config.m_ClLaserDoorInnerColor, ColorRGBA(0.262745f, 0.760784f, 0.639215f, 1.0f), false);
		ColorHSLA LaserFreezeOutlineColor = DoLine_ColorPicker(&s_LaserFreezeOutResetID, ColorPickerLineSize, ColorPickerLabelSize, ColorPickerLineSpacing, &Section, Localize("Freeze Laser Outline Color"), &g_Config.m_ClLaserFreezeOutlineColor, ColorRGBA(0.131372f, 0.123529f, 0.182352f, 1.0f), false);
		ColorHSLA LaserFreezeInnerColor = DoLine_ColorPicker(&s_LaserFreezeInResetID, ColorPickerLineSize, ColorPickerLabelSize, ColorPickerLineSpacing, &Section, Localize("Freeze Laser Inner Color"), &g_Config.m_ClLaserFreezeInnerColor, ColorRGBA(0.482352f, 0.443137f, 0.564705f, 1.0f), false);

		static CButtonContainer s_AllToRifleResetID, s_AllToDefaultResetID;

		LeftView.HSplitTop(20.0f, 0x0, &LeftView);
		LeftView.HSplitTop(20.0f, &Button, &LeftView);
		if(DoButton_Menu(&s_AllToRifleResetID, Localize("Set all to Rifle"), 0, &Button))
		{
			g_Config.m_ClLaserShotgunOutlineColor = g_Config.m_ClLaserRifleOutlineColor;
			g_Config.m_ClLaserShotgunInnerColor = g_Config.m_ClLaserRifleInnerColor;
			g_Config.m_ClLaserDoorOutlineColor = g_Config.m_ClLaserRifleOutlineColor;
			g_Config.m_ClLaserDoorInnerColor = g_Config.m_ClLaserRifleInnerColor;
			g_Config.m_ClLaserFreezeOutlineColor = g_Config.m_ClLaserRifleOutlineColor;
			g_Config.m_ClLaserFreezeInnerColor = g_Config.m_ClLaserRifleInnerColor;
		}

		// values taken from the CL commands
		LeftView.HSplitTop(10.0f, 0x0, &LeftView);
		LeftView.HSplitTop(20.0f, &Button, &LeftView);
		if(DoButton_Menu(&s_AllToDefaultResetID, Localize("Reset to defaults"), 0, &Button))
		{
			g_Config.m_ClLaserRifleOutlineColor = 11176233;
			g_Config.m_ClLaserRifleInnerColor = 11206591;
			g_Config.m_ClLaserShotgunOutlineColor = 1866773;
			g_Config.m_ClLaserShotgunInnerColor = 1467241;
			g_Config.m_ClLaserDoorOutlineColor = 7667473;
			g_Config.m_ClLaserDoorInnerColor = 7701379;
			g_Config.m_ClLaserFreezeOutlineColor = 11613223;
			g_Config.m_ClLaserFreezeInnerColor = 12001153;
		}

		// ***** Laser Preview ***** //
		RightView.HSplitTop(HeadlineAndVMargin, &Label, &RightView);
		UI()->DoLabel(&Label, Localize("Preview"), HeadlineFontSize, TEXTALIGN_ML);

		RightView.HSplitTop(SectionTotalMargin + 50.0f, &Section, &RightView);
		Section.Margin(SectionMargin, &Section);
		DoLaserPreview(&Section, LaserRifleOutlineColor, LaserRifleInnerColor, LASERTYPE_RIFLE);

		RightView.HSplitTop(SectionTotalMargin + 50.0f, &Section, &RightView);
		Section.Margin(SectionMargin, &Section);
		DoLaserPreview(&Section, LaserShotgunOutlineColor, LaserShotgunInnerColor, LASERTYPE_SHOTGUN);

		RightView.HSplitTop(SectionTotalMargin + 50.0f, &Section, &RightView);
		Section.Margin(SectionMargin, &Section);
		DoLaserPreview(&Section, LaserDoorOutlineColor, LaserDoorInnerColor, LASERTYPE_DOOR);

		RightView.HSplitTop(SectionTotalMargin + 50.0f, &Section, &RightView);
		Section.Margin(SectionMargin, &Section);
		DoLaserPreview(&Section, LaserFreezeOutlineColor, LaserFreezeInnerColor, LASERTYPE_DOOR);
	}
}

void CMenus::RenderSettingsYT(CUIRect MainView)
{
	
	CUIRect Container = MainView, Space;
	float LineMargin = 20.0f;

	Container.Margin(10.0f, &MainView);

	Container.HSplitTop(LineMargin, &Space, &Container);
	if(DoButton_CheckBox(&g_Config.m_ClYoutubePet, "Show Youtube Pet", g_Config.m_ClYoutubePet, &Space))
	{
		g_Config.m_ClYoutubePet ^= 1;

		// int Amount = 360;

        // for(int i = 0; i < Amount; i++) {
        //     m_pClient->m_DamageInd.Create(m_pClient->m_LocalCharacterPos, direction(i), 1);
        // }
		// m_pClient->m_Effects.PowerupShine(m_pClient->m_LocalCharacterPos*0.2, vec2(96, 18), 1.0f);
	}
	GameClient()->m_Tooltips.DoToolTip(&g_Config.m_ClYoutubePet, &Space, "Show the Pet");
		
	if(g_Config.m_ClYoutubePet) {  
			
		Container.HSplitTop(LineMargin, &Space, &Container);
		if(DoButton_CheckBox(&g_Config.m_ClYoutubePetPositionLine, "Show Pet Line", g_Config.m_ClYoutubePetPositionLine, &Space))
		{
			g_Config.m_ClYoutubePetPositionLine ^= 1;
		}
		GameClient()->m_Tooltips.DoToolTip(&g_Config.m_ClYoutubePetPositionLine, &Space, Localize("Show a line between the player and the pet"));
			
		Container.HSplitTop(LineMargin, &Space, &Container);
		if(DoButton_CheckBox(&g_Config.m_ClYoutubePetTrail, "Render Pet Trail", g_Config.m_ClYoutubePetTrail, &Space))
		{
			g_Config.m_ClYoutubePetTrail ^= 1;
			
		}
		GameClient()->m_Tooltips.DoToolTip(&g_Config.m_ClYoutubePetTrail, &Space, "Render Tee Trail");

		if(g_Config.m_ClYoutubePetTrail) {  
			static CButtonContainer s_YoutubePetTrailResetID;
			
			Container.HSplitTop(LineMargin, &Space, &Container);
			DoLine_ColorPicker(&s_YoutubePetTrailResetID, 25.0f, 13.0f, 5.0f, &Space, Localize("Trail color"), &g_Config.m_ClYoutubePetTrailColor, ColorRGBA(1,1,1,1), false);
			GameClient()->m_Tooltips.DoToolTip(&g_Config.m_ClYoutubePetTrailColor, &Space, Localize("Trail color"));
			
			Container.HSplitTop(LineMargin, &Space, &Container);
			UI()->DoScrollbarOption(&g_Config.m_ClYoutubePetTrailRadius, &g_Config.m_ClYoutubePetTrailRadius, &Space, Localize("Trail radius"), 10, 1000);

		}
		
		Container.HSplitTop(LineMargin, &Space, &Container);
		if(DoButton_CheckBox(&g_Config.m_ClRenderPetLikeTee, "Render Pet like a Tee", g_Config.m_ClRenderPetLikeTee, &Space))
		{
			g_Config.m_ClRenderPetLikeTee ^= 1;
			
		}
		GameClient()->m_Tooltips.DoToolTip(&g_Config.m_ClRenderPetLikeTee, &Space, "Render the Pet like a Tee");
		
	}	


	Container.HSplitTop(LineMargin, &Space, &Container);
	if(DoButton_CheckBox(&g_Config.m_ClYoutubeTeeTrail, "Render Tee Trail", g_Config.m_ClYoutubeTeeTrail, &Space))
	{
		g_Config.m_ClYoutubeTeeTrail ^= 1;
		
	}
	GameClient()->m_Tooltips.DoToolTip(&g_Config.m_ClYoutubeTeeTrail, &Space, "Render Tee Trail");

	if(g_Config.m_ClYoutubeTeeTrail) {  
		static CButtonContainer s_YoutubeTeeTrailResetID;
		
		Container.HSplitTop(LineMargin, &Space, &Container);
		DoLine_ColorPicker(&s_YoutubeTeeTrailResetID, 25.0f, 13.0f, 5.0f, &Space, Localize("Trail color"), &g_Config.m_ClYoutubeTeeTrailColor, ColorRGBA(1,1,1,1), false);
		GameClient()->m_Tooltips.DoToolTip(&g_Config.m_ClYoutubeTeeTrailColor, &Space, Localize("Trail color"));
		
		Container.HSplitTop(LineMargin, &Space, &Container);
		UI()->DoScrollbarOption(&g_Config.m_ClYoutubeTeeTrailRadius, &g_Config.m_ClYoutubeTeeTrailRadius, &Space, Localize("Trail radius"), 10, 1000);

	}

	Container.HSplitTop(LineMargin, &Space, &Container);
	if(DoButton_CheckBox(&g_Config.m_ClYoutubeMagicParticles, "Render magic particles", g_Config.m_ClYoutubeMagicParticles, &Space))
	{
		g_Config.m_ClYoutubeMagicParticles ^= 1;
		
	}
	GameClient()->m_Tooltips.DoToolTip(&g_Config.m_ClYoutubeMagicParticles, &Space, "Render magic particles");

	Container.HSplitTop(LineMargin, &Space, &Container);
	if(DoButton_CheckBox(&g_Config.m_ClYoutubeMagicParticles2, "Render magic particles2", g_Config.m_ClYoutubeMagicParticles2, &Space))
	{
		g_Config.m_ClYoutubeMagicParticles2 ^= 1;
		
	}
	GameClient()->m_Tooltips.DoToolTip(&g_Config.m_ClYoutubeMagicParticles2, &Space, "Render magic particles2");

	//FAKE PING HERE
	Container.HSplitTop(LineMargin, &Space, &Container);
	if(DoButton_CheckBox(&g_Config.m_ClFakePingEnabled, "Enable fake ping (your ping to 999)", g_Config.m_ClFakePingEnabled, &Space))
	{
		g_Config.m_ClFakePingEnabled ^= 1;
	}

	if(g_Config.m_ClFakePingEnabled) {  
		g_Config.m_ClFakePingEnabled1 = 0;

		Container.HSplitTop(LineMargin, &Space, &Container);
		UI()->DoScrollbarOption(&g_Config.m_ClFakePing, &g_Config.m_ClFakePing, &Space, "Fake ping", 0, 1000.f);

		if(g_Config.m_ClFakePing < currentRealPing) {
			g_Config.m_ClFakePing = currentRealPing + 1;
		}
	}

	Container.HSplitTop(LineMargin, &Space, &Container);
	if(DoButton_CheckBox(&g_Config.m_ClFakePingEnabled1, "Enable fake ping (0-9)", g_Config.m_ClFakePingEnabled1, &Space))
	{
		g_Config.m_ClFakePingEnabled1 ^= 1;
	}

	if(g_Config.m_ClFakePingEnabled1) {
		g_Config.m_ClFakePingEnabled = 0;

		if(g_Config.m_ClFakePing > g_Config.m_ClWhatsMyPing + 5 || g_Config.m_ClFakePing < g_Config.m_ClWhatsMyPing - 5) {
			g_Config.m_ClFakePing = g_Config.m_ClWhatsMyPing;
		}
	}

	if((!g_Config.m_ClFakePingEnabled && !g_Config.m_ClFakePingEnabled1) || currentRealPing == 0.f) {
		currentRealPing = g_Config.m_ClWhatsMyPing;
	}

	Container.HSplitTop(LineMargin, &Space, &Container);
	if(DoButton_CheckBox(&g_Config.m_ClFreezePing, "Enable freeze your current ping", g_Config.m_ClFreezePing, &Space))
	{
		g_Config.m_ClFreezePing ^= 1;
	}

	Container.HSplitTop(LineMargin, &Space, &Container);
	if(DoButton_CheckBox_Number(&g_Config.m_ClRecording, !g_Config.m_ClRecording ? "Record" : "Stop recording", g_Config.m_ClRecording, &Space))
	{
		g_Config.m_ClRecording = (g_Config.m_ClRecording + 1)%2;
	}
	if(g_Config.m_ClRecording){
		g_Config.m_ClPlaying = 0;
		Container.HSplitTop(LineMargin, &Space, &Container);
		UI()->DoLabel(&Container, "Recording...", 20.0f, TEXTALIGN_ML);
	} else{
		if(g_Config.m_ClWasRecording != 0)
			g_Config.m_ClWasRecording = 0;
	}
	
	Container.HSplitTop(LineMargin, &Space, &Container);

	const std::vector<const char*> records = m_pClient->m_YouTube.GetBinaryFilesInFolder(g_Config.m_ClMapRecordsPath);

	static CUI::SDropDownState s_RecordsDropDownState;
	static CScrollRegion s_RecordsDropDownScrollRegion;
	s_RecordsDropDownState.m_SelectionPopupContext.m_pScrollRegion = &s_RecordsDropDownScrollRegion;
	const int NewRecord = UI()->DoDropDown(&Space, g_Config.m_ClSelectedRecord, const_cast<const char**>(records.data()), static_cast<int>(records.size()), s_RecordsDropDownState);
	if(g_Config.m_ClSelectedRecord != NewRecord)
	{
		g_Config.m_ClSelectedRecord = NewRecord;

		dbg_msg("youtube", "Selected record: %s", records[g_Config.m_ClSelectedRecord]);
	}

	if(g_Config.m_ClSelectedRecord != -1 && !g_Config.m_ClRecording && g_Config.m_ClSelectedRecord < static_cast<int>(records.size())) {

		static CButtonContainer s_YoutubeLoadRecordID,s_YoutubeDeleteRecordID;

		Container.HSplitTop(LineMargin, &Space, &Container);
		if(DoButton_Menu(&s_YoutubeLoadRecordID, "Load selected record", 0, &Space))
		{
			m_pClient->m_YouTube.LoadRecordsFromFile(records[g_Config.m_ClSelectedRecord]);
		}

		Container.HSplitTop(LineMargin, &Space, &Container);
		if(DoButton_Menu(&s_YoutubeDeleteRecordID, "Delete selected record", 0, &Space))
		{
			m_pClient->m_YouTube.DeleteRecord(records[g_Config.m_ClSelectedRecord]);
			g_Config.m_ClSelectedRecord = -1;
		}
	}

	if(!g_Config.m_ClRecording && m_pClient->m_YouTube.records.recordsActions.size() > 0 && m_pClient->m_YouTube.records.recordsMouse.size() > 0) {
		
		static CButtonContainer saveRecordID;
		Container.HSplitTop(LineMargin, &Space, &Container);
		if(DoButton_Menu(&saveRecordID, "Save record (you must save before play it)", 0, &Space))
		{
			m_pClient->m_YouTube.SaveRecords();
		}

		Container.HSplitTop(LineMargin, &Space, &Container);
		if(DoButton_CheckBox_Number(&g_Config.m_ClPlaying, !g_Config.m_ClPlaying ? "Play" : "Stop playing", g_Config.m_ClPlaying, &Space))
		{
			g_Config.m_ClPlaying = (g_Config.m_ClPlaying + 1)%2;
		}
		if(g_Config.m_ClPlaying){
			g_Config.m_ClRecording = 0;
			Container.HSplitTop(LineMargin, &Space, &Container);
			UI()->DoLabel(&Container, "Playing...", 20.0f, TEXTALIGN_ML);
		}

	}

}

void CMenus::RenderSettingsDDNet(CUIRect MainView)
{
	CUIRect Button, Left, Right, LeftLeft, Demo, Gameplay, Miscellaneous, Label, Background;

	MainView.HSplitTop(100.0f, &Demo, &MainView);

	Demo.HSplitTop(30.0f, &Label, &Demo);
	UI()->DoLabel(&Label, Localize("Demo"), 20.0f, TEXTALIGN_ML);
	Demo.Margin(5.0f, &Demo);
	Demo.VSplitMid(&Left, &Right);
	Left.VSplitRight(5.0f, &Left, 0);
	Right.VMargin(5.0f, &Right);

	Left.HSplitTop(20.0f, &Button, &Left);
	if(DoButton_CheckBox(&g_Config.m_ClAutoRaceRecord, Localize("Save the best demo of each race"), g_Config.m_ClAutoRaceRecord, &Button))
	{
		g_Config.m_ClAutoRaceRecord ^= 1;
	}

	{
		Left.HSplitTop(20.0f, &Button, &Left);

		if(DoButton_CheckBox(&g_Config.m_ClReplays, Localize("Enable replays"), g_Config.m_ClReplays, &Button))
		{
			g_Config.m_ClReplays ^= 1;
			if(!g_Config.m_ClReplays)
			{
				// stop recording and remove the tmp demo file
				Client()->DemoRecorder_Stop(RECORDER_REPLAYS, true);
			}
			else
			{
				// start recording
				Client()->DemoRecorder_HandleAutoStart();
			}
		}

		Left.HSplitTop(20.0f, &Button, &Left);
		if(g_Config.m_ClReplays)
			UI()->DoScrollbarOption(&g_Config.m_ClReplayLength, &g_Config.m_ClReplayLength, &Button, Localize("Default length"), 10, 600, &CUI::ms_LinearScrollbarScale, CUI::SCROLLBAR_OPTION_NOCLAMPVALUE);
	}

	Right.HSplitTop(20.0f, &Button, &Right);
	if(DoButton_CheckBox(&g_Config.m_ClRaceGhost, Localize("Ghost"), g_Config.m_ClRaceGhost, &Button))
	{
		g_Config.m_ClRaceGhost ^= 1;
	}
	GameClient()->m_Tooltips.DoToolTip(&g_Config.m_ClRaceGhost, &Button, Localize("When you cross the start line, show a ghost tee replicating the movements of your best time"));

	if(g_Config.m_ClRaceGhost)
	{
		Right.HSplitTop(20.0f, &Button, &Right);
		if(DoButton_CheckBox(&g_Config.m_ClRaceShowGhost, Localize("Show ghost"), g_Config.m_ClRaceShowGhost, &Button))
		{
			g_Config.m_ClRaceShowGhost ^= 1;
		}

		Right.HSplitTop(20.0f, &Button, &Right);
		if(DoButton_CheckBox(&g_Config.m_ClRaceSaveGhost, Localize("Save ghost"), g_Config.m_ClRaceSaveGhost, &Button))
		{
			g_Config.m_ClRaceSaveGhost ^= 1;
		}
	}

	MainView.HSplitTop(330.0f, &Gameplay, &MainView);

	Gameplay.HSplitTop(30.0f, &Label, &Gameplay);
	UI()->DoLabel(&Label, Localize("Gameplay"), 20.0f, TEXTALIGN_ML);
	Gameplay.Margin(5.0f, &Gameplay);
	Gameplay.VSplitMid(&Left, &Right);
	Left.VSplitRight(5.0f, &Left, 0);
	Right.VMargin(5.0f, &Right);

	{
		Left.HSplitTop(20.0f, &Button, &Left);
		UI()->DoScrollbarOption(&g_Config.m_ClOverlayEntities, &g_Config.m_ClOverlayEntities, &Button, Localize("Overlay entities"), 0, 100);
	}

	{
		Left.HSplitTop(20.0f, &Button, &Left);
		Button.VSplitMid(&LeftLeft, &Button);

		if(DoButton_CheckBox(&g_Config.m_ClTextEntities, Localize("Show text entities"), g_Config.m_ClTextEntities, &LeftLeft))
			g_Config.m_ClTextEntities ^= 1;

		if(g_Config.m_ClTextEntities)
			UI()->DoScrollbarOption(&g_Config.m_ClTextEntitiesSize, &g_Config.m_ClTextEntitiesSize, &Button, Localize("Size"), 0, 100);
	}

	{
		Left.HSplitTop(20.0f, &Button, &Left);
		Button.VSplitMid(&LeftLeft, &Button);

		if(DoButton_CheckBox(&g_Config.m_ClShowOthers, Localize("Show others"), g_Config.m_ClShowOthers == SHOW_OTHERS_ON, &LeftLeft))
			g_Config.m_ClShowOthers = g_Config.m_ClShowOthers != SHOW_OTHERS_ON ? SHOW_OTHERS_ON : SHOW_OTHERS_OFF;

		UI()->DoScrollbarOption(&g_Config.m_ClShowOthersAlpha, &g_Config.m_ClShowOthersAlpha, &Button, Localize("Opacity"), 0, 100);

		GameClient()->m_Tooltips.DoToolTip(&g_Config.m_ClShowOthersAlpha, &Button, Localize("Adjust the opacity of entities belonging to other teams, such as tees and nameplates"));
	}

	Left.HSplitTop(20.0f, &Button, &Left);
	static int s_ShowOwnTeamID = 0;
	if(DoButton_CheckBox(&s_ShowOwnTeamID, Localize("Show others (own team only)"), g_Config.m_ClShowOthers == SHOW_OTHERS_ONLY_TEAM, &Button))
	{
		g_Config.m_ClShowOthers = g_Config.m_ClShowOthers != SHOW_OTHERS_ONLY_TEAM ? SHOW_OTHERS_ONLY_TEAM : SHOW_OTHERS_OFF;
	}

	Left.HSplitTop(20.0f, &Button, &Left);
	if(DoButton_CheckBox(&g_Config.m_ClShowQuads, Localize("Show quads"), g_Config.m_ClShowQuads, &Button))
	{
		g_Config.m_ClShowQuads ^= 1;
	}
	GameClient()->m_Tooltips.DoToolTip(&g_Config.m_ClShowQuads, &Button, Localize("Quads are used for background decoration"));

	Right.HSplitTop(20.0f, &Button, &Right);
	UI()->DoScrollbarOption(&g_Config.m_ClDefaultZoom, &g_Config.m_ClDefaultZoom, &Button, Localize("Default zoom"), 0, 20);

	Right.HSplitTop(20.0f, &Button, &Right);
	if(DoButton_CheckBox(&g_Config.m_ClAntiPing, Localize("AntiPing"), g_Config.m_ClAntiPing, &Button))
	{
		g_Config.m_ClAntiPing ^= 1;
	}
	GameClient()->m_Tooltips.DoToolTip(&g_Config.m_ClAntiPing, &Button, Localize("Tries to predict other entities to give a feel of low latency"));

	if(g_Config.m_ClAntiPing)
	{
		Right.HSplitTop(20.0f, &Button, &Right);
		if(DoButton_CheckBox(&g_Config.m_ClAntiPingPlayers, Localize("AntiPing: predict other players"), g_Config.m_ClAntiPingPlayers, &Button))
		{
			g_Config.m_ClAntiPingPlayers ^= 1;
		}

		Right.HSplitTop(20.0f, &Button, &Right);
		if(DoButton_CheckBox(&g_Config.m_ClAntiPingWeapons, Localize("AntiPing: predict weapons"), g_Config.m_ClAntiPingWeapons, &Button))
		{
			g_Config.m_ClAntiPingWeapons ^= 1;
		}

		Right.HSplitTop(20.0f, &Button, &Right);
		if(DoButton_CheckBox(&g_Config.m_ClAntiPingGrenade, Localize("AntiPing: predict grenade paths"), g_Config.m_ClAntiPingGrenade, &Button))
		{
			g_Config.m_ClAntiPingGrenade ^= 1;
		}
	}
	else
	{
		Right.HSplitTop(60.0f, 0, &Right);
	}

	Right.HSplitTop(40.0f, 0, &Right);

	Left.HSplitTop(5.0f, &Button, &Left);
	Left.VSplitRight(10.0f, &Left, 0x0);
	Right.VSplitLeft(10.0f, 0x0, &Right);
	Left.HSplitTop(25.0f, 0x0, &Left);
	CUIRect TempLabel;
	Left.HSplitTop(25.0f, &TempLabel, &Left);
	Left.HSplitTop(5.0f, 0x0, &Left);

	UI()->DoLabel(&TempLabel, Localize("Background"), 20.0f, TEXTALIGN_ML);

	Right.HSplitTop(25.0f, &TempLabel, &Right);
	Right.HSplitTop(5.0f, 0x0, &Miscellaneous);

	UI()->DoLabel(&TempLabel, Localize("Miscellaneous"), 20.0f, TEXTALIGN_ML);

	static CButtonContainer s_ResetID2;
	ColorRGBA GreyDefault(0.5f, 0.5f, 0.5f, 1);
	DoLine_ColorPicker(&s_ResetID2, 25.0f, 13.0f, 5.0f, &Left, Localize("Entities Background color"), &g_Config.m_ClBackgroundEntitiesColor, GreyDefault, false);

	Left.VSplitLeft(5.0f, 0x0, &Left);
	Left.HSplitTop(25.0f, &Background, &Left);
	Background.HSplitTop(20.0f, &Background, 0);
	Background.VSplitLeft(100.0f, &Label, &TempLabel);
	UI()->DoLabel(&Label, Localize("Map"), 14.0f, TEXTALIGN_ML);
	static CLineInput s_BackgroundEntitiesInput(g_Config.m_ClBackgroundEntities, sizeof(g_Config.m_ClBackgroundEntities));
	UI()->DoEditBox(&s_BackgroundEntitiesInput, &TempLabel, 14.0f);

	Left.HSplitTop(20.0f, &Button, &Left);
	bool UseCurrentMap = str_comp(g_Config.m_ClBackgroundEntities, CURRENT_MAP) == 0;
	static int s_UseCurrentMapID = 0;
	if(DoButton_CheckBox(&s_UseCurrentMapID, Localize("Use current map as background"), UseCurrentMap, &Button))
	{
		if(UseCurrentMap)
			g_Config.m_ClBackgroundEntities[0] = '\0';
		else
			str_copy(g_Config.m_ClBackgroundEntities, CURRENT_MAP);
	}

	Left.HSplitTop(20.0f, &Button, &Left);
	if(DoButton_CheckBox(&g_Config.m_ClBackgroundShowTilesLayers, Localize("Show tiles layers from BG map"), g_Config.m_ClBackgroundShowTilesLayers, &Button))
		g_Config.m_ClBackgroundShowTilesLayers ^= 1;

	static CButtonContainer s_ResetID1;
	Miscellaneous.HSplitTop(25.0f, &Button, &Right);
	DoLine_ColorPicker(&s_ResetID1, 25.0f, 13.0f, 5.0f, &Button, Localize("Regular Background Color"), &g_Config.m_ClBackgroundColor, GreyDefault, false);

	static CButtonContainer s_ButtonTimeout;
	Right.HSplitTop(10.0f, 0x0, &Right);
	Right.HSplitTop(20.0f, &Button, &Right);
	if(DoButton_Menu(&s_ButtonTimeout, Localize("New random timeout code"), 0, &Button))
	{
		Client()->GenerateTimeoutSeed();
	}

	Right.HSplitTop(5.0f, 0, &Right);
	Right.HSplitTop(20.0f, &Label, &Right);
	Label.VSplitLeft(5.0f, 0, &Label);
	UI()->DoLabel(&Label, Localize("Run on join"), 14.0f, TEXTALIGN_ML);
	Right.HSplitTop(20.0f, &Button, &Right);
	Button.VSplitLeft(5.0f, 0, &Button);
	static CLineInput s_RunOnJoinInput(g_Config.m_ClRunOnJoin, sizeof(g_Config.m_ClRunOnJoin));
	s_RunOnJoinInput.SetEmptyText(Localize("Chat command (e.g. showall 1)"));
	UI()->DoEditBox(&s_RunOnJoinInput, &Button, 14.0f);

#if defined(CONF_FAMILY_WINDOWS)
	static CButtonContainer s_ButtonUnregisterShell;
	Right.HSplitTop(10.0f, nullptr, &Right);
	Right.HSplitTop(20.0f, &Button, &Right);
	if(DoButton_Menu(&s_ButtonUnregisterShell, Localize("Unregister protocol and file extensions"), 0, &Button))
	{
		Client()->ShellUnregister();
	}
#endif

	// Updater
#if defined(CONF_AUTOUPDATE)
	{
		MainView.VSplitMid(&Left, &Right);
		Left.w += 20.0f;
		Left.HSplitBottom(20.0f, 0x0, &Label);
		bool NeedUpdate = str_comp(Client()->LatestVersion(), "0");
		int State = Updater()->GetCurrentState();

		// Update Button
		char aBuf[256];
		if(NeedUpdate && State <= IUpdater::CLEAN)
		{
			str_format(aBuf, sizeof(aBuf), Localize("DDNet %s is available:"), Client()->LatestVersion());
			Label.VSplitLeft(TextRender()->TextWidth(14.0f, aBuf, -1, -1.0f) + 10.0f, &Label, &Button);
			Button.VSplitLeft(100.0f, &Button, 0);
			static CButtonContainer s_ButtonUpdate;
			if(DoButton_Menu(&s_ButtonUpdate, Localize("Update now"), 0, &Button))
			{
				Updater()->InitiateUpdate();
			}
		}
		else if(State >= IUpdater::GETTING_MANIFEST && State < IUpdater::NEED_RESTART)
			str_format(aBuf, sizeof(aBuf), Localize("Updating..."));
		else if(State == IUpdater::NEED_RESTART)
		{
			str_format(aBuf, sizeof(aBuf), Localize("DDNet Client updated!"));
			m_NeedRestartUpdate = true;
		}
		else
		{
			str_format(aBuf, sizeof(aBuf), Localize("No updates available"));
			Label.VSplitLeft(TextRender()->TextWidth(14.0f, aBuf, -1, -1.0f) + 10.0f, &Label, &Button);
			Button.VSplitLeft(100.0f, &Button, 0);
			static CButtonContainer s_ButtonUpdate;
			if(DoButton_Menu(&s_ButtonUpdate, Localize("Check now"), 0, &Button))
			{
				Client()->RequestDDNetInfo();
			}
		}
		UI()->DoLabel(&Label, aBuf, 14.0f, TEXTALIGN_ML);
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
	}
#endif
}
