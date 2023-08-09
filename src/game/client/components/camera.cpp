/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include <engine/shared/config.h>

#include <base/math.h>
#include <game/client/gameclient.h>
#include <game/collision.h>
#include <game/mapitems.h>

#include "camera.h"
#include "controls.h"

#include <limits>

const float ZoomStep = 0.866025f;

CCamera::CCamera()
{
	m_CamType = CAMTYPE_UNDEFINED;
	m_ZoomSet = false;
	m_Zoom = 1.0f;
	m_Zooming = false;
	m_ForceFreeviewPos = vec2(-1, -1);
	m_GotoSwitchOffset = 0;
	m_GotoTeleOffset = 0;
	m_GotoSwitchLastPos = ivec2(-1, -1);
	m_GotoTeleLastPos = ivec2(-1, -1);
}

float CCamera::ZoomProgress(float CurrentTime) const
{
	return (CurrentTime - m_ZoomSmoothingStart) / (m_ZoomSmoothingEnd - m_ZoomSmoothingStart);
}

void CCamera::ScaleZoom(float Factor)
{
	float CurrentTarget = m_Zooming ? m_ZoomSmoothingTarget : m_Zoom;
	ChangeZoom(CurrentTarget * Factor, g_Config.m_ClSmoothZoomTime);
}

float CCamera::MaxZoomLevel()
{
	return (g_Config.m_ClLimitMaxZoomLevel) ? ((Graphics()->IsTileBufferingEnabled() ? 60 : 30)) : std::numeric_limits<float>::max();
}

float CCamera::MinZoomLevel()
{
	return 0.01f;
}

void CCamera::ChangeZoom(float Target, int Smoothness)
{
	if(Target > MaxZoomLevel() || Target < MinZoomLevel())
	{
		return;
	}

	float Now = Client()->LocalTime();
	float Current = m_Zoom;
	float Derivative = 0.0f;
	if(m_Zooming)
	{
		float Progress = ZoomProgress(Now);
		Current = m_ZoomSmoothing.Evaluate(Progress);
		Derivative = m_ZoomSmoothing.Derivative(Progress);
	}

	m_ZoomSmoothingTarget = Target;
	m_ZoomSmoothing = CCubicBezier::With(Current, Derivative, 0, m_ZoomSmoothingTarget);
	m_ZoomSmoothingStart = Now;
	m_ZoomSmoothingEnd = Now + (float)Smoothness / 1000;

	m_Zooming = true;
}

