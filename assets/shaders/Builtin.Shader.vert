#version 450
// #extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_texcoord;

layout(set = 0, binding = 0) uniform global_uniform_object {
    mat4 projection;
	mat4 view;
} global_ubo;

layout(push_constant) uniform push_constants {
	// Only guaranteed a total of 128 bytes.
	mat4 model; // 64 bytes
} u_push_constants;

// layout(location = 0) out vec3 out_position;

// Data transfer object (DTO) for the fragment shader.
// This is a struct that can be extended with more fields as needed,
// and it keeps the interface between the vertex and fragment shaders organized.
layout(location = 1) out struct dto {
    vec2 tex_coord;
} out_dto;

void main() {
    out_dto.tex_coord = in_texcoord;
    gl_Position = global_ubo.projection * global_ubo.view * u_push_constants.model * vec4(in_position, 1.0);
    // out_position = in_position;
}
