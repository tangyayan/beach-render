#ifndef LIGHT_H
#define LIGHT_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <string>
#include <shader.h>

#include <cube.h>

struct DirLight {
    // glm::vec3 position;
	glm::vec3 direction;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};  

struct PointLight {
	glm::vec3 position;

	float constant;
	float linear;
	float quadratic;

	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;
};
class Light
{
private:
	DirLight parallel_light;
	std::vector<PointLight> point_lights;
	Shader& LightboxShader;
public:
	Light(DirLight parallel_light, Shader& lightbox):parallel_light(parallel_light), LightboxShader(lightbox){}
	void AddPointLight(PointLight light)
	{
		point_lights.push_back(light);
	}
	void Draw(Camera& cam, glm::vec4 clipping_plane)
	{
		std::vector<Texture> textures;//Load a null texture
		Cube lightcube(textures, LightboxShader);
		
		for (int i = 0; i < point_lights.size(); i++)
		{
			// printf("Drawing light at position: %f, %f, %f\n", point_lights[i].position.x, point_lights[i].position.y, point_lights[i].position.z);
			glm::mat4 model(1.0f);
			model = glm::translate(model, point_lights[i].position);
			model = glm::scale(model, glm::vec3(2.0f, 2.0f, 2.0f));
			lightcube.Draw(cam, clipping_plane, model ,false);
		}
	};
	void SetLight(Shader& SomeEntities)
	{
		SomeEntities.use();
		SomeEntities.setVec3("dirLight.direction", parallel_light.direction);
		SomeEntities.setVec3("dirLight.ambient", parallel_light.ambient);
		SomeEntities.setVec3("dirLight.diffuse", parallel_light.diffuse);
		SomeEntities.setVec3("dirLight.specular", parallel_light.specular);

		std::string s_lights = "pointLights[";
		std::string s_position = "].position";
		std::string s_constant = "].constant";
		std::string s_linear = "].linear";
		std::string s_quadratic = "].quadratic";
		std::string s_ambient = "].ambient";
		std::string s_diffuse = "].diffuse";
		std::string s_specular = "].specular";
		for (unsigned int i = 0; i < point_lights.size(); i++)
		{
			SomeEntities.setVec3(s_lights + std::to_string(i) +  s_position, point_lights[i].position);
			SomeEntities.setFloat(s_lights + std::to_string(i) +  s_constant, point_lights[i].constant);
			SomeEntities.setFloat(s_lights + std::to_string(i) + s_linear, point_lights[i].linear);
			SomeEntities.setFloat(s_lights + std::to_string(i) + s_quadratic, point_lights[i].quadratic);
			SomeEntities.setVec3(s_lights + std::to_string(i) + s_ambient, point_lights[i].ambient);
			SomeEntities.setVec3(s_lights + std::to_string(i) + s_diffuse, point_lights[i].diffuse);
			SomeEntities.setVec3(s_lights + std::to_string(i) + s_specular, point_lights[i].specular);
		}
	}
	glm::vec3 GetDirLightDirection()
	{
		return parallel_light.direction;
	}

	unsigned int numOfPointLight()
	{
		return point_lights.size();
	}

	PointLight* getPointLightAt(unsigned int idx)
	{
		return &(point_lights[idx]);
	}

	DirLight* getDirectionLight()
	{
		return &parallel_light;
	}

	glm::mat4 GetLightSpaceMatrix()
	{
		float near_plane = 1.0f, far_plane = 1000.0f;
		glm::mat4 lightProjection = glm::ortho(-500.0f, 500.0f, -500.0f, 500.0f, near_plane, far_plane);
		glm::mat4 lightView = glm::lookAt(-parallel_light.direction*500.0f, 
                                  glm::vec3( 0.0f, 0.0f,  0.0f), 
                                  glm::vec3( 0.0f, 1.0f,  0.0f));
		glm::mat4 lightSpaceMatrix = lightProjection * lightView;
		return lightSpaceMatrix;
	}

    void UpdateDirLight(const glm::vec3& direction,
                        const glm::vec3& ambient,
                        const glm::vec3& diffuse,
                        const glm::vec3& specular)
    {
        parallel_light.direction = direction;
        parallel_light.ambient  = ambient;
        parallel_light.diffuse  = diffuse;
        parallel_light.specular = specular;
    }
    Shader& GetLightboxShader()
    {
        return LightboxShader;
    }
};

#endif // LIGHT_H