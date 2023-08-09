/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_MENUS_H
#define GAME_CLIENT_COMPONENTS_MENUS_H

#include <base/types.h>
#include <base/vmath.h>

#include <chrono>
#include <unordered_set>
#include <vector>

#include <engine/console.h>
#include <engine/demo.h>
#include <engine/friends.h>
#include <engine/serverbrowser.h>
#include <engine/shared/config.h>
#include <engine/shared/linereader.h>
#include <engine/textrender.h>

#include <game/client/component.h>
#include <game/client/components/mapimages.h>
#include <game/client/render.h>
#include <game/client/ui.h>
#include <game/voting.h>

struct CServerProcess
{
	PROCESS m_Process;
};

// component to fetch keypresses, override all other input
class CMenusKeyBinder : public CComponent
{
public:
	bool m_TakeKey;
	bool m_GotKey;
	IInput::CEvent m_Key;
	int m_ModifierCombination;
	CMenusKeyBinder();
	virtual int Sizeof() const override { return sizeof(*this); }
	virtual bool OnInput(const IInput::CEvent &Event) override;
};

class CMenus : public CComponent
{
	static ColorRGBA ms_GuiColor;
	static ColorRGBA ms_ColorTabbarInactiveOutgame;
	static ColorRGBA ms_ColorTabbarActiveOutgame;
	static ColorRGBA ms_ColorTabbarHoverOutgame;
	static ColorRGBA ms_ColorTabbarInactiveIngame;
	static ColorRGBA ms_ColorTabbarActiveIngame;
	static ColorRGBA ms_ColorTabbarHoverIngame;
	static ColorRGBA ms_ColorTabbarInactive;
	static ColorRGBA ms_ColorTabbarActive;
	static ColorRGBA ms_ColorTabbarHover;

	int DoButton_FontIcon(CButtonContainer *pButtonContainer, const char *pText, int Checked, const CUIRect *pRect, int Corners = IGraphics::CORNER_ALL, bool Enabled = true);
	int DoButton_Toggle(const void *pID, int Checked, const CUIRect *pRect, bool Active);
	int DoButton_Menu(CButtonContainer *pButtonContainer, const char *pText, int Checked, const CUIRect *pRect, const char *pImageName = nullptr, int Corners = IGraphics::CORNER_ALL, float r = 5.0f, float FontFactor = 0.0f, vec4 ColorHot = vec4(1.0f, 1.0f, 1.0f, 0.75f), vec4 Color = vec4(1, 1, 1, 0.5f));
	int DoButton_MenuTab(CButtonContainer *pButtonContainer, const char *pText, int Checked, const CUIRect *pRect, int Corners, SUIAnimator *pAnimator = nullptr, const ColorRGBA *pDefaultColor = nullptr, const ColorRGBA *pActiveColor = nullptr, const ColorRGBA *pHoverColor = nullptr, float EdgeRounding = 10);

	int DoButton_CheckBox_Common(const void *pID, const char *pText, const char *pBoxText, const CUIRect *pRect);
	int DoButton_CheckBox(const void *pID, const char *pText, int Checked, const CUIRect *pRect);
	int DoButton_CheckBoxAutoVMarginAndSet(const void *pID, const char *pText, int *pValue, CUIRect *pRect, float VMargin);
	int DoButton_CheckBox_Number(const void *pID, const char *pText, int Checked, const CUIRect *pRect);

	ColorHSLA DoLine_ColorPicker(CButtonContainer *pResetID, float LineSize, float LabelSize, float BottomMargin, CUIRect *pMainRect, const char *pText, unsigned int *pColorValue, ColorRGBA DefaultColor, bool CheckBoxSpacing = true, int *pCheckBoxValue = nullptr, bool Alpha = false);
	ColorHSLA DoButton_ColorPicker(const CUIRect *pRect, unsigned int *pHslaColor, bool Alpha);
	void DoLaserPreview(const CUIRect *pRect, ColorHSLA OutlineColor, ColorHSLA InnerColor, const int LaserType);
	int DoButton_GridHeader(const void *pID, const char *pText, int Checked, const CUIRect *pRect);

	void DoButton_KeySelect(const void *pID, const char *pText, const CUIRect *pRect);
	int DoKeyReader(void *pID, const CUIRect *pRect, int Key, int ModifierCombination, int *pNewModifierCombination);

	void DoSettingsControlsButtons(int Start, int Stop, CUIRect View);

