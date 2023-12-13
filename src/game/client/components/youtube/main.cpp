#include <base/vmath.h>
#include <game/client/animstate.h>
#include <game/client/gameclient.h>
#include <game/client/render.h>
#include <game/generated/client_data.h>
#include <game/generated/protocol.h>

#include <base/system.h>
#include <engine/graphics.h>
#include <engine/shared/config.h>

#include "base/math.h"
#include "main.h"
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sys/stat.h>

namespace fs = std::filesystem;

vec2 CYoutube::normalize(vec2 v)
{
	float length = sqrt(v.x * v.x + v.y * v.y);
	return vec2(v.x / length, v.y / length);
}

float CYoutube::lerp(float a, float b, float t)
{
	return a + (b - a) * t;
}

vec2 CYoutube::PlayerPathNormalization(vec2 player)
{
	vec2 d = vec2(player.x - PetPos.x, player.y - PetPos.y);
	return vec2(normalize(d).x, normalize(d).y);
}

void CYoutube::GoTo(vec2 path)
{
	float distanceFactor = 1.0f - (distance(m_PetTarget, PetPos) / maxDistance);
	float distanceAcceleration = acceleration * distanceFactor;

	speed += distanceAcceleration * Client()->RenderFrameTime();

	PetPos.x += path.x * speed * Client()->RenderFrameTime();
	PetPos.y += path.y * speed * Client()->RenderFrameTime();
}

void CYoutube::SetIdle()
{
	PetState = IDLE;
	timerIdle += Client()->RenderFrameTime() * 50;
	vec2 newTarget = PlayerPathNormalization(vec2(PetPos.x, PetPos.y + sin(timerIdle))); // oscillation
	target.x = lerp(target.x, newTarget.x, 0.1f); // Smoothly interpolate the new target
	target.y = lerp(target.y, newTarget.y, 0.1f); // Smoothly interpolate the new target
	speed = 15 + sin(timerIdle);
}

void CYoutube::SetFollow()
{
	PetState = FOLLOW;
	target = PlayerPathNormalization(m_PetTarget);
	speed = 400;
}

void CYoutube::SetAngry()
{
	PetState = ANGRY;
	StateTimer = ANGRY_TIMER;
	speed = 0;
}

/*void CYoutube::SetCloseEyes(){
    PetState = CLOSE_EYES;
    StateTimer = CLOSE_EYES_TIMER;
    speed = 0;
}*/

void CYoutube::PetStateUpdate()
{
	switch(PetState)
	{
	case IDLE:
		StateTimer -= Client()->RenderFrameTime();
		if(StateTimer <= 0 && g_Config.m_ClRenderPetLikeTee)
		{
			SetAngry();
		}
		else
		{
			SetIdle();
		}
		petMoving = false;
		break;
	case ANGRY:
		StateTimer -= Client()->RenderFrameTime();
		if(StateTimer <= 0)
		{
			StateTimer = IDLE_TIMER;
			SetIdle();
		}
		petMoving = false;
		break;
	/*case CLOSE_EYES:
	    StateTimer -= Client()->RenderFrameTime();
	    if(StateTimer <= 0) {
		StateTimer = IDLE_TIMER;
		SetIdle();
	    }
	    break;*/
	case FOLLOW:
		if(distance(m_PetTarget, PetPos) <= minDistance)
		{
			StateTimer = IDLE_TIMER;
			SetIdle();
		}
		else
		{
			target = PlayerPathNormalization(m_PetTarget);
		}
		petMoving = true;
		break;

	default:
		break;
	}

	if(distance(m_PetTarget, PetPos) >= maxDistance || (PetPos.x < 0 || PetPos.y < 0))
		SetFollow();

	// if(random_float(0,1)) SetCloseEyes();
}

