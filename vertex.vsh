#version 330 core

// 顶点着色器文件//着色器版本声明


////输入属性 layout它指定了这个输入变量在 Vertex Attribute 数组中的位置(Location)
//顶点位置属性,位置为 0
layout (location = 0) in vec3 aPos;
//顶点纹理坐标属性,位置为 1
layout (location = 1) in vec2 aTexCord;

////输出变量
// 将纹理坐标从顶点着色器传递给片段着色器。
out vec2 TexCord;    // 纹理坐标
void main()
{
    //计算顶点在 OpenGL 坐标系中的最终位置。注意这里对 Y 轴进行了反转,因为 OpenGL 坐标系的 Y 轴与图像坐标系相反。
    gl_Position =  vec4(aPos.x, -aPos.y, aPos.z, 1.0);
    //将输入的顶点纹理坐标直接赋值给输出变量
    TexCord = aTexCord;
}