	float RenderSettingsControlsJoystick(CUIRect View);
	void DoJoystickAxisPicker(CUIRect View);
	void DoJoystickBar(const CUIRect *pRect, float Current, float Tolerance, bool Active);

	void RefreshSkins();

	void RandomSkin();

	// menus_settings_assets.cpp
public:
	struct SCustomItem
	{
		IGraphics::CTextureHandle m_RenderTexture;

		char m_aName[50];

		bool operator<(const SCustomItem &Other) const { return str_comp(m_aName, Other.m_aName) < 0; }
	};

	struct SCustomEntities : public SCustomItem
	{
		struct SEntitiesImage
		{
			IGraphics::CTextureHandle m_Texture;
		};
		SEntitiesImage m_aImages[MAP_IMAGE_MOD_TYPE_COUNT];
	};

	struct SCustomGame : public SCustomItem
	{
	};

	struct SCustomEmoticon : public SCustomItem
	{
	};

	struct SCustomParticle : public SCustomItem
	{
	};

	struct SCustomHud : public SCustomItem
	{
	};

	struct SCustomExtras : public SCustomItem
	{
	};

protected:
	std::vector<SCustomEntities> m_vEntitiesList;
	std::vector<SCustomGame> m_vGameList;
	std::vector<SCustomEmoticon> m_vEmoticonList;
	std::vector<SCustomParticle> m_vParticlesList;
	std::vector<SCustomHud> m_vHudList;
	std::vector<SCustomExtras> m_vExtrasList;

	bool m_IsInit = false;

	static void LoadEntities(struct SCustomEntities *pEntitiesItem, void *pUser);
	static int EntitiesScan(const char *pName, int IsDir, int DirType, void *pUser);

	static int GameScan(const char *pName, int IsDir, int DirType, void *pUser);
	static int EmoticonsScan(const char *pName, int IsDir, int DirType, void *pUser);
	static int ParticlesScan(const char *pName, int IsDir, int DirType, void *pUser);
	static int HudScan(const char *pName, int IsDir, int DirType, void *pUser);
	static int ExtrasScan(const char *pName, int IsDir, int DirType, void *pUser);