void CYoutube::OnRender()
{

	if(g_Config.m_ClYoutubePet)
	{

        m_PetTarget = m_pClient->m_aClients[m_pClient->m_aLocalIDs[g_Config.m_ClYoutubePetTarget]].m_Predicted.m_Pos;

		Pet();

		if(g_Config.m_ClYoutubePetPositionLine)
		{
			RenderPetPositionLine();
		}

		if(g_Config.m_ClYoutubePetTrail && petMoving)
		{
			RenderTrail(g_Config.m_ClYoutubePetTrailRadius, g_Config.m_ClYoutubePetTrailColor, PetPos);
		}
	}

	// tee trail
	if(g_Config.m_ClYoutubeTeeTrail && m_pClient->m_PredictedChar.m_Vel != vec2(0, 0))
	{
		RenderTrail(g_Config.m_ClYoutubeTeeTrailRadius, g_Config.m_ClYoutubeTeeTrailColor, m_pClient->m_LocalCharacterPos);
	}

	if(g_Config.m_ClYoutubeMagicParticles)
	{
		MagicParticles(50.f);
	}

	if(g_Config.m_ClYoutubeMagicParticles2)
	{
		MagicParticles2(80.f, m_pClient->m_LocalCharacterPos);
	}

	if(records.recordsPositions.size() > 0)
	{
		if(g_Config.m_ClPlaying)
		{
			RenderPath();
		}
		else if(!g_Config.m_ClPlaying && !g_Config.m_ClRecording)
		{
			RenderStart();
		}
	}
}

void CYoutube::RenderStart()
{
	Graphics()->TextureClear();
	RenderTools()->MapScreenToInterface(m_pClient->m_Camera.m_Center.x, m_pClient->m_Camera.m_Center.y, m_pClient->m_Camera.m_Zoom);

	CTeeRenderInfo pInfo = m_pClient->m_aClients[m_pClient->m_Snap.m_LocalClientID].m_RenderInfo;

	RenderTools()->RenderTee(CAnimState::GetIdle(), &pInfo, EMOTE_SURPRISE, vec2(1, 0.4f), records.recordsPositions[0], .2f);
}

void CYoutube::RenderPath()
{
	Graphics()->TextureClear();
	RenderTools()->MapScreenToInterface(m_pClient->m_Camera.m_Center.x, m_pClient->m_Camera.m_Center.y, m_pClient->m_Camera.m_Zoom);

	for(int i = 0; i < (int)records.recordsPositions.size() - 1; i++)
	{
		Graphics()->LinesBegin();
		Graphics()->SetColor(0.18, 0.56, 0.9, .3f);
		IGraphics::CLineItem Line(records.recordsPositions[i].x, records.recordsPositions[i].y, records.recordsPositions[i + 1].x, records.recordsPositions[i + 1].y);
		Graphics()->LinesDraw(&Line, 1);
		Graphics()->LinesEnd();

		if(i % 10 == 0)
		{
			Graphics()->QuadsBegin();
			Graphics()->SetColor(0.22, 0.10, 0.9, 1);
			Graphics()->DrawCircle(records.recordsPositions[i].x, records.recordsPositions[i].y, 3.0f, 64);
			Graphics()->QuadsEnd();
		}
	}
}

void CYoutube::Pet()
{
	if(m_pClient->m_aLocalIDs[g_Config.m_ClYoutubePetTarget] == -1 || !m_pClient->m_Snap.m_aCharacters[m_pClient->m_aLocalIDs[g_Config.m_ClYoutubePetTarget]].m_Active)
		return; // no action if player not in game

	if(distance(m_PetTarget, PetPos) > 1000)
		speed = 15000;

	GoTo(target);

	RenderPet();
}

