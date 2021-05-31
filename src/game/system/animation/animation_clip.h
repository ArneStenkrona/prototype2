#ifndef ANIMATION_CLIP_H
#define ANIMATION_CLIP_H

class AnimationClip {
public:
    AnimationClip();
    void setClip(const char * clipName);
    void resetClip();

    void update(float deltaTime);

    float isCompleted() const { return m_completed; }

    float m_time = 0.0f;
    float m_playBackSpeed = 1.0f;
    bool m_paused = false;
    bool m_loop = false;
private:
    char m_clipName[64] = {0};
    bool m_completed = false;

    friend class Model;
};

#endif