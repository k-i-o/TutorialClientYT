#include <base/vmath.h>
#include <game/client/component.h>
#include <engine/graphics.h>
#include <game/generated/protocol.h>

#pragma once

#define IDLE_TIMER 1.f;
#define ANGRY_TIMER .3f;
//#define CLOSE_EYES_TIMER .05f;

typedef struct MovementRecord{
    std::vector<CNetObj_PlayerInput> recordsActions;
    std::vector<vec2> recordsMouse;
    std::vector<vec2> recordsPositions;
}MovementRecord;

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

    void RenderTrail(float rSize, int color, vec2 pos);
    void Trail(vec2 pos, float timePassed, ColorRGBA c);
    void MagicParticles(float radius);
    void MagicParticles2(float radius, vec2 pos);

    std::vector<const char*> GetBinaryFilesInFolder(const std::string& folderPath);
    void SaveRecordsToFile(const std::string& filename);
    void LoadRecordsFromFile(const std::string& filename);
    void SaveRecords();
    void DeleteRecord(const std::string& filename);
    void RenderPath();
    void RenderStart();
    void Record();
    void Play();

    MovementRecord records;

    virtual int Sizeof() const override { return sizeof(*this); }
    virtual void OnRender() override;

private:
    enum PetState{
        IDLE,
        FOLLOW,
        ANGRY,
        //CLOSE_EYES
    };

    float timerValue = 0.2f;
    float timer = timerValue;

    vec2 PetPos;
    vec2 PetPosOld;
    vec2 PetVel;
    float speed = 15000;
    float acceleration = 100;
    PetState PetState = FOLLOW;

    float maxDistance = 250;
    float minDistance = 70;

    float StateTimer = IDLE_TIMER;
    vec2 target;

    float timerIdle = 0;

    bool petMoving = false;
};