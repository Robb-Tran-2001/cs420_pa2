#version 150

in vec3 viewPosition;
in vec3 viewNormal;

out vec4 c;

uniform vec4 La;
uniform vec4 Ld;
uniform vec4 Ls;
uniform vec3 viewLightDirection;
uniform vec4 ka;
uniform vec4 kd;
uniform vec4 ks;
uniform float alpha;
uniform sampler2D textureImage;

void main()
{
  vec3 eyedir = normalize(vec3(0, 0, 0) - viewPosition);
  vec3 reflectDir = -reflect(viewLightDirection, viewNormal);
  float d = max(dot(viewLightDirection, viewNormal), 0.0f);
  float s = max(dot(reflectDir, eyedir), 0.0f);
  c = ka * La + d * kd * Ld + pow(s, alpha) * ks * Ls;
  //  c = texture(textureImage, tc);
}