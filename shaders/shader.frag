#version 450

#extension GL_EXT_debug_printf : enable

layout(set=0, binding = 1) uniform LightObject{
  vec3 cameraPos;
  vec3 position;
  float intensity;
}light;

layout(set = 1,binding = 0) uniform sampler2D texSampler;
layout(set = 2,binding =0) uniform MaterialObject{
  vec3 uKd;
  vec3 uKs;
}material;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragPos;
layout(location = 3) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PushConstant {
  layout(offset = 64) vec3 color;
} pc;

void main() {
  vec3 color =pow(texture(texSampler, fragTexCoord).rgb, vec3(2.2));   
  vec3 ambient = 0.05 * color;
  vec3 lightDir = normalize(light.position - fragPos);
  vec3 normal = normalize(fragNormal);
  float diff = max(dot(lightDir, normal), 0.0);
  float light_atten_coff = light.intensity / length(light.position - fragPos);
  vec3 diffuse =  diff * light_atten_coff * color;
  vec3 viewDir = normalize(light.cameraPos - fragPos);
  float spec = 0.0;
  vec3 reflectDir = reflect(-lightDir, normal);
  spec = pow (max(dot(viewDir, reflectDir), 0.0), 35.0);
  vec3 specular =material.uKs * light_atten_coff * spec;  
  // debugPrintfEXT("Light intensity float is %f", light.intensity);
  outColor = vec4(pow((ambient + diffuse + specular), vec3(1.0/2.2)), 1.0);
}