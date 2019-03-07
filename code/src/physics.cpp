#include <imgui\imgui.h>
#include <imgui\imgui_impl_sdl_gl3.h>
#include <glm\gtc\matrix_transform.hpp>
#include "ParticleSystem.h"
#include "Tools.h"

#pragma region Program

namespace Box {
	void drawCube();
}
namespace Axis {
	void drawAxis();
}

namespace Sphere {
	extern void updateSphere(glm::vec3 pos, float radius = 1.f);
	extern void drawSphere();
}
namespace Capsule {
	extern void updateCapsule(glm::vec3 posA, glm::vec3 posB, float radius = 1.f);
	extern void drawCapsule();
}
namespace Particles {
	extern const int maxParticles;
	extern void updateParticles(int startIdx, int count, float* array_data);
	extern void drawParticles(int startIdx, int count);
}
namespace Mesh {
	extern const int numCols;
	extern const int numRows;
	extern void updateMesh(float* array_data);
	extern void drawMesh();
}
namespace Fiber {
extern const int numVerts;
	extern void updateFiber(float* array_data);
	extern void drawFiber();
}
namespace Cube {
	extern void updateCube(const glm::mat4& transform);
	extern void drawCube();
}



// Boolean variables allow to show/hide the primitives
bool renderSphere = true;
bool renderCapsule = true;
bool renderParticles = true;
bool renderMesh = false;
bool renderFiber = false;
bool renderCube = false;

//You may have to change this code
void renderPrims() {
	Box::drawCube();
	Axis::drawAxis();


	if (renderSphere)
		Sphere::drawSphere();
	if (renderCapsule)
		Capsule::drawCapsule();

	if (renderParticles) {
		int startDrawingFromParticle = 0;
		int numParticlesToDraw = PARTICLE_COUNT;// Particles::maxParticles;
		Particles::drawParticles(startDrawingFromParticle, numParticlesToDraw);
	}

	if (renderMesh)
		Mesh::drawMesh();
	if (renderFiber)
		Fiber::drawFiber();

	if (renderCube)
		Cube::drawCube();
}


void GUI() {
	bool show = true;
	ImGui::Begin("Physics Parameters", &show, 0);

	// Do your GUI code here....
	{	
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);//FrameRate
		
	}
	// .........................
	
	ImGui::End();

	// Example code -- ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
	bool show_test_window = 1;
	if(show_test_window) {
		ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
		ImGui::ShowTestWindow(&show_test_window);
	}
}
#pragma endregion

float Magnitude(glm::vec3 a, glm::vec3 b);

// ------------------------------------------------------------------------------------------
#pragma region GlobalData
static float TIME_FACTOR = 0.2f;

static float GRAVITY_FORCE = 9.81f;
static float BOUNCE_ELASTICITY = 0.8f;
static float FRICTION_FACTOR = .2f;
static glm::vec3 GRAVITY_VECTOR = { 0,-1,0 };

static glm::vec3 SPHERE_POS = { 1,5,0 };
static float SPHERE_RAD = 1.5f;

static glm::vec3 CAPSULE_POS_A = { -2,1,1 };
static glm::vec3 CAPSULE_POS_B = { 2,1,1 };
static float CAPSULE_RAD = 1.f;
#pragma endregion

// Force Actuators
struct ForceActuator { 
	virtual glm::vec3 computeForce(float mass, const glm::vec3& position) = 0; 
};

struct GravityForce : ForceActuator {
	glm::vec3 computeForce(float mass, const glm::vec3& position) override {
		return GRAVITY_VECTOR * (mass * GRAVITY_FORCE);
	}
};

struct PositionalGravityForce : ForceActuator {
	glm::vec3 computeForce(float mass, const glm::vec3& position) override {

	}
};
// ------------------------------------------------------------------------------------------

// Colliders
struct Collider {
	virtual bool checkCollision(const glm::vec3& prev_pos, const glm::vec3& next_pos) = 0; 
	virtual void getPlane(glm::vec3& normal, float& d) = 0; 
	void computeCollision(const glm::vec3& old_pos, const glm::vec3& old_vel, glm::vec3& new_pos, glm::vec3& new_vel) 
	{
		if (checkCollision(old_pos, new_pos)) {
			glm::vec3 normal;
			float d;
			getPlane(normal, d);
			
			new_pos = old_pos - (1 + BOUNCE_ELASTICITY) * (glm::dot(normal, old_pos) + d) * normal;
			new_vel = old_vel - (1 + BOUNCE_ELASTICITY) * glm::dot(normal, old_vel) * normal;

			glm::vec3 nVel, tVel;
			nVel = glm::dot(normal, old_vel) * normal;
			tVel = old_vel - nVel;

			new_vel = old_vel - (FRICTION_FACTOR * tVel);
		}
	}
};
struct PlaneCol : Collider {
	//...
	glm::vec3 planePosition, planeNormal;
	PlaneCol(glm::vec3 pos, glm::vec3 norm) {
		planePosition = pos;
		planeNormal = norm;
	}
	bool checkCollision(const glm::vec3& prev_pos, const glm::vec3& next_pos) override {
		glm::vec3 norm;
		float d;
		getPlane(norm, d);

		float distance = -((next_pos.x * norm.x) + (next_pos.y * norm.y) + (next_pos.z * norm.z) + d);//glm::dot(norm, (next_pos - planePosition));

		if (distance <= 0) return true;
		else return false;

	}
	void getPlane(glm::vec3& normal, float& d) override {
		normal = planeNormal;
		d = glm::dot(-planeNormal, planePosition);
	}
};
struct SphereCol : Collider {
	//...
	glm::vec3 position, collisionPoint;
	float radius;

