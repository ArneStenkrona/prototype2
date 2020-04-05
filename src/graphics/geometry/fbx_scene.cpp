#include "fbx_scene.h"

FBX::Scene::Scene(char const* path)
    : globalSettings{} {
    FBX::Document doc(path);
    auto const & globalSettings = doc.getNode("GlobalSettings");
    assert(globalSettings && "FBX file must contain Global Settings node!");
    parseGlobalSettings(*globalSettings);

    auto const & objects = doc.getNode("Objects");
    assert(objects && "FBX file must contain Objects node!");
    auto const & children = objects->getChildren();

    for (auto const & child : children) {
        if (strcmp(child.getName(), "Geometry") == 0) {
            char const *type = reinterpret_cast<char const*>(child.getProperty(2).data());
            if (strcmp(type, "Mesh") == 0) {
                parseMesh(child);
            } 
        } else if (strcmp(child.getName(), "Model") == 0) {
            parseModel(child);
        } else if (strcmp(child.getName(), "Material") == 0) {
            parseMaterial(child);
        } else if (strcmp(child.getName(), "Texture") == 0) {
            parseTexture(child);
        } else if (strcmp(child.getName(), "AnimationCurveNode") == 0) {
            parseAnimationCurveNode(child);
        } else if (strcmp(child.getName(), "AnimationCurve") == 0) {
            parseAnimationCurve(child);
        }
    }

    auto const & connections = doc.getNode("Connections");
    assert(connections && "FBX file must contain Connections node!");
    parseConnections(*connections);
}

void FBX::Scene::parseGlobalSettings(FBX::Document::Node const & node) {
    auto const & prop70 = node.find("Properties70")->getChildren();
    for (auto const & prop : prop70) {
        char const *type = reinterpret_cast<char const*>(prop.getProperty(0).data());
        if (strcmp(type, "UpAxis") == 0) {
            globalSettings.upAxis = *reinterpret_cast<const int32_t*>(prop.getProperty(4).data());
        } else if (strcmp(type, "UpAxisSign") == 0) {
            globalSettings.upAxisSign = *reinterpret_cast<const int32_t*>(prop.getProperty(4).data());
        } else if (strcmp(type, "FrontAxis") == 0) {
            globalSettings.frontAxis = *reinterpret_cast<const int32_t*>(prop.getProperty(4).data());
        } else if (strcmp(type, "FrontAxisSign") == 0) {
            globalSettings.frontAxisSign = *reinterpret_cast<const int32_t*>(prop.getProperty(4).data());
        } else if (strcmp(type, "CoordAxis") == 0) {
            globalSettings.coordAxis = *reinterpret_cast<const int32_t*>(prop.getProperty(4).data());
        } else if (strcmp(type, "CoordAxisSign") == 0) {
            globalSettings.coordAxisSign = *reinterpret_cast<const int32_t*>(prop.getProperty(4).data());
        } else if (strcmp(type, "OriginalUpAxis") == 0) {
            globalSettings.originalUpAxis = *reinterpret_cast<const int32_t*>(prop.getProperty(4).data());
        } else if (strcmp(type, "OriginalUpAxisSign") == 0) {
            globalSettings.originalUpAxisSign = *reinterpret_cast<const int32_t*>(prop.getProperty(4).data());
        }
    }
}



