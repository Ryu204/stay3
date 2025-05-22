struct vertex_input {
    @location(0) color: vec4f,
    @location(1) position: vec3f,
    @location(2) normal: vec3f,
    @location(3) uv: vec2f,
};

struct vertex_output {
    @builtin(position) position: vec4f,
    @location(0) color: vec4f,
    @location(1) normal: vec3f,
	@location(2) uv: vec2f,
};

struct material {
    @location(0) color: vec4f,
};

@group(0) @binding(0) var u_texture: texture_2d<f32>;
@group(0) @binding(1) var u_sampler: sampler;
@group(0) @binding(2) var<uniform> u_material: material;

@group(1) @binding(0) var<uniform> u_mvp_matrix: mat4x4f;

@vertex
fn vs_main(in: vertex_input) -> vertex_output {
    var out: vertex_output;
    out.position = u_mvp_matrix * vec4f(in.position, 1.0);
    out.color = in.color;
    out.normal = in.normal;
    out.uv = in.uv;
    return out;
}

@fragment
fn fs_main(in: vertex_output) -> @location(0) vec4f {
    let texture_color = textureSample(u_texture, u_sampler, in.uv).rgba;
    let unlit_color = texture_color * in.color * u_material.color;
    
    // I should make 2 shaders, this condition is used when render decal-like objects (text, hair, particle)
    // if (unlit_color.a < 0.5) { discard; }

    // Temporary lighting to check depth attachement
    // let light_dir_a = 1.5 * normalize(vec3f(0.5, 0.5, -1.0));
    // let light_color_a = vec3f(1.0, 0.9, 0.6);
    // let light_dir_b = normalize(vec3f(-1.0, 0.5, 0.5));
    // let light_color_b = vec3f(0.6, 0.9, 1.0);
    // let shading_a = max(0.0, dot(light_dir_a, in.normal));
    // let shading_b = max(0.0, dot(light_dir_b, in.normal));
    // let shading = shading_a * light_color_a + shading_b * light_color_b;
    // return vec4f(unlit_color.rgb * shading, 1.0);
    return unlit_color;
}
