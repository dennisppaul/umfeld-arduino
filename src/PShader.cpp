#include <fstream>
#include <sstream>
#include <iostream>

#include "UmfeldFunctionsAdditional.h"
#include "PShader.h"

using namespace umfeld;

PShader::PShader() : programID(0) {}

PShader::~PShader() {
    if (programID) {
        glDeleteProgram(programID);
    }
}

bool PShader::load(const std::string& vertex_code, const std::string& fragment_code, const std::string& geometry_code) {
    const GLuint vertexShader   = compileShader(vertex_code, GL_VERTEX_SHADER);
    const GLuint fragmentShader = compileShader(fragment_code, GL_FRAGMENT_SHADER);
    GLuint       geometryShader = 0;

    if (!geometry_code.empty()) {
#ifndef OPENGL_3_3_CORE
        error("geometry shader requires `OPENGL_3_3_CORE` to be defined. e.g `-DOPENGL_3_3_CORE` in CLI or `set(UMFELD_OPENGL_VERSION \"OPENGL_3_3_CORE\")` in `CMakeLists.txt`");
        return false;
#else
        geometryShader = compileShader(geometry_code, GL_GEOMETRY_SHADER);
#endif
    }

    programID = glCreateProgram();
    glAttachShader(programID, vertexShader);
    glAttachShader(programID, fragmentShader);
    if (!geometry_code.empty()) {
        glAttachShader(programID, geometryShader);
    }

    glLinkProgram(programID);
    checkLinkErrors(programID);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    if (!geometry_code.empty()) {
        glDeleteShader(geometryShader);
    }

    check_for_matrix_uniforms();

    return true;
}

void PShader::use() {
    if (!programID) { return; }
    glUseProgram(programID);
    in_use = true;
}

void PShader::unuse() {
    glUseProgram(0);
    in_use = false;
}

void PShader::check_for_matrix_uniforms() {
    if (!programID) { return; }
    GLint location        = glGetUniformLocation(programID, SHADER_UNIFORM_MODEL_MATRIX.c_str());
    has_model_matrix      = location != -1;
    location              = glGetUniformLocation(programID, SHADER_UNIFORM_VIEW_MATRIX.c_str());
    has_view_matrix       = location != -1;
    location              = glGetUniformLocation(programID, SHADER_UNIFORM_PROJECTION_MATRIX.c_str());
    has_projection_matrix = location != -1;
    location              = glGetUniformLocation(programID, SHADER_UNIFORM_TEXTURE_UNIT.c_str());
    has_texture_unit      = location != -1;
}

GLuint PShader::compileShader(const std::string& source, const GLenum type) {
    const GLuint shader = glCreateShader(type);
    const char*  src    = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    checkCompileErrors(shader, type);
    return shader;
}

void PShader::checkCompileErrors(const GLuint shader, const GLenum type) {
    GLint  success;
    GLchar infoLog[1024];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
        error("Shader Compilation Error (Type: ", type, ")\n", infoLog);
    }
}

void PShader::checkLinkErrors(const GLuint program) {
    GLint  success;
    GLchar infoLog[1024];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 1024, nullptr, infoLog);
        error("Shader Linking Error\n", infoLog);
    }
}

GLint PShader::getUniformLocation(const std::string& name) {
#ifdef DEBUG_SHADER_PROGRAM_ID
    GLint currentlyBoundProgram;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentlyBoundProgram);
    if (currentlyBoundProgram != programID) {
        std::cout << "⚠️  set_uniform called while wrong program is bound!"
                  << " expected " << programID << ", got " << currentlyBoundProgram << std::endl;
    }
#endif
    // ReSharper disable once CppDFAUnreachableCode
    if (!programID) { return 0; }
    if (uniformLocations.find(name) == uniformLocations.end()) {
        uniformLocations[name] = glGetUniformLocation(programID, name.c_str());
        // console("uniformLocations[name]: ", name, " = ", uniformLocations[name]);
        if (uniformLocations[name] < 0) {
            warning("Shader uniform '", name, "' was not found or is not used. this might be intentional or maybe the uniform name is misspelled.");
        }
    }
    return uniformLocations[name];
}

void PShader::check_uniform_location(const std::string& name) const {
    if (!debug_uniform_location) { return; }
    const GLint loc = glGetUniformLocation(programID, name.c_str());
    if (loc == -1) {
        std::cerr << "WARNING: uniform '" << name << "' not active or not found!\n";
    }
}

void PShader::set_uniform(const std::string& name, const int value) {
    if (!programID) { return; }
    check_uniform_location(name);
    glUniform1i(getUniformLocation(name), value);
}

void PShader::set_uniform(const std::string& name, const int value_a, const int value_b) {
    if (!programID) { return; }
    check_uniform_location(name);
    glUniform2i(getUniformLocation(name), value_a, value_b);
}

void PShader::set_uniform(const std::string& name, const float value) {
    if (!programID) { return; }
    check_uniform_location(name);
    glUniform1f(getUniformLocation(name), value);
}

void PShader::set_uniform(const std::string& name, const float value_a, const float value_b) {
    if (!programID) { return; }
    check_uniform_location(name);
    glUniform2f(getUniformLocation(name), value_a, value_b);
}

void PShader::set_uniform(const std::string& name, const glm::vec2& value) {
    if (!programID) { return; }
    check_uniform_location(name);
    glUniform2fv(getUniformLocation(name), 1, &value[0]);
}

void PShader::set_uniform(const std::string& name, const glm::vec3& value) {
    if (!programID) { return; }
    check_uniform_location(name);
    glUniform3fv(getUniformLocation(name), 1, &value[0]);
}

void PShader::set_uniform(const std::string& name, const glm::vec4& value) {
    if (!programID) { return; }
    check_uniform_location(name);
    glUniform4fv(getUniformLocation(name), 1, &value[0]);
}

void PShader::set_uniform(const std::string& name, const glm::mat3& value) {
    if (!programID) { return; }
    check_uniform_location(name);
    glUniformMatrix3fv(getUniformLocation(name), 1, GL_FALSE, &value[0][0]);
}

void PShader::set_uniform(const std::string& name, const glm::mat4& value) {
    if (!programID) { return; }
    check_uniform_location(name);
    glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, &value[0][0]);
}