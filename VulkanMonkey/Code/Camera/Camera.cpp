#include "Camera.h"
#include "../GUI/GUI.h"

using namespace vm;

Camera::Camera()
{
	// gltf is right handed, reversing the x orientation makes the models left handed
	// reversing the y axis to match the vulkan y axis too
	worldOrientation = vec3(-1.f, -1.f, 1.f);

	// total pitch, yaw, roll
	euler = vec3(0.f, radians(180.f), 0.f);
	orientation = quat(euler);
	position = vec3(0.f, 0.f, 0.f);

	nearPlane = 500.0f;
	farPlane = 0.005f;
	FOV = 45.0f;
	speed = 0.35f;
	rotationSpeed = 0.05f;

	renderArea.viewport.x = GUI::winPos.x;
	renderArea.viewport.y = GUI::winPos.y;
	renderArea.viewport.width = GUI::winSize.x;
	renderArea.viewport.height = GUI::winSize.y;
	renderArea.viewport.minDepth = 0.f;
	renderArea.viewport.maxDepth = 1.f;
	renderArea.scissor.offset.x = static_cast<int32_t>(GUI::winPos.x);
	renderArea.scissor.offset.y = static_cast<int32_t>(GUI::winPos.y);
	renderArea.scissor.extent.width = static_cast<int32_t>(GUI::winSize.x);
	renderArea.scissor.extent.height = static_cast<int32_t>(GUI::winSize.y);
}

void vm::Camera::update()
{
	front = orientation * worldFront();
	right = orientation * worldRight();
	up = orientation * worldUp();
	previousView = view;
	updatePerspective();
	updateView();
	invView = inverse(view);
	invProjection = inverse(projection);
	invViewProjection = invView * invProjection;
	ExtractFrustum();
}

void vm::Camera::updatePerspective()
{
	const float aspect = renderArea.viewport.width / renderArea.viewport.height;
	const float tanHalfFovy = tan(radians(FOV) * .5f);
	const float m00 = 1.f / (aspect * tanHalfFovy);
	const float m11 = 1.f / (tanHalfFovy);
	const float m22 = farPlane / (farPlane - nearPlane) * worldOrientation.z;
	const float m23 = worldOrientation.z;
	const float m32 = -(farPlane * nearPlane) / (farPlane - nearPlane);
	projection = mat4(
		m00, 0.f, 0.f, 0.f,
		0.f, m11, 0.f, 0.f,
		0.f, 0.f, m22, m23,
		0.f, 0.f, m32, 0.f
	);
}

void vm::Camera::updateView()
{
	const vec3& r = right;
	const vec3& u = up;
	const vec3& f = front;

	const float m30 = -dot(r, position);
	const float m31 = -dot(u, position);
	const float m32 = -dot(f, position);

	view = mat4(
		r.x, u.x, f.x, 0.f,
		r.y, u.y, f.y, 0.f,
		r.z, u.z, f.z, 0.f,
		m30, m31, m32, 1.f
	);
}

void Camera::move(RelativeDirection direction, float velocity)
{
	if (direction == RelativeDirection::FORWARD)	position += front * (velocity * worldOrientation.z);
	if (direction == RelativeDirection::BACKWARD)	position -= front * (velocity * worldOrientation.z);
	if (direction == RelativeDirection::RIGHT)		position += right * velocity;
	if (direction == RelativeDirection::LEFT)		position -= right * velocity;
}

void Camera::rotate(float xoffset, float yoffset)
{
	yoffset *= rotationSpeed;
	xoffset *= rotationSpeed;
	euler.x += radians(-yoffset) * worldOrientation.y;	// pitch
	euler.y += radians(xoffset) * worldOrientation.x;	// yaw

	orientation = quat(euler);
}

vec3 Camera::worldRight() const
{
	return vec3(worldOrientation.x, 0.f, 0.f);
}

vec3 Camera::worldUp() const
{
	return vec3(0.f, worldOrientation.y, 0.f);
}

vec3 Camera::worldFront() const
{
	return vec3(0.f, 0.f, worldOrientation.z);
}

void Camera::ExtractFrustum()
{
	// transpose just to make the calculations look simpler
	mat4 pvm = transpose(projection * view);
	vec4 temp;

	/* Extract the numbers for the RIGHT plane */
	temp = pvm[3] - pvm[0];
	temp /= length(vec3(temp));

	frustum[0].normal = vec3(temp);
	frustum[0].d = temp.w;

	/* Extract the numbers for the LEFT plane */
	temp = pvm[3] + pvm[0];
	temp /= length(vec3(temp));

	frustum[1].normal = vec3(temp);
	frustum[1].d = temp.w;

	/* Extract the BOTTOM plane */
	temp = pvm[3] + pvm[1];
	temp /= length(vec3(temp));

	frustum[2].normal = vec3(temp);
	frustum[2].d = temp.w;

	/* Extract the TOP plane */
	temp = pvm[3] - pvm[1];
	temp /= length(vec3(temp));

	frustum[3].normal = vec3(temp);
	frustum[3].d = temp.w;

	/* Extract the FAR plane */
	temp = pvm[3] - pvm[2];
	temp /= length(vec3(temp));

	frustum[4].normal = vec3(temp);
	frustum[4].d = temp.w;

	/* Extract the NEAR plane */
	temp = pvm[3] + pvm[2];
	temp /= length(vec3(temp));

	frustum[5].normal = vec3(temp);
	frustum[5].d = temp.w;
}

// center x,y,z - radius w 
bool Camera::SphereInFrustum(const vec4& boundingSphere) const
{
	for (unsigned i = 0; i < 6; i++) {
		const float dist = dot(frustum[i].normal, vec3(boundingSphere)) + frustum[i].d;

		if (dist < -boundingSphere.w)
			return false;

		if (fabs(dist) < boundingSphere.w)
			return true;
	}
	return true;
}

void Camera::SurfaceTargetArea::update(const vec2& position, const vec2& size, float minDepth, float maxDepth)
{
	viewport.x = position.x;
	viewport.y = position.y;
	viewport.width = size.x;
	viewport.height = size.y;
	viewport.minDepth = minDepth;
	viewport.maxDepth = maxDepth;

	scissor.offset.x = static_cast<int32_t>(position.x);
	scissor.offset.y = static_cast<int32_t>(position.y);
	scissor.extent.width = static_cast<int32_t>(size.x);
	scissor.extent.height = static_cast<int32_t>(size.y);
}
