#include "scene_serialization.h"

#include "src/game/scene/scene.h"

#include "src/container/vector.h"

#include <stdio.h>
#include <stdlib.h>

void SceneSerialization::loadScene(char const * file, Scene & scene) {
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
            // case POINT_LIGHT :
            // parsePointLight(data, scene);
            //     break;
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
    } /*else if (strcmp(tokenStr, "PointLight") == 0) {
        type = TokenType::POINT_LIGHT
    } */else if (strcmp(tokenStr, "Character") == 0) {
        type = TokenType::CHARACTER;
    }

    return type;
}

void SceneSerialization::parseStaticSolidEntity(char const *& buf, Scene & scene) {
    size_t index = scene.m_staticSolidEntities.size;
    ++scene.m_staticSolidEntities.size;

    uint32_t modelID; 
    char modelPath[128] = {0};
    parseString(buf, modelPath);
    char const * modelPathBuf = modelPath;
    scene.m_assetManager.loadModels(&modelPathBuf, 1, &modelID, false);

    scene.m_staticSolidEntities.modelIDs[index] = modelID;
    scene.m_staticSolidEntities.transforms[index].position = parseVec3(buf);
    scene.m_staticSolidEntities.transforms[index].rotation = parseQuat(buf);
    scene.m_staticSolidEntities.transforms[index].scale = parseVec3(buf);
}

void SceneSerialization::parseSun(char const *& buf, Scene & scene) {
    scene.m_lights.sun.direction = glm::normalize(parseVec3(buf));
    scene.m_lights.sun.color = parseVec3(buf);

}

// SceneSerialization::void parsePointLight(char const *& buf, Scene & scene) {}
void SceneSerialization::parseCharacter(char const *& buf, Scene & scene) {
    auto & characters = scene.m_characterSystem.m_characters;
    size_t index = characters.size;
    assert(index < characters.maxSize && "Character amount exceeded!");
    ++characters.size;

    uint32_t modelID; 
    char modelPath[128] = {0};
    parseString(buf, modelPath);
    char const * modelPathBuf = modelPath;
    scene.m_assetManager.loadModels(&modelPathBuf, 1, &modelID, true);

    characters.modelIDs[index] = modelID;

    characters.transforms[index].position = parseVec3(buf);
    characters.transforms[index].rotation = parseQuat(buf);
    characters.transforms[index].scale = parseVec3(buf);

    characters.physics[index].colliderID = scene.m_physicsSystem.addEllipsoidCollider(parseVec3(buf), index);

    characters.animationClips[index].idle = scene.m_assetManager.getModelManager().getAnimationIndex(modelID, "idle");
    characters.animationClips[index].walk = scene.m_assetManager.getModelManager().getAnimationIndex(modelID, "walk");
    characters.animationClips[index].run = scene.m_assetManager.getModelManager().getAnimationIndex(modelID, "run");
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
