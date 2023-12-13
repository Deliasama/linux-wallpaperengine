#pragma once

#include <glm/gtc/type_ptr.hpp>
#include <utility>

#include "WallpaperEngine/Assets/ITexture.h"
#include "WallpaperEngine/Core/Objects/Effects/Constants/CShaderConstant.h"
#include "WallpaperEngine/Render/CFBO.h"
#include "WallpaperEngine/Render/Objects/Effects/CMaterial.h"
#include "WallpaperEngine/Render/Shaders/Compiler.h"
#include "WallpaperEngine/Render/Shaders/Variables/CShaderVariable.h"

#include "WallpaperEngine/Render/Helpers/CContextAware.h"

namespace WallpaperEngine::Render::Objects::Effects {
using namespace WallpaperEngine::Assets;
using namespace WallpaperEngine::Render::Shaders::Variables;
using namespace WallpaperEngine::Core::Objects::Effects::Constants;

class CMaterial;

class CPass final : public Helpers::CContextAware {
  public:
    CPass (CMaterial* material, Core::Objects::Images::Materials::CPass* pass);

    void render ();

    void setDestination (const CFBO* drawTo);
    void setInput (const ITexture* input);
    void setTexCoord (GLuint texcoord);
    void setPosition (GLuint position);
    void setModelViewProjectionMatrix (const glm::mat4* projection);
    void setModelViewProjectionMatrixInverse (const glm::mat4* projection);
    void setModelMatrix (const glm::mat4* model);
    void setViewProjectionMatrix (const glm::mat4* viewProjection);

    const CMaterial* getMaterial () const;
    Core::Objects::Images::Materials::CPass* getPass ();

  private:
    enum UniformType {
        Float = 0,
        Matrix3 = 1,
        Matrix4 = 2,
        Integer = 3,
        Vector2 = 4,
        Vector3 = 5,
        Vector4 = 6,
        Double = 7
    };

    class UniformEntry {
      public:
        UniformEntry (const GLint id, std::string name, UniformType type, const void* value, int count) :
            id (id),
            name (std::move (name)),
            type (type),
            value (value),
            count (count) {}

        const GLint id;
        std::string name;
        UniformType type;
        const void* value;
        int count;
    };

    class ReferenceUniformEntry {
      public:
        ReferenceUniformEntry (const GLint id, std::string name, UniformType type, const void** value) :
            id (id),
            name (std::move (name)),
            type (type),
            value (value) {}

        const GLint id;
        std::string name;
        UniformType type;
        const void** value;
    };

    class AttribEntry {
      public:
        AttribEntry (const GLint id, std::string name, GLint type, GLint elements, const GLuint* value) :
            id (id),
            name (std::move (name)),
            type (type),
            elements (elements),
            value (value) {}

        const GLint id;
        std::string name;
        GLint type;
        GLint elements;
        const GLuint* value;
    };

    static GLuint compileShader (Render::Shaders::Compiler* shader, GLuint type);
    void setupTextures ();
    void setupShaders ();
    void setupShaderVariables ();
    void setupUniforms ();
    void setupAttributes ();
    void addAttribute (const std::string& name, GLint type, GLint elements, const GLuint* value);
    void addUniform (CShaderVariable* value);
    void addUniform (const std::string& name, CShaderConstant* value);
    void addUniform (const std::string& name, int value);
    void addUniform (const std::string& name, double value);
    void addUniform (const std::string& name, float value);
    void addUniform (const std::string& name, glm::vec2 value);
    void addUniform (const std::string& name, glm::vec3 value);
    void addUniform (const std::string& name, glm::vec4 value);
    void addUniform (const std::string& name, const glm::mat3& value);
    void addUniform (const std::string& name, glm::mat4 value);
    void addUniform (const std::string& name, const int* value, int count = 1);
    void addUniform (const std::string& name, const double* value, int count = 1);
    void addUniform (const std::string& name, const float* value, int count = 1);
    void addUniform (const std::string& name, const glm::vec2* value);
    void addUniform (const std::string& name, const glm::vec3* value);
    void addUniform (const std::string& name, const glm::vec4* value);
    void addUniform (const std::string& name, const glm::mat3* value);
    void addUniform (const std::string& name, const glm::mat4* value);
    void addUniform (const std::string& name, const int** value);
    void addUniform (const std::string& name, const double** value);
    void addUniform (const std::string& name, const float** value);
    void addUniform (const std::string& name, const glm::vec2** value);
    void addUniform (const std::string& name, const glm::vec3** value);
    void addUniform (const std::string& name, const glm::vec4** value);
    void addUniform (const std::string& name, const glm::mat3** value);
    void addUniform (const std::string& name, const glm::mat4** value);
    template <typename T> void addUniform (const std::string& name, UniformType type, T value);
    template <typename T> void addUniform (const std::string& name, UniformType type, T* value, int count = 1);
    template <typename T> void addUniform (const std::string& name, UniformType type, T** value);

    const ITexture* resolveTexture (const ITexture* expected, int index, const ITexture* previous = nullptr);

    CMaterial* m_material;
    Core::Objects::Images::Materials::CPass* m_pass;
    std::vector<const ITexture*> m_textures;
    std::map<int, const CFBO*> m_fbos;
    std::map<std::string, bool> m_foundCombos;
    std::vector<AttribEntry*> m_attribs;
    std::map<std::string, UniformEntry*> m_uniforms;
    std::map<std::string, ReferenceUniformEntry*> m_referenceUniforms;
    const glm::mat4* m_modelViewProjectionMatrix;
    const glm::mat4* m_modelViewProjectionMatrixInverse;
    const glm::mat4* m_modelMatrix;
    const glm::mat4* m_viewProjectionMatrix;

    /**
     * Contains the final map of textures to be used
     */
    std::map<int, const ITexture*> m_finalTextures;

    Render::Shaders::Compiler* m_fragShader;
    Render::Shaders::Compiler* m_vertShader;

    const CFBO* m_drawTo;
    const ITexture* m_input;

    GLuint m_programID;

    // shader variables used temporary
    GLint g_Texture0Rotation;
    GLint g_Texture0Translation;
    GLuint a_TexCoord;
    GLuint a_Position;
};
} // namespace WallpaperEngine::Render::Objects::Effects
