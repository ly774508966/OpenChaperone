#include "ImageCompositor.hpp"
#include "ImageCompositor.hpp"
#include "OpenChaperoneGlobals.hpp"

using namespace cv;

ImageCompositor::ImageCompositor(const int width, const int height) {

	messageFont.loadFont("fonts/ProggyClean.ttf", 26);
	
	//ofDisableArbTex();

	ofFbo::Settings windowFboSettings = ofFbo::Settings();
	windowFboSettings.width = width;
	windowFboSettings.height = height;
	windowFboSettings.internalformat = GL_RGB; //Can use GL_BGR?
	windowFboSettings.textureTarget = GL_TEXTURE_2D;
	windowFboSettings.numSamples = fboNumSamples;

	ofFbo::Settings imageFboSettings = windowFboSettings;
	imageFboSettings.width = width / 2;

	//ofSetLogLevel(OF_LOG_SILENT);
	windowFbo.allocate(windowFboSettings);
	leftFbo.allocate(imageFboSettings);
	rightFbo.allocate(imageFboSettings);
	//ofSetLogLevel(OF_LOG_NOTICE);

	//ofEnableArbTex();

	distortion = DistortionManager();
	distortion.Init();

}

ImageCompositor::~ImageCompositor() {
	windowFbo.destroy();
	leftFbo.destroy();
	rightFbo.destroy();
}

void ImageCompositor::DrawGUI() {
	if (showGUI) {
		ImGui::Begin("Compositor Settings", &showGUI, ImGuiWindowFlags_AlwaysAutoResize);
		ImGui::Checkbox("Swap Sides", &swapStereoSides);
		ImGui::Checkbox("Mirror Images", &mirror);
		if (ImGui::SliderInt("Convergence", &convergence, convergenceMin, convergenceMax)) {
			//Constrain to [convergenceMin, convergenceMax] in case the user enters some crazy value
			convergence = CLAMP(convergence, convergenceMin, convergenceMax);
		}
		ImGui::End();
	}
}

void ImageCompositor::DrawWindowFbo() {
	/*if (doDistortion) {
		distortion.RenderStereoTargets(leftFbo, rightFbo);
	}*/
	TS_START_NIF("Draw Window FBO");
	ofClear(0, 0, 0);
	windowFbo.begin();
		ofClear(0, 20, 20);
		//Draw the FBOs in here
		leftFbo.draw(0, 0, windowFbo.getWidth() / 2, leftFbo.getHeight());
		rightFbo.draw(windowFbo.getWidth() / 2, 0, windowFbo.getWidth() / 2, rightFbo.getHeight());
	windowFbo.end();
	//windowFbo.draw(0, 0, windowFbo.getWidth(), windowFbo.getHeight());
	//Fit windowFbo to ofWindow aspect
	ofRectangle windowRect = ofRectangle(0, 0, ofGetWindowWidth(), ofGetWindowHeight());
	ofRectangle windowFboRect = ofRectangle(0, 0, windowFbo.getWidth(), windowFbo.getHeight());
	ofRectangle scaled = AspectFitRectToTarget(windowFboRect, windowRect);
	windowFbo.draw(scaled);
	TS_STOP_NIF("Draw Window FBO");

	if (doDistortion) {
		//distortion.RenderStereoTargets(leftFbo, rightFbo);
		distortion.RenderDistortion(leftFbo, rightFbo);
	}
}

void ImageCompositor::DrawMatsToFbo(const cv::InputArray leftMat, cv::InputArray rightMat)
{
	//Draw two images side by side
	if (swapStereoSides) {
		DrawMatToFbo(rightMat, leftFbo, convergence);
		DrawMatToFbo(leftMat, rightFbo, -convergence);
	}
	else {
		DrawMatToFbo(leftMat, leftFbo, convergence);
		DrawMatToFbo(rightMat, rightFbo, -convergence);
	}
}

