#include "ofApp.h"

// 조명계산 최적화를 위해, 쉐이더에서 반복계산하지 않도록, c++ 에서 한번만 계산해줘도 되는 작업들을 수행하는 보조함수들
glm::vec3 getLightDirection(DirectionalLight& l) { // 참조자 & 관련 설명 하단 필기 참고
    // 조명벡터 direction에 -1을 곱해서 조명벡터의 방향을 뒤집어주고, 셰이더에서 내적계산을 해주기 위해 길이를 1로 정규화해서 맞춰줌.
    return glm::normalize(l.direction * -1.0f);
}

glm::vec3 getLightColor(DirectionalLight& l) {
    // vec3 값인 조명색상에 float 값인 조명강도를 스칼라배로 곱해줘서 조명색상의 밝기를 지정함.
    return l.color * l.intensity;
}

//--------------------------------------------------------------
void ofApp::setup(){
    ofDisableArbTex(); // 스크린 픽셀 좌표를 사용하는 텍스쳐 관련 오픈프레임웍스 레거시 지원 설정 비활성화. (uv좌표계랑 다르니까!)
    ofEnableDepthTest(); // 깊이테스트를 활성화하여 z좌표값을 깊이버퍼에 저장해서 z값을 기반으로 앞뒤를 구분하여 렌더링할 수 있도록 함.
    // ofSetBackgroundColor(128, 128, 128); // 배경색을 회색으로 지정하고, 이와 유사한 색상의 환경광(앰비언트 라이트)이 적용되도록 할거임.
    
    /**
     참고로
     ofSetBackgroundColor() 함수는
     배경색을 지정할 때,
     
     0.0 ~ 1.0 사이의 정규화된 값이 아닌,
     0 ~ 255 사이의 실수로 r, g, b 값을 받음!
     */
    
    // 이번에는 ofApp 맨 처음 설정에서 shieldMesh 를 바라보기 적당한 카메라 위치와 시야각을 지정함.
    cam.pos = glm::vec3(0, 0.85f, 1.0f); // 카메라 위치는 z축으로 1만큼 안쪽으로 들어가게 하고, 조명 연산 결과를 확인하기 위해 y축으로도 살짝 올려줌
    cam.fov = glm::radians(90.0f); // 원근 프러스텀의 시야각은 일반 PC 게임에서는 90도 전후의 값을 사용함. -> 라디안 각도로 변환하는 glm 내장함수 radians() 를 사용함.
    
    shieldMesh.load("shield.ply"); // shieldMesh 메쉬로 사용할 모델링 파일 로드 (bin/data 디렉토리에 저장했으므로, 파일명만 넣어주면 됨.)
    blinnPhongShader.load("lit_mesh.vert", "blinnPhongTextures.frag"); // shieldMesh 에 텍스쳐를 활용한 Blinn-phong 반사모델을 적용하기 위한 셰이더 파일 로드
    diffuseTex.load("shield_diffuse.png"); // shieldMesh 의 조명계산에서 디퓨즈 라이팅 계산에 사용할 텍스쳐 로드
    specTex.load("shield_spec.png"); // shieldMesh 의 조명계산에서 스펙큘러 라이팅 계산에 사용할 텍스쳐 로드
}

//--------------------------------------------------------------
void ofApp::update(){

}

