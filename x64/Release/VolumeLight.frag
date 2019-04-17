#version 330

uniform sampler2D shadowMap;
uniform sampler2D depthTexture;

uniform vec3 LightPosition;
uniform mat4 ViewInv;
uniform mat4 ProjectionInv;
uniform mat4 ViewProjectionInv;
uniform mat4 ShadowViewProjection;
uniform vec3 EyePosition;


out vec4 outputColor;

void main()
{

	float depthTextureValue;
	float shadowValue;

	//The bias controls the offset... the bug we were getting was from it
    float bias =0.0001f;//0.0; //-0.005f;
	float steps = 0.1f;

	float yellow_value = 1.0;
	float yellow_steps = 0.0;
	
	//this is the texture with and height
	vec2 texCoord = vec2(gl_FragCoord.x/(1920/4),  (gl_FragCoord.y/(1080/4)));
	
	depthTextureValue = texture2D(depthTexture,texCoord.xy).x;
	depthTextureValue =  depthTextureValue ;
	float z = depthTextureValue*2.0 - 1.0;

	//get the position of the trace OPEN GL STYLE -1 1
	vec4 clipPosition = vec4(2.0*texCoord.x - 1.0, 2.0*texCoord.y - 1.0,z, 1.0);
	
	vec4 DepthPositionInWorldSpace = ProjectionInv*clipPosition;
	
	//get world coords
	DepthPositionInWorldSpace = ViewInv*DepthPositionInWorldSpace;
	DepthPositionInWorldSpace.w = 1.0/ DepthPositionInWorldSpace.w;
	DepthPositionInWorldSpace = vec4((DepthPositionInWorldSpace.xyz *DepthPositionInWorldSpace.w),1.0);
	
	
	//TODO: Find the pixel position and trace from there...
	//It's the same as the texCoord but at depth of 0;
	vec4 clipCoordPixel = vec4(2.0*texCoord.x - 1.0, 2.0*texCoord.y - 1.0, 0.0, 1.0);
	
	vec4 PixelPositionInViewSpace = ProjectionInv * clipCoordPixel;
	vec4 PixelPositionInWorldSpace = ViewInv * PixelPositionInViewSpace;
	
	PixelPositionInWorldSpace = vec4(PixelPositionInWorldSpace.xyz/PixelPositionInWorldSpace.w,1.0);
	//PixelPositionInWorldSpace = vec4(EyePosition.x, EyePosition.y,EyePosition.z,1.0);
	vec4 PixelVector = DepthPositionInWorldSpace - PixelPositionInWorldSpace;
	vec4 PixelVectorDirection = normalize(PixelVector);
	
	
	vec3 trace_steps = vec3(0.0f);
	float max_trace_steps = 0.0;
	//Prevent division by 0
	if(PixelVectorDirection.x != 0.0)
	{
	trace_steps.x = PixelVector.x/PixelVectorDirection.x;
	trace_steps.x = abs(trace_steps.x);
	}
	else if(PixelVectorDirection.y != 0.0)
	{
	trace_steps.y = PixelVector.y/PixelVectorDirection.y; 
	trace_steps.y = abs(trace_steps.y);
	}
	else if(PixelVectorDirection.z !=0.0)
	{
	trace_steps.z = PixelVector.z/PixelVectorDirection.z; 
	trace_steps.z = abs(trace_steps.z);
	}
	
	max_trace_steps = max(max(trace_steps.x,trace_steps.y),trace_steps.z);



	float max_steps = 256;
	float current_steps = 0;
	//Trace the positions
	if(max_trace_steps !=0)
	{
    yellow_steps = 1.0/max_steps;
	}
	
	
    float current_trace_steps = 0.0;
	//IMPORTANT: IT STEPS 0.1 UNIT AWAY IN WORLD SPACE. NOT PER PIXEL
	for(vec4 currentPosition = PixelPositionInWorldSpace; current_trace_steps <= max_trace_steps; currentPosition+=(PixelVectorDirection*steps))
	{
		current_trace_steps += steps;
	
		 vec4 currentPos = vec4(currentPosition.xyz,1.0);
		
		 //IMPORTANT: ALWAYS MULTIPLY THE MATRIX FIRST
		 vec4 PositionInShadowSpace = ShadowViewProjection * currentPos;
		 PositionInShadowSpace.w = 1.0/PositionInShadowSpace.w;
		 PositionInShadowSpace = vec4((PositionInShadowSpace.xyz * PositionInShadowSpace.w),1.0);

		 //at last transform to 0 to 1 in texture coords IMPORNTANT: Z MUST BE FROM 0 - 1
		 //IMPORNTANT: THE BIAS MATRIX DOES THIS, Keep it here for a reminder.
		 //PositionInShadowSpace.xyz = vec3((PositionInShadowSpace.x + 1.0)/2.0, (PositionInShadowSpace.y+ 1.0)/2.0,(PositionInShadowSpace.z+1.0)/2.0);

	     shadowValue = texture2D(shadowMap,PositionInShadowSpace.xy).x;
		 if(shadowValue  >= PositionInShadowSpace.z + bias)
		 {
				
			//decrease the blue value so it goes from white to yellow
			if(current_steps != max_steps)
			{ 
						
			yellow_value -= (yellow_steps);
			current_steps++;
			}
			else
			{
			
			break;
			}
		 }	
	  }	

	float transparency = 1.0 - yellow_value;
	//Get more yellowishness
	yellow_value = yellow_value*0.2;
	float red_value = 1.0;
	if(yellow_value <0.25)
	{
	yellow_value = 0.25f;
	red_value = yellow_value;
	}
    vec4 yellow = vec4(1.0,1.0 ,yellow_value,transparency);
	outputColor = yellow;
 
}
