#include <algorithm>
#include <wx/wx.h>
#include <wx/glcanvas.h>
#include <wx/dcclient.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#ifdef MACOS
	#include <OpenGL/gl.h>
#else
	#include <GL/gl.h>
#endif
#include "spinning_cube_view.h"

namespace {

struct Vec3 {
	float x;
	float y;
	float z;
};

Vec3 subtract(const Vec3 &a, const Vec3 &b) {
	return {a.x - b.x, a.y - b.y, a.z - b.z};
}

Vec3 cross(const Vec3 &a, const Vec3 &b) {
	return {
		a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z,
		a.x * b.y - a.y * b.x
	};
}

float length(const Vec3 &value) {
	return sqrtf(value.x * value.x + value.y * value.y + value.z * value.z);
}

Vec3 normalize(const Vec3 &value) {
	const float valueLength = length(value);
	if(valueLength <= 0.0001f)
		return {0.0f, 0.0f, 1.0f};

	return {value.x / valueLength, value.y / valueLength, value.z / valueLength};
}

void applyLookAt(const Vec3 &eye, const Vec3 &target, const Vec3 &upHint) {
	const Vec3 forward = normalize(subtract(target, eye));
	const Vec3 right = normalize(cross(forward, upHint));
	const Vec3 up = cross(right, forward);

	const float matrix[] = {
		right.x, up.x, -forward.x, 0.0f,
		right.y, up.y, -forward.y, 0.0f,
		right.z, up.z, -forward.z, 0.0f,
		0.0f,    0.0f, 0.0f,       1.0f
	};

	glMultMatrixf(matrix);
	glTranslatef(-eye.x, -eye.y, -eye.z);
}

class PreviewCanvas : public wxGLCanvas {
	public:
		PreviewCanvas(wxWindow *parent, const int *attribList) :
			wxGLCanvas(parent, wxID_ANY, attribList),
			context(new wxGLContext(this))
		{
			SetMinSize(wxSize(420, 260));
			Bind(wxEVT_PAINT, &PreviewCanvas::paint, this);
			Bind(wxEVT_SIZE, &PreviewCanvas::resize, this);
			Bind(wxEVT_LEFT_DOWN, &PreviewCanvas::onLeftDown, this);
			Bind(wxEVT_LEFT_UP, &PreviewCanvas::onLeftUp, this);
			Bind(wxEVT_RIGHT_DOWN, &PreviewCanvas::onRightDown, this);
			Bind(wxEVT_RIGHT_UP, &PreviewCanvas::onRightUp, this);
			Bind(wxEVT_MOTION, &PreviewCanvas::onMouseMove, this);
			Bind(wxEVT_MOUSEWHEEL, &PreviewCanvas::onMouseWheel, this);
			Bind(wxEVT_LEFT_DCLICK, &PreviewCanvas::onDoubleClick, this);
			resetView();
		}

		~PreviewCanvas() override {
			delete context;
		}

		void setPreparedPrintJob(const PreparedPrintJob &newJob, size_t newCompletedCommandCount) {
			job = newJob;
			completedCommandCount = newCompletedCommandCount;
			resetView();
			Refresh(false);
		}

	private:
		void resetView() {
			yaw = 35.0f;
			pitch = 55.0f;
			panX = 0.0f;
			panY = 0.0f;

			const float dx = job.maxX - job.minX;
			const float dy = job.maxY - job.minY;
			const float dz = job.maxZ - job.minZ;
			sceneCenterX = (job.minX + job.maxX) * 0.5f;
			sceneCenterY = (job.minY + job.maxY) * 0.5f;
			sceneCenterZ = (job.minZ + job.maxZ) * 0.5f;
			sceneRadius = std::max(10.0f, sqrtf(dx * dx + dy * dy + dz * dz) * 0.5f);
			distance = std::max(60.0f, (sceneRadius / tanf(22.5f * 3.14159265f / 180.0f)) * 1.35f);
		}

		void resize(wxSizeEvent &event) {
			Refresh(false);
			event.Skip();
		}