//--------------------------------------------------------------
void ofApp::draw(){
    using namespace glm; // 이제부터 현재 블록 내에서 glm 라이브러리에서 꺼내 쓸 함수 및 객체들은 'glm::' 을 생략해서 사용해도 됨.
    
    // 조명구조체 dirLight 에 조명데이터를 할당해 줌.
    DirectionalLight dirLight; // 조명데이터 구조체인 DirectionLight 타입의 객체 변수 dirLight 선언
    dirLight.direction = normalize(vec3(0.5, -1, -0.5)); // (0, 0, 0) 원점에서 (0.5, -1, -0.5) 으로 향하는 조명벡터 계산
    dirLight.color = vec3(1, 1, 1); // 조명색상은 평범하게 흰색으로 지정
    dirLight.intensity = 1.0f; // 조명강도도 1로 지정. 참고로, 1보다 큰값으로 조명강도를 조명색상에 곱해줘봤자, 프래그먼트 셰이더는 (1, 1, 1, 1) 이상의 색상값을 처리할 수 없음.
        
    // 카메라 변환시키는 뷰행렬 계산. 이동행렬 및 회전행렬 적용
    // -> 뷰행렬은 카메라 움직임에 반대방향으로 나머지 대상들을 움직이는 변환행렬이므로, 반드시 glm::inverse() 내장함수로 역행렬을 구해야 함.
    float cAngle = radians(-45.0f); // 카메라 변환을 위한 뷰행렬을 계산할 시, 회전행렬에 사용할 각도값을 -45도 틀어주도록 구해놓음.
    vec3 right = vec3(1, 0, 0); // 카메라 및 shield 모델의 회전행렬을 계산할 시, x축 방향으로만 회전할 수 있도록 회전축 벡터를 구해놓음.
    mat4 view = inverse(translate(cam.pos) * rotate(cAngle, right)); // 카메라 회전행렬도 x축 기준으로 cAngle(-45도) 회전시킴.
    
    // 모데행렬은 회전행렬 및 크기행렬만 적용.
    // 크기 * 회전 * 이동 순 곱셈을 열 우선 행렬에서는 역순으로 곱하여 이동 * 회전 * 크기 순으로 곱함. 이때, 이동변환은 없으므로, 회전 * 크기 순으로 곱함.
    static float rotAngle = 0.0f; // static 을 특정 함수 내에서 사용하는 것을 '정적 지역 변수'라고 하며, 이 할당문은 draw() 함수 최초 호출 시 1번만 실행됨. (하단 정적지역변수 관련 필기 참고)
    rotAngle += 0.01f;
    vec3 up = vec3(0, 1, 0); // y축 회전축 벡터 구함.
    mat4 rotation = rotate(radians(-45.0f), right) * rotate(rotAngle, up); // x축 기준으로 -45도, y축 기준으로 매 프레임마다 0.01도씩 중가하는 rotAngle 만큼 회전하는 회전행렬을 구함.
    mat4 model = rotation * scale(vec3(1.5, 1.5, 1.5)); // 열 우선 행렬이므로, 원하는 행렬 곱셈과 반대순서인 회전행렬 * 크기행렬 순으로 곱해줌
    
    // 투영행렬 계산
    float aspect = 1024.0f / 768.0f; // main.cpp 에서 정의한 윈도우 실행창 사이즈를 기준으로 원근투영행렬의 종횡비(aspect)값을 계산함.
    mat4 proj = perspective(cam.fov, aspect, 0.01f, 10.0f); // glm::perspective() 내장함수를 사용해 원근투영행렬 계산.
    
    // 최적화를 위해 c++ 단에서 투영 * 뷰 * 모델행렬을 한꺼번에 곱해서 버텍스 셰이더에 전송함.
    mat4 mvp = proj * view * model; // 열 우선 행렬이라 원래의 곱셈 순서인 '모델 -> 뷰 -> 투영'의 반대 순서로 곱해줘야 함.
    
    /**
     모델의 버텍스가 갖고있는 기본 노말벡터는 오브젝트공간을 기준으로 되어있음.
     그러나, 조명계산을 하려면 이러한 노말벡터를 월드공간으로 변환해야 함.
         
     그럼 노말벡터도 그냥 모델행렬인 mat4 model 을 버텍스 셰이더에서 곱해주면 되는거 아닌가?
     이렇게 할 수도 있지만, 만약 모델행렬에 '비일률적 크기행렬' (예를들어, (0.5, 1.0, 1.0) 이런거)
     가 적용되어 있다면, 특정 축마다 scale 이 다르게 늘어나는 과정에서 노말벡터의 방향이 휘어버리게 됨. -> p.190 참고
         
     이걸 똑바르게 세워주려면, '노멀행렬' 이라는 새로운 행렬이 필요함.
     노말행렬은 '모델행렬의 상단 3*3 역행렬의 전치행렬' 로 정의할 수 있음.
         
     역행렬, 전치행렬, 상단 3*3 행렬에 대한 각각의 개념은 위키백과, 구글링, 북마크한거 참고...
         
     어쨋든 위의 정의에 따라 아래와 같이 노말행렬을 구하고, 버텍스 셰이더로 쏴주면 됨.
     */
    mat3 normalMatrix = transpose(inverse(mat3(model)));
    

    // blinnPhongShader 를 바인딩하여 사용 시작
    blinnPhongShader.begin();
    
    blinnPhongShader.setUniformMatrix4f("model", model); // 버텍스 좌표를 월드좌표로 변환하기 위해 모델행렬만 따로 버텍스 셰이더 유니폼 변수로 전송
    blinnPhongShader.setUniformMatrix4f("mvp", mvp); // 위에서 한꺼번에 합쳐준 mvp 행렬을 버텍스 셰이더 유니폼 변수로 전송
    blinnPhongShader.setUniformMatrix3f("normalMatrix", normalMatrix); // 노말행렬을 버텍스 셰이더 유니폼 변수로 전송
    blinnPhongShader.setUniform3f("cameraPos", cam.pos); // 프래그먼트 셰이더에서 뷰 벡터를 계산하기 위해 카메라 좌표(카메라 월드좌표)를 프래그먼트 셰이더 유니폼 변수로 전송
    blinnPhongShader.setUniform3f("lightDir", getLightDirection(dirLight)); // 조명벡터를 음수화하여 뒤집어주고, 다시 정규화하여 길이를 1로 맞춘 뒤, 유니폼 변수로 전송
    blinnPhongShader.setUniform3f("lightCol", getLightColor(dirLight)); // 조명색상을 조명강도와 곱해준 뒤, 유니폼 변수로 전송
    blinnPhongShader.setUniform3f("ambientCol", glm::vec3(0.5, 0.5, 0.5)); // 배경색과 동일한 앰비언트 라이트 색상값을 유니폼 변수로 전송.
    blinnPhongShader.setUniformTexture("diffuseTex", diffuseTex, 0); // 디퓨즈 라이팅 계산에 사용할 텍스쳐 유니폼 변수로 전송
    blinnPhongShader.setUniformTexture("specTex", specTex, 1); // 스펙큘러 라이팅 계산에 사용할 텍스쳐 유니폼 변수로 전송 (여러 개의 텍스쳐를 사용할 경우 테스쳐 로케이션값을 구분해서 보냄으로써 여러 개의 텍스쳐 변수들을 구분할 수 있게 해줌.)
    shieldMesh.draw(); // shieldMesh 메쉬 드로우콜 호출하여 그려줌.
    
    blinnPhongShader.end();
    // blinnPhongShader 사용 중단
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}

