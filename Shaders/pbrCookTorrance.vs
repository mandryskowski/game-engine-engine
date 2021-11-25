#if !defined(POINT_LIGHT) && !defined(DIRECTIONAL_LIGHT) && !defined(SPOT_LIGHT) && !defined(IBL_PASS)
#error Cannot compile Cook-Torrance shader without defining POINT_LIGHT, DIRECTIONAL_LIGHT, SPOT_LIGHT or IBL_PASS.
#endif

#if defined(DIRECTIONAL_LIGHT)
layout (location = 0) in vec2 vPosition;

void main()
{
	gl_Position = vec4(vPosition, 0.0, 1.0);
}
#else
layout (location = 0) in vec3 vPosition;

//uniform
uniform mat4 MVP;

void main()
{
	gl_Position = MVP * vec4(vPosition, 1.0);
}
#endif