	static void ConchainAssetsEntities(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainAssetGame(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainAssetParticles(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainAssetEmoticons(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainAssetHud(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainAssetExtras(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);

	void ClearCustomItems(int CurTab);

	int m_MenuPage;
	int m_GamePage;
	int m_Popup;
	int m_ActivePage;
	bool m_ShowStart;
	bool m_MenuActive;
	bool m_JoinTutorial;

	char m_aNextServer[256];

	// images
	struct CMenuImage
	{
		char m_aName[64];
		IGraphics::CTextureHandle m_OrgTexture;
		IGraphics::CTextureHandle m_GreyTexture;
	};
	std::vector<CMenuImage> m_vMenuImages;

	static int MenuImageScan(const char *pName, int IsDir, int DirType, void *pUser);

	const CMenuImage *FindMenuImage(const char *pName);

	// loading
	int m_LoadCurrent;
	int m_LoadTotal;

	//
	char m_aMessageTopic[512];
	char m_aMessageBody[512];
	char m_aMessageButton[512];

	CUIElement m_RefreshButton;
	CUIElement m_ConnectButton;

	// generic popups
	typedef void (CMenus::*FPopupButtonCallback)();
	void DefaultButtonCallback()
	{
		// do nothing
	}
	enum
	{
		BUTTON_CONFIRM = 0, // confirm / yes / close / ok
		BUTTON_CANCEL, // cancel / no
		NUM_BUTTONS
	};
	char m_aPopupTitle[128];
	char m_aPopupMessage[IO_MAX_PATH_LENGTH + 256];
	struct
	{
		char m_aLabel[64];
		int m_NextPopup;
		FPopupButtonCallback m_pfnCallback;
	} m_aPopupButtons[NUM_BUTTONS];

	void PopupMessage(const char *pTitle, const char *pMessage,
		const char *pButtonLabel, int NextPopup = POPUP_NONE, FPopupButtonCallback pfnButtonCallback = &CMenus::DefaultButtonCallback);
	void PopupConfirm(const char *pTitle, const char *pMessage,
		const char *pConfirmButtonLabel, const char *pCancelButtonLabel,
		FPopupButtonCallback pfnConfirmButtonCallback = &CMenus::DefaultButtonCallback, int ConfirmNextPopup = POPUP_NONE,
		FPopupButtonCallback pfnCancelButtonCallback = &CMenus::DefaultButtonCallback, int CancelNextPopup = POPUP_NONE);

	// some settings
	static float ms_ButtonHeight;
	static float ms_ListheaderHeight;
	static float ms_ListitemAdditionalHeight;

	// for settings
	bool m_NeedRestartGeneral;
	bool m_NeedRestartSkins;
	bool m_NeedRestartGraphics;
	bool m_NeedRestartSound;
	bool m_NeedRestartUpdate;
	bool m_NeedRestartDDNet;
	bool m_NeedSendinfo;
	bool m_NeedSendDummyinfo;
	int m_SettingPlayerPage;

	// for map download popup
	int64_t m_DownloadLastCheckTime;
	int m_DownloadLastCheckSize;
	float m_DownloadSpeed;

	// for password popup
	CLineInput m_PasswordInput;

	// for call vote
	int m_CallvoteSelectedOption;
	int m_CallvoteSelectedPlayer;
	CLineInputBuffered<VOTE_REASON_LENGTH> m_CallvoteReasonInput;
	CLineInputBuffered<64> m_FilterInput;
	bool m_ControlPageOpening;

	// demo
	enum
	{
		SORT_DEMONAME = 0,
		SORT_MARKERS,
		SORT_LENGTH,
		SORT_DATE,
	};

	struct CDemoItem
	{
		char m_aFilename[IO_MAX_PATH_LENGTH];
		char m_aName[IO_MAX_PATH_LENGTH];
		bool m_IsDir;
		bool m_IsLink;
		int m_StorageType;
		time_t m_Date;

		bool m_InfosLoaded;
		bool m_Valid;
		CDemoHeader m_Info;
		CTimelineMarkers m_TimelineMarkers;
		CMapInfo m_MapInfo;

		int NumMarkers() const
		{
			return clamp<int>(bytes_be_to_uint(m_TimelineMarkers.m_aNumTimelineMarkers), 0, MAX_TIMELINE_MARKERS);
		}

		int Length() const
		{
			return bytes_be_to_uint(m_Info.m_aLength);
		}

		unsigned Size() const
		{
			return bytes_be_to_uint(m_Info.m_aMapSize);
		}

		bool operator<(const CDemoItem &Other) const
		{
			if(!str_comp(m_aFilename, ".."))
				return true;
			if(!str_comp(Other.m_aFilename, ".."))
				return false;
			if(m_IsDir && !Other.m_IsDir)
				return true;
			if(!m_IsDir && Other.m_IsDir)
				return false;

			const CDemoItem &Left = g_Config.m_BrDemoSortOrder ? Other : *this;
			const CDemoItem &Right = g_Config.m_BrDemoSortOrder ? *this : Other;

			if(g_Config.m_BrDemoSort == SORT_DEMONAME)
				return str_comp_filenames(Left.m_aFilename, Right.m_aFilename) < 0;
			if(g_Config.m_BrDemoSort == SORT_DATE)
				return Left.m_Date < Right.m_Date;

			if(!Other.m_InfosLoaded)
				return m_InfosLoaded;
			if(!m_InfosLoaded)
				return !Other.m_InfosLoaded;

			if(g_Config.m_BrDemoSort == SORT_MARKERS)
				return Left.NumMarkers() < Right.NumMarkers();
			if(g_Config.m_BrDemoSort == SORT_LENGTH)
				return Left.Length() < Right.Length();

			// Unknown sort
			return true;
		}
	};

	char m_aCurrentDemoFolder[IO_MAX_PATH_LENGTH];
	char m_aCurrentDemoSelectionName[IO_MAX_PATH_LENGTH];
	CLineInputBuffered<IO_MAX_PATH_LENGTH> m_DemoRenameInput;
	CLineInputBuffered<IO_MAX_PATH_LENGTH> m_DemoSliceInput;
#if defined(CONF_VIDEORECORDER)
	CLineInputBuffered<IO_MAX_PATH_LENGTH> m_DemoRenderInput;
#endif
	int m_DemolistSelectedIndex;
	bool m_DemolistSelectedReveal = false;
	int m_DemolistStorageType;
	bool m_DemolistMultipleStorages = false;
	int m_Speed = 4;

	std::chrono::nanoseconds m_DemoPopulateStartTime{0};

	void DemolistOnUpdate(bool Reset);
	static int DemolistFetchCallback(const CFsFileInfo *pInfo, int IsDir, int StorageType, void *pUser);

	// friends
	class CFriendItem
	{
		char m_aName[MAX_NAME_LENGTH];
		char m_aClan[MAX_CLAN_LENGTH];
		const CServerInfo *m_pServerInfo;
		int m_FriendState;
		bool m_IsPlayer;
		// skin
		char m_aSkin[24 + 1];
		bool m_CustomSkinColors;
		int m_CustomSkinColorBody;
		int m_CustomSkinColorFeet;

	public:
		CFriendItem() {}
		CFriendItem(const CFriendInfo *pFriendInfo) :
			m_pServerInfo(nullptr),
			m_IsPlayer(false),
			m_CustomSkinColors(false),
			m_CustomSkinColorBody(0),
			m_CustomSkinColorFeet(0)
		{
			str_copy(m_aName, pFriendInfo->m_aName);
			str_copy(m_aClan, pFriendInfo->m_aClan);
			m_FriendState = m_aName[0] == '\0' ? IFriends::FRIEND_CLAN : IFriends::FRIEND_PLAYER;
			m_aSkin[0] = '\0';
		}
		CFriendItem(const CServerInfo::CClient &CurrentClient, const CServerInfo *pServerInfo) :
			m_pServerInfo(pServerInfo),
			m_FriendState(CurrentClient.m_FriendState),
			m_IsPlayer(CurrentClient.m_Player),
			m_CustomSkinColors(CurrentClient.m_CustomSkinColors),
			m_CustomSkinColorBody(CurrentClient.m_CustomSkinColorBody),
			m_CustomSkinColorFeet(CurrentClient.m_CustomSkinColorFeet)
		{
			str_copy(m_aName, CurrentClient.m_aName);
			str_copy(m_aClan, CurrentClient.m_aClan);
			str_copy(m_aSkin, CurrentClient.m_aSkin);
		}

		const char *Name() const { return m_aName; }
		const char *Clan() const { return m_aClan; }
		const CServerInfo *ServerInfo() const { return m_pServerInfo; }
		int FriendState() const { return m_FriendState; }
		bool IsPlayer() const { return m_IsPlayer; }
		const char *Skin() const { return m_aSkin; }
		bool CustomSkinColors() const { return m_CustomSkinColors; }
		int CustomSkinColorBody() const { return m_CustomSkinColorBody; }
		int CustomSkinColorFeet() const { return m_CustomSkinColorFeet; }

		const void *ListItemId() const { return &m_aName; }
		const void *RemoveButtonId() const { return &m_FriendState; }

		bool operator<(const CFriendItem &Other) const
		{
			const int Result = str_comp(m_aName, Other.m_aName);
			return Result < 0 || (Result == 0 && str_comp(m_aClan, Other.m_aClan) < 0);
		}
	};

	enum
	{
		FRIEND_PLAYER_ON = 0,
		FRIEND_CLAN_ON,
		FRIEND_OFF,
		NUM_FRIEND_TYPES
	};
	std::vector<CFriendItem> m_avFriends[NUM_FRIEND_TYPES];
	const CFriendItem *m_pRemoveFriend = nullptr;

	void FriendlistOnUpdate();

	// found in menus.cpp
	int Render();
#if defined(CONF_VIDEORECORDER)
	void PopupConfirmDemoReplaceVideo();
#endif
	int RenderMenubar(CUIRect r);
	void RenderNews(CUIRect MainView);
	static void ConchainUpdateMusicState(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	void UpdateMusicState();

	// found in menus_demo.cpp
	vec2 m_DemoControlsPositionOffset = vec2(0.0f, 0.0f);
	static bool DemoFilterChat(const void *pData, int Size, void *pUser);
	bool FetchHeader(CDemoItem &Item);
	void FetchAllHeaders();
	void HandleDemoSeeking(float PositionToSeek, float TimeToSeek);
	void RenderDemoPlayer(CUIRect MainView);
	void RenderDemoPlayerSliceSavePopup(CUIRect MainView);
	void RenderDemoList(CUIRect MainView);
	void PopupConfirmDeleteDemo();
	void PopupConfirmDeleteFolder();

	// found in menus_start.cpp
	void RenderStartMenu(CUIRect MainView);

	// found in menus_ingame.cpp
	STextContainerIndex m_MotdTextContainerIndex;
	void RenderGame(CUIRect MainView);
	void PopupConfirmDisconnect();
	void PopupConfirmDisconnectDummy();
	void RenderPlayers(CUIRect MainView);
	void RenderServerInfo(CUIRect MainView);
	void RenderServerInfoMotd(CUIRect Motd);
	void RenderServerControl(CUIRect MainView);
	bool RenderServerControlKick(CUIRect MainView, bool FilterSpectators);
	bool RenderServerControlServer(CUIRect MainView);
	void RenderIngameHint();

	// found in menus_browser.cpp
	int m_SelectedIndex;
	bool m_ServerBrowserShouldRevealSelection;
	void RenderServerbrowserServerList(CUIRect View);
	void Connect(const char *pAddress);
	void PopupConfirmSwitchServer();
	void RenderServerbrowserServerDetail(CUIRect View);
	void RenderServerbrowserFilters(CUIRect View);
	struct SPopupCountrySelectionContext
	{
		CMenus *m_pMenus;
		int m_Selection;
		bool m_New;
	};
	static CUI::EPopupMenuFunctionResult PopupCountrySelection(void *pContext, CUIRect View, bool Active);
	void RenderServerbrowserFriends(CUIRect View);
	void PopupConfirmRemoveFriend();
	void RenderServerbrowser(CUIRect MainView);
	template<typename F>
	bool PrintHighlighted(const char *pName, F &&PrintFn);
	static void ConchainFriendlistUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainServerbrowserUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);

	// skin favorite list
	bool m_SkinFavoritesChanged = false;
	std::unordered_set<std::string> m_SkinFavorites;
	static void Con_AddFavoriteSkin(IConsole::IResult *pResult, void *pUserData);
	static void Con_RemFavoriteSkin(IConsole::IResult *pResult, void *pUserData);
	static void ConfigSaveCallback(IConfigManager *pConfigManager, void *pUserData);
	void OnConfigSave(IConfigManager *pConfigManager);

	// found in menus_settings.cpp
	bool RenderLanguageSelection(CUIRect MainView);
	void RenderThemeSelection(CUIRect MainView);
	void RenderSettingsGeneral(CUIRect MainView);
	void RenderSettingsPlayer(CUIRect MainView);
	void RenderSettingsDummyPlayer(CUIRect MainView);
	void RenderSettingsTee(CUIRect MainView);
	void RenderSettingsControls(CUIRect MainView);
	void ResetSettingsControls();
	void RenderSettingsGraphics(CUIRect MainView);
	void RenderSettingsSound(CUIRect MainView);
	void RenderSettings(CUIRect MainView);
	void RenderSettingsCustom(CUIRect MainView);
	void RenderSettingsYT(CUIRect MainView);

	void SetNeedSendInfo();
	void SetActive(bool Active);
	void UpdateColors();

	IGraphics::CTextureHandle m_TextureBlob;

	bool CheckHotKey(int Key) const;

	class CMenuBackground *m_pBackground;

public:
	void RenderBackground();

	void SetMenuBackground(class CMenuBackground *pBackground) { m_pBackground = pBackground; }

	static CMenusKeyBinder m_Binder;

	CMenus();
	virtual int Sizeof() const override { return sizeof(*this); }

	void RenderLoading(const char *pCaption, const char *pContent, int IncreaseCounter, bool RenderLoadingBar = true, bool RenderMenuBackgroundMap = true);

	bool IsInit() { return m_IsInit; }

	bool IsActive() const { return m_MenuActive; }
	void KillServer();

	virtual void OnInit() override;
	void OnConsoleInit() override;

	virtual void OnStateChange(int NewState, int OldState) override;
	virtual void OnWindowResize() override;
	virtual void OnReset() override;
	virtual void OnRender() override;
	virtual bool OnInput(const IInput::CEvent &Event) override;
	virtual bool OnCursorMove(float x, float y, IInput::ECursorType CursorType) override;
	virtual void OnShutdown() override;

	enum
	{
		PAGE_NEWS = 1,
		PAGE_GAME,
		PAGE_PLAYERS,
		PAGE_SERVER_INFO,
		PAGE_CALLVOTE,
		PAGE_INTERNET,
		PAGE_LAN,
		PAGE_FAVORITES,
		PAGE_DDNET,
		PAGE_KOG,
		PAGE_DEMOS,
		PAGE_SETTINGS,
		PAGE_SYSTEM,
		PAGE_NETWORK,
		PAGE_GHOST,

		PAGE_LENGTH,

		SETTINGS_LANGUAGE = 0,
		SETTINGS_GENERAL,
		SETTINGS_PLAYER,
		SETTINGS_TEE,
		SETTINGS_APPEARANCE,
		SETTINGS_CONTROLS,
		SETTINGS_GRAPHICS,
		SETTINGS_SOUND,
		SETTINGS_DDNET,
		SETTINGS_ASSETS,
		SETTINGS_YT,

		SETTINGS_LENGTH,

		BIG_TAB_NEWS = 0,
		BIG_TAB_INTERNET,
		BIG_TAB_LAN,
		BIG_TAB_FAVORITES,
		BIG_TAB_DDNET,
		BIG_TAB_KOG,
		BIG_TAB_DEMOS,

		BIG_TAB_LENGTH,

		SMALL_TAB_HOME = 0,
		SMALL_TAB_QUIT,
		SMALL_TAB_SETTINGS,
		SMALL_TAB_EDITOR,
		SMALL_TAB_DEMOBUTTON,
		SMALL_TAB_SERVER,

		SMALL_TAB_LENGTH,
	};

	SUIAnimator m_aAnimatorsBigPage[BIG_TAB_LENGTH];
	SUIAnimator m_aAnimatorsSmallPage[SMALL_TAB_LENGTH];
	SUIAnimator m_aAnimatorsSettingsTab[SETTINGS_LENGTH];

	// DDRace
	int DoButton_CheckBox_Tristate(const void *pID, const char *pText, TRISTATE Checked, const CUIRect *pRect);
	std::vector<CDemoItem> m_vDemos;
	void DemolistPopulate();
	void DemoSeekTick(IDemoPlayer::ETickOffset TickOffset);
	bool m_Dummy;

	const char *GetCurrentDemoFolder() const { return m_aCurrentDemoFolder; }

	// Ghost
	struct CGhostItem
	{
		char m_aFilename[IO_MAX_PATH_LENGTH];
		char m_aPlayer[MAX_NAME_LENGTH];

		int m_Time;
		int m_Slot;
		bool m_Own;

		CGhostItem() :
			m_Slot(-1), m_Own(false) { m_aFilename[0] = 0; }

		bool operator<(const CGhostItem &Other) const { return m_Time < Other.m_Time; }

		bool Active() const { return m_Slot != -1; }
		bool HasFile() const { return m_aFilename[0]; }
	};

	std::vector<CGhostItem> m_vGhosts;

	std::chrono::nanoseconds m_GhostPopulateStartTime{0};

	void GhostlistPopulate();
	CGhostItem *GetOwnGhost();
	void UpdateOwnGhost(CGhostItem Item);
	void DeleteGhostItem(int Index);

	int GetCurPopup() { return m_Popup; }
	bool CanDisplayWarning();

	void PopupWarning(const char *pTopic, const char *pBody, const char *pButton, std::chrono::nanoseconds Duration);

	std::chrono::nanoseconds m_PopupWarningLastTime;
	std::chrono::nanoseconds m_PopupWarningDuration;

	int m_DemoPlayerState;

	enum
	{
		POPUP_NONE = 0,
		POPUP_MESSAGE, // generic message popup (one button)
		POPUP_CONFIRM, // generic confirmation popup (two buttons)
		POPUP_FIRST_LAUNCH,
		POPUP_POINTS,
		POPUP_CONNECTING,
		POPUP_DISCONNECTED,
		POPUP_LANGUAGE,
		POPUP_RENAME_DEMO,
		POPUP_RENDER_DEMO,
		POPUP_PASSWORD,
		POPUP_QUIT,
		POPUP_WARNING,

		// demo player states
		DEMOPLAYER_NONE = 0,
		DEMOPLAYER_SLICE_SAVE,
	};

private:
	static int GhostlistFetchCallback(const char *pName, int IsDir, int StorageType, void *pUser);
	void SetMenuPage(int NewPage);
	void RefreshBrowserTab(int UiPage);

	// found in menus_ingame.cpp
	void RenderInGameNetwork(CUIRect MainView);
	void RenderGhost(CUIRect MainView);

	// found in menus_settings.cpp
	void RenderSettingsDDNet(CUIRect MainView);
	void RenderSettingsAppearance(CUIRect MainView);
	ColorHSLA RenderHSLScrollbars(CUIRect *pRect, unsigned int *pColor, bool Alpha = false, bool ClampedLight = false);

	CServerProcess m_ServerProcess;
};
#endif