void CYoutube::RenderPet()
{
	if(m_pClient->m_aLocalIDs[g_Config.m_ClYoutubePetTarget] == -1 || !m_pClient->m_Snap.m_aCharacters[m_pClient->m_aLocalIDs[g_Config.m_ClYoutubePetTarget]].m_Active)
		return; // no action if player not in game

	Graphics()->TextureClear();
	RenderTools()->MapScreenToInterface(m_pClient->m_Camera.m_Center.x, m_pClient->m_Camera.m_Center.y, m_pClient->m_Camera.m_Zoom);

	if(g_Config.m_ClRenderPetLikeTee)
	{
		CTeeRenderInfo pInfo = m_pClient->m_aClients[m_pClient->m_aLocalIDs[g_Config.m_ClYoutubePetTarget]].m_RenderInfo;

		RenderTools()->RenderTee(CAnimState::GetIdle(), &pInfo, (PetState == ANGRY ? EMOTE_ANGRY : EMOTE_NORMAL), vec2(1, 0.4f), PetPos, 1);
	}
	else
	{
		int petWidth = 35;
		int petHeight = 45;

		Graphics()->TextureSet(g_pData->m_aImages[IMAGE_PET].m_Id);
		Graphics()->QuadsBegin();
		Graphics()->SetColor(1, 1, 1, 1);
		IGraphics::CQuadItem QuadItem = IGraphics::CQuadItem(PetPos.x - petWidth / 2, PetPos.y - petHeight / 2, petWidth, petHeight);
		Graphics()->QuadsDrawTL(&QuadItem, 1);
		Graphics()->QuadsEnd();
	}

	PetVel = vec2((PetPos.x - PetPosOld.x) / Client()->RenderFrameTime(), (PetPos.y - PetPosOld.y) / Client()->RenderFrameTime());

	PetPosOld = PetPos;
}

void CYoutube::RenderPetPositionLine()
{
	Graphics()->TextureClear();
	RenderTools()->MapScreenToInterface(m_pClient->m_Camera.m_Center.x, m_pClient->m_Camera.m_Center.y, m_pClient->m_Camera.m_Zoom);

	Graphics()->LinesBegin();
	Graphics()->SetColor(.5, .7, 0, 1);

	IGraphics::CLineItem Line(PetPos.x, PetPos.y, m_PetTarget.x, m_PetTarget.y);
	Graphics()->LinesDraw(&Line, 1);
	Graphics()->LinesEnd();
}

void CYoutube::RenderTrail(float rSize, int color, vec2 pos)
{
	ColorRGBA c = color_cast<ColorRGBA>(ColorHSLA(color));

	int r = random_float(-rSize / 10, rSize / 10);

	Trail({pos.x + r, pos.y + r}, Client()->RenderFrameTime(), c);
}

void CYoutube::Trail(vec2 pos, float timePassed, ColorRGBA c)
{
	if(timePassed < 0.001f)
		return;

	for(int i = 0; i < 5; i++)
	{
		CParticle p;
		p.SetDefault();
		p.m_Spr = SPRITE_PART_BALL;
		p.m_Pos = pos;
		p.m_LifeSpan = random_float(0.25f, 0.5f);
		p.m_StartSize = 8.0f;
		p.m_EndSize = 0;
		p.m_Friction = 0.7f;
		p.m_Color = c;
		p.m_StartAlpha = c.a;
		m_pClient->m_Particles.Add(CParticles::GROUP_PROJECTILE_TRAIL, &p, timePassed);
	}
}

void CYoutube::MagicParticles(float radius)
{
	for(int i = 0; i < 10; i++)
	{
		CParticle p;
		p.SetDefault();
		p.m_Spr = SPRITE_PART_BALL;

		float angle = random_float(0, 2 * pi);
		float dist = random_float(0, radius);
		vec2 offset = vec2(cos(angle) * dist, sin(angle) * dist);

		p.m_Pos = m_pClient->m_LocalCharacterPos + offset;

		p.m_LifeSpan = random_float(0.25f, 0.5f);
		p.m_StartSize = 8.0f;
		p.m_EndSize = 0;
		p.m_Friction = 0.7f;
		p.m_Color = ColorRGBA(random_float(0, 1.0f), random_float(0, 1.0f), random_float(0, 1.0f), random_float(0.5f, 1.0f));
		p.m_StartAlpha = p.m_Color.a;
		m_pClient->m_Particles.Add(CParticles::GROUP_PROJECTILE_TRAIL, &p, Client()->RenderFrameTime());
	}
}

