struct vertex_input {
    @location(0) color: vec4f,
    @location(1) position: vec3f,
    @location(2) normal: vec3f,
    @location(3) uv: vec2f,
};

struct vertex_output {
    @builtin(position) position: vec4f,
    @location(0) color: vec4f,
};

@group(0) @binding(0) var<uniform> u_mvp_matrix: mat4x4f;

@vertex
fn vs_main(in: vertex_input) -> vertex_output {
    var out: vertex_output;
    out.position = u_mvp_matrix * vec4f(in.position, 1.0);
    out.color = in.color;
    return out;
}

@fragment
fn fs_main(in: vertex_output) -> @location(0) vec4f {
    return in.color;
}
