#pragma once

#include "ofMain.h"

// 카메라의 현재 위치 및 fov(시야각)값을 받는 구조체 타입 지정. (구조체 타입은 ts interface 랑 비슷한 개념이라고 생각하면 될 것 같음.)
struct CameraData {
    glm::vec3 pos;
    float fov;
};

// 조명벡터 연산에 필요한 라이트(정확히는 Directional Light)의 데이터를 모아둔 구조체 타입 지정.
struct DirectionalLight {
    glm::vec3 direction; // 조명의 방향 (이 값을 c++ 에서 뒤집고, 정규화하여 셰이더에서 조명벡터로 사용할거임)
    glm::vec3 color; // 조명의 색상 (셰이더에서 계산된 노말벡터와 조명벡터의 내적값(= 디퓨즈 라이팅 값)과 곱해줄거임)
    float intensity; // 조명의 강도 (c++ 에서 조명의 색상과 곱해줘서 조명색의 밝기를 결정함.)
};

class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void mouseEntered(int x, int y);
		void mouseExited(int x, int y);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);
		
        ofMesh torusMesh; // torus.ply 모델링 파일을 로드해서 사용할 메쉬 객체 변수 선언
        ofShader specularShader; // torus.ply 에 '스펙큘러 라이팅 + 디퓨즈 라이팅'을 적용할 때 사용할 셰이더 객체 변수 선언
        CameraData cam; // 카메라 위치 및 fov(시야각)의 현재 상태값을 나타내는 구조체를 타입으로 갖는 멤버변수 cam 선언
};
