# CT_Viewer

- author: ZOU Yiwen (Wilson)
- email: wilson.zouyiwen@qq.com

## Main Function

A simple application made from C++, QT, VTK to read in and view the CT images, planning the screws and some other utilities.

## TODO

1. 使用ITK读取和导出文件（7.12）（进行）
2. 获取宏信息并显示（7.12) （完成)
3. 导入源改为钉子，或者提供更多选项（7.13）(如何确定钉子的大小和现实一样？)
4. 在多个视图定位光标位置（7.15）
5. 加入进度条（7.14）
6. 在3D图像中选点并在该位置插入钉子（7.18）
7. 优化图形界面，重构代码，文档 （进行）

## Bugs:

1. 每次confirm都会把现有的screws再加一次
    解决方案：每次都从最初状态开始把每个钉子加上去
    - 或者： 重新写加物体的逻辑，不用imagedata的merge。直接把投影盖在reslice上。（这样screw也能有不一样的颜色）
