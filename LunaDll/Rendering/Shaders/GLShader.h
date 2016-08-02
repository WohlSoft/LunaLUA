#ifndef GLShader_hhhhh
#define GLShader_hhhhh

#include "../GL/GLCompat.h"
#include <string>

class GLShader
{
private:
    static bool compileShaderSource(GLuint shaderID, const std::string& source);
    static std::string getLastShaderError(GLuint shaderID);

    GLuint m_shaderID;
    std::string m_name;
    std::string m_vertexSource;
    std::string m_fragmentSource;
    std::string m_lastErrorMsg;
    bool m_isValid;

public:
    GLShader(const std::string& name, const std::string& vertexSource, const std::string& fragmentSource);
    GLShader(const GLShader& other) = delete;
    ~GLShader();

    inline bool isValid() const { return m_isValid; }
    inline std::string getLastErrorMsg() const { return m_lastErrorMsg; }
    void bind();
    void unbind();

    GLuint getAttribute(const std::string& name) const;
    GLuint getUniform(const std::string& name) const;
private:
    void load();

public:
    static GLShader* fromData(const std::string& name, const std::string& vertexSource, const std::string& fragementSource);
    static GLShader* fromFile(const std::string& name, const std::string& vertexFile, const std::string& fragmentFile);

};

#endif
