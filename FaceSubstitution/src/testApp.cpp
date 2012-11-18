#include "testApp.h"

using namespace ofxCv;

void testApp::setup() {
#ifdef TARGET_OSX
	ofSetDataPathRoot("../../../data/");
#endif
	ofSetVerticalSync(true);
	cloneReady = false;
	cam.initGrabber(640, 480);
	clone.setup(cam.getWidth(), cam.getHeight());
	ofFbo::Settings settings;
	settings.width = cam.getWidth();
	settings.height = cam.getHeight();
	maskFbo.allocate(settings);
	srcFbo.allocate(settings);
	camTracker.setup();
	srcTracker.setup();
	srcTracker.setIterations(25);
	srcTracker.setAttempts(4);
	
	displayMode = CAMERA_DISPLAY_MODE;
}

void testApp::update() {
	switch (displayMode) {
		case CAMERA_DISPLAY_MODE:
			cam.update();
			if(cam.isFrameNew()) {
				camTracker.update(toCv(cam));
				
				cloneReady = camTracker.getFound();
				if(cloneReady) {
					ofMesh camMesh = camTracker.getImageMesh();
					camMesh.clearTexCoords();
					camMesh.addTexCoords(srcPoints);
					
					maskFbo.begin();
					ofClear(0, 255);
					camMesh.draw();
					maskFbo.end();
					
					srcFbo.begin();
					ofClear(0, 255);
					src.bind();
					camMesh.draw();
					src.unbind();
					srcFbo.end();
					
					clone.setStrength(16);
					clone.update(srcFbo.getTextureReference(),
								 cam.getTextureReference(),
								 maskFbo.getTextureReference());
					
				}
			}
			break;
		case SOURCE_DISPLAY_MODE:
			break;
	}
}

void testApp::draw() {
	ofSetColor(255);
	
	switch (displayMode) {
		case CAMERA_DISPLAY_MODE:
			if(src.getWidth() > 0 && cloneReady) {
				clone.draw(0, 0);
			} else {
				cam.draw(0, 0);
			}
			
			if(!camTracker.getFound()) {
				drawHighlightString("camera face not found", 10, 10);
			}
			if(src.getWidth() == 0) {
				drawHighlightString("drag an image here", 10, 30);
			} else if(!srcTracker.getFound()) {
				drawHighlightString("image face not found", 10, 30);
			}
			break;
		
		case SOURCE_DISPLAY_MODE:
			if (src.getWidth() > 0) {
				src.draw(0, 0);
			}
			
			ofFill();
			for (int i = 0; i < srcPoints.size(); i++) {
				ofVec2f p = srcPoints[i];
				ofSetColor(255,128,0);
				ofCircle(p[0],p[1],6);
				ofSetColor(255,255,255);
				ofCircle(p[0],p[1],4);
			}
			break;
	}
}

void testApp::loadFace(string face){
	src.loadImage(face);
}

void testApp::dragEvent(ofDragInfo dragInfo) {
	loadFace(dragInfo.files[0]);
}

void testApp::mouseMoved(int x, int y ) {
	
}

void testApp::mouseDragged(int x, int y, int button) {
	if (button == 0) {
		if (selectedSrcPointIndex >= 0) {
			srcPoints[selectedSrcPointIndex].set(x,y);
		}
	}
}

void testApp::mousePressed(int x, int y, int button) {
	float nearestDsq = std::numeric_limits<float>::max();
	int nearestIndex = -1;
	for (int i = 0; i < srcPoints.size(); i++) {
		float dx = srcPoints[i][0] - x;
		float dy = srcPoints[i][1] - y;
		float dsq = dx * dx + dy * dy;
		if (dsq < nearestDsq) {
			nearestDsq = dsq;
			nearestIndex = i;
		}
	}
	
	if (nearestDsq < 25) {
		selectedSrcPointIndex = nearestIndex;
	}
	else {
		selectedSrcPointIndex = -1;
	}
}

void testApp::mouseReleased(int x, int y, int button) {
	
}

void testApp::keyPressed(int key) {
	ofFile file;
	ofBuffer buff;
	
	switch (key) {
		case 't': // Toggle display between camera and source image.
			displayMode = displayMode == CAMERA_DISPLAY_MODE ? SOURCE_DISPLAY_MODE : CAMERA_DISPLAY_MODE;
			break;
		
		case 'q': // Read point locations from source image.
			if(src.getWidth() > 0) {
				srcTracker.update(toCv(src));
				srcPoints = srcTracker.getImagePoints();
			}
			cout << "Calculated points from source image." << endl;
			break;
		
		case 'r': // Read points from points.tsv.
			file.open(ofToDataPath("points.tsv"), ofFile::ReadWrite, false);
			buff = file.readToBuffer();
			
			// Discard the header line.
			if (!buff.isLastLine()) buff.getNextLine();
			
			srcPoints = vector<ofVec2f>();
			
			while (!buff.isLastLine()) {
				string line = buff.getNextLine();
				vector<string> tokens = ofSplitString(line, "\t");
				srcPoints.push_back(ofVec2f(ofToFloat(tokens[0]), ofToFloat(tokens[1])));
			}
			cout << "Read points.tsv." << endl;
			break;
			
		case 's': // Save points to points.tsv.
			if (srcPoints.size() > 0) {
				ofBuffer points;
				string header = "x\ty\n";
				points.append(header.c_str(), header.size());
				
				for (int i = 0; i < srcPoints.size(); i++) {
					string srcPoint = ofToString(srcPoints[i][0]) + "\t" + ofToString(srcPoints[i][1]) + "\n";
					points.append(srcPoint.c_str(), srcPoint.size());
				}
				
				bool wrote = ofBufferToFile("points.tsv", points);
				cout << "Wrote points.tsv." << endl;
			}
			break;
	}
}
