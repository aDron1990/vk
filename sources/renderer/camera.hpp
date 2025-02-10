#pragma once

#include <glm/glm.hpp>

class Camera
{
public:
	Camera();
	void move(glm::vec3 direction);
	void rotate(glm::vec2 rotation);
	glm::mat4 getViewMatrix();
	glm::vec3 getPosition();

private:
	void updateCamera();

private:
	glm::vec3 m_position;
	glm::vec3 m_front;
	glm::vec3 m_right;
	glm::vec3 m_up;
	float m_yaw = -90.0f;
	float m_pitch = 00.0f;
	const float SPEED = 5.0f;
	const float RSPEED = 150.0f;
};