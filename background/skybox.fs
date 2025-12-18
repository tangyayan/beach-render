#version 330 core
out vec4 FragColor;
in vec3 TexCoords;

uniform samplerCube skybox;
uniform int isAbove;

void main()
{
    if(isAbove == 0&&TexCoords.y<.47)FragColor = vec4(0,0,.2,1);
    else FragColor = texture(skybox, TexCoords);
}