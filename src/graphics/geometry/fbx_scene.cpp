#include "fbx_scene.h"

FBX_Scene::FBX_Scene(char const* path) {
    FBX_Document doc(path);
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
            char const *type = reinterpret_cast<char const*>(child.getProperty(2).data());
            if (strcmp(type, "Mesh") == 0) {
                parseModel(child);
            }
        } else if (strcmp(child.getName(), "Material") == 0) {
            parseMaterial(child);
        } else if (strcmp(child.getName(), "Texture") == 0) {
            parseTexture(child);
        } 
    }

    auto const & connections = doc.getNode("Connections");
    assert(connections && "FBX file must contain Connections node!");
    parseConnections(*connections);
}


void FBX_Scene::parseMesh(FBX_Document::FBX_Node const & node) {
    meshes.push_back({});
    FBX_Mesh & mesh = meshes.back();
    // get id
    mesh.id = *reinterpret_cast<int64_t const*>(node.getProperty(0).data());
    // insertion into search table
    idToIndex.insert(mesh.id, {FBX_TYPE::MESH, int16_t(meshes.size() - 1)});
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
    FBX_Document::FBX_Node const *layerNormal = node.find("LayerElementNormal");
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
    const FBX_Document::FBX_Node *layerUV = node.find("LayerElementUV");
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

void FBX_Scene::parseModel(FBX_Document::FBX_Node const & node) {
    models.push_back({});
    FBX_Model & model = models.back();
    // get id
    model.id = *reinterpret_cast<int64_t const*>(node.getProperty(0).data());
    // insertion into search table
    idToIndex.insert(model.id, {FBX_TYPE::MODEL, int16_t(models.size() - 1)});
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

void FBX_Scene::parseMaterial(FBX_Document::FBX_Node const & node) {
    materials.push_back({});
    FBX_Material & material = materials.back();
    // get id
    material.id = *reinterpret_cast<int64_t const*>(node.getProperty(0).data());
    // insertion into search table
    idToIndex.insert(material.id, {FBX_TYPE::MATERIAL, int16_t(materials.size() - 1)});
    // get name
    char const *name = reinterpret_cast<char const*>(node.getProperty(1).data());
    strcpy(material.name, name);
}

void FBX_Scene::parseTexture(FBX_Document::FBX_Node const & node) {
    textures.push_back({});
    FBX_Texture & texture = textures.back();
    // get id
    texture.id = *reinterpret_cast<int64_t const*>(node.getProperty(0).data());
    // insertion into search table
    idToIndex.insert(texture.id, {FBX_TYPE::TEXTURE, int16_t(textures.size() - 1)});
    // get relative filename
    char const *relativeFilename = reinterpret_cast<char const*>(node.find("RelativeFilename")->getProperty(0).data());
    strcpy(texture.relativeFilename, relativeFilename);
}

void FBX_Scene::parseConnections(FBX_Document::FBX_Node const & node) {
    auto const & children = node.getChildren();
    connections.reserve(children.size());
    for (auto const & child : children) {
        int64_t firstId = *reinterpret_cast<int64_t const*>(child.getProperty(1).data());
        int64_t secondId = *reinterpret_cast<int64_t const*>(child.getProperty(2).data());
        connections.emplace_back(firstId, secondId);
    }
}
