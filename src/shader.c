#include "shader.h"
#include <glad/gl.h>
#include <stdio.h>

int compile_shader(const char *filepath, const int shader_type) {
    FILE *fp = fopen(filepath, "r");
    if (fp == NULL) {
        fprintf(stderr, "Failed to open file: %s\n", filepath);
        return -1;
    }

    // Get file size
    fseek(fp, 0, SEEK_END);
    size_t file_size = ftell(fp);

    // Read file contents
    char buffer[file_size + 1];
    fseek(fp, 0, SEEK_SET);
    fread(buffer, sizeof(char), file_size, fp);

    // Create shader
    GLuint shader = glCreateShader(shader_type);
    const GLchar *source = buffer;
    glShaderSource(shader, 1, &source, (GLint *)&file_size);
    glCompileShader(shader);

    // Check compile status
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        // Read info log
        GLint info_log_len;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_log_len);
        GLchar info_log[info_log_len];
        glGetShaderInfoLog(shader, info_log_len, NULL, (GLchar *)&info_log);

        fprintf(stderr, "Unable to compile shader at %s:\n%s", filepath,
                info_log);
        return -1;
    }

    return shader;
}

int build_shader(const char *vertex_shader_path,
                 const char *fragment_shader_path) {
    // Compile vertex and fragment shaders
    int vert = compile_shader(vertex_shader_path, GL_VERTEX_SHADER);
    int frag = compile_shader(fragment_shader_path, GL_FRAGMENT_SHADER);
    if (vert == -1 || frag == -1)
        return -1;

    // Create and link shader program
    GLuint program = glCreateProgram();
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glLinkProgram(program);

    // Check link status
    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
        // Read info log
        GLint info_log_len;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &info_log_len);
        GLchar info_log[info_log_len];
        glGetProgramInfoLog(program, info_log_len, NULL, (GLchar *)&info_log);

        fprintf(stderr, "Unable to link shaders at %s and %s:\n%s",
                vertex_shader_path, fragment_shader_path, info_log);
        return -1;
    }

    return program;
}
