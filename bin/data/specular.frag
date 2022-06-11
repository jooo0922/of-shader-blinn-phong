#version 410

// c++ 미리 계산된 후 받아온 조명연산에 필요한 유니폼 변수들
uniform vec3 lightDir; // 뒤집어지고 정규화된 조명벡터
uniform vec3 lightCol; // 조명색상 (조명강도가 곱해짐)
uniform vec3 meshCol; // torus 의 원 색상
uniform vec3 meshSpecCol; // 스펙큘러 하이라이트의 색상
uniform vec3 cameraPos; // 각 프래그먼트 -> 카메라 방향의 벡터 (이하 '뷰 벡터' 또는 '카메라 벡터') 계산에 필요한 카메라 월드공간 좌표
uniform vec3 ambientCol; // 앰비언트 라이트(환경광 또는 글로벌 조명(전역 조명))의 색상 

in vec3 fragNrm; // 버텍스 셰이더에서 받아온 torus 모델의 (월드공간) 노멀벡터가 보간되어 들어온 값
in vec3 fragWorldPos; // 버텍스 셰이더에서 받아온 torus 모델의 월드공간 위치 좌표가 보간되어 들어온 값

out vec4 outCol; // 최종 출력할 색상을 계산하여 다음 파이프라인으로 넘겨줄 변수

void main(){
  vec3 normal = normalize(fragNrm); // 프래그먼트 셰이더에서 보간된 노멀벡터는 길이가 1로 보존되지 않으므로, 연산에 사용하기 전 다시 정규화해줘야 함.

  // 스펙큘러 라이팅 계산 (반사벡터와 노멀벡터를 내적한 뒤, '광택'값만큼 내적값을 거듭제곱함)
  // vec3 refl = reflect(-lightDir, normal); // reflect() 내장함수로 조명벡터와 노멀벡터를 이용하여 반사벡터 계산. (이미 음수화된 반사벡터를 다시 음수화? -> reflect() 함수 관련 하단 필기)
  // vec3 viewDir = normalize(cameraPos - fragWorldPos); // 카메라의 월드공간 좌표 - 각 프래그먼트 월드공간 좌표를 빼서 각 프래그먼트 -> 카메라 방향의 벡터인 뷰 벡터 계산
  
  // float specAmt = max(0.0, dot(refl, viewDir)); // 반사벡터와 뷰 벡터의 내적값을 구한 뒤, max() 함수로 음수인 내적값 제거. (다른 조명 및 색상값과 더해줘도 음수값이면 어두운 색으로 찍힐 우려가 있어서) 
  // float specBright = pow(specAmt, 16.0); // 내적값을 '광택' 값만큼 거듭제곱하여 최종 스펙큘러 라이트 값을 구함. (광택값이 클수록 스펙큘러 영역이 좁아지면서 더 매끄러운 표면으로 묘사됨.)
  /*
    내적의 거듭제곱 지수, 즉 '광택'값이 작아질수록, 
    조명벡터와 카메라벡터의 각도가 90도를 넘어가서 내적값이 음수로 나오고, 
    그로 인해 스펙큘러 라이트 값이 0으로 초기화되는 부분과
    그렇지 않은 부분의 스펙큘러 라이트 값의 갭이 너무 커지다보니
    각도가 90도를 넘어가는 지점에서부터 날카로운 경계가 생기는 문제가 발생함.
  */
  // float specBright = pow(specAmt, 0.5); 
  // vec3 specCol = lightCol * meshSpecCol * specBright; // '조명색상 * 스펙큘러 하이라이트 색상 * 스펙큘러 라이트값' 을 곱해 스펙큘러 라이트 색상값 결정

  // Blinn-Phong Lighting 공식으로 스펙큘러 라이팅 계산 (하프 벡터 사용)
  /*
    위에서 말했듯이, 광택값이 지나치게 작은 경우
    발생할 수 있는 문제를 해결하기 위해, 또, 내적계산을 줄여주는 성능 최적화를 위해 
    노멀벡터와 조명벡터 사이의 하프벡터를 구해 사용하는 블린-퐁 모델을 사용함. 
  */
  vec3 viewDir = normalize(cameraPos - fragWorldPos); // 카메라의 월드공간 좌표 - 각 프래그먼트 월드공간 좌표를 빼서 각 프래그먼트 -> 카메라 방향의 벡터인 뷰 벡터 계산
  vec3 halfVec = normalize(viewDir + lightDir); // 뷰 벡터와 조명벡터 사이의 하프벡터를 구함
  float specAmt = max(0.0, dot(halfVec, normal)); // 하프벡터와 노멀벡터의 내적값을 구한 뒤, max() 함수로 음수값 제거
  float specBright = pow(specAmt, 2.0); // 퐁 반사모델에서와 동일한 스펙큘러 하이라이트를 얻으려면, 퐁 반사모델에서 사용했던 광택값의 2~4배 값을 거듭제곱해야 함. 따라서 0.5의 4배인 2를 광택값으로 사용함.
  vec3 specCol = lightCol * meshSpecCol * specBright; // '조명색상 * 스펙큘러 하이라이트 색상 * 스펙큘러 라이트값' 을 곱해 스펙큘러 라이트 색상값 결정

  // 디퓨즈 라이팅 계산 (노멀벡터와 조명벡터를 내적)
  float diffAmt = max(0.0, dot(normal, lightDir)); // 정규화된 노멀벡터와 조명벡터의 내적값을 구한 뒤, max() 함수로 음수인 내적값 제거.
  vec3 diffCol = meshCol * lightCol * diffAmt; // '물체의 원색상 * 조명색상 * 디퓨즈 라이트값' 을 곱해 디퓨즈 라이트 색상값 결정

  // 앰비언트 라이트 계산 (앰비언트 라이트 색상값과 물체의 원색상을 곱함)
  vec3 ambient = ambientCol * meshCol; // 물체의 원 색상과 다른 쌩뚱맞은 색이 나오면 안되어서 물체의 원 색상을 곱해주는 것

  // outCol = vec4(lightCol * meshSpecCol * specBright, 1.0); // '조명 색상 * 스펙큘러 하이라이트 색상 * 스펙큘러 라이트값' 을 곱해 최종 색상 결정
  outCol = vec4(diffCol + specCol + ambient, 1.0); // '스펙큘러 라이트 색상값 + 디퓨즈 라이트 색상값 + 앰비언트 라이트 색상값(물체의 원색상 반영됨)' 을 합쳐서 최종 색상값 결정
}

/*
  reflect(조명벡터, 노멀벡터) 

  reflect() 함수는 복잡한 반사벡터 계산 공식을
  GLSL 내장함수로 쉽게 처리할 수 있게 해줌.

  단, 주의할 점은, 첫 번째 인자로 들어가는 조명벡터는
  reflect() 함수가 이미 '광원 -> 메쉬표면 방향으로 향하는 벡터' 로 인지하고 있음.

  이 말은 뭐냐면, 원래의 조명벡터를 거꾸로 뒤집지 않고,
  광원 -> 메쉬표면의 원래 방향 그대로 넣어줘야 한다는 뜻임.

  그러나, 우리는 디퓨즈 라이팅 계산을 위해서
  c++ 에서 이미 조명벡터를 음수화하여 한 번 뒤집어줬던 걸 
  유니폼 변수로 전달받았지?

  그래서 이 조명벡터를 reflect() 함수에서 사용할 때에는,
  다시 -1 을 곱해 뒤집어진걸 다시 뒤집어줘서
  원래대로 만들어줘야 함.

  그래서 이미 음수화된 조명벡터를 다시 음수화한 것임.
*/