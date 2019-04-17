#version 330 

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec2 vertUV;
layout (location = 2) in vec3 vertNormals;

uniform mat4 MVP;
uniform mat4 lightModelViewProjectionMatrix;

out vec2 UV;


out vec4 LightPositionTransformed;

out mat4 LightMVP;

void main() 
{ 
 
   gl_Position = MVP * vec4(vPosition,1.0);
  
   
   UV = vertUV;
   LightMVP = lightModelViewProjectionMatrix;
   LightPositionTransformed =lightModelViewProjectionMatrix *vec4(vPosition,1.0);
  



}