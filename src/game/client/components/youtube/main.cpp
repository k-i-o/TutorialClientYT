#include <game/client/gameclient.h>
#include <base/vmath.h>
#include <game/client/animstate.h>
#include <game/client/render.h>
#include <game/generated/client_data.h>
#include <game/generated/protocol.h>

#include <base/system.h>
#include <engine/graphics.h>
#include <engine/shared/config.h>

#include "base/math.h"
#include "main.h"

vec2 CYoutube::normalize(vec2 v){
    float length = sqrt(v.x * v.x + v.y * v.y);
    return vec2(v.x / length, v.y / length);
}

float CYoutube::lerp(float a, float b, float t){
    return a + (b - a) * t;
}

vec2 CYoutube::PlayerPathNormalization(vec2 player){
    vec2 d = vec2(player.x - PetPos.x, player.y - PetPos.y);
    return vec2(normalize(d).x, normalize(d).y);
}

void CYoutube::GoTo(vec2 path){
    float distanceFactor = 1.0f - (distance(m_pClient->m_LocalCharacterPos, PetPos) / maxDistance);
    float distanceAcceleration = acceleration * distanceFactor;

    speed += distanceAcceleration * Client()->RenderFrameTime();

    PetPos.x += path.x * speed * Client()->RenderFrameTime();
    PetPos.y += path.y * speed * Client()->RenderFrameTime();
}

void CYoutube::SetIdle(){
    PetState = IDLE;
    timerIdle += Client()->RenderFrameTime() * 50;
    vec2 newTarget = PlayerPathNormalization(vec2(PetPos.x, PetPos.y + sin(timerIdle))); //oscillation
    target.x = lerp(target.x, newTarget.x, 0.1f); // Smoothly interpolate the new target
    target.y = lerp(target.y, newTarget.y, 0.1f); // Smoothly interpolate the new target
    speed = 15 + sin(timerIdle);
}

void CYoutube::SetFollow(){
    PetState = FOLLOW;
    target = PlayerPathNormalization(m_pClient->m_LocalCharacterPos);
    speed = 400;
}

void CYoutube::SetAngry(){
    PetState = ANGRY;
    StateTimer = ANGRY_TIMER;
    speed = 0;
}

/*void CYoutube::SetCloseEyes(){
    PetState = CLOSE_EYES;
    StateTimer = CLOSE_EYES_TIMER;
    speed = 0;
}*/

void CYoutube::PetStateUpdate(){

    switch (PetState) {

        case IDLE:
            StateTimer -= Client()->RenderFrameTime();
            if(StateTimer <= 0 && g_Config.m_ClRenderPetLikeTee) {
                SetAngry();
            } else{
                SetIdle();
            }
        
            break;
        case ANGRY:
            StateTimer -= Client()->RenderFrameTime();
            if(StateTimer <= 0) {
                StateTimer = IDLE_TIMER;
                SetIdle();
            }
            break;        
        /*case CLOSE_EYES:
            StateTimer -= Client()->RenderFrameTime();
            if(StateTimer <= 0) {
                StateTimer = IDLE_TIMER;
                SetIdle();
            }
            break;*/
        case FOLLOW:
            if(distance(m_pClient->m_LocalCharacterPos, PetPos) <= minDistance) {
                StateTimer = IDLE_TIMER;
                SetIdle();
            } else {
                target = PlayerPathNormalization(m_pClient->m_LocalCharacterPos);
            }
            break;

        default:
            break;
    
    }

    if(distance(m_pClient->m_LocalCharacterPos, PetPos) >= maxDistance || (PetPos.x < 0 || PetPos.y < 0)) 
        SetFollow();
  
    //if(random_float(0,1)) SetCloseEyes();  

}        

void CYoutube::OnRender(){
    if(g_Config.m_ClYoutubePet){
        Pet();

        if(g_Config.m_ClYoutubePetPositionLine)
            RenderPetPositionLine();

        //pet trail
        if(g_Config.m_ClYoutubePetTrail){
            RenderTrail(g_Config.m_ClYoutubePetTrailRadius, g_Config.m_ClYoutubePetTrailColor, PetPos);
        }

    }

    //tee trail
    if(g_Config.m_ClYoutubeTeeTrail && m_pClient->m_PredictedChar.m_Vel != vec2(0,0)){
        RenderTrail(g_Config.m_ClYoutubeTeeTrailRadius, g_Config.m_ClYoutubeTeeTrailColor, m_pClient->m_LocalCharacterPos);
    }

    if(g_Config.m_ClYoutubeMagicParticles) {
        MagicParticles(50.f);
    }

    if(g_Config.m_ClYoutubeMagicParticles2) {
        MagicParticles2(80.f);
    }

}

