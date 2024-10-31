#version 430

// Vertex input
layout(location = 0) in vec2 pos;

// Push constants
layout(push_constant) uniform PushConstants {
	vec2 cameraPos;
	vec2 cameraSize;
} push;

void main() {
	// Calculate the point's screen position
	vec2 screenPos = (pos - push.cameraPos) / push.cameraSize;
	screenPos.y = -screenPos.y;

	gl_Position = vec4(screenPos, 0.0, 1.0);
	gl_PointSize = 1.0;
}