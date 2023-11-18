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
            if(distance(m_pClient->m_LocalCharacterPos, PetPos) <= 70) {
                StateTimer = IDLE_TIMER;
                SetIdle();
            } else {
                target = PlayerPathNormalization(m_pClient->m_LocalCharacterPos);
            }
            break;

        default:
            break;
    
    }

    if(distance(m_pClient->m_LocalCharacterPos, PetPos) >= 250 || (PetPos.x < 0 || PetPos.y < 0)) 
        SetFollow();
  
    //if(random_float(0,1)) SetCloseEyes();  

}

void CYoutube::OnRender(){
    if(g_Config.m_ClYoutubePet){
        Pet();

        if(g_Config.m_ClYoutubePetPositionLine)
            RenderPetPositionLine();
    }

}

void CYoutube::Pet(){
    
	if(m_pClient->m_Snap.m_LocalClientID == -1 || !m_pClient->m_Snap.m_aCharacters[m_pClient->m_Snap.m_LocalClientID].m_Active)
		return; //no action if player not in game

    if (distance(m_pClient->m_LocalCharacterPos, PetPos) > 1000) speed = 15000;

    GoTo(target);

    RenderPet();

}

void CYoutube::RenderPet(){
    if(m_pClient->m_Snap.m_LocalClientID == -1 || !m_pClient->m_Snap.m_aCharacters[m_pClient->m_Snap.m_LocalClientID].m_Active)
		return; //no action if player not in game
        
    Graphics()->TextureClear();
    RenderTools()->MapScreenToInterface(m_pClient->m_Camera.m_Center.x, m_pClient->m_Camera.m_Center.y);

    if(g_Config.m_ClRenderPetLikeTee){
        CTeeRenderInfo pInfo = m_pClient->m_aClients[m_pClient->m_Snap.m_LocalClientID].m_RenderInfo;

        RenderTools()->RenderTee(CAnimState::GetIdle(), &pInfo, (PetState == ANGRY ? EMOTE_ANGRY : EMOTE_NORMAL), vec2(1, 0.4f), PetPos, 1);
    }else{
        Graphics()->TextureSet(g_pData->m_aImages[IMAGE_PET].m_Id);//(GameClient()->m_GameSkin.m_SpriteWeaponLaser);
    //     // RenderTools()->SelectSprite(SPRITE_WEAPON_LASER_BODY);
        Graphics()->QuadsBegin();
        Graphics()->SetColor(1,1,1,1);//(.9, .3, 1, 1);
        RenderTools()->DrawSprite(PetPos.x, PetPos.y, 30.f);
        Graphics()->QuadsEnd();
    }

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
