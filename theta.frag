#version 430

//
//   RICOH THETA S �̃��C�u�X�g���[�~���O�f���̕��ʓW�J
//

// �w�i�e�N�X�`��
uniform sampler2D image;

// �e�N�X�`�����W
in vec2 texcoord_f;
in vec2 texcoord_b;

// �O��̃e�N�X�`���̍�����
in float blend;

// �t���O�����g�̐F
layout (location = 0) out vec4 fc;

void main(void)
{
  // �O��̃e�N�X�`���̐F���T���v�����O����
  vec4 color_f = texture(image, texcoord_f);
  vec4 color_b = texture(image, texcoord_b);

  // �T���v�����O�����F���u�����h���ăt���O�����g�̐F�����߂�
  fc = mix(color_f, color_b, blend);
}
