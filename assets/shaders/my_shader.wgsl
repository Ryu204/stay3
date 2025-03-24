struct vertex_input {
    @location(0) color: vec4f,
    @location(1) position: vec3f,
    @location(2) normal: vec3f,
    @location(3) uv: vec2f,
};

struct vertex_output {
    @builtin(position) position: vec4f,
    @location(0) color: vec4f,
	@location(1) uv: vec2f,
};

@group(0) @binding(0) var u_texture: texture_2d<f32>;
@group(0) @binding(1) var u_sampler: sampler;

@group(1) @binding(0) var<uniform> u_mvp_matrix: mat4x4f;

@vertex
fn vs_main(in: vertex_input) -> vertex_output {
    var out: vertex_output;
    out.position = u_mvp_matrix * vec4f(in.position, 1.0);
    out.color = in.color;
    out.uv = in.uv;
    return out;
}

@fragment
fn fs_main(in: vertex_output) -> @location(0) vec4f {
    let color = textureSample(u_texture, u_sampler, in.uv).rgba;
    return color * in.color;
}
