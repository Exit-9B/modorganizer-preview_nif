#version 150

uniform mat4 modelViewMatrix;
uniform mat4 mvpMatrix;
uniform mat3 normalMatrix;
uniform vec3 lightDirection;
uniform vec4 ambientColor;
uniform vec4 diffuseColor;

in vec3 position;
in vec3 normal;
in vec3 tangent;
in vec3 bitangent;
in vec2 texCoord;
in vec4 color;

out vec2 TexCoord;
out vec3 LightDir;
out vec3 ViewDir;

out vec3 N;
out vec3 t;
out vec3 b;
out vec3 v;

out vec4 A;
out vec4 C;
out vec4 D;

void main( void )
{
    vec4 boneIndex = vec4(0);
    vec4 boneWeight = vec4(0);
    gl_Position = mvpMatrix * vec4(position, 1.0);
    TexCoord = texCoord;

    N = normalize(normalMatrix * normal);
    t = normalize(normalMatrix * tangent);
    b = normalize(normalMatrix * bitangent);
    v = vec3(modelViewMatrix * vec4(position, 1.0));

    mat3 tbnMatrix = mat3(b.x, t.x, N.x,
                          b.y, t.y, N.y,
                          b.z, t.z, N.z);

    ViewDir = tbnMatrix * -v;
    LightDir = tbnMatrix * lightDirection;

    A = ambientColor;
    C = color;
    D = diffuseColor;
}
