#version 120

uniform mat4 modelViewMatrix;
uniform mat4 mvpMatrix;
uniform mat3 normalMatrix;
uniform vec3 lightDirection;
uniform vec4 ambientColor;
uniform vec4 diffuseColor;

attribute vec3 position;
attribute vec3 normal;
attribute vec3 tangent;
attribute vec3 bitangent;
attribute vec2 texCoord;
attribute vec4 color;

varying vec2 TexCoord;
varying vec3 LightDir;
varying vec3 ViewDir;

varying vec4 C;

varying vec3 N;
varying vec3 t;
varying vec3 b;
varying vec3 v;

void main( void )
{
    gl_Position = mvpMatrix * vec4(position, 1);
    TexCoord = texCoord;

    N = normalize(normalMatrix * normal);
    t = normalize(normalMatrix * tangent);
    b = normalize(normalMatrix * bitangent);
    v = vec3(modelViewMatrix * vec4(position, 1));

    mat3 tbnMatrix = mat3(b.x, t.x, N.x,
                          b.y, t.y, N.y,
                          b.z, t.z, N.z);

    ViewDir = tbnMatrix * -v.xyz;
    LightDir = tbnMatrix * lightDirection.xyz;

    C = color;
}
