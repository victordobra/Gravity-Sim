#version 430

// Vertex input
layout(location = 0) in vec2 screenPos;
layout(location = 1) in float mass;

// Output
layout(location = 0) out vec4 outColor;

void main() {
	// Check if the current fragment position is outsize of the inner circle
	vec2 screenCoord = gl_PointCoord - vec2(0.5);
	if(dot(screenCoord, screenCoord) > 0.25)
		discard;

	// Set the out color
	outColor = vec4(1.0, screenPos.xy * 0.5 + vec2(0.5), 1.0);
}