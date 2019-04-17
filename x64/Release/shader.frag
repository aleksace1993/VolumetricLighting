#version 330 


smooth in vec2 UV;



out vec4 outputColor; 

uniform sampler2D gSampler;
uniform sampler2D shadowMap;
uniform sampler2D volumeLightTexture;
uniform vec4 vColor;



smooth in vec4 LightPositionTransformed;



void main() 
{ 

vec4 positionTransformed =vec4((gl_FragCoord.xyz / gl_FragCoord.w), 1.0f);
float z = gl_FragCoord.z / gl_FragCoord.w;
	if(z > 20.0f)
	{//Depth in clip space
		//discard;
	}	
	if(gl_FragCoord.x > 1920.0f/2)
	{
	//discard;
	}
	
   //TODO: PUT BOTH in a different VOLUME LIGHT SHADER
   // Draw a fullscreen quad. 2 triangles -1/1
   //SAMPLE the values and put the color value in the texture.
   //sample from that texture on another QUAD in MAIN shader. with + VolumeValue(0.0,0.0,0.0,1.0)
   //glEnable (GL_BLEND); glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); on the object
 
 //Shadows
  float ShadowVisibility=0.45f;
   float ColorVisibility = 1.0f;
   //The bias controls the offset... the bug we were getting was from it
  // float bias =-0.000001f;
  float bias = -0.00001f;
  
  
  //This is for shadows
  vec4 LightPositionTransformed3 = LightPositionTransformed;

  	//ALWAYS DIVIDE BY W in the fragment shader. the gl_position is automatically divided
	//Multiplication is faster than division.
  LightPositionTransformed3.w = 1/LightPositionTransformed3.w;
  LightPositionTransformed3 = vec4((LightPositionTransformed3.xyz*LightPositionTransformed3.w) ,1.0f);
  //LightPositionTransformed3.xyz = vec3((LightPositionTransformed3.x + 1.0)/2.0, (LightPositionTransformed3.y+ 1.0)/2.0,(LightPositionTransformed3.z +1.0)/2.0);
   
   vec2 poissonDisk[4] = vec2[](
  vec2( -0.94201624, -0.39906216 ),
  vec2( 0.94558609, -0.76890725 ),
  vec2( -0.094184101, -0.92938870 ),
  vec2( 0.34495938, 0.29387760 )
);
  
     
  float depthValue;

  for(int i = 0; i < 4; i++)
  {
    depthValue = texture2D(shadowMap,LightPositionTransformed3.xy+ poissonDisk[i]/1200.0).x;
    //the depth bit       //the distance from light
	if(depthValue  > LightPositionTransformed3.z+ bias)
    {
    //There is light
      ShadowVisibility += 0.25f;
    }
  }
 

 
//480-270 is the volumeLight tex resolution

//OUTPUT
vec4 vTexColor = texture2D(gSampler,UV);
vec2 texCoord = vec2(gl_FragCoord.x/(1920),gl_FragCoord.y/(1080));
vec4 vVolumeColor = texture2D(volumeLightTexture,texCoord.xy);


 //divide it so it can come out as not being double
 outputColor = vColor*(vTexColor/vVolumeColor)*ShadowVisibility*vVolumeColor;

 //outputColor = vColor*vTexColor*ShadowVisibility;
// outputColor = vVolumeColor;
}