void ImageCompositor::DrawMatToFbo(const cv::InputArray input, ofFbo fbo, int convergence)
{
	cv::Mat mat = input.getMat();
	//Draw warning message if the mat is empty
	if (mat.empty()) {
		fbo.begin();
			ofClear(180, 0, 0, 1.0f);
			ofSetColor(255);
			string warning = "Camera is not initialized";
			messageFont.drawString(warning, 
							(fbo.getWidth() / 2) - (messageFont.stringWidth(warning) / 2), 
							(fbo.getHeight() / 2) - (messageFont.stringHeight(warning) / 2)
			);
		fbo.end();
		//fbo.draw(0, 0, ofGetWindowWidth(), ofGetWindowHeight());
		return;
	}

	//OpenCV conversion
	//cv::Mat convertedMat;
	//(mat, convertedMat, COLOR_BGR2RGB);

	//Draw the one image into the window fbo
	TS_START_NIF("Draw Mat");
	fbo.begin();
		ofClear(0, 180, 0, 1.0f);
		//Draw image from mat
		ofImage output;
		//ofTexture output;
		TS_START_NIF("Allocate");
		//TODO: Optimization here. Preallocate output and only reallocate if the mat is a different size
		output.allocate(mat.cols, mat.rows, OF_IMAGE_COLOR);
		//output.allocate(mat.cols, mat.rows, GL_RGB);
		TS_STOP_NIF("Allocate");

		TS_START_NIF("BGR to RGB Convert");
		cv::Mat converted;
		cvtColor(mat, converted, COLOR_BGR2RGB);
		output.setFromPixels(converted.data, converted.cols, converted.rows, OF_IMAGE_COLOR);
		//output.loadData(mat.data, mat.cols, mat.rows, GL_BGR_EXT);
		TS_STOP_NIF("BGR to RGB Convert");

		if (mirror) {
			TS_START_NIF("Mirror Output");
			cv::Mat flipped;
			cv::flip(converted, flipped, 1);
			output.setFromPixels(flipped.data, flipped.cols, flipped.rows, OF_IMAGE_COLOR);
			output.mirror(false, true); //Twice as slow
			TS_STOP_NIF("Mirror Output");
		}
		
		TS_START_NIF("Output Draw");
		//output.update();
		output.draw(0 + convergence, 0, fbo.getWidth(), fbo.getHeight());
		TS_STOP_NIF("Output Draw");
	fbo.end();
	TS_STOP_NIF("Draw Mat");

	//TS_START_NIF("Window Draw");
	//windowFbo.draw(0, 0, ofGetWindowWidth(), ofGetWindowHeight());
	//TS_STOP_NIF("Window Draw");
}

/**
* Draws a cv::Mat directly to the screen via OpenFrameworks.
*/
void ImageCompositor::DrawMatDirect(const cv::Mat& mat, ofImageType type, int x, int y, string caption = "") {
	DrawMatDirect(mat, type, x, y, mat.cols, mat.rows, caption);
}

/**
* Draws a cv::Mat directly to the screen via OpenFrameworks.
*/
void ImageCompositor::DrawMatDirect(const cv::Mat& mat, ofImageType type, int x, int y, int w, int h, string caption = "") {
	//Draw background box
	int borderWidth = 2;
	ofSetColor(255, 255, 255); //Box color
	ofDrawPlane(x - borderWidth, y - borderWidth, w + (borderWidth * 2), h + (borderWidth * 2) + 10);

	//Draw image from mat
	ofImage output;
	output.allocate(mat.cols, mat.rows, type);
	output.setFromPixels(mat.data, mat.cols, mat.rows, type);

	windowFbo.begin();
		output.draw(x, y, w, h);
	windowFbo.end();

	//Draw the caption if available
	ofSetColor(255, 255, 255); //Text color
	if (caption != "") {
		ofDrawBitmapString(caption, x, y + h + 10);
	}
}

void ImageCompositor::IncrementConvergence(int inc)
{
	convergence += inc;
	//Constrain to [convergenceMin, convergenceMax]
	convergence = CLAMP(convergence, convergenceMin, convergenceMax);

}

//Adapted from https://gist.github.com/tomasbasham/10533743
ofRectangle ImageCompositor::AspectFitRectToTarget(ofRectangle original, ofRectangle target) {
	ofRectangle scaledRect;

	float aspectWidth = target.width / original.width;
	float aspectHeight = target.height / original.height;
	float aspectRatio = std::min(aspectWidth, aspectHeight); //use max for aspect fill

	scaledRect.width = original.width * aspectRatio;
	scaledRect.height = original.height * aspectRatio;
	scaledRect.x = (target.width - scaledRect.width) / 2.0f;
	scaledRect.y = (target.height - scaledRect.height) / 2.0f;

	return scaledRect;
}

ofRectangle ImageCompositor::AspectFillRectToTarget(ofRectangle original, ofRectangle target) {
	ofRectangle scaledRect;

	float aspectWidth = target.width / original.width;
	float aspectHeight = target.height / original.height;
	float aspectRatio = std::max(aspectWidth, aspectHeight); //use min for aspect fit

	scaledRect.width = original.width * aspectRatio;
	scaledRect.height = original.height * aspectRatio;
	scaledRect.x = (target.width - scaledRect.width) / 2.0f;
	scaledRect.y = (target.height - scaledRect.height) / 2.0f;

	return scaledRect;
}

void ImageCompositor::Update()
{
}
