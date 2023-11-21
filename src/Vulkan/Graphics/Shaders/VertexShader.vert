#version 430

// Vertex input
layout(location = 0) in dvec2 pos;
layout(location = 1) in dvec2 vel;
layout(location = 2) in double mass;

// Fragment output
layout(location = 0) out vec2 screenPos;
layout(location = 1) out float outMass;

// Push constants
layout(push_constant) uniform PushConstants {
	dvec2 screenPos;
	dvec2 screenSize;
} push;

void main() {
	// Calculate the point's screen position
	screenPos = vec2((pos - push.screenPos) / push.screenSize);

	gl_Position = vec4(screenPos, 0.0, 1.0);

	// Calculate the point size
	gl_PointSize = 2.0 * float(sqrt(mass));

	// Set the output mass
	outMass = float(mass);
}