#pragma once

int compile_shader(const char *filepath, const int shader_type);
int build_shader(const char *vertex_shader_path,
                 const char *fragment_shader_path);
