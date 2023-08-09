#include <base/vmath.h>
#include <game/client/component.h>

#pragma once

#define IDLE_TIMER 1.f;
#define ANGRY_TIMER .3f;
//#define CLOSE_EYES_TIMER .05f;

class CYoutube : public CComponent{
public:
    vec2 normalize(vec2 v);
    float lerp(float a, float b, float t);

    void SetIdle();
    void SetFollow();
    void SetAngry();
    void SetCloseEyes();

    void RenderPetPositionLine();
    vec2 PlayerPathNormalization(vec2 player);
    void Pet();
    void RenderPet();
    void GoTo(vec2 path);
    void PetStateUpdate();

    virtual int Sizeof() const override { return sizeof(*this); }
    virtual void OnRender() override;

private:
    enum PetState{
        IDLE,
        FOLLOW,
        ANGRY,
        //CLOSE_EYES
    };

    vec2 PetPos;
    float speed = 15000;
    PetState PetState = FOLLOW;

    float StateTimer = IDLE_TIMER;
    vec2 target;

    float timerIdle = 0;
};