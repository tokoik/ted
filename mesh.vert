#version 150 core

//
//   ���჌���Y�摜�ɕό`
//

// �X�N���[���̊i�q�Ԋu
uniform vec2 gap;

// �X�N���[���̍����ƕ�
uniform vec2 screen;

// �X�N���[������]����ϊ��s��
uniform mat4 rotation;

// �e�N�X�`�����W
out vec2 texcoord;

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

  // ���_�ʒu�����̂܂܃e�N�X�`�����W�Ɏg��
  texcoord = position * 0.5 + 0.5;

  // �����x�N�g��
  //   position �ɃX�N���[���̑傫�� screen.st �������Ē��S�ʒu screen.pq �𑫂��΁A
  //   �X�N���[����̓_�̈ʒu p �������邩��A���_�ɂ��鎋�_���炱�̓_�Ɍ����������́A
  //   �œ_������ 1 �X�N���[���̑傫���̓񕪂̈�� size �Ƃ����Ƃ� (p * size, -1) �ƂȂ�B
  //   �������]�������Ɛ��K�����āA���̕����̎����P�ʃx�N�g���𓾂�B
  vec4 vector = normalize(rotation * vec4(position * screen, -1.0, 0.0));

  // ���_�ʒu������摜��Ԃɕϊ�����
  gl_Position = vec4(acos(-vector.z) * normalize(vector.xy) * 0.31830987, 0.0, 1.0);
}
