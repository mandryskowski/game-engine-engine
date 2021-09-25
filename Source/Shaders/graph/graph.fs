#define GRAPH_MAX_DATA_POINTS 128
#define EDGE_SIZE 0.004
#define POINT_SIZE EDGE_SIZE * 3.0
#define EDGE_COLOR vec4(1.0, 0.5, 0.31, 1.0)
#define BELOW_EDGE_COLOR vec4(0.5, 0.25, 0.155, 1.0)
#define BACKGROUND_COLOR vec4(0.1, 0.05, 0.031, 1.0)
//in
in vec2 currentPoint;

//out
out vec4 fragColor;

//uniform
uniform vec2 dataPoints[GRAPH_MAX_DATA_POINTS];	//
uniform int dataPointsCount;	//WARNING: must be smaller than GRAPH_MAX_DATA_POINTS

uniform vec2 selectedRangeEndpoints;

#define lengthSquared(v) dot(v, v)

bool isOnEdge()
{
	if (currentPoint.x < dataPoints[0].x || currentPoint.x > dataPoints[dataPointsCount-1].x)
		return false;
		
	for (int i = 0; i < dataPointsCount - 1; i++)
	{
		vec2 o = dataPoints[i];
		vec2 u = normalize(dataPoints[i + 1] - dataPoints[i]);
		vec2 c = currentPoint;
		float max_d = distance(dataPoints[i], dataPoints[i+1]);
		
		float delta = pow(dot(u, o-c), 2.0) - (lengthSquared(o-c) - EDGE_SIZE*EDGE_SIZE);
		
		if (delta >= 0.0) // at least one real solution (square root of delta is real)
		{
			float d_without_delta = -dot(u, o-c);
			
			float d1 = d_without_delta + sqrt(delta);
			float d2 = d_without_delta - sqrt(delta);
			
			if ((d1 > 0.0 && d1 < max_d) || (d2 > 0.0 && d2 < max_d))
				return true;
		}
	}
	return false;
}

bool isOnPoint()
{
	for (int i = 0; i < dataPointsCount; i++)
		if (lengthSquared(currentPoint - dataPoints[i]) <= POINT_SIZE * POINT_SIZE)
			return true;
	return false;
}

bool isBelowEdge()
{
	vec2 closestPointLeft = vec2(-1.0, 0.0), closestPointRight = vec2(1.0, 0.0);	//x-component: distance to data point ||| y-component: data point index
	for (int i = 0; i < dataPointsCount; i++)
	{
		float distToPoint = dataPoints[i].x - currentPoint.x;
		if (distToPoint < 0.0 && closestPointLeft.x <= distToPoint)
			closestPointLeft = vec2(distToPoint, float(i));
		else if (distToPoint > 0.0 && closestPointRight.x > distToPoint)
			closestPointRight = vec2(distToPoint, float(i));
	}
		
	float T = abs(closestPointLeft.x) + closestPointRight.x;
	(T == 0.0) ? (T = 0.5) : (T = abs(closestPointLeft.x) / T);
	float interpolatedHeight = mix(dataPoints[int(closestPointLeft.y)].y, dataPoints[int(closestPointRight.y)].y, T);
	
	if (closestPointLeft.x == -1.0 || closestPointRight.x == 1.0)
		interpolatedHeight = -1.0;
		
	return currentPoint.y < interpolatedHeight;
}

bool isInSelectedRange()
{
	return step(selectedRangeEndpoints.x, currentPoint.x) * step(currentPoint.x, selectedRangeEndpoints.y) == 1.0;
}

void main()
{		
	if (dataPointsCount >= 2 && isOnEdge())
		fragColor = EDGE_COLOR;
	else if (dataPointsCount >= 1 && isOnPoint())
		fragColor = EDGE_COLOR;
	else if (dataPointsCount >= 2 && isBelowEdge())
		fragColor = BELOW_EDGE_COLOR;
	else
		fragColor = BACKGROUND_COLOR;
		
	if (abs(currentPoint.y - 0.5) < 0.01 ||
		abs(currentPoint.y - 0.995) < 0.01 ||
		abs(currentPoint.y - 0.005) < 0.01)
		fragColor.rgb += vec3(0.1);
		
	if (isInSelectedRange())
		fragColor.rgb += vec3(0.1);
}
/*void main()
{
	vec2 closestPointLeft = vec2(-1.0, 0.0), closestPointRight = vec2(1.0, 0.0);	//x-component: distance to data point ||| y-component: data point index
	for (int i = 0; i < dataPointsCount; i++)
	{
		float distToPoint = dataPoints[i].x - currentPoint.x;
		if (distToPoint < 0.0 && closestPointLeft.x < distToPoint)
			closestPointLeft = vec2(distToPoint, float(i));
		else if (distToPoint > 0.0 && closestPointRight.x > distToPoint)
			closestPointRight = vec2(distToPoint, float(i));
	}
		
	float T = abs(closestPointLeft.x) + closestPointRight.x;
	(T == 0.0) ? (T = 0.5) : (T = abs(closestPointLeft.x) / T);
	float interpolatedHeight = mix(dataPoints[int(closestPointLeft.y)].y, dataPoints[int(closestPointRight.y)].y, T);
	
	if (closestPointLeft.x == -1.0 || closestPointRight.x == 1.0)
		interpolatedHeight = -1.0;
		
	float gradient = (closestPointRight.y - closestPointLeft.y) / max(closestPointRight.x - closestPointLeft.x, 0.001);
	
	if (abs(currentPoint.y - interpolatedHeight) < 0.01)	// on curve
		fragColor = vec4(1.0, 0.5, 0.31, 1.0);
	else if (currentPoint.y < interpolatedHeight)			// under curve
		fragColor = vec4(0.5, 0.25, 0.155, 1.0);
	else													// above curve
		fragColor = vec4(0.1, 0.05, 0.031, 1.0);
	
	// Draw points
	if (distance(currentPoint, dataPoints[int(closestPointLeft.y)]) < 0.04)
		fragColor.rgb += vec3(1.0 - smoothstep(0.01, 0.02, distance(currentPoint, dataPoints[int(closestPointLeft.y)])));
	if (distance(currentPoint, dataPoints[int(closestPointRight.y)]) < 0.04)
		fragColor.rgb += vec3(1.0 - smoothstep(0.01, 0.02, distance(currentPoint, dataPoints[int(closestPointRight.y)])));
}*/