		void onLeftDown(wxMouseEvent &event) {
			leftDragging = true;
			lastMousePosition = event.GetPosition();
			CaptureMouse();
		}

		void onLeftUp(wxMouseEvent &event) {
			leftDragging = false;
			if(HasCapture())
				ReleaseMouse();
		}

		void onRightDown(wxMouseEvent &event) {
			rightDragging = true;
			lastMousePosition = event.GetPosition();
			CaptureMouse();
		}

		void onRightUp(wxMouseEvent &event) {
			rightDragging = false;
			if(HasCapture())
				ReleaseMouse();
		}

		void onMouseMove(wxMouseEvent &event) {
			const wxPoint position = event.GetPosition();
			const wxPoint delta = position - lastMousePosition;
			lastMousePosition = position;

			if(leftDragging) {
				yaw += delta.x * 0.45f;
				pitch = std::max(-89.0f, std::min(89.0f, pitch + delta.y * 0.45f));
				Refresh(false);
			}

			else if(rightDragging) {
				panX += delta.x * sceneRadius * 0.004f;
				panY -= delta.y * sceneRadius * 0.004f;
				Refresh(false);
			}
		}

		void onMouseWheel(wxMouseEvent &event) {
			const int rotation = event.GetWheelRotation();
			const float factor = rotation > 0 ? 0.88f : 1.14f;
			distance = std::max(sceneRadius * 0.3f, distance * factor);
			Refresh(false);
		}

		void onDoubleClick(wxMouseEvent &event) {
			resetView();
			Refresh(false);
		}

		void drawGrid(float size, int steps) {
			glLineWidth(1.0f);
			glBegin(GL_LINES);
			for(int i = -steps; i <= steps; i++) {
				const float offset = (size * i) / steps;
				const bool axisLine = i == 0;
				glColor3f(axisLine ? 0.28f : 0.16f, axisLine ? 0.34f : 0.16f, axisLine ? 0.40f : 0.18f);
				glVertex3f(sceneCenterX - size, sceneCenterY + offset, 0.0f);
				glVertex3f(sceneCenterX + size, sceneCenterY + offset, 0.0f);
				glVertex3f(sceneCenterX + offset, sceneCenterY - size, 0.0f);
				glVertex3f(sceneCenterX + offset, sceneCenterY + size, 0.0f);
			}
			glEnd();
		}

		void drawAxes(float size) {
			glLineWidth(2.0f);
			glBegin(GL_LINES);
				glColor3f(0.90f, 0.25f, 0.25f);
				glVertex3f(sceneCenterX, sceneCenterY, 0.0f);
				glVertex3f(sceneCenterX + size, sceneCenterY, 0.0f);
				glColor3f(0.20f, 0.78f, 0.38f);
				glVertex3f(sceneCenterX, sceneCenterY, 0.0f);
				glVertex3f(sceneCenterX, sceneCenterY + size, 0.0f);
				glColor3f(0.24f, 0.56f, 0.94f);
				glVertex3f(sceneCenterX, sceneCenterY, 0.0f);
				glVertex3f(sceneCenterX, sceneCenterY, size);
			glEnd();
		}

		void drawSegments(bool extrudingSegments, bool completedSegments) {
			if(extrudingSegments)
				glLineWidth(completedSegments ? 3.8f : 2.0f);
			else
				glLineWidth(completedSegments ? 1.8f : 1.0f);

			glBegin(GL_LINES);
			for(size_t i = 0; i < job.previewSegments.size(); i++) {
				const PrintPreviewSegment &segment = job.previewSegments[i];
				if(segment.extruding != extrudingSegments)
					continue;

				const bool completed = segment.commandIndex < completedCommandCount;
				if(completed != completedSegments)
					continue;

				if(segment.extruding) {
					if(completed)
						glColor3f(1.00f, 0.44f, 0.10f);
					else
						glColor3f(0.40f, 0.25f, 0.10f);
				}
				else {
					if(completed)
						glColor3f(0.52f, 0.72f, 1.00f);
					else
						glColor3f(0.16f, 0.20f, 0.26f);
				}

				glVertex3f(segment.startX, segment.startY, segment.startZ);
				glVertex3f(segment.endX, segment.endY, segment.endZ);
			}
			glEnd();
		}

