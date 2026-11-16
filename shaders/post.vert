#version 330 core
// Full-screen triangle made from gl_VertexID, used by the menu blur.
out vec2 vUv;

void main()
{
	vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);   // (0,0) (2,0) (0,2)
	vUv = p;
	gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
}
