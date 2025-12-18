#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include <glm/glm.hpp>
#include <Shader.h>
#include <FileSystem.h>
#include <Model.h>

#include <list>

class GameObject
{
	// 全局变量
public:
	static std::list<GameObject*> gameObjList; // 储存所有游戏物体
public:
	glm::vec3 pos; // 位移
	float rotY; // 自旋
	glm::vec3 sca; // 缩放
	glm::mat4 modelMat;
	string modelPath;
private:
	Model* model;
public:
	GameObject(const std::string& modelPath,
		const glm::mat4& modelMat = glm::mat4(1.0f), const glm::vec3 position = glm::vec3(0.0f),
		const float rotateY = 0.0f, const glm::vec3 scale = glm::vec3(0.2f))
		: pos(position), rotY(rotateY), sca(scale)
	{
		this->modelMat = modelMat;
		this->modelPath = modelPath;
		if (Model::modelList.find(FileSystem::getPath(modelPath)) == Model::modelList.end())
		{
			// 该模型未加载到List
			Model* model = new Model(modelPath);
			this->model = model;
		}
		else
			// 模型已加载
			this->model = Model::modelList[FileSystem::getPath(modelPath)];
		gameObjList.push_back(this);
	}

	~GameObject()
	{

	}

	void Update()
	{
		modelMat = glm::translate(glm::mat4(1.0f), pos); // 位移
		modelMat = glm::rotate(modelMat, rotY, glm::vec3(0.0f, 1.0f, 0.0f));
		modelMat = glm::scale(modelMat, sca); // 缩放
	}

	void Draw(Shader& shader,
		const glm::mat4& projectionMat,
		const glm::mat4& viewMat = glm::mat4(1.0f),
		const glm::vec4& clippling_plane = glm::vec4(0.0f, -1.0f, 0.0f, 999999.0f),
		bool isHit = false)
	{
		shader.use();
		shader.setMat4("projection", projectionMat);
		shader.setMat4("view", viewMat);
		// shader.setVec4("plane", clippling_plane);
		shader.setMat4("model", modelMat);
		model->Draw(shader);
	}

	static void Draw(Mesh& mesh, Shader& shader,
		const glm::mat4& projectionMat,
		const glm::mat4& viewMat = glm::mat4(1.0f),
		const glm::mat4& modelMat = glm::mat4(1.0f),
		const glm::vec4& clippling_plane = glm::vec4(0.0f, -1.0f, 0.0f, 999999.0f),
		bool isHit = false)
	{
		shader.use();
		shader.setMat4("projection", projectionMat);
		shader.setMat4("view", viewMat);
		// shader.setVec4("plane", clippling_plane);
		shader.setMat4("model", modelMat);
		mesh.Draw(shader);
	}
};

std::list<GameObject*> GameObject::gameObjList;

#endif // GAMEOBJECT_H