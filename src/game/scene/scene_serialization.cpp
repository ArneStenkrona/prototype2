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
        scene.m_entities.lightTags[i].type = LightType::LIGHT_TYPE_NONE;
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
            case ENTITY :
                parseEntity(data, scene);
                break;
            case SUN :
                parseSun(data, scene);
                break;
            case END :
                return;
            case ERROR :
                assert(false && "Failed to parse file!");
                break;        
            }
        // read rest of line
        while (*data != '\n' && *data != '\0') {
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
    if (strcmp(tokenStr, "Entity") == 0) {
        type = TokenType::ENTITY;
    } else if (strcmp(tokenStr, "Sun") == 0) {
        type = TokenType::SUN;
    } else if (strcmp(tokenStr, "") == 0) {
        type = TokenType::END;
    }

    return type;
}

SceneSerialization::ComponentType SceneSerialization::readComponentType(char const *& buf) {
    char tokenStr[64] = {0};
    int i = 0;
    while (*buf != '{' && *buf != '\n' && *buf != '\0') {
        tokenStr[i] = *buf;
        ++i;
        ++buf;
    }

    ComponentType type = ComponentType::COMPONENT_TYPE_END;
    if (*buf != '\n' && *buf != '\0') {
        type = ComponentType(atoi(tokenStr));
    }

    return type;
}

void SceneSerialization::parseEntity(char const *& buf, Scene & scene) {
    EntityID id = scene.m_entities.addEntity();

    ComponentType type;

    while ((type = readComponentType(buf)) != ComponentType::COMPONENT_TYPE_END) {
        switch (type) {
            case COMPONENT_TYPE_NAME: {
                parseString(buf, scene.m_entities.names[id]);
                break;
            }
            case COMPONENT_TYPE_TRANSFORM: {
                scene.m_entities.transforms[id].position = parseVec3(buf);
                scene.m_entities.transforms[id].rotation = parseQuat(buf);
                scene.m_entities.transforms[id].scale = parseVec3(buf);
                break;
            }
            case COMPONENT_TYPE_MODEL: {
                char modelPath[128] = {0};
                parseString(buf, modelPath);
                char const * modelPathBuf = modelPath;

                bool animated = parseBool(buf);

                ModelID modelID = scene.m_assetManager.getModelManager().loadModel(modelPathBuf, animated);
                scene.m_entities.modelIDs[id] = modelID;

                if (animated) {
                    scene.m_entities.animationIDs[id] = scene.m_animationSystem.addAnimation(id);
                }
                break;
            }
            case COMPONENT_TYPE_COLLIDER: {
                char cType[128] = {0};
                parseString(buf, cType);
                if (strcmp(cType, "MODEL") == 0) {
                    Model const * models;
                    size_t nModels;
                    scene.m_assetManager.getModelManager().getModels(models, nModels);

                    scene.m_entities.colliderTags[id] = scene.m_physicsSystem.addModelCollider(models[scene.m_entities.modelIDs[id]], 
                                                                                               scene.m_entities.transforms[id]);
                } else if (strcmp(cType, "ELLIPSOID") == 0) {
                    glm::vec3 radii = parseVec3(buf);
                    glm::vec3 offset = parseVec3(buf);

                    scene.m_entities.colliderTags[id] = scene.m_physicsSystem.addEllipsoidCollider(radii, offset);
                } else {
                    assert(false);
                }

                break;
            }
            case COMPONENT_TYPE_CHARACTER: {
                CharacterID characterID = scene.m_characterSystem.addCharacter(id, scene.m_entities.colliderTags[id]);
                scene.m_entities.characterIDs[id] = characterID;
                ++buf;

                while (*buf == '<') {
                    ++buf;
                    char bone[128] = {0};
                    char model[128] = {0};
                    parseString(buf, bone);
                    parseString(buf, model);
                    int boneIndex = scene.getModel(id).getBoneIndex(bone);

                    EntityID equipID = scene.m_entities.addEntity();

                    ModelID modelID = scene.m_assetManager.getModelManager().loadModel(model, false);
                    scene.m_entities.modelIDs[equipID] = modelID;

                    Transform offset;
                    offset.position = parseVec3(buf);
                    offset.rotation = parseQuat(buf);
                    offset.scale = parseVec3(buf);
                    ++buf;

                    scene.m_characterSystem.addEquipment(characterID, boneIndex, equipID, offset);
                }

                break;
            }
            case COMPONENT_TYPE_POINTLIGHT: {
                PointLight light;
                light.color = parseVec3(buf);
                light.constant = parseFloat(buf);
                light.linear = parseFloat(buf);
                light.quadratic = parseFloat(buf);

                scene.addPointLight(id, light);
                break;
            }
            case COMPONENT_TYPE_END: {
                break;
            }
            default: {
                break;
            }
        }
        while (*buf != '}' && *buf != '\0') {
            ++buf;
        }
        ++buf;
    }
}