void FBX::Scene::parseMesh(FBX::Document::Node const & node) {
    meshes.push_back({});
    Mesh & mesh = meshes.back();
    // get id
    mesh.id = *reinterpret_cast<int64_t const*>(node.getProperty(0).data());
    // insertion into search table
    idToIndex.insert(mesh.id, {NODE_TYPE::MESH, int16_t(meshes.size() - 1)});
    // Retrieve name
    char const *name = reinterpret_cast<char const*>(node.getProperty(1).data());
    strcpy(mesh.name, name);

    // Retrieve vertices
    auto const & vertices = node.find("Vertices")->getProperty(0);
    mesh.vertices.resize(vertices.bytes() / sizeof(glm::dvec3));
    memcpy(mesh.vertices.data(), vertices.data(), vertices.bytes());

    // Retrieve indices
    auto const & indices = node.find("PolygonVertexIndex")->getProperty(0);
    mesh.polygonVertexIndex.resize(indices.bytes() / sizeof(int32_t));
    memcpy(mesh.polygonVertexIndex.data(), indices.data(), indices.bytes());

    // Retrieve normals
    Document::Node const *layerNormal = node.find("LayerElementNormal");
    char const *normalMap = reinterpret_cast<char const*>(layerNormal->find("MappingInformationType")->getProperty(0).data());
    if (strcmp(normalMap, "ByPolygonVertex") != 0) {
        assert(false && "Mapping Information Type for normal has to be By Polygon Vertex!");
        return;
    }
    char const *normalRef = reinterpret_cast<char const*>(layerNormal->find("ReferenceInformationType")->getProperty(0).data());
    if (strcmp(normalRef, "Direct") != 0) {
        assert(false && "Reference Information Type for normal has to be Direct!");
        return;
    }
    auto const & normals = layerNormal->find("Normals")->getProperty(0);
    mesh.normals.resize(normals.bytes() / sizeof(glm::dvec3));
    std::memcpy(mesh.normals.data(), normals.data(), normals.bytes());
    // Retrieve uv coordinates
    const Document::Node *layerUV = node.find("LayerElementUV");
    char const *uvMap = reinterpret_cast<char const*>(layerUV->find("MappingInformationType")->getProperty(0).data());
    if (strcmp(uvMap, "ByPolygonVertex") != 0) {
        assert(false && "Mapping Information Type for UV has to be By Polygon Vertex!");
        return;
    }
    char const *uvRef = reinterpret_cast<char const*>(layerUV->find("ReferenceInformationType")->getProperty(0).data());
    if (strcmp(uvRef, "IndexToDirect") != 0) {
        assert(false && "Reference Information Type for UV has to be Index To Direct!");
        return;
    }
    auto const & uv = layerUV->find("UV")->getProperty(0);
    mesh.uv.resize(uv.bytes() / sizeof(glm::dvec2));
    std::memcpy(mesh.uv.data(), uv.data(), uv.bytes());
    // Retrieve uv indices
    auto const & uvIndex = layerUV->find("UVIndex")->getProperty(0);
    mesh.uvIndex.resize(uvIndex.bytes() / sizeof(int32_t));
    std::memcpy(mesh.uvIndex.data(), uvIndex.data(), uvIndex.bytes());
}

void FBX::Scene::parseModel(FBX::Document::Node const & node) {
    models.push_back({});
    Model & model = models.back();
    // get id
    model.id = *reinterpret_cast<int64_t const*>(node.getProperty(0).data());
    // insertion into search table
    idToIndex.insert(model.id, {NODE_TYPE::MODEL, int16_t(models.size() - 1)});
    // get model type
    char const *type = reinterpret_cast<char const*>(node.getProperty(2).data());
    assert(strlen(type) < 32);
    strcpy(model.type, type);
    // get properties70
    auto const & prop70 = node.find("Properties70")->getChildren();
    for (auto const & prop : prop70) {
        char const *type = reinterpret_cast<char const*>(prop.getProperty(0).data());
        if (strcmp(type, "Lcl Translation") == 0) {
            model.localTranslation.x = *reinterpret_cast<double const*>(prop.getProperty(4).data());
            model.localTranslation.y = *reinterpret_cast<double const*>(prop.getProperty(5).data());
            model.localTranslation.z = *reinterpret_cast<double const*>(prop.getProperty(6).data());
        } else if (strcmp(type, "Lcl Rotation") == 0) {
            model.localRotation.x = *reinterpret_cast<double const*>(prop.getProperty(4).data());
            model.localRotation.y = *reinterpret_cast<double const*>(prop.getProperty(5).data());
            model.localRotation.z = *reinterpret_cast<double const*>(prop.getProperty(6).data());
        } else if (strcmp(type, "Lcl Scaling") == 0) {
            model.localScaling.x = *reinterpret_cast<double const*>(prop.getProperty(4).data());
            model.localScaling.y = *reinterpret_cast<double const*>(prop.getProperty(5).data());
            model.localScaling.z = *reinterpret_cast<double const*>(prop.getProperty(6).data());
        }
    }
}

