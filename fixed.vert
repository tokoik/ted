#version 430

//
// �����̉�]���s��Ȃ�
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

// �w�i�e�N�X�`���̃e�N�X�`����ԏ�̃X�P�[��
vec2 scale = vec2(-0.5 * size.y / size.x, 0.5) / circle.st;

// �w�i�e�N�X�`���̃e�N�X�`����ԏ�̒��S�ʒu
vec2 center = circle.pq + 0.5;

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

  // ���_�ʒu�����̂܂܃��X�^���C�U�ɑ���΃N���b�s���O��ԑS�ʂɕ`��
  gl_Position = vec4(position, 0.0, 1.0);

  // �X�N���[����̓_�̈ʒu
  //   position �ɃX�N���[���̑傫�� screen.st �������Ē��S�ʒu screen.pq �𑫂��΁A
  //   �X�N���[����̓_�̈ʒu p �������邩��A���_�ɂ��鎋�_���炱�̓_�Ɍ����������́A
  //   �œ_���� focal �� Z ���W�ɗp���� (p, focal) �ƂȂ�B
  vec2 p = position * screen.st + screen.pq;

  // �����x�N�g��
  vec4 vector = vec4(p, -focal, 0.0);

  // �e�N�X�`�����W
  texcoord = vector.xy * scale / vector.z + center;
}
