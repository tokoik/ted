#version 150 core
#extension GL_ARB_explicit_attrib_location : enable

//
// �e�N�X�`�����W�̈ʒu�̉�f�F�����̂܂܎g��
//

// �w�i�e�N�X�`��
uniform sampler2D image;

// �e�N�X�`�����W
in vec2 texcoord;

// �t���O�����g�̐F
layout (location = 0) out vec4 fc;

void main(void)
{
  // ��f�̉A�e�����߂�
  fc = texture(image, texcoord);
}
