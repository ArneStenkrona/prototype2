#include "scene_serialization.h"

#include "src/game/scene/scene.h"

#include "src/container/vector.h"

#include <stdio.h>
#include <stdlib.h>

void SceneSerialization::loadScene(char const * file, Scene & scene) {
    // set components to undefined
    for (size_t i = 0; i < scene.m_entities.maxSize; ++i) {
        sprintf(scene.m_entities.names[i], "entity_%lu", i);
        scene.m_entities.modelIDs[i] = -1;
        scene.m_entities.characterIDs[i] = -1;
        scene.m_entities.colliderTags[i].type = ColliderType::COLLIDER_TYPE_NONE;
    }

    // load file into buffer
    prt::vector<char> buf;
    FILE *fp = fopen(file, "r");
    if (fp != NULL) {
        // go to the end of the file
        if (fseek(fp, 0L, SEEK_END) == 0) {
            // get the size of the file
            long bufsize = ftell(fp);
            if (bufsize == -1) { assert(false && "Failed to get file size!"); }

            // allocate our buffer to that size
            buf.resize(bufsize + 1);

            // go back to the start of the file
            if (fseek(fp, 0L, SEEK_SET) != 0) { assert(false && "Failed to return to beginning of file!"); }

            // read the entire file into memory
            size_t newLen = fread(buf.data(), sizeof(char), bufsize, fp);
            if (ferror( fp ) != 0 ) {
                fputs("Error reading file", stderr);
                assert(false && "Failed to read file!");
            } else {
                buf[newLen++] = '\0'; // null-terminate
            }
        }
        fclose(fp);
    }
    // parse
    char const * data = buf.data();
    TokenType type;
    while (data < buf.end()) {
        type = readToken(data);
        switch (type) {
            case STATIC_SOLID_ENTITY :
                parseStaticSolidEntity(data, scene);
                break;
            case SUN :
                parseSun(data, scene);
                break;
            case POINT_LIGHT :
            parsePointLight(data, scene);
                break;
            case BOX_LIGHT :
            parseBoxLight(data, scene);
                break;
            case CHARACTER :
                parseCharacter(data, scene);
                break;
            case ERROR :
                assert(false && "Failed to parse file!");
                break;        
            }
        // read rest of line
        while (*data != '\n') {
            ++data;
        }
        ++data;
    }

}

SceneSerialization::TokenType SceneSerialization::readToken(char const *& buf) {
    TokenType type = TokenType::ERROR;
    char tokenStr[64] = {0};
    int i = 0;
    while (*buf != '[' && *buf != '\0') {
        tokenStr[i] = *buf;
        ++i;
        ++buf;
    }

    // TODO: replace with hash-table for performance
    if (strcmp(tokenStr, "StaticSolidEntity") == 0) {
        type = TokenType::STATIC_SOLID_ENTITY;
    } else if (strcmp(tokenStr, "Sun") == 0) {
        type = TokenType::SUN;
    } else if (strcmp(tokenStr, "PointLight") == 0) {
        type = TokenType::POINT_LIGHT;
    } else if (strcmp(tokenStr, "BoxLight") == 0) {
        type = TokenType::BOX_LIGHT;
    } else if (strcmp(tokenStr, "Character") == 0) {
        type = TokenType::CHARACTER;
    }

    return type;
}

void SceneSerialization::parseStaticSolidEntity(char const *& buf, Scene & scene) {
    EntityID id = scene.m_entities.addEntity();

    char modelPath[128] = {0};
    parseString(buf, modelPath);
    char const * modelPathBuf = modelPath;
    ModelID modelID = scene.m_assetManager.getModelManager().loadModel(modelPathBuf, false);

    scene.m_entities.modelIDs[id] = modelID;
    scene.m_entities.transforms[id].position = parseVec3(buf);
    scene.m_entities.transforms[id].rotation = parseQuat(buf);
    scene.m_entities.transforms[id].scale = parseVec3(buf);

    Model const * models;
    size_t nModels;
    scene.m_assetManager.getModelManager().getModels(models, nModels);

    scene.m_entities.colliderTags[id] = scene.m_physicsSystem.addModelCollider(models[modelID], 
                                                                               scene.m_entities.transforms[id]);
}

