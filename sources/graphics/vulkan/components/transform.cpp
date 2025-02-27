#include "graphics/vulkan/components/transform.hpp"

#include <glm/gtc/matrix_transform.hpp>

glm::mat4 Transform::getMatrix()
{
	auto model = glm::translate(glm::mat4(1.0f), position);
	model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, scale);
	return model;
}