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

varying vec3 v;

varying vec4 A;
varying vec4 C;
varying vec4 D;


void main( void )
{
    gl_Position = mvpMatrix * vec4(position, 1);
    TexCoord = texCoord;

    v = vec3(modelViewMatrix * vec4(position, 1));

    ViewDir = -v.xyz;
    LightDir = lightDirection;

    A = ambientColor;
    C = color;
    D = diffuseColor;
}
