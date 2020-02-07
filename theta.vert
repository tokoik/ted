#version 430

//
// RICOH THETA S �̃��C�u�X�g���[�~���O�f���̕��ʓW�J
//

// �X�N���[���̊i�q�Ԋu
uniform vec2 gap;

// �X�N���[���̑傫���ƒ��S�ʒu
uniform vec4 screen;

// �X�N���[���܂ł̏œ_����
uniform float focal;

// �X�N���[������]����ϊ��s��
uniform mat4 rotation;

// �w�i�e�N�X�`���̔��a�ƒ��S�ʒu
uniform vec4 circle;

// �w�i�e�N�X�`��
uniform sampler2D image;

// �w�i�e�N�X�`���̃T�C�Y
vec2 size = textureSize(image, 0);

// �w�i�e�N�X�`���̌���J�������̃e�N�X�`����ԏ�̔��a�ƒ��S
vec2 radius_b = circle.st * vec2(-0.25, 0.25 * size.x / size.y);
vec2 center_b = vec2(radius_b.s - circle.p + 0.5, radius_b.t - circle.q);

// �w�i�e�N�X�`���̑O���J�������̃e�N�X�`����ԏ�̔��a�ƒ��S
vec2 radius_f = vec2(-radius_b.s, radius_b.t);
vec2 center_f = vec2(center_b.s + 0.5, center_b.t);

// �e�N�X�`�����W
out vec2 texcoord_b;
out vec2 texcoord_f;

// �O��̃e�N�X�`���̍�����
out float blend;

void main(void)
{
  // ���_�ʒu
  //   �e���_�ɂ����� gl_VertexID �� 0, 1, 2, 3, ... �̂悤�Ɋ��蓖�Ă��邩��A
  //     x = gl_VertexID >> 1      = 0, 0, 1, 1, 2, 2, 3, 3, ...
  //     y = 1 - (gl_VertexID & 1) = 1, 0, 1, 0, 1, 0, 1, 0, ...
  //   �̂悤�� GL_TRIANGLE_STRIP �����̒��_���W�l��������B
  //   y �� gl_InstaceID �𑫂��� glDrawArrayInstanced() �̃C���X�^���X���Ƃ� y ���ω�����B
  //   ����Ɋi�q�̊Ԋu gap �������� 1 �������Ώc�� [-1, 1] �͈̔͂̓_�Q position ��������B
  int x = gl_VertexID >> 1;
  int y = gl_InstanceID + 1 - (gl_VertexID & 1);
  vec2 position = vec2(x, y) * gap - 1.0;

  // ���_�ʒu�����̂܂܃��X�^���C�U�ɑ���΃N���b�s���O��ԑS�ʂɕ`��
  gl_Position = vec4(position, 0.0, 1.0);

  //   position �ɃX�N���[���̑傫�� screen.st �������Ē��S�ʒu screen.pq �𑫂��΁A
  //   �X�N���[����̓_�̈ʒu p �������邩��A���_�ɂ��鎋�_���炱�̓_�Ɍ����������́A
  //   �œ_���� focal �� Z ���W�ɗp���� (p, focal) �ƂȂ�B
  vec2 p = position * screen.st + screen.pq;

  // �����x�N�g��
  //   �������]�������Ɛ��K�����āA���̕����̎����P�ʃx�N�g���𓾂�B
  vec4 vector = normalize(rotation * vec4(p, -focal, 0.0));

  // ���̕����x�N�g���̑��ΓI�ȋp
  //   1 - acos(vector.z) * 2 / �� �� [-1, 1]
  float angle = 1.0 - acos(vector.z) * 0.63661977;

  // �O��̃e�N�X�`���̍�����
  blend = smoothstep(-0.02, 0.02, angle);

  // ���̕����x�N�g���� yx ��ł̕����x�N�g��
  vec2 orientation = normalize(vector.yx) * 0.885;

  // �e�N�X�`�����W
  texcoord_b = (1.0 - angle) * orientation * radius_b + center_b;
  texcoord_f = (1.0 + angle) * orientation * radius_f + center_f;
}
