#version 430

//
// target.vert
//
//   �^�[�Q�b�g�`��p�̃V�F�[�_
//

// �ϊ��s��
uniform mat4 mc;                                    // �N���b�s���O���W�n�ւ̕ϊ��s��

// ���_����
layout (location = 0) in vec4 pv;                   // ���[�J�����W�n�̒��_�ʒu

void main(void)
{
  gl_Position = mc * pv;
}