void CYoutube::MagicParticles2(float radius, vec2 pos)
{
	timer -= Client()->RenderFrameTime();
	if(timer <= 0)
	{
		timer = timerValue;
		for(int i = 0; i < 30; i++)
		{
			CParticle p;
			p.SetDefault();
			p.m_Spr = SPRITE_PART_BALL;

			float angle = random_float(0, 2 * pi);
			float dist = random_float(0, radius);
			vec2 offset = vec2(cos(angle) * dist, sin(angle) * dist);

			p.m_Pos = pos + offset;

			p.m_LifeSpan = random_float(0.5f, 1.5f);
			p.m_StartSize = 8.0f;
			p.m_EndSize = 0;
			p.m_Friction = 0.7f;
			p.m_Color = ColorRGBA(random_float(0, 1.0f), random_float(0, 1.0f), random_float(0, 1.0f), random_float(0.2f, 1.0f));
			p.m_StartAlpha = p.m_Color.a;
			m_pClient->m_Particles.Add(CParticles::GROUP_PROJECTILE_TRAIL, &p, Client()->RenderFrameTime());
		}
	}
}

std::vector<const char *> CYoutube::GetBinaryFilesInFolder(const std::string &folderPath)
{
	std::vector<const char *> fileNames;

	for(const auto &entry : fs::directory_iterator(folderPath))
	{
		if(entry.is_regular_file() && entry.path().extension() == ".bin")
		{
			std::string fileName = entry.path().string();
			const char *cString = fileName.c_str();

			fileNames.push_back(strdup(cString));
		}
	}

	return fileNames;
}

template<typename T>
void serializeVector(std::ostream &os, const std::vector<T> &vec)
{
	size_t size = vec.size();
	os.write(reinterpret_cast<const char *>(&size), sizeof(size));
	os.write(reinterpret_cast<const char *>(vec.data()), sizeof(T) * size);
}

template<typename T>
void deserializeVector(std::istream &is, std::vector<T> &vec)
{
	size_t size;
	is.read(reinterpret_cast<char *>(&size), sizeof(size));

	vec.resize(size);
	is.read(reinterpret_cast<char *>(vec.data()), sizeof(T) * size);
}

void CYoutube::SaveRecordsToFile(const std::string &filename)
{
	std::ofstream ofs(filename, std::ios::binary);

	if(ofs.is_open())
	{
		serializeVector(ofs, records.recordsActions);
		serializeVector(ofs, records.recordsMouse);
		serializeVector(ofs, records.recordsPositions);

		ofs.close();
		dbg_msg("YOUTUBE", "Data saved to: %s", filename.c_str());
	}
	else
	{
		dbg_msg("YOUTUBE", "Error opening the file: %s", filename.c_str());
	}
}

void CYoutube::LoadRecordsFromFile(const std::string &filename)
{
	std::ifstream ifs(filename, std::ios::binary);

	if(ifs.is_open())
	{
		deserializeVector(ifs, records.recordsActions);
		deserializeVector(ifs, records.recordsMouse);
		deserializeVector(ifs, records.recordsPositions);

		ifs.close();
	}
	else
	{
		dbg_msg("YOUTUBE", "Error opening the file: %s", filename.c_str());
	}
}

void CYoutube::SaveRecords()
{
	const std::string mapName = Client()->GetCurrentMap();

	SaveRecordsToFile(std::string(g_Config.m_ClMapRecordsPath) + "\\" + mapName + ".bin");
}

void CYoutube::DeleteRecord(const std::string &filename)
{
	if(std::remove(filename.c_str()) != 0)
	{
		dbg_msg("YOUTUBE", "Error deleting the file: %s", filename.c_str());
	}
	else
	{
		dbg_msg("YOUTUBE", "File successfully deleted: %s", filename.c_str());
	}
}

void CYoutube::Record()
{
	records.recordsActions.push_back(m_pClient->m_Controls.m_aInputData[g_Config.m_ClDummy]);
	records.recordsMouse.push_back(m_pClient->m_Controls.m_aMousePos[g_Config.m_ClDummy]);
	records.recordsPositions.push_back(m_pClient->m_LocalCharacterPos);
}

void CYoutube::Play()
{
	m_pClient->m_Controls.m_aInputData[g_Config.m_ClDummy] = records.recordsActions[0];
	m_pClient->m_Controls.m_aMousePos[g_Config.m_ClDummy] = records.recordsMouse[0];

	records.recordsActions.erase(records.recordsActions.begin());
	records.recordsMouse.erase(records.recordsMouse.begin());
}
