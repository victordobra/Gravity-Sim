#version 430

// Vertex input
layout(location = 0) in vec2 pos;
layout(location = 1) in vec2 vel;
layout(location = 2) in float mass;

// Push constants
layout(push_constant) uniform PushConstants {
	float systemSize;
} push;

void main() {
	// Calculate the point's screen position
	vec2 screenPos = pos / (push.systemSize * 0.5);

	gl_Position = vec4(screenPos, 0.0, 1.0);
	gl_PointSize = 1.0;
}