void CYoutube::Pet(){
    
	if(m_pClient->m_Snap.m_LocalClientID == -1 || !m_pClient->m_Snap.m_aCharacters[m_pClient->m_Snap.m_LocalClientID].m_Active)
		return; //no action if player not in game

    if (distance(m_pClient->m_LocalCharacterPos, PetPos) > 1000) speed = 15000;

    GoTo(target);

    RenderPet();

}

void CYoutube::RenderPet()
{
    if(m_pClient->m_Snap.m_LocalClientID == -1 || !m_pClient->m_Snap.m_aCharacters[m_pClient->m_Snap.m_LocalClientID].m_Active)
		return; //no action if player not in game
        
    Graphics()->TextureClear();
    RenderTools()->MapScreenToInterface(m_pClient->m_Camera.m_Center.x, m_pClient->m_Camera.m_Center.y);

    if(g_Config.m_ClRenderPetLikeTee)
    {
        CTeeRenderInfo pInfo = m_pClient->m_aClients[m_pClient->m_Snap.m_LocalClientID].m_RenderInfo;

        RenderTools()->RenderTee(CAnimState::GetIdle(), &pInfo, (PetState == ANGRY ? EMOTE_ANGRY : EMOTE_NORMAL), vec2(1, 0.4f), PetPos, 1);
    }
    else
    {
        int petWidth = 35;
        int petHeight = 45;

        Graphics()->TextureSet(g_pData->m_aImages[IMAGE_PET].m_Id);
        Graphics()->QuadsBegin();
        Graphics()->SetColor(1,1,1,1);
	    IGraphics::CQuadItem QuadItem = IGraphics::CQuadItem(PetPos.x-petWidth/2, PetPos.y-petHeight/2, petWidth, petHeight);
	    Graphics()->QuadsDrawTL(&QuadItem, 1);
        Graphics()->QuadsEnd();
    }

    PetVel = vec2((PetPos.x - PetPosOld.x)/Client()->RenderFrameTime(), (PetPos.y - PetPosOld.y)/Client()->RenderFrameTime());

    PetPosOld = PetPos;
}

void CYoutube::RenderPetPositionLine(){

    Graphics()->TextureClear();
    RenderTools()->MapScreenToInterface(m_pClient->m_Camera.m_Center.x, m_pClient->m_Camera.m_Center.y);

    Graphics()->LinesBegin();
    Graphics()->SetColor(.5, .7, 0, 1);

    IGraphics::CLineItem Line(PetPos.x, PetPos.y, m_pClient->m_LocalCharacterPos.x, m_pClient->m_LocalCharacterPos.y);
    Graphics()->LinesDraw(&Line, 1);
    Graphics()->LinesEnd();
}

void CYoutube::RenderTrail(float rSize, int color, vec2 pos) {
    ColorRGBA c = color_cast<ColorRGBA>(ColorHSLA(color));

    int r = random_float(-rSize/10, rSize/10);

    Trail({ pos.x + r, pos.y + r }, Client()->RenderFrameTime(), c);
}

void CYoutube::Trail(vec2 pos, float timePassed, ColorRGBA c) {
    if(timePassed < 0.001f) return;

    for (int i = 0; i < 5; i++) {
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

void CYoutube::MagicParticles(float radius) {
    for(int i = 0; i < 10; i++) {
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

void CYoutube::MagicParticles2(float radius) {
    timer -= Client()->RenderFrameTime();
    if(timer <= 0) {
        timer = timerValue;
        for(int i = 0; i < 30; i++) {

            CParticle p;
            p.SetDefault();
            p.m_Spr = SPRITE_PART_BALL;

            float angle = random_float(0, 2 * pi);
            float dist = random_float(0, radius);
            vec2 offset = vec2(cos(angle) * dist, sin(angle) * dist);

            p.m_Pos = m_pClient->m_LocalCharacterPos + offset;

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