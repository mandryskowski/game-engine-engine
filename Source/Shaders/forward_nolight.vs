layout (location = 0) in vec3 vPosition;

//out
out vec2 texCoord1;
out vec2 texCoord2;
out float blend;

//uniform
uniform vec2 atlasData;	//x - texture id, y - number of columns in atlas
uniform vec2 atlasTexOffset;
uniform mat4 MVP;


void main()
{
	gl_Position = MVP * vec4(vPosition, 1.0);
	texCoord1 = vPosition.xy + vec2(0.5);
	texCoord2 = texCoord1;
	blend = 1.0;
	
	if (atlasData != vec2(0.0))
	{
		blend = atlasData.x - floor(atlasData.x);
		float texID = floor(atlasData.x);
		
		vec2 positionInAtlas = vec2(mod(texID, atlasData.y), floor(texID / atlasData.y));
		vec2 nextPositionInAtlas = vec2(mod((texID + 1.0), atlasData.y), floor((texID + 1.0) / atlasData.y));
		
		texCoord1 = (positionInAtlas * atlasTexOffset) + (texCoord1 * atlasTexOffset);
		texCoord2 = (nextPositionInAtlas * atlasTexOffset) + (texCoord2 * atlasTexOffset);
	}
}