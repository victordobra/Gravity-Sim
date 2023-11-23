#version 430

// Vertex input
layout(location = 0) in vec2 pos;
layout(location = 1) in vec2 vel;
layout(location = 2) in float mass;

// Fragment output
layout(location = 0) out vec2 screenPos;
layout(location = 1) out float outMass;

// Push constants
layout(push_constant) uniform PushConstants {
	vec2 screenPos;
	vec2 screenSize;
} push;

void main() {
	// Calculate the point's screen position
	screenPos = (pos - push.screenPos) / push.screenSize;

	gl_Position = vec4(screenPos, 0.0, 1.0);

	// Calculate the point size
	gl_PointSize = 0.5 * sqrt(mass);

	// Set the output mass
	outMass = mass;
}