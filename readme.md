# CT_Viewer

- author: ZOU Yiwen (Wilson)
- email: wilson.zouyiwen@qq.com

## Main Function

A simple application made from C++, QT, VTK to read in and view the CT images, planning the screws and some other utilities.

## Environment

- vtk 8.2
- itk 5.2.1
- QT 5.9.2

## Ackknowlegement

The idea of this project borrows from two popular applications in the medical view, [ITK-SNAP](http://www.itksnap.org/pmwiki/pmwiki.php) and [3D Slicer](https://www.slicer.org/). The CT_Viewer is just a toy project with minimal functionalities. I am still learning about software engineering and data visualization. Thanks for the great community of VTK to guide me overcoming all the difficulties in writing this project.

## TODO

1. 使用ITK读取和导出文件（导出没做,导入也是假的）
2. 获取宏信息并显示（完成)
3. 导入源改为钉子，或者提供更多选项(如何确定钉子的大小和现实一样？)
4. 在多个视图定位光标位置（完成）
5. 加入进度条（7.14）（完成）
6. 在3D图像中选点并在该位置插入钉子（进行）
7. 优化图形界面，重构代码，文档 （进行）
8. 设置初始页面（类似itk snap）
9. undo redo 操作？要实现是否重写imageblend？
10. 加入测量工具
11. 设置不同灰度的显示(完成)
12. 改善UI
13. details改为table(完成)

## Bugs:

1. 每次confirm都会把现有的screws再加一次
    解决方案：每次都从最初状态开始把每个钉子加上去
    - 或者： 重新写加物体的逻辑，不用imagedata的merge。直接把投影盖在reslice上。（这样screw也能有不一样的颜色）

下一步：
1. clear button有问题，报错 （复现不了）
2. 需要加入一个显示每个图钉信息的table吗？