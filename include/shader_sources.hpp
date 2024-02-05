#ifndef SHADER_SOURCES_HPP
#define SHADER_SOURCES_HPP

#include <string>

static constexpr auto triangleMeshVertexShader = R"(
#version 460

layout (location = 0) in vec3 p;
layout (location = 1) in vec3 n;
layout (location = 2) in vec2 uv;

uniform mat4 M;
uniform mat4 V;
uniform mat4 P;

out vec3 v_p;
out vec3 v_n;
out vec2 v_uv;

void main(void) {
  mat4 MV = V * M;
  v_p = vec3(MV * vec4(p, 1));
  v_n = transpose(inverse(mat3(MV))) * n;
  v_uv = uv;
  gl_Position = P * vec4(v_p, 1);
}

)";

static constexpr auto triangleMeshFragShader = R"(
#version 460

#define MAX_LIGHTS 100

uniform struct Material {
  vec3 Ka, Kd, Ks;
  float Ns, Ni, d;
} material;

uniform struct Light {
  vec3 color;
  mat4 transform;
} lights[64];

uniform uint lightCount;
uniform mat4 V;
uniform vec3 ambient;
uniform bool selected;
uniform bool toneMap;
uniform bool wireframe;
uniform bool desaturate;
uniform bool textured;

uniform sampler2D tex;

in vec3 v_p;
in vec3 v_n;
in vec2 v_uv;

out vec4 fragColor;

void main(void) {
  if (wireframe) {
    fragColor = vec4(0, 0.5, 1, 1);
    if (selected)
      fragColor = 1.0 - fragColor;
    return;
  }

  const vec3 lightColor = vec3(100);
  const vec3 lightPosition = vec3(0);

  vec3 n = normalize(v_n);
  vec3 v = normalize(-v_p);

  vec3 color = ambient * material.Ka;
  for (int i = 0; i < lightCount; ++i) {
    vec3 lp = vec3(
        V * lights[i].transform * vec4(0, 0, 0, 1));
    vec3 l = lp - v_p;
    float invd = 1.0 / length(l);
    l *= invd;

    float diffuse = 0.5 * dot(l, n) + 0.5;
    diffuse *= diffuse;

    vec3 r = normalize(2.0 * dot(n, l) * n - l);
    float specular = pow(max(dot(v, r), 0.0), material.Ns);

    vec3 t = vec3(1);
    if (textured)
      t = texture(tex, v_uv).xyz;

    color += diffuse  * material.Kd * t * lights[i].color * invd * invd
           + specular * material.Ks * lights[i].color * invd * invd;
  }

  if (desaturate) {
    float luma = dot(vec3(0.2126, 0.7152, 0.0722), color);
    float desat = smoothstep(0.0, 1.0, luma);
    color = mix(color, vec3(luma), desat);
  }

  if (toneMap)
    color = color / (color + 1.0);

  if (selected) {
    vec3 fresnel = vec3(1, 0.5, 0);
    float d = 1.0 - abs(dot(v, n));
    color = mix(color, pow(d, 2.0) * fresnel, d);
  }

  fragColor = vec4(color, 1);
}

)";

#endif // SHADER_SOURCES_HPP