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
        STATIC_SOLID_ENTITY,
        SUN,
        POINT_LIGHT,
        BOX_LIGHT,
        CHARACTER,
        ERROR
    };
    static TokenType readToken(char const *& buf);

    static void parseStaticSolidEntity(char const *& buf, Scene & scene);
    static void parseSun(char const *& buf, Scene & scene);
    static void parsePointLight(char const *& buf, Scene & scene);
    static void parseBoxLight(char const *& buf, Scene & scene);
    static void parseCharacter(char const *& buf, Scene & scene);

    static void parseString(char const *& buf, char * dest);
    static float parseFloat(char const *& buf);
    static glm::vec3 parseVec3(char const *& buf);
    static glm::quat parseQuat(char const *& buf);
};

#endif