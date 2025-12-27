#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include <glm/glm.hpp>
#include <Shader.h>
#include <FileSystem.h>
#include <Model.h>
#include <Light.h>

#include <list>
#include <terrain.h>

struct AABB {
	glm::vec3 min;
	glm::vec3 max;
	
	glm::vec3 GetCenter() const {
		return (min + max) * 0.5f;
	}
	
	glm::vec3 GetSize() const {
		return max - min;
	}
};

class GameObject
{
	// 全局变量
public:
	static std::list<GameObject*> gameObjList;
	static GameObject* selectedObject; // 当前选中的物体
	static GameObject* movingObject; // 当前正在移动的物体
	
public:
	glm::vec3 pos;
	float rotY;
	glm::vec3 sca;
	glm::mat4 modelMat;
	string modelPath;
	bool isSelected; // 是否被选中
	bool isGround; // 是否地面物体
	
private:
	Model* model;
	Light* light;

	// 获取世界空间的包围盒
	AABB GetWorldAABB() const
	{
		glm::vec3 localMin, localMax;
		model->GetBoundingBox(localMin, localMax);
		
		glm::vec3 corners[8] = {
			glm::vec3(localMin.x, localMin.y, localMin.z),
			glm::vec3(localMax.x, localMin.y, localMin.z),
			glm::vec3(localMin.x, localMax.y, localMin.z),
			glm::vec3(localMax.x, localMax.y, localMin.z),
			glm::vec3(localMin.x, localMin.y, localMax.z),
			glm::vec3(localMax.x, localMin.y, localMax.z),
			glm::vec3(localMin.x, localMax.y, localMax.z),
			glm::vec3(localMax.x, localMax.y, localMax.z),
		};
		
		glm::vec3 worldMin = glm::vec3(FLT_MAX);
		glm::vec3 worldMax = glm::vec3(-FLT_MAX);
		
		for (int i = 0; i < 8; i++)
		{
			glm::vec4 worldPos = modelMat * glm::vec4(corners[i], 1.0f);
			glm::vec3 worldPos3 = glm::vec3(worldPos);
			
			worldMin = glm::min(worldMin, worldPos3);
			worldMax = glm::max(worldMax, worldPos3);
		}
		
		AABB box;
		box.min = worldMin;
		box.max = worldMax;

		// std::cout << "Updated model bounding box: Min("
		// << worldMin.x << ", " << worldMin.y << ", " << worldMin.z << "), Max("
		// << worldMax.x << ", " << worldMax.y << ", " << worldMax.z
		// << ")\n";
		return box;
	}
	
public:
	GameObject(const std::string& modelPath,
		const glm::mat4& modelMat = glm::mat4(1.0f), 
		const glm::vec3 position = glm::vec3(0.0f),
		const float rotateY = 0.0f, 
		const glm::vec3 scale = glm::vec3(0.2f),
		bool isGround = true, bool gamma = false
	)
		: pos(position), rotY(rotateY), sca(scale), isSelected(false), isGround(isGround)
	{
		this->modelMat = modelMat;
		this->modelPath = modelPath;
		if (Model::modelList.find(FileSystem::getPath(modelPath)) == Model::modelList.end())
		{
			// 该模型未加载到List
			Model* model = new Model(modelPath, gamma);
			this->model = model;
			glm::vec3 min, max;
			model->GetBoundingBox(min, max);
			std::cout << "Loaded model bounding box: Min("
				 << min.x << ", " << min.y << ", " << min.z << "), Max("
				 << max.x << ", " << max.y << ", " << max.z << ")\n";
		}
		else
			// 模型已加载
			this->model = Model::modelList[FileSystem::getPath(modelPath)];
		gameObjList.push_back(this);
	}

	~GameObject()
	{
		if (selectedObject == this)
			selectedObject = nullptr;
		gameObjList.remove(this);
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
		const glm::vec4& clippling_plane = glm::vec4(0.0f, -1.0f, 0.0f, 999999.0f))
	{
		shader.use();
		shader.setMat4("projection", projectionMat);
		shader.setMat4("view", viewMat);
		shader.setVec4("clipPlane", clippling_plane);
		shader.setMat4("model", modelMat);
		
		// 如果被选中，可以设置高亮效果
		if (isSelected)
		{
			shader.setInt("isSelected", 1);
			shader.setVec3("selectColor", glm::vec3(1.0f, 1.0f, 0.0f)); // 黄色高亮
		}
		else
		{
			shader.setInt("isSelected", 0);
		}
		
		model->Draw(shader);
	}

	// 射线与AABB相交检测
    bool RayIntersect(const glm::vec3& rayOrigin, const glm::vec3& rayDir, 
                          float maxDistance = 1000.0f)
    {
        AABB box = GetWorldAABB();
        
        std::cout << "object: " << modelPath << "\n";
        std::cout << "World AABB Min: (" << box.min.x << ", " << box.min.y << ", " << box.min.z << ")\n";
        std::cout << "World AABB Max: (" << box.max.x << ", " << box.max.y << ", " << box.max.z << ")\n";
        std::cout << "AABB Center: (" << box.GetCenter().x << ", " << box.GetCenter().y << ", " << box.GetCenter().z << ")\n";
        std::cout << "AABB Size: (" << box.GetSize().x << ", " << box.GetSize().y << ", " << box.GetSize().z << ")\n";
        
        float tmin = 0.0f;
        float tmax = maxDistance;
        
        // 对每个轴进行 slab 测试
        for (int i = 0; i < 3; i++)
        {
            if (abs(rayDir[i]) < 0.0001f)
            {
                // 射线与该轴平行
                if (rayOrigin[i] < box.min[i] || rayOrigin[i] > box.max[i])
                {
                    return false;
                }
            }
            else
            {
                float t1 = (box.min[i] - rayOrigin[i]) / rayDir[i];
                float t2 = (box.max[i] - rayOrigin[i]) / rayDir[i];
                
                if (t1 > t2) std::swap(t1, t2);
                
                tmin = std::max(tmin, t1);
                tmax = std::min(tmax, t2);
                
                // std::cout << i << ": t1=" << t1 << ", t2=" << t2 
                //          << ", tmin=" << tmin << ", tmax=" << tmax << "\n";
                
                if (tmin > tmax)
                {
                    // std::cout << i << " fail(tmin > tmax)\n";
                    return false;
                }
            }
        }
        
        std::cout << " select! tmin=" << tmin << ", tmax=" << tmax << "\n";
        return true;
    }

	// 选中物体
	void Select()
	{
		// 取消之前选中的物体
		if (selectedObject != nullptr)
			selectedObject->isSelected = false;
		
		isSelected = true;
		selectedObject = this;
	}

	// 取消选中
	void Deselect()
	{
		isSelected = false;
		if (selectedObject == this)
			selectedObject = nullptr;
	}

	void snaptoterrain(Terrain* terrain)
	{
		if(!isGround)return;
		/*
		* 吸附到地面
		*/
		AABB box = GetWorldAABB();
		float obj_bottom_y = box.min.y;
		float ground_y = terrain->GetHeight(pos.x, pos.z);
		float offset = obj_bottom_y - ground_y - 0.01f;// 让物体稍微埋入地面一点点，防止浮空
		pos.y -= offset;
		Update();
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
GameObject* GameObject::selectedObject = nullptr;

#endif