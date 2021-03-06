#version 410

// c++ 미리 계산된 후 받아온 조명연산에 필요한 유니폼 변수들
uniform vec3 lightDir; // 뒤집어지고 정규화된 조명벡터
uniform vec3 lightCol; // 조명색상 (조명강도가 곱해짐)
// uniform vec3 meshCol; // shield 의 원 색상 (디퓨즈 라이팅 계산 시, 이거 대신 디퓨즈 텍스챠에서 샘플링한 텍셀값을 사용할거임)
// uniform vec3 meshSpecCol; // 스펙큘러 하이라이트의 색상 (스펙큘러 라이팅 계산 시, 이거 대신 스펙큘러 텍스쳐에서 샘플링한 텍셀값을 사용할거임)
uniform vec3 cameraPos; // 각 프래그먼트 -> 카메라 방향의 벡터 (이하 '뷰 벡터' 또는 '카메라 벡터') 계산에 필요한 카메라 월드공간 좌표
uniform vec3 ambientCol; // 앰비언트 라이트(환경광 또는 글로벌 조명(전역 조명))의 색상 
uniform sampler2D diffuseTex; // 디퓨즈 라이팅 계산에 사용할 텍스쳐를 담는 변수
uniform sampler2D specTex; // 스펙큘러 라이팅 계산에 사용할 텍스쳐를 담는 변수

in vec3 fragNrm; // 버텍스 셰이더에서 받아온 shield 모델의 (월드공간) 노멀벡터가 보간되어 들어온 값
in vec3 fragWorldPos; // 버텍스 셰이더에서 받아온 shield 모델의 월드공간 위치 좌표가 보간되어 들어온 값
in vec2 fragUV; // 라이팅 계산에 사용할 텍스쳐들을 샘플링하기 위해 shield 모델의 버텍스의 uv좌표들을 보간하여 들어온 값

out vec4 outCol; // 최종 출력할 색상을 계산하여 다음 파이프라인으로 넘겨줄 변수

void main(){
  vec3 normal = normalize(fragNrm); // 프래그먼트 셰이더에서 보간된 노멀벡터는 길이가 1로 보존되지 않으므로, 연산에 사용하기 전 다시 정규화해줘야 함.
  vec3 viewDir = normalize(cameraPos - fragWorldPos); // 카메라의 월드공간 좌표 - 각 프래그먼트 월드공간 좌표를 빼서 각 프래그먼트 -> 카메라 방향의 벡터인 뷰 벡터 계산

  // Blinn-Phong 공식에서의 스펙큘러 라이팅 계산
  vec3 halfVec = normalize(viewDir + lightDir); // 뷰 벡터와 조명벡터 사이의 하프벡터를 구함
  float specAmt = max(0.0, dot(halfVec, normal)); // 하프벡터와 노멀벡터의 내적값을 구한 뒤, max() 함수로 음수값 제거
  float specBright = pow(specAmt, 2.0); // 퐁 반사모델에서와 동일한 스펙큘러 하이라이트를 얻으려면, 퐁 반사모델에서 사용했던 광택값의 2~4배 값을 거듭제곱해야 함. 따라서 0.5의 4배인 2를 광택값으로 사용함.
  // vec3 specCol = lightCol * meshSpecCol * specBright; // '조명색상 * 스펙큘러 하이라이트 색상 * 스펙큘러 라이트값' 을 곱해 스펙큘러 라이트 색상값 결정
  vec3 specCol = texture(specTex, fragUV).x * lightCol * specBright; // c++ 에서 전달해준 스펙큘러 하이라이트 색상 대신, 스펙큘러 맵에서 샘플링한 텍셀값을 사용할거임. 
  // 스펙큘러 맵은 흑백이므로, 텍셀값의 r채널 하나만으로 스칼라배를 해줘도 무방함.

  // 디퓨즈 라이팅 계산 (노멀벡터와 조명벡터를 내적)
  float diffAmt = max(0.0, dot(normal, lightDir)); // 정규화된 노멀벡터와 조명벡터의 내적값을 구한 뒤, max() 함수로 음수인 내적값 제거.
  vec3 meshCol = texture(diffuseTex, fragUV).xyz; // 물체의 원색상은 c++ 에서 전달해준 색상값 대신, 디퓨즈 텍스쳐에서 샘플링한 텍셀값을 사용할거임.
  vec3 diffCol = meshCol  * lightCol * diffAmt; // '물체의 원색상 * 조명색상 * 디퓨즈 라이트값' 을 곱해 디퓨즈 라이트 색상값 결정

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