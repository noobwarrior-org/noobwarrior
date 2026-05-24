shader_type canvas_item;

uniform float blur_strength = 1.0; // Blur strength

uniform float blur_amount : hint_range(0.0, 5.0) = 2.0;
uniform sampler2D SCREEN_TEXTURE : hint_screen_texture, filter_linear_mipmap;

// Gaussian weight calculation
float gaussian(vec2 i, float sigma) {
    return exp(-0.5 * dot(i/sigma, i/sigma)) / (6.28318530718 * sigma * sigma);
}

void fragment() {
    vec2 pixel_size = blur_strength / SCREEN_PIXEL_SIZE;
    vec4 blur_color = vec4(0.0);
    float total_weight = 0.0;
    
    // Sample size - adjust for quality vs performance
    const int SAMPLE_COUNT = 15;
    float sigma = blur_amount;
    
    // Perform blur
    for (int x = -SAMPLE_COUNT; x <= SAMPLE_COUNT; x++) {
        for (int y = -SAMPLE_COUNT; y <= SAMPLE_COUNT; y++) {
            vec2 offset = vec2(float(x), float(y)) / pixel_size;
            float weight = gaussian(offset, sigma);
            blur_color += texture(SCREEN_TEXTURE, SCREEN_UV + offset) * weight;
            total_weight += weight;
        }
    }
    
    // Normalize the result
    COLOR = blur_color / total_weight;
}