void SceneSerialization::parseSun(char const *& buf, Scene & scene) {
    scene.m_lights.sun.direction = glm::normalize(parseVec3(buf));
    scene.m_lights.sun.color = parseVec3(buf);
}

void SceneSerialization::parseString(char const *& buf, char * dest) {
    while (*buf != '<' && *buf != '\0') {
        ++buf;
    }
    if (*buf == '\0') {
        assert(false);
    }
    buf += 2;

    int i = 0;
    while (*buf != '"') {
        dest[i] = *buf;
        ++i;
        ++buf;
    }
    while (*buf != '>' && *buf != '\0') {
        ++buf;
    }
    if (*buf == '\0') {
        assert(false);
    }
    ++buf;
    dest[i] = '\0';
}

bool SceneSerialization::parseBool(char const *& buf) {
    while (*buf != '<' && *buf != '\0') {
        ++buf;
    }
    if (*buf == '\0') {
        assert(false);
    }
    ++buf;

    return tolower(*buf) == 't';
}

float SceneSerialization::parseFloat(char const *& buf) {
    float f;
    while (*buf != '<' && *buf != '\0') {
        ++buf;
    }
    if (*buf == '\0') {
        assert(false);
    }
    ++buf;

    int ret = sscanf(buf, "%f",
                     &f);

    if (ret != 1) {
        assert(false && "Failed to parse float");
    }

    while (*buf != '>' && *buf != '\0') {
        ++buf;
    }
    if (*buf == '\0') {
        assert(false);
    }
    ++buf;
    return f;
}

glm::vec3 SceneSerialization::parseVec3(char const *& buf) {
    glm::vec3 vec;
    while (*buf != '<' && *buf != '\0') {
        ++buf;
    }
    if (*buf == '\0') {
        assert(false);
    }
    ++buf;

    int ret = sscanf(buf, "%f,%f,%f",
                     &vec.x, &vec.y, &vec.z);

    if (ret != 3) {
        assert(false && "Failed to parse vec3");
    }

    while (*buf != '>' && *buf != '\0') {
        ++buf;
    }
    if (*buf == '\0') {
        assert(false);
    }
    ++buf;
    return vec;
}

glm::quat SceneSerialization::parseQuat(char const *& buf) {
    glm::quat quat;
    while (*buf != '<' && *buf != '\0') {
        ++buf;
    }
    if (*buf == '\0') {
        assert(false);
    }
    ++buf;

    int ret = sscanf(buf, "%f,%f,%f,%f",
                     &quat.w, &quat.x, &quat.y, &quat.z);
    if (ret != 4) {
        assert(false && "Failed to parse quat");
    }

    while (*buf != '>' && *buf != '\0') {
        ++buf;
    }
    if (*buf == '\0') {
        assert(false);
    }
    ++buf;
    return glm::normalize(quat);
}
