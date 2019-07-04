#include "stdafx.h"
#include "glm\glm.hpp"
#include "glm\gtc\constants.hpp"
#include "glm\gtc\matrix_transform.hpp"
#include "glm\gtx\quaternion.hpp"
#include "glm\gtx\euler_angles.hpp"
#include "glm\ext.hpp"

namespace Utils {

	static const float rad2deg = 57.295779513082320876798154814105f;
	static const float deg2rad = 0.01745329251994329576923690768489f;

void decomposeM(const float *m, float *r, float *s, float *t)
{
	float epsilon = 0.00001f;

	for (int i = 0; i < 3; ++i)
	{
		const float *v = m + i * 4;
		s[i] = sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
		if (s[i] < epsilon)
			s[i] = 0.0f;
		if (abs(s[i] - 1.0) < epsilon)
			s[i] = 1.0f;

		t[i] = m[12 + i];
	}

	enum { ALL = 0, ONE = 1, NONE = 2 };
	int scaleType = ALL;
	if (abs(s[0] - s[1]) < epsilon && abs(s[1] - s[2]) < epsilon) {
		scaleType = ONE;
		if (abs(s[0] - 1.0f) < epsilon) {
			scaleType = NONE;
		}
	}


	glm::mat4 tm(
		m[0] / s[0], m[1] / s[0], m[2] / s[0], 0,
		m[4] / s[1], m[5] / s[1], m[6] / s[1], 0,
		m[8] / s[2], m[9] / s[2], m[10] / s[2], 0,
		0, 0, 0, 1
	);

	glm::quat q = glm::toQuat(tm);
	glm::vec3 e = glm::eulerAngles(q);
	
	r[0] = e.x * rad2deg;
	r[2] = e.y * rad2deg;
	r[1] = e.z * rad2deg;

	if ((abs(abs(r[0]) - 180.0f) < epsilon && abs(abs(r[1]) - 180.0f) < epsilon) ||
		(abs(abs(r[0]) - 180.0f) < epsilon && abs(abs(r[2]) - 180.0f) < epsilon) ||
		(abs(abs(r[1]) - 180.0f) < epsilon && abs(abs(r[2]) - 180.0f) < epsilon))
	{
		for (int i = 0; i < 3; ++i) {
			r[i] = r[i] + (r[i] < 0 ? +180.0f : -180.0f);
		}
	}

	for (int i = 0; i < 3; ++i)
	{
		while (r[i] < -180.0f)
			r[i] += 180.0f;

		while (r[i] > 180.0f)
			r[i] -= 180.0f;

		if (abs(r[i]) < epsilon)
			r[i] = 0.0f;

		if (abs(r[i] + 180.0f) < epsilon)
			r[i] = -180.0f;

		if (abs(r[i] - 180.0f) < epsilon)
			r[i] = 180.0f;
	}
}

void composeM(float *m, float *r, float *s, float *t)
{
	float nr[3] = {
	r[0] * deg2rad,
	r[2] * deg2rad,
	r[1] * deg2rad
	};
	static glm::mat4 I(1);
	auto S = glm::scale(I, *reinterpret_cast<glm::vec3 *>(s));
	auto R = glm::orientate4(*reinterpret_cast<glm::vec3 *>(nr));
	auto T = glm::translate(I, *reinterpret_cast<glm::vec3 *>(t));

	auto &M = *reinterpret_cast<glm::mat4 *>(m);
	M = T * R*S;
}

}; // Utils namespace