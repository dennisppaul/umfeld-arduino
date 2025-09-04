#include <fstream>
#include <sstream>
#include <iostream>

#include "UmfeldSDLOpenGL.h"
#include "UmfeldFunctionsAdditional.h"
#include "PShader.h"
#include "UShape.h"
#include "PGraphicsOpenGL.h"

using namespace umfeld;

static void checkLinkErrors(const uint32_t program) {
    int32_t success;
    GLchar  infoLog[1024];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 1024, nullptr, infoLog);
        error("Shader Linking Error\n", infoLog);
    }
}

static void checkCompileErrors(const uint32_t shader, const GLenum type) {
    int32_t success;
    GLchar  infoLog[1024];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
        error("Shader Compilation Error (Type: ", type, ")\n", infoLog);
    }
}

static uint32_t compileShader(const std::string& source, const GLenum type) {
    const uint32_t shader = glCreateShader(type);
    const char*    src    = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    checkCompileErrors(shader, type);
    return shader;
}

static bool uniform_available(const GLuint loc) { return loc != ShaderUniforms::UNINITIALIZED && loc != ShaderUniforms::NOT_FOUND; }

PShader::PShader(const std::string& shader_name) : shader_name(shader_name) {}

PShader::~PShader() {
    if (program.id) {
        glDeleteProgram(program.id);
    }
}

bool PShader::load(const std::string& vertex_code, const std::string& fragment_code, const std::string& geometry_code) {
    const uint32_t vertexShader   = compileShader(vertex_code, GL_VERTEX_SHADER);
    const uint32_t fragmentShader = compileShader(fragment_code, GL_FRAGMENT_SHADER);
    uint32_t       geometryShader = 0;

    if (!geometry_code.empty()) {
#ifndef OPENGL_3_3_CORE
        error("geometry shader requires `OPENGL_3_3_CORE` to be defined. e.g `-DOPENGL_3_3_CORE` in CLI or `set(UMFELD_OPENGL_VERSION \"OPENGL_3_3_CORE\")` in `CMakeLists.txt`");
        return false;
#else
        geometryShader = compileShader(geometry_code, GL_GEOMETRY_SHADER);
#endif
    }

    program.id = glCreateProgram();
    glAttachShader(program.id, vertexShader);
    glAttachShader(program.id, fragmentShader);
    if (!geometry_code.empty()) {
        glAttachShader(program.id, geometryShader);
    }

    glLinkProgram(program.id);
    checkLinkErrors(program.id);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    if (!geometry_code.empty()) {
        glDeleteShader(geometryShader);
    }

    init_default_uniforms();

    return true;
}

void PShader::use() {
    if (!program.id) { return; }
    glUseProgram(program.id);
    in_use = true;
}

void PShader::unuse() {
    glUseProgram(0);
    in_use = false;
}

int32_t PShader::get_uniform_location(const std::string& name) {
#ifdef DEBUG_SHADER_PROGRAM_ID
    int32_t currentlyBoundProgram;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentlyBoundProgram);
    if (currentlyBoundProgram != program.id) {
        std::cout << "⚠️  set_uniform called while wrong program is bound!"
                  << " expected " << program.id << ", got " << currentlyBoundProgram << std::endl;
    }
#endif
    // ReSharper disable once CppDFAUnreachableCode
    if (!program.id) { return 0; }
    if (uniformLocations.find(name) == uniformLocations.end()) {
        uniformLocations[name] = glGetUniformLocation(program.id, name.c_str());
        if (uniformLocations[name] < 0) {
            warning("shader uniform '", name, "' was not found or is not used. this might be intentional or maybe the uniform name is misspelled.");
        }
    }
    return uniformLocations[name];
}

bool PShader::check_uniform_location(const std::string& name) const {
    if (!program.id) { return false; }
    const int32_t loc = glGetUniformLocation(program.id, name.c_str());
    if (loc == GL_INVALID_INDEX) {
        if (debug_uniform_location) {
            warning("shader uniform '", name, "' was not found or is not used. this might be intentional or maybe the uniform name is misspelled.");
        }
        return false;
    }
    return true;
}