void SceneSerialization::parseSun(char const *& buf, Scene & scene) {
    scene.m_lights.sun.direction = glm::normalize(parseVec3(buf));
    scene.m_lights.sun.color = parseVec3(buf);

}

void SceneSerialization::parsePointLight(char const *& buf, Scene & scene) {
    scene.m_lights.pointLights.push_back({});
    PointLight & light = scene.m_lights.pointLights.back();
    light.pos = parseVec3(buf);
    light.color = parseVec3(buf);
    light.a = parseFloat(buf);
    light.b = parseFloat(buf);
    light.c = parseFloat(buf);
}

void SceneSerialization::parseBoxLight(char const *& buf, Scene & scene) {
    scene.m_lights.boxLights.push_back({});
    BoxLight & light = scene.m_lights.boxLights.back();
    light.min = parseVec3(buf);
    light.max = parseVec3(buf);
    light.color = parseVec3(buf);
    light.position = parseVec3(buf);
    light.rotation = parseQuat(buf);
    light.scale = parseVec3(buf);
}

void SceneSerialization::parseCharacter(char const *& buf, Scene & scene) {
    EntityID id = scene.m_entities.addEntity();

    char modelPath[128] = {0};
    parseString(buf, modelPath);
    char const * modelPathBuf = modelPath;
    ModelID modelID = scene.m_assetManager.getModelManager().loadModel(modelPathBuf, true);

    scene.m_entities.animationIDs[id] = scene.m_animationSystem.addAnimation(id);

    scene.m_entities.modelIDs[id] = modelID;

    scene.m_entities.transforms[id].position = parseVec3(buf);
    scene.m_entities.transforms[id].rotation = parseQuat(buf);
    scene.m_entities.transforms[id].scale = parseVec3(buf);

    scene.m_entities.colliderTags[id] = scene.m_physicsSystem.addEllipsoidCollider(parseVec3(buf));

    CharacterAnimationClips clips;
    clips.idle = scene.m_assetManager.getModelManager().getAnimationIndex(modelID, "idle");
    clips.walk = scene.m_assetManager.getModelManager().getAnimationIndex(modelID, "walk");
    clips.run = scene.m_assetManager.getModelManager().getAnimationIndex(modelID, "run");
    clips.jump = scene.m_assetManager.getModelManager().getAnimationIndex(modelID, "jump");
    clips.fall = scene.m_assetManager.getModelManager().getAnimationIndex(modelID, "fall");

    CharacterID characterID = scene.m_characterSystem.addCharacter(id, scene.m_entities.colliderTags[id], clips);

    scene.m_entities.characterIDs[id] = characterID;
}

void SceneSerialization::parseString(char const *& buf, char * dest) {
    while (*buf != '"') {
        ++buf;
    }
    ++buf;

    int i = 0;
    while (*buf != '"') {
        dest[i] = *buf;
        ++i;
        ++buf;
    }
}

float SceneSerialization::parseFloat(char const *& buf) {
    float f;
    while (*buf != '<') {
        ++buf;
    }
    ++buf;

    int ret = sscanf(buf, "%f",
                     &f);

    if (ret != 1) {
        assert(false && "Failed to parse float");
    }
    return f;
}

glm::vec3 SceneSerialization::parseVec3(char const *& buf) {
    glm::vec3 vec;
    while (*buf != '<') {
        ++buf;
    }
    ++buf;

    int ret = sscanf(buf, "%f,%f,%f",
                     &vec.x, &vec.y, &vec.z);

    if (ret != 3) {
        assert(false && "Failed to parse vec3");
    }
    return vec;
}

glm::quat SceneSerialization::parseQuat(char const *& buf) {
    glm::quat quat;
    while (*buf != '<') {
        ++buf;
    }
    ++buf;

    int ret = sscanf(buf, "%f,%f,%f,%f",
                     &quat.w, &quat.x, &quat.y, &quat.z);
    if (ret != 4) {
        assert(false && "Failed to parse quat");
    }
    return glm::normalize(quat);
}
