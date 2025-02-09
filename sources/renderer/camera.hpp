#pragma once

#include <glm/glm.hpp>

class Camera
{
public:
	Camera();
	glm::mat4 getViewMatrix();
	void move(glm::vec3 direction);
	void rotate(glm::vec2 rotation);

private:
	void updateCamera();

private:
	glm::vec3 m_position;
	glm::vec3 m_front;
	glm::vec3 m_right;
	glm::vec3 m_up;
	float m_yaw = -90.0f;
	float m_pitch = 00.0f;
	const float SPEED = 0.001f;
	const float RSPEED = 0.1f;
};