void CCamera::OnRender()
{
	if(m_Zooming)
	{
		float Time = Client()->LocalTime();
		if(Time >= m_ZoomSmoothingEnd)
		{
			m_Zoom = m_ZoomSmoothingTarget;
			m_Zooming = false;
		}
		else
		{
			const float OldLevel = m_Zoom;
			m_Zoom = m_ZoomSmoothing.Evaluate(ZoomProgress(Time));
			if((OldLevel < m_ZoomSmoothingTarget && m_Zoom > m_ZoomSmoothingTarget) || (OldLevel > m_ZoomSmoothingTarget && m_Zoom < m_ZoomSmoothingTarget))
			{
				m_Zoom = m_ZoomSmoothingTarget;
				m_Zooming = false;
			}
		}
		m_Zoom = clamp(m_Zoom, MinZoomLevel(), MaxZoomLevel());
	}

	if(!(m_pClient->m_Snap.m_SpecInfo.m_Active || GameClient()->m_GameInfo.m_AllowZoom || Client()->State() == IClient::STATE_DEMOPLAYBACK))
	{
		m_ZoomSet = false;
		m_Zoom = 1.0f;
		m_Zooming = false;
	}
	else if(!m_ZoomSet && g_Config.m_ClDefaultZoom != 10)
	{
		m_ZoomSet = true;
		OnReset();
	}

	// update camera center
	if(m_pClient->m_Snap.m_SpecInfo.m_Active && !m_pClient->m_Snap.m_SpecInfo.m_UsePosition)
	{
		if(m_CamType != CAMTYPE_SPEC)
		{
			m_aLastPos[g_Config.m_ClDummy] = m_pClient->m_Controls.m_aMousePos[g_Config.m_ClDummy];
			m_pClient->m_Controls.m_aMousePos[g_Config.m_ClDummy] = m_PrevCenter;
			m_pClient->m_Controls.ClampMousePos();
			m_CamType = CAMTYPE_SPEC;
		}
		m_Center = m_pClient->m_Controls.m_aMousePos[g_Config.m_ClDummy];
	}
	else
	{
		if(m_CamType != CAMTYPE_PLAYER)
		{
			m_pClient->m_Controls.m_aMousePos[g_Config.m_ClDummy] = m_aLastPos[g_Config.m_ClDummy];
			m_pClient->m_Controls.ClampMousePos();
			m_CamType = CAMTYPE_PLAYER;
		}

		float DeltaTime = Client()->RenderFrameTime();
		static vec2 s_LastMousePos(0, 0);
		static vec2 s_aCurrentCameraOffset[NUM_DUMMIES] = {vec2(0, 0), vec2(0, 0)};
		static float s_SpeedBias = 0.5f;

		if(g_Config.m_ClDyncamSmoothness > 0)
		{
			float CameraSpeed = (1.0f - (g_Config.m_ClDyncamSmoothness / 100.0f)) * 9.5f + 0.5f;
			float CameraStabilizingFactor = 1 + g_Config.m_ClDyncamStabilizing / 100.0f;

			s_SpeedBias += CameraSpeed * DeltaTime;
			if(g_Config.m_ClDyncam)
			{
				s_SpeedBias -= length(m_pClient->m_Controls.m_aMousePos[g_Config.m_ClDummy] - s_LastMousePos) * std::log10(CameraStabilizingFactor) * 0.02f;
				s_SpeedBias = clamp(s_SpeedBias, 0.5f, CameraSpeed);
			}
			else
			{
				s_SpeedBias = maximum(5.0f, CameraSpeed); // make sure toggle back is fast
			}
		}

		vec2 TargetCameraOffset(0, 0);
		s_LastMousePos = m_pClient->m_Controls.m_aMousePos[g_Config.m_ClDummy];
		float l = length(s_LastMousePos);
		if(l > 0.0001f) // make sure that this isn't 0
		{
			float DeadZone = g_Config.m_ClDyncam ? g_Config.m_ClDyncamDeadzone : g_Config.m_ClMouseDeadzone;
			float FollowFactor = (g_Config.m_ClDyncam ? g_Config.m_ClDyncamFollowFactor : g_Config.m_ClMouseFollowfactor) / 100.0f;
			float OffsetAmount = maximum(l - DeadZone, 0.0f) * FollowFactor;

			TargetCameraOffset = normalize(m_pClient->m_Controls.m_aMousePos[g_Config.m_ClDummy]) * OffsetAmount;
		}

		if(g_Config.m_ClDyncamSmoothness > 0)
			s_aCurrentCameraOffset[g_Config.m_ClDummy] += (TargetCameraOffset - s_aCurrentCameraOffset[g_Config.m_ClDummy]) * minimum(DeltaTime * s_SpeedBias, 1.0f);
		else
			s_aCurrentCameraOffset[g_Config.m_ClDummy] = TargetCameraOffset;

		if(m_pClient->m_Snap.m_SpecInfo.m_Active)
			m_Center = m_pClient->m_Snap.m_SpecInfo.m_Position + s_aCurrentCameraOffset[g_Config.m_ClDummy];
		else
			m_Center = m_pClient->m_LocalCharacterPos + s_aCurrentCameraOffset[g_Config.m_ClDummy];
	}

	if(m_ForceFreeviewPos != vec2(-1, -1) && m_CamType == CAMTYPE_SPEC)
	{
		m_Center = m_pClient->m_Controls.m_aMousePos[g_Config.m_ClDummy] = m_ForceFreeviewPos;
		m_ForceFreeviewPos = vec2(-1, -1);
	}
	m_PrevCenter = m_Center;
}

void CCamera::OnConsoleInit()
{
	Console()->Register("zoom+", "", CFGFLAG_CLIENT, ConZoomPlus, this, "Zoom increase");
	Console()->Register("zoom-", "", CFGFLAG_CLIENT, ConZoomMinus, this, "Zoom decrease");
	Console()->Register("zoom", "?i", CFGFLAG_CLIENT, ConZoom, this, "Change zoom");
	Console()->Register("set_view", "i[x]i[y]", CFGFLAG_CLIENT, ConSetView, this, "Set camera position to x and y in the map");
	Console()->Register("goto_switch", "i[number]?i[offset]", CFGFLAG_CLIENT, ConGotoSwitch, this, "View switch found (at offset) with given number");
	Console()->Register("goto_tele", "i[number]?i[offset]", CFGFLAG_CLIENT, ConGotoTele, this, "View tele found (at offset) with given number");
}

void CCamera::OnReset()
{
	m_Zoom = std::pow(ZoomStep, g_Config.m_ClDefaultZoom - 10);
	m_Zooming = false;
}