/**
 c++ 참조자 '&' 관련
 
 c++ 에서 참조자란, 특정 변수의 실제 이름 대신,
 별칭처럼 사용할 수 있는 개념.
 
 주로 struct 같은 구조체나 class 같이 사용자에 의해 정의되고,
 구조가 큰 데이터들을 함수의 인자로 전달해야 할 경우 많이 사용됨.
 
 그래서 이제
 '타입& 참조자이름 = 변수이름'
 요런 식으로 주로 표현이 되는데
 
 좌항의 참조자는 '타입'에 해당하는 타입의 변수를
 참조하며, 참조의 대상이 되는 변수는 '변수이름' 이다.
 라고 선언하는 것임.
 
 즉, '참조자이름' 인 참조자는 '타입'형 변수인 '변수이름'에 부여된 메모리 공간과
 동일한 메모리를 참조하겠다는 뜻임.
 
 이게 구조체에서 사용될 경우
 '구조체& 참조자이름'
 이런 식으로 함수의 인자를 표현하는 경우가 있는데
 
 즉, '참조자이름' 에 해당하는 함수인자는
 '구조체' 타입의 변수를 참조한다 는 의미로 해석하면 됨.
 */

/**
 c++ 정적지역변수 (static) 관련
 
 정적 지역 변수란,
 함수 블록 내에서 선언되었지만,
 static 키워드를 붙임으로써,
 마치 전역변수와 동일한 생명주기를 갖는 변수를 의미함.
 
 위에 보면 rotAngle 변수는 함수 블록 내에서 선언된 변수이므로,
 해당 함수블록이 끝나면 소멸하는 '자동 주기'를 갖는 게 원칙이나,
 
 static 키워드를 붙인 지역변수는 완전히 다른 의미를 가져서
 해당 함수블록이 끝나도 마치 전역변수처럼 변수의 값이 계속 유지되는
 '정적 주기'를 가짐.
 
 이 때, 정적지역변수의 초기화는
 해당 변수가 선언되는 함수의 최초 호출 시
 딱 한 번만 수행되기 때문에
 
 위에 draw() 함수가 처음 호출될 때,
 rotAngle 변수에 0을 넣어 초기화되는 할당문은
 최초 1번만 호출됨.
 
 그 이후부터는 draw() 가 반복호출 될때마다 += 연산자에 의해
 변수의 값이 0.01씩 더해지면서 점점 값이 커지기만 할거임.
 */
