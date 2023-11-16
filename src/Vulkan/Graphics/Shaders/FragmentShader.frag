#version 430

// Vertex input
layout(location = 0) in vec2 screenPos;

// Output
layout(location = 0) out vec4 outColor;

void main() {
	// Set the out color
	outColor = vec4(1.0, screenPos.x * 0.5 + 0.5, screenPos.y * 0.5 + 0.5, 1.0);
}