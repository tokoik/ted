#version 430

//
// simple.frag
//
//   �P���ȉA�e�t�����s���ăI�u�W�F�N�g��`�悷��V�F�[�_
//

// ���X�^���C�U����󂯎�钸�_�����̕�Ԓl
in vec4 idiff;                                      // �g�U���ˌ����x
in vec4 ispec;                                      // ���ʔ��ˌ����x

// �t���[���o�b�t�@�ɏo�͂���f�[�^
layout (location = 0) out vec4 fc;                  // �t���O�����g�̐F

void main(void)
{
  // ��f�̉A�e�����߂�
  fc = idiff + ispec;
}