void CCamera::ConZoomPlus(IConsole::IResult *pResult, void *pUserData)
{
	CCamera *pSelf = (CCamera *)pUserData;
	if(pSelf->m_pClient->m_Snap.m_SpecInfo.m_Active || pSelf->GameClient()->m_GameInfo.m_AllowZoom || pSelf->Client()->State() == IClient::STATE_DEMOPLAYBACK)
	{
		pSelf->ScaleZoom(ZoomStep);

		if(pSelf->GameClient()->m_MultiViewActivated)
			pSelf->GameClient()->m_MultiViewPersonalZoom++;
	}
}
void CCamera::ConZoomMinus(IConsole::IResult *pResult, void *pUserData)
{
	CCamera *pSelf = (CCamera *)pUserData;
	if(pSelf->m_pClient->m_Snap.m_SpecInfo.m_Active || pSelf->GameClient()->m_GameInfo.m_AllowZoom || pSelf->Client()->State() == IClient::STATE_DEMOPLAYBACK)
	{
		pSelf->ScaleZoom(1 / ZoomStep);

		if(pSelf->GameClient()->m_MultiViewActivated)
			pSelf->GameClient()->m_MultiViewPersonalZoom--;
	}
}
void CCamera::ConZoom(IConsole::IResult *pResult, void *pUserData)
{
	CCamera *pSelf = (CCamera *)pUserData;
	float TargetLevel = pResult->NumArguments() ? pResult->GetFloat(0) : g_Config.m_ClDefaultZoom;
	pSelf->ChangeZoom(std::pow(ZoomStep, TargetLevel - 10), g_Config.m_ClSmoothZoomTime);

	if(pSelf->GameClient()->m_MultiViewActivated)
		pSelf->GameClient()->m_MultiViewPersonalZoom = 0;
}
void CCamera::ConSetView(IConsole::IResult *pResult, void *pUserData)
{
	CCamera *pSelf = (CCamera *)pUserData;
	// wait until free view camera type to update the position
	pSelf->SetView(ivec2(pResult->GetInteger(0), pResult->GetInteger(1)));
}
void CCamera::ConGotoSwitch(IConsole::IResult *pResult, void *pUserData)
{
	CCamera *pSelf = (CCamera *)pUserData;
	pSelf->GotoSwitch(pResult->GetInteger(0), pResult->NumArguments() > 1 ? pResult->GetInteger(1) : -1);
}
void CCamera::ConGotoTele(IConsole::IResult *pResult, void *pUserData)
{
	CCamera *pSelf = (CCamera *)pUserData;
	pSelf->GotoTele(pResult->GetInteger(0), pResult->NumArguments() > 1 ? pResult->GetInteger(1) : -1);
}

void CCamera::SetView(ivec2 Pos)
{
	m_ForceFreeviewPos = vec2(
		clamp(Pos.x * 32.0f, 200.0f, Collision()->GetWidth() * 32 - 200.0f),
		clamp(Pos.y * 32.0f, 200.0f, Collision()->GetWidth() * 32 - 200.0f));
}

void CCamera::GotoSwitch(int Number, int Offset)
{
	if(Collision()->SwitchLayer() == nullptr)
		return;

	int Match = -1;
	ivec2 MatchPos = ivec2(-1, -1);

	auto FindTile = [this, &Match, &MatchPos, &Number, &Offset]() {
		for(int x = 0; x < Collision()->GetWidth(); x++)
		{
			for(int y = 0; y < Collision()->GetHeight(); y++)
			{
				int i = y * Collision()->GetWidth() + x;
				if(Number == Collision()->GetSwitchNumber(i))
				{
					Match++;
					if(Offset != -1)
					{
						if(Match == Offset)
						{
							MatchPos = ivec2(x, y);
							m_GotoSwitchOffset = Match;
							return;
						}
						continue;
					}
					MatchPos = ivec2(x, y);
					if(Match == m_GotoSwitchOffset)
						return;
				}
			}
		}
	};
	FindTile();

	if(MatchPos == ivec2(-1, -1))
		return;
	if(Match < m_GotoSwitchOffset)
		m_GotoSwitchOffset = -1;
	SetView(MatchPos);
	m_GotoSwitchOffset++;
}

void CCamera::GotoTele(int Number, int Offset)
{
	if(Collision()->TeleLayer() == nullptr)
		return;

	int Match = -1;
	ivec2 MatchPos = ivec2(-1, -1);

	auto FindTile = [this, &Match, &MatchPos, &Number, &Offset]() {
		for(int x = 0; x < Collision()->GetWidth(); x++)
		{
			for(int y = 0; y < Collision()->GetHeight(); y++)
			{
				int i = y * Collision()->GetWidth() + x;
				int Tele = Collision()->TeleLayer()[i].m_Number;
				if(Number == Tele)
				{
					Match++;
					if(Offset != -1)
					{
						if(Match == Offset)
						{
							MatchPos = ivec2(x, y);
							m_GotoTeleOffset = Match;
							return;
						}
						continue;
					}
					MatchPos = ivec2(x, y);
					if(m_GotoTeleLastPos != ivec2(-1, -1))
					{
						if(distance(m_GotoTeleLastPos, MatchPos) < 10.0f)
						{
							m_GotoTeleOffset++;
							continue;
						}
					}
					m_GotoTeleLastPos = MatchPos;
					if(Match == m_GotoTeleOffset)
						return;
				}
			}
		}
	};
	FindTile();

	if(MatchPos == ivec2(-1, -1))
		return;
	if(Match < m_GotoTeleOffset)
		m_GotoTeleOffset = -1;
	SetView(MatchPos);
	m_GotoTeleOffset++;
}

void CCamera::SetZoom(float Target, int Smoothness)
{
	ChangeZoom(Target, Smoothness);
}