	SphereCol(glm::vec3 _pos, float _rad) {
		radius = _rad;
		position = _pos;
	}
	
	bool checkCollision(const glm::vec3& prev_pos, const glm::vec3& next_pos) override {
		glm::vec3 vector = next_pos - position;
		float magnitude = glm::sqrt(glm::pow(vector.x, 2) + glm::pow(vector.y, 2) + glm::pow(vector.z, 2));
		float distance = glm::abs(magnitude);
		
		if (distance <= radius) {
			collisionPoint = next_pos;
			return true;
		}
		return false;
	}
	void getPlane(glm::vec3& normal, float& d) override {
		normal = collisionPoint - position;
		float mag = glm::sqrt(glm::pow(normal.x, 2) + glm::pow(normal.y, 2) + glm::pow(normal.z, 2));
		normal = normal / mag;
		d = glm::dot(-normal, collisionPoint);
	}
};
struct CapsuleCol : Collider {
	//...
	glm::vec3 posA, posB, collisionPoint;
	float radius;

	CapsuleCol(glm::vec3 _posA, glm::vec3 _posB, float _rad) {
		posA = _posA;
		posB = _posB;
		radius = _rad;
	}
	bool checkCollision(const glm::vec3& prev_pos, const glm::vec3& next_pos) override {
		glm::vec3 colPos;
		float a;

		glm::vec3 ab = (posA - posB);
		float abMag = Magnitude(posA, posB);

		a = glm::clamp(((glm::dot((next_pos - posB), ab) / abMag) / abMag), 0.f, 1.f);
		colPos = posB + (posA - posB) * a;

		float distance = Magnitude(next_pos, colPos) - radius;

		if (distance <= 0) {
			collisionPoint = next_pos;
			return true;
		}
		return false;
	}
	void getPlane(glm::vec3& normal, float& d) override {

	}
};

void euler(float dt, ParticleSystem& particles, const std::vector<Collider*>& colliders, const std::vector<ForceActuator*>& force_acts) {
	for (int i = 0; i < PARTICLE_COUNT; i++) {
		glm::vec3 forces;
		for (int j = 0; j < force_acts.size(); j++) {
			forces += force_acts[j]->computeForce(GLOBAL_PARTICLE_MASS, particles.particlePositions[i]);
		}
		glm::vec3 accel = glm::vec3(forces.x / GLOBAL_PARTICLE_MASS, forces.y / GLOBAL_PARTICLE_MASS, forces.z / GLOBAL_PARTICLE_MASS);
		particles.particleVelocities[i] += (dt * accel);
		particles.particlePositions[i] += (dt * particles.particleVelocities[i]);

		for (int j = 0; j < colliders.size(); j++) {
			colliders[j]->computeCollision(particles.particlePositions[i], particles.particleVelocities[i], 
											particles.particlePositions[i], particles.particleVelocities[i]);
		}

	}
}

ParticleSystem ps = ParticleSystem();
std::vector<ForceActuator*> forces;
std::vector<Collider*> colliders;

void PhysicsInit() {
	// Do your initialization code here...
	for (int i = 0; i < PARTICLE_COUNT; i++) {
		float f = Tools::Random();
		glm::vec3 newPos(
			Tools::Map(Tools::Random(), 0, 1, -5, 5), 
			Tools::Map(Tools::Random(), 0, 1, 5, 10), 
			Tools::Map(Tools::Random(), 0, 1, -5, 5));

		//glm::vec3 newVel(
		//	Tools::Random() * 5,
		//	Tools::Random() * 5,
		//	Tools::Random() * 5);

		ps.SetParticle(i, newPos, glm::vec3(0));
	}

	forces.push_back(new GravityForce());

	colliders.push_back(new PlaneCol(glm::vec3(0, 0, 0), glm::vec3(0, -1, 0)));
	colliders.push_back(new PlaneCol(glm::vec3(0, 10, 0), glm::vec3(0, 1, 0)));
	colliders.push_back(new PlaneCol(glm::vec3(-5, 0, 0), glm::vec3(-1, 0, 0)));
	colliders.push_back(new PlaneCol(glm::vec3(5, 0, 0), glm::vec3(1, 0, 0)));
	colliders.push_back(new PlaneCol(glm::vec3(0, 0, -5), glm::vec3(0, 0, -1)));
	colliders.push_back(new PlaneCol(glm::vec3(0, 0, 5), glm::vec3(0, 0, 1)));
	colliders.push_back(new SphereCol(SPHERE_POS, SPHERE_RAD));
	colliders.push_back(new CapsuleCol(CAPSULE_POS_A, CAPSULE_POS_B, CAPSULE_RAD));

	Sphere::updateSphere(SPHERE_POS, SPHERE_RAD);
	Capsule::updateCapsule(CAPSULE_POS_A, CAPSULE_POS_B, CAPSULE_RAD);
	// ...................................
}

void PhysicsUpdate(float dt) {
	// Do your update code here...

	euler(dt * TIME_FACTOR, ps, colliders, forces);

	Particles::updateParticles(0, PARTICLE_COUNT, ps.ParticlesPtr());
	// ...........................
}

void PhysicsCleanup() {
	// Do your cleanup code here...
	// ............................
}

float Magnitude(glm::vec3 a, glm::vec3 b) {
	glm::vec3 x = b - a;
	return glm::sqrt(glm::pow(x.x, 2) + glm::pow(x.y, 2) + glm::pow(x.z, 2));
}