void FBX::Scene::parseMaterial(FBX::Document::Node const & node) {
    materials.push_back({});
    Material & material = materials.back();
    // get id
    material.id = *reinterpret_cast<int64_t const*>(node.getProperty(0).data());
    // insertion into search table
    idToIndex.insert(material.id, {NODE_TYPE::MATERIAL, int16_t(materials.size() - 1)});
    // get name
    char const *name = reinterpret_cast<char const*>(node.getProperty(1).data());
    strcpy(material.name, name);
}

void FBX::Scene::parseTexture(FBX::Document::Node const & node) {
    textures.push_back({});
    Texture & texture = textures.back();
    // get id
    texture.id = *reinterpret_cast<int64_t const*>(node.getProperty(0).data());
    // insertion into search table
    idToIndex.insert(texture.id, {NODE_TYPE::TEXTURE, int16_t(textures.size() - 1)});
    // get relative filename
    char const *relativeFilename = reinterpret_cast<char const*>(node.find("RelativeFilename")->getProperty(0).data());
    strcpy(texture.relativeFilename, relativeFilename);
}

void FBX::Scene::parseAnimationCurveNode(FBX::Document::Node const & node) {
    animationCurveNodes.push_back({});
    AnimationCurveNode & curveNode = animationCurveNodes.back();
    // get id
    curveNode.id = *reinterpret_cast<int64_t const*>(node.getProperty(0).data());
    // insertion into search table
    idToIndex.insert(curveNode.id, {NODE_TYPE::ANIMATION_CURVE_NODE, int16_t(animationCurveNodes.size() - 1)});
    // get properties70
    auto const & prop70 = node.find("Properties70")->getChildren();
    for (auto const & prop : prop70) {
        char const *type = reinterpret_cast<char const*>(prop.getProperty(0).data());
        if (strcmp(type, "d|X") == 0) {
            curveNode.dx = *reinterpret_cast<double const*>(prop.getProperty(4).data());
        } else if (strcmp(type, "d|Y") == 0) {
            curveNode.dy = *reinterpret_cast<double const*>(prop.getProperty(4).data());
        } else if (strcmp(type, "d|Z") == 0) {
            curveNode.dz = *reinterpret_cast<double const*>(prop.getProperty(4).data());
        }
    }
}
void FBX::Scene::parseAnimationCurve(FBX::Document::Node const & node) {
    animationCurves.push_back({});
    AnimationCurve & curve = animationCurves.back();
    // get id
    curve.id = *reinterpret_cast<int64_t const*>(node.getProperty(0).data());
    // insertion into search table
    idToIndex.insert(curve.id, {NODE_TYPE::ANIMATION_CURVE_NODE, int16_t(animationCurves.size() - 1)});
    // get properties70
    for (auto const & child : node.getChildren()) {
        char const *type = reinterpret_cast<char const*>(child.getProperty(0).data());
        if (strcmp(type, "Default") == 0) {
            curve.defaultVal = *reinterpret_cast<double const*>(child.getProperty(4).data());
        } else if (strcmp(type, "KeyTime") == 0) {
            int64_t const *ptr = reinterpret_cast<int64_t const*>(child.getProperty(4).data());
            curve.keyTime[0] = ptr[0];
            curve.keyTime[1] = ptr[1];
        } else if (strcmp(type, "KeyValueFloat") == 0) {
            float const *ptr = reinterpret_cast<float const*>(child.getProperty(4).data());
            curve.keyValue[0] = ptr[0];
            curve.keyValue[1] = ptr[1];
        }
    }
}

void FBX::Scene::parseConnections(FBX::Document::Node const & node) {
    auto const & children = node.getChildren();
    connections.reserve(children.size());
    for (auto const & child : children) {
        int64_t firstId = *reinterpret_cast<int64_t const*>(child.getProperty(1).data());
        int64_t secondId = *reinterpret_cast<int64_t const*>(child.getProperty(2).data());
        connections.emplace_back(firstId, secondId);
    }
}
