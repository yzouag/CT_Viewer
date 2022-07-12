# CT_Viewer

- author: ZOU Yiwen (Wilson)
- email: wilson.zouyiwen@qq.com

## Main Function

A simple application made from C++, QT, VTK to read in and view the CT images, planning the screws and some other utilities.

## TODO

1. 使用ITK读取和导出文件（7.12）
2. 获取宏信息并显示（7.12)
    - 0010|0010: patient name
    - 0008|0012: instance creation date
    - 0008|0032: Acquisition Time
    - 0008|0060: Modality
    - 0008|0080: Institution Name
    - 0008|0070: Manufacturer
    - 0009|1030: Study Description
    - 0008|103e: Series Description
    - 0008|1050: Performing Physician's Name
    - 0008|1060: Name of Physician(s) Reading Study
    - 0008|1070: Operators' Name
    - 0008|1090: Manufacturer's Model Name
    - 0010|0020: Patient ID
    - 0010|0030: Patient's Birth Date
    - 0010|0040: Patient's Sex
    - 0010|1010: Patient's Age
    - 0018|0050: Slice Thickness
    - 0018|0060: KVP
    - 0018|0088: Spacing Between Slices
3. 导入源改为钉子，或者提供更多选项（7.13）
4. 在多个视图定位光标位置（7.14）
5. 加入进度条（7.13）
6. 在3D图像中选点并在该位置插入钉子（7.15）