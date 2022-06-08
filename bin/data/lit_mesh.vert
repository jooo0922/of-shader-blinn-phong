#version 410

// layout 을 이용해서 버텍스 셰이더에서 각 버텍스 데이터가 저장된 순서를 알려줌. (오픈프레임웍스가 버텍스 데이터를 저장하는 순서는 p.74 참고)
layout(location = 0) in vec3 pos;
layout(location = 2) in vec3 nrm;

uniform mat4 mvp; // c++ (오픈프레임웍스)에서 합쳐준 투영 * 뷰 * 모델 행렬을 전달받는 유니폼 변수
uniform mat4 model; // 각 버텍스의 월드좌표를 구하기 위해 mvp 행렬과 별도로 전달받는 모델행렬을 저장할 유니폼 변수
uniform mat3 normalMatrix; // 조명계산에 필요한 노멀벡터(즉, 월드공간으로 변환된 노멀벡터)를 계산하려면, 노말행렬을 따로 구해서 버텍스 셰이더에 가져옴.

out vec3 fragNrm; // 프래그먼트 셰이더로 전송할 torus 모델의 노멀벡터
out vec3 fragWorldPos; // 각 버텍스 셰이더의 월드좌표를 구한 뒤 보간해서 프래그먼트 셰이더로 내보낼 때 사용할 out 변수

void main() {
  fragNrm = (normalMatrix * nrm).xyz; // 노말행렬과 오브젝트공간 기준의 노말벡터를 구해서 월드공간으로 변환된 노말벡터를 구하고, 보간해서 프래그먼트 셰이더로 넘김.
  fragWorldPos = (model * vec4(pos, 1.0)).xyz; // 버텍스 좌표를 동차좌표로 변환해서 모델행렬과 곱함으로써 월드좌표로 변환하고, vec4값의 xyz만 swizzle 하여 프래그먼트 셰이더로 보간해서 내보냄.

  gl_Position = mvp * vec4(pos, 1.0); // 동차좌표계로 변환한 버텍스 위치좌표에 mvp 행렬을 곱해서 변환을 처리한 뒤, 최종 버텍스 위치값을 결정함.
}