void PShader::set_uniform(const std::string& name, const int value) {
    if (!program.id) { return; }
    if (check_uniform_location(name)) {
        glUniform1i(get_uniform_location(name), value);
    }
}

void PShader::set_uniform(const std::string& name, const int value_a, const int value_b) {
    if (!program.id) { return; }
    if (check_uniform_location(name)) {
        glUniform2i(get_uniform_location(name), value_a, value_b);
    }
}

void PShader::set_uniform(const std::string& name, const float value) {
    if (!program.id) { return; }
    if (check_uniform_location(name)) {
        glUniform1f(get_uniform_location(name), value);
    }
}

void PShader::set_uniform(const std::string& name, const float value_a, const float value_b) {
    if (!program.id) { return; }
    if (check_uniform_location(name)) {
        glUniform2f(get_uniform_location(name), value_a, value_b);
    }
}

void PShader::set_uniform(const std::string& name, const glm::vec2& value) {
    if (!program.id) { return; }
    if (check_uniform_location(name)) {
        glUniform2fv(get_uniform_location(name), 1, &value[0]);
    }
}

void PShader::set_uniform(const std::string& name, const glm::vec3& value) {
    if (!program.id) { return; }
    if (check_uniform_location(name)) {
        glUniform3fv(get_uniform_location(name), 1, &value[0]);
    }
}

void PShader::set_uniform(const std::string& name, const glm::vec4& value) {
    if (!program.id) { return; }
    if (check_uniform_location(name)) {
        glUniform4fv(get_uniform_location(name), 1, &value[0]);
    }
}

void PShader::set_uniform(const std::string& name, const glm::mat3& value) {
    if (!program.id) { return; }
    if (check_uniform_location(name)) {
        glUniformMatrix3fv(get_uniform_location(name), 1, GL_FALSE, &value[0][0]);
    }
}

void PShader::set_uniform(const std::string& name, const glm::mat4& value) {
    if (!program.id) { return; }
    if (check_uniform_location(name)) {
        glUniformMatrix4fv(get_uniform_location(name), 1, GL_FALSE, &value[0][0]);
    }
}

void PShader::init_default_uniforms() {
    program.uniforms.u_model_matrix.id      = PGraphicsOpenGL::OGL_get_uniform_location(program.id, "u_model_matrix");
    program.uniforms.u_view_matrix.id       = PGraphicsOpenGL::OGL_get_uniform_location(program.id, "u_view_matrix");
    program.uniforms.u_projection_matrix.id = PGraphicsOpenGL::OGL_get_uniform_location(program.id, "u_projection_matrix");
    program.uniforms.u_texture_unit.id      = PGraphicsOpenGL::OGL_get_uniform_location(program.id, "u_texture_unit");
    // TODO program.uniforms.Transforms.id = PGraphicsOpenGL::OGL_get_uniform_location(program.id, "Transforms");
}

void PShader::check_default_uniforms() const {
    PGraphicsOpenGL::OGL_evaluate_shader_uniforms(shader_name, program.uniforms);
}

void PShader::update_uniforms(const glm::mat4& model_matrix, const glm::mat4& view_matrix, const glm::mat4& projection_matrix, const uint32_t texture_unit) {
    if (auto_update_uniforms) {
        if (uniform_available(program.uniforms.u_model_matrix.id)) {
            glUniformMatrix4fv(program.uniforms.u_model_matrix.id, 1, GL_FALSE, &model_matrix[0][0]);
        }
        if (uniform_available(program.uniforms.u_view_matrix.id)) {
            glUniformMatrix4fv(program.uniforms.u_view_matrix.id, 1, GL_FALSE, &view_matrix[0][0]);
        }
        if (uniform_available(program.uniforms.u_projection_matrix.id)) {
            glUniformMatrix4fv(program.uniforms.u_projection_matrix.id, 1, GL_FALSE, &projection_matrix[0][0]);
        }
        if (uniform_available(program.uniforms.u_texture_unit.id)) {
            glUniform1i(program.uniforms.u_texture_unit.id, texture_unit);
        }
    }
}