		void paint(wxPaintEvent &event) {
			wxPaintDC dc(this);
			if(!context)
				return;

			SetCurrent(*context);
			const wxSize size = GetClientSize();
			const int width = std::max(size.GetWidth(), 1);
			const int height = std::max(size.GetHeight(), 1);
			const float aspect = static_cast<float>(width) / height;

			glViewport(0, 0, width, height);
			glEnable(GL_DEPTH_TEST);
			glClearColor(0.05f, 0.06f, 0.09f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			const float nearPlane = std::max(1.0f, distance * 0.05f);
			const float farPlane = std::max(1000.0f, distance * 10.0f + sceneRadius * 4.0f);
			const float frustumHalfHeight = tanf(30.0f * 3.14159265f / 180.0f) * nearPlane;
			const float frustumHalfWidth = frustumHalfHeight * aspect;

			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glFrustum(-frustumHalfWidth, frustumHalfWidth, -frustumHalfHeight, frustumHalfHeight, nearPlane, farPlane);

			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			const float yawRadians = yaw * 3.14159265f / 180.0f;
			const float pitchRadians = pitch * 3.14159265f / 180.0f;
			const Vec3 target = {sceneCenterX + panX, sceneCenterY + panY, sceneCenterZ};
			const Vec3 eye = {
				target.x + distance * cosf(pitchRadians) * cosf(yawRadians),
				target.y + distance * cosf(pitchRadians) * sinf(yawRadians),
				target.z + distance * sinf(pitchRadians)
			};
			applyLookAt(eye, target, {0.0f, 0.0f, 1.0f});

			const float gridSize = std::max(50.0f, sceneRadius * 2.0f);
			drawGrid(gridSize, 10);
			drawAxes(std::max(15.0f, sceneRadius * 0.4f));
			// Draw future path first as a subdued preview, then overlay completed path.
			drawSegments(false, false);
			drawSegments(true, false);
			drawSegments(false, true);
			drawSegments(true, true);

			glFlush();
			SwapBuffers();
		}

		wxGLContext *context = nullptr;
		PreparedPrintJob job;
		size_t completedCommandCount = 0;
		float yaw = 35.0f;
		float pitch = 55.0f;
		float distance = 150.0f;
		float panX = 0.0f;
		float panY = 0.0f;
		float sceneCenterX = 0.0f;
		float sceneCenterY = 0.0f;
		float sceneCenterZ = 0.0f;
		float sceneRadius = 50.0f;
		bool leftDragging = false;
		bool rightDragging = false;
		wxPoint lastMousePosition;
};

}

SpinningCubeView::SpinningCubeView(wxWindow *parent) :
	wxPanel(parent, wxID_ANY)
{
	wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	const int glAttributes[] = {WX_GL_RGBA, WX_GL_DOUBLEBUFFER, WX_GL_DEPTH_SIZE, 16, 0};
	if(wxGLCanvas::IsDisplaySupported(glAttributes))
		content = new PreviewCanvas(this, glAttributes);
	else
		content = new wxStaticText(this, wxID_ANY, "3D preview unavailable on this system");

	sizer->Add(content, dynamic_cast<PreviewCanvas *>(content) ? 1 : 0, dynamic_cast<PreviewCanvas *>(content) ? wxEXPAND : wxALIGN_CENTER | wxALL, dynamic_cast<PreviewCanvas *>(content) ? 0 : 16);
}

void SpinningCubeView::setPreparedPrintJob(const PreparedPrintJob &job, size_t completedCommandCount) {
	PreviewCanvas *canvas = dynamic_cast<PreviewCanvas *>(content);
	if(canvas)
		canvas->setPreparedPrintJob(job, completedCommandCount);
}
