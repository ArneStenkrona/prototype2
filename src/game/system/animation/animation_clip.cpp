#include "animation_clip.h"

#include <string.h>

AnimationClip::AnimationClip() {
}

void AnimationClip::setClip(const char * clipName) {
    strcpy(m_clipName, clipName);
}

void AnimationClip::resetClip() {
    m_completed = false;
    m_time = 0.0f;
}

void AnimationClip::update(float deltaTime) {
    if (!m_paused) {
        m_time += m_playBackSpeed * deltaTime;
    }
}