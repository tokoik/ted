#version 150 core
#extension GL_ARB_explicit_attrib_location : enable

//
// simple.vert
//
//   �P���ȉA�e�t�����s���ăI�u�W�F�N�g��`�悷��V�F�[�_
//

// ����
layout (std140) uniform Light
{
  vec4 lamb;                                          // ��������
  vec4 ldiff;                                         // �g�U���ˌ�����
  vec4 lspec;                                         // ���ʔ��ˌ�����
  vec4 lpos;                                          // �ʒu
};

// �ގ�
layout (std140) uniform Material
{
  vec4 kamb;                                          // �����̔��ˌW��
  vec4 kdiff;                                         // �g�U���ˌW��
  vec4 kspec;                                         // ���ʔ��ˌW��
  float kshi;                                         // �P���W��
};

// �ϊ��s��
uniform mat4 mv;                                      // ���f���r���[�ϊ��s��
uniform mat4 mp;                                      // ���e�ϊ��s��
uniform mat4 mn;                                      // �@���x�N�g���̕ϊ��s��

// ���_����
layout (location = 0) in vec4 pv;                     // ���[�J�����W�n�̒��_�ʒu
layout (location = 1) in vec4 nv;                     // ���_�̖@���x�N�g��

// ���X�^���C�U�ɑ��钸�_����
out vec4 idiff;                                       // �g�U���ˌ����x
out vec4 ispec;                                       // ���ʔ��ˌ����x

void main(void)
{
  // ���W�v�Z
  vec4 p = mv * pv;                                   // ���_���W�n�̒��_�̈ʒu
  vec3 v = normalize(p.xyz);                          // �����x�N�g��
  vec3 l = normalize((lpos * p.w - p * lpos.w).xyz);  // �����x�N�g��
  vec3 n = normalize((mn * nv).xyz);                  // �@���x�N�g��
  vec3 h = normalize(l - v);                          // ���ԃx�N�g��

  // �A�e�v�Z
  idiff = max(dot(n, l), 0.0) * kdiff * ldiff + kamb * lamb;
  ispec = pow(max(dot(n, h), 0.0), kshi) * kspec * lspec;

  // ���_�̃X�N���[�����W�l
  gl_Position = mp * p;
}
