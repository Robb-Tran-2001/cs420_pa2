#version 150

in vec3 position;
in vec4 color;
out vec4 col;

uniform int mode;
in vec3 pLeftPos, pRightPos, pDownPos, pUpPos;

uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;

void main()
{
  // compute the transformed and projected vertex position (into gl_Position) 
  // compute the vertex color (into col)
	if (mode == 0) {
		gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0f);
		col = color;
	}
	else if (mode == 1) {
		//float smoothHeight = float(position.y);
		float smoothHeight = float(pLeftPos.y + pRightPos.y + pDownPos.y + pUpPos.y) / 4.0f;
		vec4 colorTemp = max(color, vec4(0.0000001f)) / max(position.y, 0.0000001f) * smoothHeight;
		colorTemp.a = 1.0f;
		gl_Position = projectionMatrix * modelViewMatrix * vec4(position.x, smoothHeight, position.z, 1.0f);
		col = colorTemp;
	}
  
}

