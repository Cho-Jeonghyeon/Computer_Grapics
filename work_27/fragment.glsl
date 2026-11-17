#version 330 core
in vec3 out_Color;
in vec3 FragPos;
in vec3 Normal;
out vec4 FragColor;

uniform vec3 objectColor;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform bool lightingEnabled;

void main()
{
    vec3 materialColor = out_Color * objectColor;
    //--- 좌표축/궤도/조명 표시와 조명을 끈 도형은 원래 색으로 출력한다.
    if (!lightingEnabled) {
        FragColor = vec4(materialColor, 1.0);
        return;
    }

    //--- 강의의 Phong 조명: 주변광 + 산란반사 + 거울반사.
    vec3 ambient = 0.15 * lightColor;
    vec3 normalVector = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diffuseLight = max(dot(normalVector, lightDir), 0.0);
    vec3 diffuse = diffuseLight * lightColor;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, normalVector);
    float specularLight = 0.0;
    if (diffuseLight > 0.0)
        specularLight = pow(max(dot(viewDir, reflectDir), 0.0), 128.0);
    vec3 specular = 0.5 * specularLight * lightColor;
    vec3 result = (ambient + diffuse + specular) * materialColor;
    FragColor = vec4(result, 1.0);
}
