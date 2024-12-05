#version 330 core
//片段着色器文件

// 纹理坐标
in  vec2 TexCord;
//存储 Y/U/V 分量的 2D 纹理采样器
uniform sampler2D tex_y;
uniform sampler2D tex_u;
uniform sampler2D tex_v;

void main()
{
    //定义2个三维向量
    vec3 yuv; //用于存储 YUV 颜色分量的 3D 向量
    vec3 rgb;
//前置知识点：
//Y 分量表示亮度,范围是 0 到 1。
//U 和 V 分量表示色度,理论上它们的范围应该是 -0.5 到 0.5。

    // YUV转RGB 通过调用 texture2D 函数从对应的纹理中采样 Y、U、V 分量,并将它们存储到 yuv 向量中。注意 U 和 V 分量需要减去 0.5 进行归一化。
    yuv.x = texture2D(tex_y, TexCord).r;
    yuv.y = texture2D(tex_u, TexCord).r-0.5;
    yuv.z = texture2D(tex_v, TexCord).r-0.5;

    //使用一个 3x3 的矩阵乘法将 YUV 颜色空间转换为 RGB 颜色空间。这个矩阵是 YUV 到 RGB 的标准转换矩阵。
    rgb = mat3(1.0, 1.0, 1.0,
               0.0, -0.39465, 2.03211,
               1.13983, -0.58060, 0.0
               ) * yuv;

    // 将转换后的 RGB 颜色值输出到片段着色器的输出变量 gl_FragColor。alpha 通道被设置为 1.0(不透明)。
    gl_FragColor = vec4(rgb, 1.0);
}
