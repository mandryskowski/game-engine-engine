//in
in vec3 nearPos;
in vec3 farPos;
in mat4 fragVP;

//out
layout (location = 0) out vec4 fragColor;
layout (location = 1) out vec4 blurColor;

//uniform
uniform float near;
uniform float far;
uniform bvec3 bExtraLinesAxis;
uniform vec3 extraLinesPos;


// Credit: Marie http://asliceofrendering.com/page2/ 
vec4 grid(vec3 fragPos, float scale, bool bRenderAxis)
{
	vec3 fragCoord = fragPos * scale;
	vec3 derivative = fwidth(fragCoord);
	vec2 grid = abs(fract(fragCoord.xz - 0.5) - 0.5) / derivative.xz;
	//float yAxisLine = 	
	float line = min(grid.x, grid.y);
    float minimumx = min(derivative.x, 1);
    float minimumy = min(derivative.y, 1);
	float minimumz = min(derivative.z, 1);
    vec4 color = vec4(0.2, 0.2, 0.2, 1.0 - min(line, 1.0));
	/*
	// x axis
    if(fragPos.z > -0.1 * minimumz && fragPos.z < 0.1 * minimumz)
	{
        color.x = 1.0;
		color.a = min(1.0, color.a * 2.0);
		
		if (!bRenderAxis)
			color = vec4(0.0);
	}
		
	// y axis
    //if(fragPos.y > -0.1 * minimumy && fragPos.y < 0.1 * minimumy)
   //	color.y = 1.0;
		
    // z axis
    if(fragPos.x > -0.1 * minimumx && fragPos.x < 0.1 * minimumx)
	{
        color.z = 1.0;
		color.a = min(1.0, color.a * 2.0);
		
		if (!bRenderAxis)
			color = vec4(0.0);
	} */
		
    return color;
}

float computeDepth(vec3 fragPos)
{
    vec4 clipPos = fragVP * vec4(fragPos, 1.0);
    return (clipPos.z / clipPos.w) * 0.5 + 0.5;
}

float linearizeDepth(float clipDepth)
{
	clipDepth = clipDepth * 2.0 - 1.0;
	float linearDepth = (2.0 * near * far) / (far + near - clipDepth * (far - near));
	return linearDepth / far;
}
/*
void main()
{
	float t = -nearPos.y / (farPos.y - nearPos.y);
	vec3 fragPos = nearPos + t * (farPos - nearPos);
	float fragDepth = computeDepth(fragPos);
	
	fragColor = (grid(fragPos, 10.0, true) + grid(fragPos, 1.0, false) + grid(fragPos, 0.1, false)) * float(t > 0.0);
	//fragColor = (grid(fragPos, 10.0, true) + grid(fragPos, 1.0, false)) * float(t > 0.0);
	//fragColor = (grid(fragPos, 0.1, true)) * float(t > 0.0);

	
	
	if (true || t <= 0.0)
	{
		vec2 lineOffsetXZ = vec2(0.0);
		vec2 test = (lineOffsetXZ - nearPos.xz) / (farPos.xz - nearPos.xz);
		if (test.x > 0.0 && test.y > 0.0)
		{
			vec3 testPos = nearPos + min(test.x, test.y) * (farPos - nearPos);
			vec3 fragCoord = testPos * 10.0;
			vec3 derivative = fwidth(fragCoord);
			vec2 grid = abs(fract(fragCoord.xz - 0.5) - 0.5) / derivative.xz;
			//float yAxisLine = 	
			float line = min(grid.x, grid.y);
			float minimumx = min(derivative.x, 1);
			float minimumy = min(derivative.y, 1);
			float minimumz = min(derivative.z, 1);
			
			if ((abs(testPos.z - lineOffsetXZ.y) < 0.1 * minimumz) && (abs(testPos.x - lineOffsetXZ.x) < 0.1 * minimumx))
			{
				fragColor = vec4(0.0, 1.0, 0.0, 1.0);
				fragDepth = computeDepth(testPos);
				//fragDepth = 0.0;
			}
		}
	}
		
	float fade = max(0.0, (0.15 - linearizeDepth(fragDepth)));
	fragColor.a *= fade;
	
	blurColor = vec4(0.0);
	if (fragColor.a == 0.0)
		discard;
		
	gl_FragDepth = fragDepth;
}*/

void lineImpl(vec2 posAxis, vec2 fragCoordAxis, vec2 lineOffsetAxis, inout vec4 fragColor, inout float fragDepth, vec3 pos3D, vec4 lineColor)
{
	vec2 minimum = min(fwidth(fragCoordAxis), vec2(1.0));
	
	if ((abs(posAxis.x - lineOffsetAxis.x) < 0.1 * minimum.x) && (abs(posAxis.y - lineOffsetAxis.y) < 0.1 * minimum.y))
	{
		fragColor = lineColor;
		fragDepth = computeDepth(pos3D);
	}
}
void lines(vec3 tVec, vec3 lineOffset, inout vec4 fragColor, inout float fragDepth, bvec3 bLines)
{
	if (bLines.x && tVec.y > 0.0 && tVec.z > 0.0)	// X line
	{
		vec3 pos = nearPos + min(tVec.y, tVec.z) * (farPos - nearPos);
		lineImpl(pos.yz, pos.yz * 10.0, lineOffset.yz, fragColor, fragDepth, pos, vec4(1.0, 0.3, 0.3, 2.0));
	}
	if (bLines.y && tVec.x > 0.0 && tVec.z > 0.0)	// Y line
	{
		vec3 pos = nearPos + min(tVec.x, tVec.z) * (farPos - nearPos);
		lineImpl(pos.xz, pos.xz * 10.0, lineOffset.xz, fragColor, fragDepth, pos, vec4(0.3, 1.0, 0.3, 2.0));
	}
	if (bLines.z && tVec.x > 0.0 && tVec.y > 0.0)	// Z line
	{
		vec3 pos = nearPos + min(tVec.x, tVec.y) * (farPos - nearPos);
		lineImpl(pos.xy, pos.xy * 10.0, lineOffset.xy, fragColor, fragDepth, pos, vec4(0.3, 0.3, 1.0, 2.0));
	}
}

void main()
{
	vec3 tVec = -nearPos / (farPos - nearPos);
	vec3 planePos = nearPos + tVec.y * (farPos - nearPos);
	float fragDepth = computeDepth(planePos);
	
	fragColor = (grid(planePos, 10.0, true) + grid(planePos, 1.0, false) + grid(planePos, 0.1, false)) * float(tVec.y > 0.0);
	
	lines(tVec, vec3(0.0), fragColor, fragDepth, bvec3(true));
	

	{
		vec3 tVec = (extraLinesPos - nearPos) / (farPos - nearPos);
		lines(tVec, extraLinesPos, fragColor, fragDepth, bExtraLinesAxis);
	}
		
	float fade = max(0.0, (0.15 - linearizeDepth(fragDepth)));
	fragColor.a *= fade;
	
	blurColor = vec4(0.0);
	if (fragColor.a == 0.0)
		discard;
		
	gl_FragDepth = fragDepth;
}