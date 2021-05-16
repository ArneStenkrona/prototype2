#ifndef SCENE_SERIALIZATION_H
#define SCENE_SERIALIZATION_H

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>

class Scene;

class SceneSerialization {
public:
    static void loadScene(char const * file, Scene & scene);
    static void saveScene(char const * file, Scene & scene);
private:
    enum TokenType {
        ENTITY,
        SUN,
        END,
        ERROR
    };

    enum ComponentType : int {
        COMPONENT_TYPE_NAME = 0,
        COMPONENT_TYPE_TRANSFORM = 1,
        COMPONENT_TYPE_MODEL = 2,
        COMPONENT_TYPE_COLLIDER = 3,
        COMPONENT_TYPE_CHARACTER = 4,
        COMPONENT_TYPE_POINTLIGHT = 5,
        COMPONENT_TYPE_END = -1
    };

    static TokenType readToken(char const *& buf);
    static ComponentType readComponentType(char const *& buf);

    static void parseSun(char const *& buf, Scene & scene);
    static void parsePointLight(char const *& buf, Scene & scene);
    static void parseEntity(char const *& buf, Scene & scene);

    static void parseString(char const *& buf, char * dest);
    static bool parseBool(char const *& buf);
    static float parseFloat(char const *& buf);
    static glm::vec3 parseVec3(char const *& buf);
    static glm::quat parseQuat(char const *& buf);
};

#endif